/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU

    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
			Guy Vega, gtvega@seabass.st.usm.edu
			Dominic Tynes, dbtynes@seabass.st.usm.edu

    Firebird Bulletin Board System
    Copyright (C) 1996, Hsien-Tsung Chang, Smallpig.bbs@bbs.cs.ccu.edu.tw
			Peng Piaw Foong, ppfoong@csie.ncu.edu.tw

    Firebird2000 Bulletin Board System
    Copyright (C) 1999-2001, deardragon, dragon@fb2000.dhs.org

    Puke Bulletin Board System
    Copyright (C) 2001-2002, Yu Chen, monster@marco.zsu.edu.cn
			     Bin Jie Lee, is99lbj@student.zsu.edu.cn

    Contains codes from YTHT & SMTH distributions of Firebird Bulletin
    Board System.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include "bbs.h"
#include "vote.h"


int ERROR_READ_SYSTEM_FILE = NA;
int RMSG = YEA;
int msg_num = 0;
int guestuser = 0;	/* is guest user */
int count_friends = 0,  /* ���ߺ������� */
    count_users = 0;	/* �ܿ�������վ����(!=curr_login_num) */
int iscolor = 1;	/* ��ɫ��ʾ */
int listmode;
int numofsig = 0;	/* ǩ��������*/
jmp_buf byebye;
int enter_uflags;

struct user_info uinfo;

char BoardName[STRLEN];	/* BBS name */
char ULIST[STRLEN];
int utmpent = -1;
time_t login_start_time;
int showansi = 1;
int started = 0;


#ifdef TALK_LOG
extern int talkrec;
#endif

#ifdef ALLOWSWITCHCODE
int convcode = 0;
extern void resolve_GbBig5Files(void);
#endif

sigjmp_buf jmpbuf;

void 
sigfault(int signo)
{
        /* recover from memory violation */
        siglongjmp(jmpbuf, signo);
}

/* monster: update uinfo according to user defines */
void
renew_uinfo(void)
{
	if (DEFINE(DEF_DELDBLCHAR))
		enabledbchar = 1;
	else
		enabledbchar = 0;

	uinfo.pager = 0;
	if (DEFINE(DEF_FRIENDCALL))
		uinfo.pager |= FRIEND_PAGER;

	if (currentuser.flags[0] & PAGER_FLAG) {
		uinfo.pager |= ALL_PAGER;
		uinfo.pager |= FRIEND_PAGER;
	}

	if (DEFINE(DEF_FRIENDMSG))
		uinfo.pager |= FRIENDMSG_PAGER;

	if (DEFINE(DEF_ALLMSG)) {
		uinfo.pager |= ALLMSG_PAGER;
		uinfo.pager |= FRIENDMSG_PAGER;
	}

	if (DEFINE(DEF_NOTHIDEIP)) {
		uinfo.hideip = 'N';
	} else {
		if (DEFINE(DEF_FRIENDSHOWIP))
			uinfo.hideip = 'F';
		else
			uinfo.hideip = 'H';
	}

	iscolor = (DEFINE(DEF_COLOR)) ? 1 : 0;
}

#ifdef NICKCOLOR
void
renew_nickcolor(int usertitle)
{
	/* monster: determine the color code for nickname display */
	if ((currentuser.userlevel & PERM_SYSOP) ||
	    (currentuser.userlevel & PERM_ACBOARD) ||
	    (currentuser.userlevel & PERM_OBOARDS) ||
	    (currentuser.userlevel & PERM_ACHATROOM))
		uinfo.nickcolor = 33;
	else
		uinfo.nickcolor = (usertitle == 0) ? 0 : 36;
}
#endif

void
setflags(int mask, int value)
{
	if (((currentuser.flags[0] & mask) && 1) != value) {
		if (value)
			currentuser.flags[0] |= mask;
		else
			currentuser.flags[0] &= ~mask;
	}
}

void
u_enter(void)
{

	enter_uflags = currentuser.flags[0];
	memset(&uinfo, 0, sizeof(uinfo));

	uinfo.active = YEA;
	uinfo.pid = getpid();
	uinfo.mode = LOGIN;
	uinfo.uid = usernum;
	uinfo.idle_time = time(NULL);
	renew_uinfo();

	/* ʹû������Ȩ�޵�ID��½ʱ�Զ��ָ���������״̬ */
	if (HAS_PERM(PERM_CLOAK) && (currentuser.flags[0] & CLOAK_FLAG)) { /* ��һ������ */
		uinfo.invisible = YEA;
	} else {
		setflags(CLOAK_FLAG, NA);
	}

	strlcpy(uinfo.from, fromhost, sizeof(uinfo.from));
	strlcpy(uinfo.userid, currentuser.userid, sizeof(uinfo.userid));
	strlcpy(uinfo.realname, currentuser.realname, sizeof(uinfo.realname));
	strlcpy(uinfo.username, currentuser.username, sizeof(uinfo.username));
	getfriendstr(); /* ����uinfo.friend */
	getrejectstr(); /* ����uinfo.reject */

	if (HAS_PERM(PERM_EXT_IDLE))
		uinfo.ext_idle = YEA;

	#ifdef NICKCOLOR
	renew_nickcolor(currentuser.usertitle);
	#endif

	listmode = 0;           /* ����һ��, ������¼���� utmpent ��λʧ�ܼ��� */
	while (1) {
		utmpent = getnewutmpent(&uinfo);
		if (utmpent >= 0 || utmpent == -1)
			break;
		if (utmpent == -2 && listmode <= 100) {
			listmode++;
			usleep(250);    /* ��Ϣ�ķ�֮һ���ٽ����� */
			continue;
		}
		if (listmode > 100) {   /* ������ */
			report("getnewutmpent(): too much times, give up.");
			prints("getnewutmpent(): ʧ��̫���, ����. ��ر�վ��.\n");
			sleep(1);
			exit(0);
		}
	}
	if (utmpent < 0) {
		report("Fault: No utmpent slot for %s\n", uinfo.userid);
		sleep(3);
		exit(0);
	}

	listmode = 0;
	digestmode = NA;
}

void
u_exit(void)
{
	int fd;
	char fname[STRLEN];
	FILE *fp;

	/*
	   ��Щ�źŵĴ���Ҫ�ص�, ����������ʱ�Ⱥ�س�ʱ����  (ylsdd)
	   �źŻᵼ����д����, ������µ��������ұ�kick user����
	 */
	signal(SIGHUP, SIG_DFL);
	signal(SIGALRM, SIG_DFL);
	signal(SIGPIPE, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);

	setflags(PAGER_FLAG, (uinfo.pager & ALL_PAGER));
	setflags(CLOAK_FLAG, HAS_PERM(PERM_CLOAK) ? uinfo.invisible : NA);

#ifdef INBOARDCOUNT
	/* inboard user count  by freestyler */
	if ( is_inboard == 1 ) {
		int idx = getbnum(currboard);
		board_setcurrentuser(idx-1, -1);
	}
#endif

	if (!ERROR_READ_SYSTEM_FILE) {
		time_t stay;

		set_safe_record();
		stay = time(NULL) - login_start_time;
		currentuser.stay += stay;
		currentuser.lastlogout = time(NULL);
		setuserfile(fname, "lastlogin");
		if ((fp = fopen(fname, "r")) != NULL) {
			int lastlogin;
			if (fscanf(fp, "%d", &lastlogin) == 1) {
				currentuser.lastlogin = lastlogin;
			}
			fclose(fp);
			unlink(fname);
		}
		substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	}

	uinfo.invisible = YEA;
	uinfo.sockactive = NA;
	uinfo.sockaddr = 0;
	uinfo.destuid = 0;
	uinfo.pid = 0;
	uinfo.active = NA;
	uinfo.deactive_time = time(NULL);

	if ((fd = open(ULIST, O_RDWR | O_CREAT, 0600)) > 0)
		f_exlock(fd);
	update_utmp();
	if (fd > 0) {
		f_unlock(fd);
		close(fd);
	}
	utmpent = -1;           // add by quickmouse to prevent supercloak
}

int
cmpuids(void *uid, void *up_ptr)
{
        struct userec *up = (struct userec *)up_ptr;

        return !strncasecmp((char *)uid, up->userid, sizeof(up->userid));
}

int
cmpuids2(int unum, struct user_info *urec)
{
	return (unum == urec->uid && urec->pid != getpid());
}

/* ����userid�����(�±�1��ʼ), ������currentuser,
 * û�ҵ�����0 */
int
dosearchuser(char *userid)
{
	if ((usernum = getuser(userid, &currentuser)) == 0)
		memset(&currentuser, 0, sizeof(currentuser));

	return usernum;
}

static void
talk_request(int signo)
{
	signal(SIGUSR1, talk_request);
	talkrequest = YEA;
	bell();
	bell();
	bell();
	sleep(1);
	bell();
	bell();
	bell();
	bell();
	bell();
	return;
}

extern int s_count, s_count2;

void
abort_bbs(void)
{
	extern int child_pid;

	safe_kill(child_pid);

	if (uinfo.mode == POSTING || uinfo.mode == SMAIL || uinfo.mode == EDIT
	    || uinfo.mode == EDITUFILE || uinfo.mode == EDITSFILE ||
	    uinfo.mode == EDITANN)
		keep_fail_post();

	/* monster: ��������� */
	brc_update();

	/* monster: ���������¼ */
	#ifdef TALK_LOG
	if (talkrec != -1) {
		time_t now;
		char talkbuf[STRLEN], tlogname[STRLEN], tlogname2[STRLEN];

		now = time(NULL);
		snprintf(talkbuf, sizeof(talkbuf), "\n\033[1;34mͨ������, ʱ��: %s \033[m\n", Cdate(&now));
		write(talkrec, talkbuf, strlen(talkbuf));
		close(talkrec);

		sethomefilewithpid(tlogname, currentuser.userid, "talklog");
		sethomefile(tlogname2, currentuser.userid, "talklog");
		rename(tlogname, tlogname2);    /* ע�⣺�Ḳ����ǰ���µ������¼ */
	}
	#endif

	if (started) {
		time_t stay;

		stay = time(NULL) - login_start_time;
		snprintf(genbuf, sizeof(genbuf), "Stay: %d (%s)", (int)(stay / 60), currentuser.username);
		log_usies("AXXED", genbuf);
		u_exit();
	}

	deattach_shm();
	shutdown(0, 2); /* �ر����� */
	close(0);
	exit(0);
}

/* monster: ���⾯����Ϣ ^_^ */
static void
abort_bbs_sig(int signo)
{
	abort_bbs();
}

/* monster: ���Ա����û�δ�浵���ļ���ʧ�ܺ��ٲ���SIGKILLǿ����ֹ���� */
void
safe_kill(int pid)
{
	int ret;

	if (pid > 0) {
		ret = kill(pid, SIGHUP);
		sleep(1);
		if (ret != 0) kill(pid, SIGKILL);
	}
}

void
multi_user_check(void)
{
	struct user_info uin;
	int logins, mustkick = NA;

	if (HAS_PERM(PERM_MULTILOG))
		return;         /* don't check sysops */

	logins = count_self();

	if (guestuser) {
		if (logins > MAXGUEST) {
			prints("\033[1;33m��Ǹ, Ŀǰ����̫�� \033[1;36mguest\033[33m, ���Ժ����ԡ�\033[m\n");
			oflush();
			sleep(3);
			exit(1);
		}
		return;		/* allow multiple guest user */
	}

	if (heavyload() && logins) {
		prints("\033[1;33m��Ǹ, Ŀǰϵͳ���ɹ���, �����ظ� Login��\033[m\n");
		mustkick = YEA;
	} else {
		if (logins >= MULTI_LOGINS + (HAS_PERM(PERM_OBOARDS) ? 1 : 0)) {
			prints("\033[1;32mΪȷ��������վȨ��, ��վ���������ø��ʺŵ�½ %d ����\n\033[0m", MULTI_LOGINS + (HAS_PERM(PERM_OBOARDS) ? 1 : 0));
			prints("\033[1;36m��Ŀǰ�Ѿ�ʹ�ø��ʺŵ�½�� %d ����������Ͽ����������ӷ��ܽ��뱾վ��\n\033[0m", logins);
			mustkick = YEA;
		}
	}

	if (mustkick) {
		getdata(0, 0, "\033[1;37m����ɾ���ظ��� login �� (Y/N)? [N]\033[m",
			genbuf, 4, DOECHO, YEA);
		if (genbuf[0] == 'N' || genbuf[0] == 'n' || genbuf[0] == '\0') {
			prints("\033[33m�ܱ�Ǹ�����Ѿ��ø��ʺŵ�½ %d �������ԣ������߽���ȡ����\033[m\n", logins);
			oflush();
			sleep(3);
			exit(1);
		} else {
			if (!search_ulist(&uin, cmpuids2, usernum))
				return; /* user isn't logged in */
			if (!uin.active || (uin.pid && kill(uin.pid, 0) == -1)) /* ���̲����� */
				return; /* stale entry in utmp file */
			safe_kill(uin.pid);
			report("kicked (multi-login)");
			log_usies("KICK ", currentuser.username);
		}
	}
}

void
system_init(void)
{
	struct sigaction act;

#ifdef MSGQUEUE
	msqid = msgget(MSQKEY, 0644); /* ���ִ���л򴴽�һ���¶���, ����log */
#endif

	gethostname(genbuf, sizeof(genbuf));
	snprintf(ULIST, sizeof(ULIST), ".UTMP.%s", genbuf); 

	signal(SIGHUP, abort_bbs_sig);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
#ifdef DOTIMEOUT
	init_alarm();
	uinfo.mode = LOGIN;
	alarm(LOGIN_TIMEOUT);
#else
	signal(SIGALRM, SIG_SIG);
#endif
	signal(SIGTERM, SIG_IGN);
	signal(SIGURG, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGUSR1, talk_request);

	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;  /* ��׽�����ź�,ִ����handlerʱ��ϵͳ���Զ��������ź� */
	act.sa_handler = r_msg_sig;
	sigaction(SIGUSR2, &act, NULL);
}

void
system_abort(void)
{
	if (started) {
		log_usies("ABORT", currentuser.username);
		u_exit();
	}
	clear();
	refresh();
	prints("лл����, �ǵó���� !\n");
	exit(0);
}

/* ��¼bad login */
void
logattempt(char *uid, char *from)
{
	char fname[STRLEN];
	time_t now;

	now = time(NULL);
	snprintf(genbuf, sizeof(genbuf), "%-12.12s  %24.24s %s\n", uid, ctime(&now), from);
	file_append("reclog/logins.bad", genbuf);
	sethomefile(fname, uid, BADLOGINFILE);
	file_append(fname, genbuf);

}

void
check_tty_lines(void)
{
	static unsigned char telopt[] = { IAC, DO, TELOPT_NAWS };

	write(0, telopt, sizeof(telopt));
}

struct max_log_record {
	int year;
	int month;
	int day;
	int logins;
	unsigned long visit;
} max_log;

void
visitlog(int curr_login_num)
{
	int vfp;
	time_t now;
	struct tm *tm;

	vfp = open(VISITLOG, O_RDWR | O_CREAT, 0644);
	if (vfp == -1) {
		report("Can NOT write visit Log to .visitlog");
		return;
	}
	f_exlock(vfp);
	lseek(vfp, (off_t) 0, SEEK_SET);
	read(vfp, &max_log, (size_t) sizeof(max_log));
	if (max_log.year < 1990 || max_log.year > 2020) {
		now = time(NULL);
		tm = localtime(&now);
		max_log.year = tm->tm_year + 1900;
		max_log.month = tm->tm_mon + 1;
		max_log.day = tm->tm_mday;
		max_log.visit = 0;
		max_log.logins = 0;
	}
	max_log.visit++;
	if (max_log.logins > utmpshm->max_login_num)
		utmpshm->max_login_num = max_log.logins;
	else
		max_log.logins = utmpshm->max_login_num;
	lseek(vfp, (off_t) 0, SEEK_SET);
	write(vfp, &max_log, (size_t) sizeof(max_log));
	f_unlock(vfp);
	close(vfp);

/*
	prints("\033[1;32m�� [\033[36m%4d��%2d��%2d��\033[32m] ��, �ۼ� \033[1;36m%ld\033[1;32m �˴η��ʱ�վ��\033[m\n",
	       max_log.year, max_log.month, max_log.day, max_log.visit);
*/

	prints("\033[1;32mĿǰ��վ����: [\033[1;36m%d/%d\033[1;32m]�� ���������¼: [\033[1;36m%d\033[1;32m]�� Ŀǰ���� \033[1;36m%d\033[1;32m ��ע���ʺš�\033[m\n",
	       curr_login_num, MAXACTIVE, max_log.logins, utmpshm->usersum);
}


void maintaining() {

	if (HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_BOARDS) ||
	    HAS_PERM(PERM_CHATCLOAK)) {
		return;
	}

#define AUTH_HOST	BBSHOME"/etc/auth_host"
	if (!check_host(AUTH_HOST, fromhost, 1)) {
		prints("ϵͳ����ͣ��ά���ڣ�ά��ʱ��Ϊ9��22����10��10��\n");
		oflush();
		sleep(1);
		exit(1);
	}
}

void
login_query(void)
{
	char uid[IDLEN + 3];
	char passbuf[PASSLEN];
	int curr_login_num;
	int attempts;

#ifdef ZSULOGIN
	/* monster: zsu specified login procedure */
/*
	clear();
	outs("\n                  Welcome to Sun Yat-sen (Zhongshan) University\n"
	     "                             Yat-sen Channel BBS\n\n"
	     "                           �� ӭ ݰ �� �� ɽ �� ѧ\n"
	     "                             ���� �� ʱ �ա� BBS\n\n"
	     "               (Login just type bbs or bb5, ���� bbs �� bb5 ��¼)\n\n");

	attempts = 5;
	while (attempts > 0) {
		attempts--;
		do {
			getdata(0, 0, "login: ", uid, IDLEN + 1, DOECHO, YEA);
		} while (uid[0] == '\0');
		if (!strcmp("bbs", uid)) {
			convcode = 0;
			attempts = 1;
			break;
		} else if (!strcmp("bb5", uid)) {
			convcode = 1;
			attempts = 1;
			break;
		}
		prints("Login incorrect\n\n");
	}
	if (!attempts)
		exit(1);
*/
#endif

#ifdef CHECKLOAD
	print_loadmsg();
#endif
	curr_login_num = num_active_users();
	if (curr_login_num >= MAXACTIVE) {
		ansimore("etc/loginfull", NA);
		oflush();
		sleep(1);
		exit(1);
	}
#ifdef BBSNAME
	strcpy(BoardName, BBSNAME);
#else
	ptr = sysconf_str("BBSNAME");
	if (ptr == NULL)
		ptr = "��δ��������վ";
	strcpy(BoardName, ptr);
#endif

	/* ��ʾ��վ���� */
	if (fill_shmfile(1, "etc/issue", "ISSUE_SHMKEY")) {
		show_issue();   /* is anibanner ready, remark this and put * \n\n */
	}
	prints("\033[1;32m��ӭ����\033[1;33m�� %s ��\033[1;32m�� ��վһ�������� \033[1;36m%d\033[1;32m ��ע��ʹ�á�\033[m\n", 
		BoardName, MAXUSERS);
	resolve_utmp();
	if (utmpshm->usersum == 0)
		utmpshm->usersum = allusers(); /* �����û�����*/
	if (utmpshm->max_login_num < curr_login_num)
		utmpshm->max_login_num = curr_login_num;
	visitlog(curr_login_num); /* ����VISITLOG ����ʾ��*/

#ifdef MUDCHECK_BEFORELOGIN
	prints("\033[1;33mΪ��ֹʹ�ó�ʽ��վ���밴 \033[1;36mCTRL + C\033[m : ");
	genbuf[0] = igetkey();
	if (genbuf[0] != Ctrl('C')) {
		prints("\n�Բ����㲢û�а��� CTRL+C ����\n");
		oflush();
		exit(1);
	} else {
		prints("[CTRL] + [C]\n");
	}
#endif

	attempts = 0;
	while (1) {
		if (attempts++ >= LOGINATTEMPTS) {
			ansimore("etc/goodbye", NA);
			oflush();
			sleep(1);
			exit(1);
		}
		prints("���������� `\033[1;36mguest\033[m', ע��������`\033[1;31mnew\033[m', add `\033[1;36m.\033[m' after your ID for BIG5\n");
		getdata(0, 0,
				"\033[1;33m�������ʺ�\033[m: ",
				uid, IDLEN + 2, DOECHO, YEA);

		if( *uid == '\0')
			continue;
		int 	len 	= strlen(uid);
		if( uid[len-1] == '.' ) {
			convcode = 1;
			uid[len-1] = '\0';
		} else 
			convcode = 0;

		if ((strcasecmp(uid, "guest") == 0) &&
		    (MAXACTIVE - curr_login_num < 10)) {
			ansimore("etc/loginfull", NA);
			oflush();
			sleep(1);
			exit(1);
		}
		if (strcasecmp(uid, "new") == 0) {
#if 0
#ifdef AUTHHOST
			if (!(valid_host_mask)) {
				prints("\033[1;37m����½�ĵ�ַδ����֤, �޷�ע��, ����\033[33m guest \033[37m����...\033[m\n");
				continue;
			}
#endif
#endif
			
#ifdef LOGINASNEW
			memset(&currentuser, 0, sizeof(currentuser));
			new_register();
			ansimore3("etc/firstlogin", YEA);
			mail_sysfile("etc/s_fill", currentuser.userid, "��ϲ�������Ѿ����ע��");
			break;
#else
			prints("\033[1;37m��ϵͳĿǰ�޷��� \033[36mnew\033[37m ע��, ����\033[36m guest\033[37m ����...\033[m\n");
#endif
		} else if (*uid == '\0') ;
		else if (!dosearchuser(uid)) { /* ���� currentuser */
			prints("\033[1;31m����֤���޴� ID (User ID Error)...\033[m\n");
		} else if (strcasecmp(uid, "guest") == 0) {
			guestuser = 1;
			currentuser.userlevel = 0;
			break;
#ifdef SYSOPLOGINPROTECT
		} else if (!strcasecmp(uid, "SYSOP") &&
			   strcmp(fromhost, "localhost")
			   && strcmp(fromhost, "127.0.0.1")) {
			prints("\033[1;32m ����: �� %s ��¼�ǷǷ���! ��������!\033[m\n", fromhost);
			prints("[ע��] Ϊ��ȫ�������վ�Ѿ��趨 SYSOP ֻ�ܴ�������½��\n       �����ȷʵ�Ǳ�վ�� SYSOP �����½���� BBS ��������Ȼ��: \n              telnet localhost port.\n");
			oflush();
			sleep(1);
			exit(1);
#endif
		} else {
#ifdef ALLOWSWITCHCODE
			if (!convcode)
				convcode = !(currentuser.userdefine & DEF_USEGB);
#endif
			getdata(0, 0, "\033[1;37m����������: \033[m", passbuf, PASSLEN, NOECHO, YEA);
			if (!checkpasswd2(passbuf, &currentuser)) {
				logattempt(currentuser.userid, fromhost); /* ��¼logins.bad */
				prints("\033[1;31m����������� (Password Error)...\033[m\n");
			} else {
				if (!HAS_ORGPERM(PERM_BASIC) && !HAS_ORGPERM(PERM_SYSOP)) {
					prints("\033[1;32m���ʺ���ͣ�������� \033[36mSYSOP\033[32m ��ѯԭ��\033[m\n");
					oflush();
					sleep(1);
					exit(1);
				}
#ifdef CHECK_SYSTEM_PASS
				if (HAS_ORGPERM(PERM_SYSOP)) {
					if (!check_systempasswd()) {
						prints("\n�������, ����ǩ�� ! !\n");
						oflush();
						sleep(2);
						exit(1);
					}
				}
#endif
				/* password ok, covert to md5 --wwj 2001/5/7 */
				if (currentuser.passwd[0])
					setpasswd(passbuf, &currentuser);
				memset(passbuf, 0, PASSLEN - 1);
				break;
			}
		}
	}

	multi_user_check();
	if (count_ip(fromhost) >= MAXPERIP) {
		if (raw_fromhost[0] != '\0' && count_ip(raw_fromhost) >= MAXPERIP) {
			prints("\n\033[1;32m ����: ���� %s ���û��Ѿ�����! ��������!\033[m\n", fromhost);
			oflush();
			sleep(1);
			abort_bbs();
		} else {
			if (raw_fromhost[0] != '\0')
				strlcpy(fromhost, raw_fromhost, sizeof(fromhost));
		}
	}

	

	login_start_time = time(NULL);
        srandom(login_start_time);

	/* cancel: ��վʱ��������Ϊ30�� */
#ifdef LOGIN_INTERVAL_CONTROL
	if ((login_start_time - currentuser.lastlogin) < 30) {
		if (strcmp(currentuser.userid, "new") && !guestuser && !HAS_PERM(PERM_SYSOP) &&
		    currentuser.numlogins > 4) {
			prints
			    ("\n\033[1;32m��վ���ܡ����� \033[36m30 ��\033[32m ���ٵ�½\033[m");
			oflush();
			sleep(1);
			abort_bbs();
		}
	}
#endif

#ifdef SETPROCTITLE
	setproctitle("%s: user %s (port %d, ppid %d)", "bbsd",
		     currentuser.userid, bbsport, getppid());
#endif

	if (!term_init(currentuser.termtype)) {
		prints("Bad terminal type.  Defaulting to 'vt100'\n");
		strcpy(currentuser.termtype, "vt100");
		term_init(currentuser.termtype);
	}

	check_tty_lines();      /* 2000.03.14 */

	sethomepath(genbuf, currentuser.userid);
	f_mkdir(genbuf, 0755); /* ����homeĿ¼(�����������)��*/

	/* monster: ��½�ɹ������㷵�� */
	if (currentuser.userlevel & PERM_SUICIDE) {
		char filename[PATH_MAX + 1];

		currentuser.userlevel &= ~PERM_SUICIDE; /* �����ɱȨ��λ */
		currentuser.userlevel |= (PERM_MESSAGE | PERM_SENDMAIL);
		if (deny_me_fullsite()) currentuser.userlevel &= ~PERM_POST;
		if (substitute_record(PASSFILE, &currentuser, sizeof(struct userec), usernum) == 0) {
			snprintf(filename, sizeof(filename), "suicide/suicide.%s", currentuser.userid);
			unlink(filename);
		} else {
			report("failed to recover from suicide");
		}
	}
}

void
write_defnotepad(void)
{
	set_safe_record();
	substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
}

void
user_login(void)
{
	char fname[STRLEN];
	FILE *fp;

	if (strcmp(currentuser.userid, "SYSOP") == 0) {
		currentuser.userlevel = ~0;     /* SYSOP gets all permission bits */
		currentuser.userlevel &= ~PERM_SUICIDE; /* monster: remove suicide bit for SYSOP */
		substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	}
	
/* 
#ifdef AUTHHOST
	count_perm_unauth();
#endif
*/
	snprintf(genbuf, sizeof(genbuf), "From: %s", fromhost);
	log_usies("ENTER", genbuf);
	u_enter();
	report("Enter - %s", fromhost);
	started = 1;
	if ((!HAS_PERM(PERM_MULTILOG | PERM_SYSOP)) &&
	    count_self() > MULTI_LOGINS + (HAS_PERM(PERM_OBOARDS) ? 1 : 0) && !guestuser) {
		report("kicked (multi-login)[©��֮��]");
		abort_bbs();
	}
	initscr();
	endline_init(); /* �ѵײ�������Ϣ���빲���ڴ�  */

	if (DEFINE(DEF_QUICKLOGIN)) /* ���ٽ�վ */
		goto skip_screens;

#ifdef USE_NOTEPAD
	if (!guestuser) {
		if (DEFINE(DEF_NOTEPAD)) {
			int noteln;

			noteln = countln("etc/notepad"); /* count lines */
			if (currentuser.noteline == 0) {
				shownotepad();
			} else if ((noteln - currentuser.noteline) > 0) { /* �������� */
				move(0, 0);
				ansimore2("etc/notepad", NA, 0,
					  noteln - currentuser.noteline + 1); /* +1 for the title '���������' */
				igetkey();
				clear();
			}
			currentuser.noteline = noteln;
			write_defnotepad();
		}
	}
#endif
	if (show_statshm("0Announce/bbslist/countusr", 0)) {
		refresh();
		pressanykey();
	}

	if ((vote_flag(NULL, '\0', 2 /* �������µ�Welcome û */ ) == 0)) {
		if (dashf("etc/Welcome")) {
			ansimore("etc/Welcome", YEA);
			vote_flag(NULL, 'R', 2 /* д������µ�Welcome */ );
		}
	}	
/*	} else { */	/* Pudding: ��վ����Ӧ���κ�ʱ����ʾ */
	if (fill_shmfile(3, "etc/Welcome2", "WELCOME_SHMKEY"))
		show_welcomeshm();
/*	} */
	
	show_statshm("etc/posts/day", 1);
	ansimore("etc/weather", NA);
	pressanykey();

	clear();
	move(t_lines - 2, 0);
	if (currentuser.numlogins < 1) {
		currentuser.numlogins = 0;
		getdatestring(time(NULL));
		prints("\033[1;36m�� �������� \033[33m1\033[36m �ΰݷñ�վ�����ס����ɡ�\n");
		prints("�� ����һ�����뱾վ��ʱ��Ϊ \033[33m%s\033[m ",
		       datestring);
	} else {
		getdatestring(currentuser.lastlogin);
		prints("\033[1;36m�� �������� \033[33m%d\033[36m �ΰݷñ�վ���ϴ����Ǵ� \033[33m%s\033[36m ������վ\n",
		       currentuser.numlogins + 1, currentuser.lasthost);
		prints("�� �ϴ�����ʱ��Ϊ \033[33m%s\033[m ",
		       datestring);
	}
	igetkey();

skip_screens:

#ifdef FILTER
	if (!has_filter_inited()) {
		init_filter();
	}
	if (!regex_strstr(currentuser.username))
	{
		clear();
		move(t_lines-2,0);
		prints("����ǳ��к��в��������ݣ������ܷ��ģ��뵽�趨���������н����޸�\n");
		igetkey();
	}
#endif

	setuserfile(fname, BADLOGINFILE);
	if (ansimore(fname, NA) != -1) {
		if (askyn("��Ҫɾ�����������������ļ�¼��", YEA, YEA) == YEA)
			unlink(fname);
	}

	set_safe_record();
	check_uinfo(&currentuser, 0); /* �ǳ�, ������ */
	strlcpy(currentuser.lasthost, raw_fromhost, sizeof(currentuser.lasthost));
	currentuser.lasthost[15] = '\0';        /* dumb mistake on my part */
	if (!(currentuser.flags[0] & CLOAK_FLAG))
		currentuser.lastlogin = time(NULL);
	else {
		setuserfile(fname, "lastlogin");
		if ((fp = fopen(fname, "w+")) != NULL) {
			fprintf(fp, "%d", (int)time(NULL));
			fclose(fp);
		}
	}
	if (currentuser.ident[0] == '\0')
		strlcpy(currentuser.ident, fromhost, sizeof(currentuser.ident)); /*�ʺ�ע���ַ */
#if 0
#ifndef AUTOGETPERM
	if (HAS_PERM(PERM_SYSOP) || guestuser)
		currentuser.lastjustify = time(NULL);
#else
	if (HAS_PERM(PERM_SYSOP) || guestuser
	    || (sysconf_str("REG_EXPIRED_LOCK") != NULL))
		currentuser.lastjustify = time(NULL);
#endif
/* modify by betterman, 06/07/12, ȡ������ע�� */
	/* monster: �����û�ӵ������Ĭ�ϻ���Ȩ��ʱ�ŷ������¸�������˵��֪ͨ */
	if (HAS_ORGPERM_ALL(PERM_DEFAULT) && 
	    (currentuser.lastjustify > 0 &&
	     abs(time(NULL) - currentuser.lastjustify) >= REG_EXPIRED * 86400)) {
#ifdef MAILCHECK 
		currentuser.email[0] = '\0';
#endif 
		currentuser.address[0] = '\0';
		currentuser.userlevel &= ~(PERM_LOGINOK | PERM_PAGE | PERM_MESSAGE);
		mail_sysfile("etc/expired", currentuser.userid, "���¸�������˵����");
	}
#endif
	currentuser.numlogins++;
	if (currentuser.firstlogin == 0) {
		currentuser.firstlogin = time(NULL) - 7 * 86400;
	}
	substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	if (currentuser.numlogins == 1 ) {
		m_activation();	/* ���� */
	}
		
	/* check_register_info(); */
	load_restrict_boards();	 /* board.ctl */
}

/* ����ǩ��������, ���� numofsig ȫ�ֱ��� */
void
set_numofsig(void)
{
	int sigln;
	char signame[STRLEN];

	setuserfile(signame, "signatures");
	sigln = countln(signame);
	numofsig = sigln / MAXSIGLINES;
	if ((sigln % MAXSIGLINES) != 0)
		numofsig += 1;
}

#ifdef CHK_FRIEND_BOOK
int
chk_friend_book()
{
	FILE *fp;
	int idnum, n = 0;
	char buf[STRLEN], *ptr;

	if ((fp = fopen("friendbook", "r")) == NULL)
		return 0;

	move(10, 0);
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		char uid[14];
		char msg[STRLEN];
		struct user_info *uin;

		ptr = strstr(buf, "@");
		if (ptr == NULL)
			continue;
		ptr++;
		strcpy(uid, ptr);
		ptr = strstr(uid, "\n");
		*ptr = '\0';
		idnum = atoi(buf);
		if (idnum != usernum || idnum <= 0)
			continue;
		uin = t_search(uid, NA);
		snprintf(msg, sizeof(msg), "%s �Ѿ���վ��", currentuser.userid);
		if (!uinfo.invisible && uin != NULL && !DEFINE(DEF_NOLOGINSEND)
		    && do_sendmsg(uin, msg, 2, uin->pid) == 1) {
			prints("\033[1m%s\033[m ���㣬ϵͳ�Ѿ�����������վ����Ϣ��\n", uid);
		} else {
			prints("\033[1m%s\033[m ���㣬ϵͳ�޷����絽��������������硣\n", uid);
		}
		n++;
		del_from_file("friendbook", buf);
		if (n > 15) {
			pressanykey();
			move(10, 0);
			clrtobot();
		}
	}
	fclose(fp);
	if (n) {
		move(8, 0);
		prints("\033[1mϵͳѰ�������б�:\033[m");
	}
	return n;
}
#endif

void
start_client(void)
{
        load_sysconf_image("sysconf.img", NA);

#ifdef ALLOWSWITCHCODE
	resolve_GbBig5Files(); /* �ѱ���ת��Ū�������ڴ� */
#endif

	system_init();		/* �����ź���صġ�*/

	if (setjmp(byebye)) {   /* ����env��jmp_buf */
		system_abort(); /* ��jmp�����ġ�*/
	}

	login_query();

	user_login();

	RMSG = NA;
	currboard[0] = 0;

	//clear();
	c_recover();		/* �༭����ҵ�������ж� */
#ifdef TALK_LOG
	tlog_recover();         /* 990713.edwardc for talk_log recover */
#endif

	if (!guestuser) {
		if (check_maxmail())
			pressanykey();
#ifdef CHK_FRIEND_BOOK
		if (chk_friend_book())
			pressanykey();
#endif
		move(9, 0);
		clrtobot();
		if (!DEFINE(DEF_NOLOGINSEND))
			if (!uinfo.invisible)
				apply_ulist(friend_login_wall);	/* ���ͺ�����վ֪ͨ */
		clear();
		set_numofsig(); /* ����ǩ��������, ���� numofsig ȫ�ֱ��� */
	}

	activeboard_init();     /* ��ʼ������� */
	b_closepolls();         /* �ر�ͶƱ */
	num_alcounter();	/* ��վ����and��������*/
	if (count_friends > 0 && DEFINE(DEF_LOGFRIEND))
		t_friends();    /* ��ʾ�������� */

#ifdef BIRTHDAY_POST
	if (is_birth(currentuser) &&
	    currentuser.lastlogout / 86400 < time(NULL) / 86400) {
		char happybirthday[STRLEN];

		snprintf(happybirthday, sizeof(happybirthday), "%s, ���տ���", currentuser.userid);
		postfile("etc/birthday", "notepad", happybirthday, 1);
	}
#endif
	check_system_vote();
	while (1) {
		if (DEFINE(DEF_NORMALSCR)) /* һ��ģʽ��*/
			domenu("TOPMENU");
		else
			domenu("TOPMENU2"); /* ����ģʽ */
		Goodbye();
	}
}

int refscreen = NA;

int
egetch(void)
{
	int rval;

	check_calltime();
	if (talkrequest) {
		talkreply();
		refscreen = YEA;
		return -1;
	}
	while (1) {
		rval = igetkey();
		if (talkrequest) {
			talkreply();
			refscreen = YEA;
			return -1;
		}
		if (rval != Ctrl('L')) /* ��Ļ�ػ� */
			break;
		redoscr();
	}
	refscreen = NA;
	return rval;
}


/* freestyler: ������ʾ������ */
char*
boardmargin(void)
{
	static char buf[STRLEN];

	if (currboard[0] == '\0') {
		brc_initial(DEFAULTBOARD);
		if (!getbnum(currboard))
			setoboard(currboard);
	}

	char	str[STRLEN] = "EGROUP0";
	char* 	boardtitleprefix = "0";
	char 	ch, numzone = '0';

	struct boardheader* bh = getbcache(currboard);
	if ( bh != NULL ) {
		for(ch = '0';  ch <= '9'; ch++ ) {
			str[6] = ch;
			boardtitleprefix = sysconf_str(str);
			if( boardtitleprefix[0] == bh->title[0] ) {
				numzone = ch;
				break;
			}
		}
		if( numzone == '\0') {
			for( ch = 'A'; ch <= 'Z'; ch ++ ) {
				str[6] = ch;
				boardtitleprefix = sysconf_str(str);
				if( boardtitleprefix[0] == bh->title[0] ) {
					numzone = ch;
					break;
				}
			}
		}
	}
	snprintf(buf, sizeof(buf), "%c�� [%s]", numzone, currboard);
	return buf;
}


/*ReWrite by SmallPig*/
void
showtitle(char *title, char *mid)
{
	char *note;
	int spc1, spc2;

	note = boardmargin();
	spc1 = 39 + num_ans_chr(title) - strlen(title) - strlen(mid) / 2;
	if (spc1 < 2) spc1 = 2;
	spc2 = 79 - (strlen(title) - num_ans_chr(title) + spc1 + strlen(note) + strlen(mid));
	if (spc2 < 1) spc2 = 1;

	clear_line(0);
	if (!strcmp(mid, BoardName)) {
		prints("\033[1;44;33m%s%*s\033[37m%s\033[1;44m", title, spc1, "", mid);
	} else if (mid[0] == '[') {
		prints("\033[1;44;33m%s%*s\033[5;36m%s\033[m\033[1;44m", title, spc1, "", mid);
	} else {
		prints("\033[1;44;33m%s%*s\033[36m%s", title, spc1, "", mid);
	}

	prints("%*s\033[33m%s\033[m\n", spc2, "", note);
	update_endline();
	move(1, 0);
}

void
firsttitle(char *title)
{
	char middoc[30];

	if (chkmail(NA)) {
		strcpy(middoc, strstr(title, "�������б�") ? "[�����ż����� M ������]" : "[�����ż�]");
/*	} else if (mailXX == 1) {
 *		strcpy(middoc, "[�ż��������������ż�!]");
 *      } else if (HAS_PERM(PERM_ACCOUNTS) && dashf("new_register")) {
 *      	strcpy(middoc, "[�����û�ע�ᣬ����ˣ�]"); */
	} else {
		strcpy(middoc, BoardName);
	}

	showtitle(title, middoc);
}

void
docmdtitle(char *title, char *prompt)
{
	firsttitle(title);
	move(1, 0);
	clrtoeol();
	outs(prompt);
	clrtoeol();
}

void
c_recover(void)
{
	struct stat st;
	char fname[PATH_MAX + 1], buf[PATH_MAX + 1];
	int a;

	snprintf(fname, sizeof(fname), "home/%c/%s/%s.deadve", mytoupper(currentuser.userid[0]),
		currentuser.userid, currentuser.userid);
	if (guestuser || stat(fname, &st) == -1 || st.st_size <= 0 || !S_ISREG(st.st_mode))
		return;

	int 	res = 0;  /* mail result */
	clear();
	getdata(0, 0,
		"\033[1;32m����һ���༭��ҵ�������жϣ�(S) д���ݴ浵 (M) �Ļ����� (Q) ���ˣ�[M]��\033[m",
		genbuf, 2, DOECHO, YEA);
	switch (genbuf[0]) {
	case 'Q':
	case 'q':
		unlink(fname);
		break;
	case 'S':
	case 's':
		while (1) {
			getdata(2, 0, "\033[1;33m��ѡ���ݴ浵 [0-7] [0]��\033[m", genbuf, 2, DOECHO, YEA);
			a = (genbuf[0] == '\0') ? 0 : atoi(genbuf);
			if (a >= 0 && a <= 7) {
				snprintf(buf, sizeof(buf), "home/%c/%s/clip_%d", mytoupper(currentuser.userid[0]), 
					 currentuser.userid, a);
				if (dashf(buf)) {
					getdata(3, 0,
						"\033[1;31m�ݴ浵�Ѵ��ڣ����ǻ򸽼�? (O)���� (A)���� [O]��\033[m",
						genbuf, 2, DOECHO, YEA);
					switch (genbuf[0]) {
					case 'A':
					case 'a':
						f_cp(fname, buf, O_APPEND);
						unlink(fname);
						break;
					default:
						unlink(buf);
						rename(fname, buf);
						break;
					}
				} else {
					rename(fname, buf);
				}
				break;
			}
		}
		break;
	default:
		mail_file(fname, currentuser.userid, "�����������������Ĳ���...");
		if( res != -1)
			unlink(fname);
		break;
	}
}

#ifdef TALK_LOG
void
tlog_recover(void)
{
	struct stat st;
	char buf[STRLEN];

	sethomefile(buf, currentuser.userid, "talklog");
	if (guestuser || stat(buf, &st) == -1 || st.st_size <= 0 || !S_ISREG(st.st_mode))
		return;

	clear();
	strcpy(genbuf, "");
	getdata(0, 0,
		"\033[1;32m����һ���������������������������¼, ��Ҫ .. (M) �Ļ����� (Q) ���ˣ�[Q]��\033[m",
		genbuf, 2, DOECHO, YEA);

	int	res = 0;  /* mail result */
	if (genbuf[0] == 'M' || genbuf[0] == 'm') {
		res = mail_sysfile(buf, currentuser.userid, "�����¼");
	}
	if( res == 0 ) 
		unlink(buf);
	return;
}
#endif

/* Added by cancel : check system vote */
/* Modified by Pudding, 2005-11-24 */

int
check_system_vote(void)
{
	char ans[8];
	char usercontrol[256];
	struct stat fs;
	time_t last_sysvote;
	time_t curr_sysvote;
	struct votebal lastvote;
	int end;

	strcpy(currboard, DEFAULTBOARD);
	if (!HAS_PERM(PERM_VOTE) || (currentuser.stay < 1800)) return 0;

	/* Check if we should promt for vote */	
	end = get_num_records(BBSHOME"/vote/"SYSTEM_VOTE"/control", sizeof(lastvote));
	if (end <= 0) return 0;
	get_record(BBSHOME"/vote/"SYSTEM_VOTE"/control", &lastvote, sizeof(lastvote), end);
	curr_sysvote = lastvote.opendate;

	setuserfile(usercontrol, "sysvote");
	if (stat(usercontrol, &fs) != 0)
		last_sysvote = 0;
	else last_sysvote = fs.st_mtime;

	if (last_sysvote < curr_sysvote) {
		getdata(t_lines - 1,0,"���µ�ϵͳͶƱ��(Y)ȥ���� (N)�´���˵ (S)����! [N]", ans, 8,
			DOECHO, YEA);
		if (tolower(ans[0] != 'y') && tolower(ans[0] != 's')) return 0;
		utimes(usercontrol, NULL);
		if (tolower(ans[0]) == 'y') x_vote();
	}
	
	return 0;
}

