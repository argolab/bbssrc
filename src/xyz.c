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

#define EXTERN

#include "bbs.h"

int use_define = 0;
int child_pid = 0;

#ifdef ALLOWSWITCHCODE
extern int switch_code(void);
extern int convcode;
#endif

#define TH_LOW	10
#define TH_HIGH	15

void
modify_user_mode(int mode)
{
	if (uinfo.mode != mode) {
		uinfo.mode = mode;
		update_ulist(&uinfo, utmpent);
	}
}

unsigned int
showperminfo(unsigned int pbits, int i, int flag)
{
	move(i + 6 - ((i > 15) ? 16 : 0), 0 + ((i > 15) ? 40 : 0));
	prints("%c. %-30s %2s", 'A' + i,
		(use_define) ? user_definestr[i] : permstrings[i],
		((pbits >> i) & 1 ? "��" : "��"));
	refresh();
	return YEA;
}

unsigned int
showtitleinfo(unsigned int pbits, int i, int flag)
{
	move(i + 6 - ((i > 15) ? 16 : 0), 0 + ((i > 15) ? 40 : 0));
	prints("%c. %-30s %2s", 'A' + i, user_titlestr[i],
		((pbits >> i) & 1 ? "��" : "��"));
	refresh();
	return YEA;
}

unsigned int
setperms(unsigned int pbits, char *prompt, int numbers, unsigned int (*showfunc)(unsigned int, int, int))
{
	int lastperm = numbers - 1;
	int i, done = NA, oldbits = pbits;
	char choice[3], buf[80];

	move(4, 0);
	prints("�밴����Ҫ�Ĵ������趨%s���� Enter ����.\n", prompt);
	move(6, 0);
	clrtobot();
	for (i = 0; i <= lastperm; i++) {
		(*showfunc) (pbits, i, NA);
	}
	while (!done) {
		snprintf(buf, sizeof(buf), "ѡ��(ENTER ����%s): ", ((strcmp(prompt, "Ȩ��") != 0)) ? "" : "��0 ͣȨ");
		getdata(t_lines - 1, 0, buf, choice, 2, DOECHO, YEA);
		if (choice[0] >= 'a' && choice[0] <= 'z')
			choice[0] -= ('a' - 'A');

		choice[0] = toupper((unsigned int)(choice[0]));
		if (choice[0] == '0') {
			return strcmp(prompt, "����") ? 0 : oldbits;
		} else if (choice[0] == '\n' || choice[0] == '\0') {
			done = YEA;
		} else if (choice[0] < 'A' || choice[0] > 'A' + lastperm) {
			bell();
		} else {
			i = choice[0] - 'A';
			pbits ^= (1 << i);
			if ((*showfunc) (pbits, i, YEA) == NA) {
				pbits ^= (1 << i);
			}
		}
	}
	return (pbits);
}

int
x_userdefine(void)
{
	int id;
	unsigned int newlevel;
	struct userec lookupuser;

	modify_user_mode(USERDEF);
	if (!(id = getuser(currentuser.userid, &lookupuser))) {
		clear_line(3);
		outs("�����ʹ���� ID...");
		pressreturn();
		clear();
		return 0;
	}
	move(1, 0);
	clrtobot();
	move(2, 0);
	use_define = 1;
	newlevel = setperms(lookupuser.userdefine, "����", NUMDEFINES, showperminfo);
	move(2, 0);
	if (newlevel == lookupuser.userdefine)
		outs("����û���޸�...\n");
	else {
#ifdef ALLOWSWITCHCODE
		if ((!convcode && !(newlevel & DEF_USEGB))
		    || (convcode && (newlevel & DEF_USEGB)))
			switch_code();
#endif
		set_safe_record();	//Added by cancel at 01.12.30
		lookupuser.userdefine = newlevel;
		currentuser.userdefine = newlevel;
		substitute_record(PASSFILE, &lookupuser, sizeof (struct userec), id);
		renew_uinfo();
		update_utmp();
		outs("�µĲ����趨���...\n\n");
	}
	pressreturn();
	clear();
	use_define = 0;
	return 0;
}

int
x_cloak(void)
{
	modify_user_mode(GMENU);
//	report("toggle cloak");
	uinfo.invisible = (uinfo.invisible) ? NA : YEA;
	update_utmp();
	if (!uinfo.in_chat) {
		clear_line(1);
		prints("������ (cloak) �Ѿ�%s��!", (uinfo.invisible) ? "����" : "ֹͣ");
		pressreturn();
	}
	return 0;
}

int
x_edits(void)
{
	char ans[7], buf[STRLEN];
	int ch, num, confirm;
	static char *e_file[] = { "plans", "signatures", "notes", "logout",
		"blockmail", "autoreply",
		NULL
	};
	static char *explain_file[] =
	    { "����˵����", "ǩ����", "�Լ��ı���¼", "��վ�Ļ���",
		"��ͣ����վ���ʼ�", "�Զ���������",
		NULL
	};

	modify_user_mode(GMENU);
	clear();
	move(1, 0);
	prints("���޸��˵���\n\n");
	for (num = 0; e_file[num] != NULL && explain_file[num] != NULL; num++) {
		prints("[\033[1;32m%d\033[m] %s\n", num + 1, explain_file[num]);
	}
	prints("[\033[1;32m%d\033[m] �������\n", num + 1);

	getdata(num + 5, 0, "��Ҫ������һ����˵���: ", ans, 2, DOECHO, YEA);
	if (ans[0] - '0' <= 0 || ans[0] - '0' > num || ans[0] == '\n' ||
	    ans[0] == '\0')
		return 0;

	ch = ans[0] - '0' - 1;
	if (ch != 5) {
		setuserfile(genbuf, e_file[ch]);
	} else {
		setmailfile(genbuf, e_file[5]);
	}
	move(3, 0);
	clrtobot();
	snprintf(buf, sizeof(buf), "(E)�༭ (D)ɾ�� %s? [E]: ", explain_file[ch]);
	getdata(3, 0, buf, ans, 2, DOECHO, YEA);
	if (ans[0] == 'D' || ans[0] == 'd') {
		confirm = askyn("��ȷ��Ҫɾ���������", NA, NA);
		if (confirm != 1) {
			move(5, 0);
			prints("ȡ��ɾ���ж�\n");
			pressreturn();
			clear();
			return 0;
		}
		unlink(genbuf);
		move(5, 0);
		prints("%s ��ɾ��\n", explain_file[ch]);
		report("delete %s", explain_file[ch]);
		pressreturn();
		clear();
		return 0;
	}
	modify_user_mode(EDITUFILE);
	if (vedit(genbuf, EDIT_MODIFYHEADER) != -1) {
		clear();
		prints("%s ���¹�\n", explain_file[ch]);
		report("edit %s", explain_file[ch]);
		if (!strcmp(e_file[ch], "signatures")) {
			set_numofsig();
			prints("ϵͳ�����趨�Լ��������ǩ����...");
		}
	} else {
		clear();
		prints("%s ȡ���޸�\n", explain_file[ch]);
	}

	pressreturn();
	return 0;
}

int
gettheuserid(int x, char *title, int *id, struct userec *lookupuser)
{
	move(x, 0);
	usercomplete(title, genbuf);
	if (*genbuf == '\0') {
		clear();
		return 0;
	}
	if ((*id = getuser(genbuf, lookupuser)) == 0) {
		clear_line(x + 3);
		prints("�����ʹ���ߴ���");
		pressreturn();
		clear();
		return 0;
	}
	return 1;
}

int
x_lockscreen(void)
{
	char passbuf[PASSLEN + 1] = { '\0' };
	time_t now = time(NULL);

	modify_user_mode(LOCKSCREEN);
	getdatestring(now);

	move(9, 0);
	clrtobot();
	outs("\033[1;37m");
	outs("\n       _       _____   ___     _   _   ___     ___       __");
	outs("\n      ( )     (  _  ) (  _`\\  ( ) ( ) (  _`\\  (  _`\\    |  |");
	outs("\n      | |     | ( ) | | ( (_) | |/'/' | (_(_) | | ) |   |  |");
	outs("\n      | |  _  | | | | | |  _  | , <   |  _)_  | | | )   |  |");
	outs("\n      | |_( ) | (_) | | (_( ) | |\\`\\  | (_( ) | |_) |   |==|");
	outs("\n      (____/' (_____) (____/' (_) (_) (____/' (____/'   |__|\n");
	prints("\n\033[1;36mӫĻ����\033[33m %s\033[36m ʱ��\033[32m %-12s \033[36m��ʱ��ס��...\033[m",
		datestring, currentuser.userid);

	do {
		update_endline();
		getdata(19, 0, "��������������Խ���: ", passbuf, sizeof(passbuf), NOECHO, YEA);
	} while (passbuf[0] == '\0' || !checkpasswd2(passbuf, &currentuser));
	return 0;
}

int
x_lockscreen_silent(void)
{
	int ch;
	char passbuf[PASSLEN + 1];
	int len = 0, old_mode;

	if (uinfo.mode == LOCKSCREEN || guestuser)
		return DONOTHING;

	old_mode = uinfo.mode;
	modify_user_mode(LOCKSCREEN);

	if (old_mode == DIGEST) { 
		update_annendline();
	} else if (old_mode == DIGESTRACE) {
		update_atraceendline();
	} else {
		update_endline();
	}

	while (1) {
		if ((ch = igetkey()) == -1)
			abort_bbs();

		/* monster: code for 'enter': *nix - 10, windows - 13 */
		if (ch == 10 || ch == 13) {
			passbuf[len] = '\0';
			if (checkpasswd2(passbuf, &currentuser))
				break;
			len = 0;
			continue;
		}

		/* monster: filter out control chars */
		if (ch >= 32) {
			passbuf[len] = (unsigned char)ch;
		}

		len = (len + 1) % PASSLEN;
	}

	/* monster: ����ҲҪupdateһ�� */
	if (old_mode != DIGEST && old_mode != DIGESTRACE) {
		update_endline();
	}

	modify_user_mode(old_mode);
	return FULLUPDATE;
}

int
heavyload(void)
{
#ifdef  CHECKLOAD
	static int load;
	static time_t uptime;

	if (time(NULL) > uptime) {
#ifndef OSF
		load = chkload(load ? TH_LOW : TH_HIGH);
#else
		load = chkload(load ? TH_LOW : TH_HIGH, 1);
#endif
		uptime = time(NULL) + load + 45;
	}
	return load;
#else
	return 0;
#endif
}

void
exec_cmd(int umode, int pager, char *cmdfile, char *param1)
{
	char buf[160];
	char *my_argv[18], *ptr;
	int save_pager, i;
	struct sigaction act;

	clear();
	move(2, 0);
	if (num_useshell() > MAX_USESHELL) {
		outs("̫����ʹ���ⲿ��ʽ�ˣ����һ�����ð�...");
		pressanykey();
		return;
	}
	if (!HAS_PERM(PERM_SYSOP) && heavyload()) {
		clear();
		outs("��Ǹ��Ŀǰϵͳ���ɹ��أ��˹�����ʱ����ִ��...");
		pressanykey();
		return;
	}
	if (!dashf(cmdfile)) {
		prints("�ļ� [%s] �����ڣ�\n", cmdfile);
		pressreturn();
		return;
	}

	/* monster: disable pager is not enough, we have to deactive
	   message reciver too */
	signal(SIGUSR2, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	modify_user_mode(umode);

	save_pager = uinfo.pager;
	if (pager == NA) {
		uinfo.pager = 0;
	}
	my_argv[0] = cmdfile;
	strlcpy(buf, param1, sizeof(buf));
	if (buf[0] != '\0') {
		ptr = strtok(buf, " \t");
	} else {
		ptr = NULL;
	}
	for (i = 1; i < 18; i++) {
		if (ptr) {
			my_argv[i] = ptr;
			ptr = strtok(NULL, " \t");
		} else {
			my_argv[i] = NULL;
		}
	}
	child_pid = fork();
	if (child_pid == -1) {
		outs("��Դ��ȱ��fork() ʧ�ܣ����Ժ���ʹ��");
		pressreturn();
	} else {
		if (child_pid == 0) {
#ifdef SETPROCTITLE
			setproctitle("%s: child process of bbsd", cmdfile);
#endif
			/* monster: disable SIGHUP processing which is not necessary for child processes */
			signal(SIGHUP, SIG_DFL);	

			report("execute [%d] - %s %s", getpid(), cmdfile, param1);
			execv(cmdfile, my_argv);
			exit(0);
		} else {
			waitpid(child_pid, NULL, 0);
		}
	}
	child_pid = 0;
	uinfo.pager = save_pager;
	clear();

	/* monster: active message reciver */
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;
	act.sa_handler = r_msg_sig;
	sigaction(SIGUSR2, &act, NULL);
}

int
x_showuser(void)
{
	char buf[STRLEN];

	modify_user_mode(SYSINFO);
	clear();
	stand_title("��վʹ�������ϲ�ѯ");
	ansimore("etc/showuser.msg", NA);
	getdata(20, 0, "Parameter: ", buf, 30, DOECHO, YEA);
	if ((buf[0] == '\0') || dashf("tmp/showuser.result"))
		return 0;
	securityreport("��ѯʹ��������");
	exec_cmd(SYSINFO, YEA, "bin/showuser", buf);
	if (dashf("tmp/showuser.result")) {
		mail_sysfile("tmp/showuser.result", currentuser.userid, "ʹ�������ϲ�ѯ���");
		unlink("tmp/showuser.result");
	}
	return 0;
}

/*
int
get_limit(char *hostname)
{
	FILE *fp;
	char filename[] = "etc/bbsnet_iplimit";
	char buf[STRLEN + 1], *pstr, *temp;
	int DEFAULT = 3;

	if ((fp = fopen(filename, "r")) == NULL) {
		report("Failed to open file %s", filename);
		return (DEFAULT);
	}

	while (fgets(buf, STRLEN, fp) != NULL) {
		if (buf[0] == '#')
			continue;
		if (strstr(buf, hostname) != NULL) {
			if ((pstr = strtok(buf, " \t")) != NULL)
				while ((pstr = strtok((char *) NULL, " \t")) != NULL)
					temp = pstr;
			if (temp != NULL)
				return (atoi(temp));
			else
				break;
		}
	}
	fclose(fp);
	return (DEFAULT);
}
*/

int
bbsnet_allow(void)
{
	int i, count = 0;

	resolve_utmp();
	for (i = 0; i < USHM_SIZE; i++) {
		if (!utmpshm->uinfo[i].active || !utmpshm->uinfo[i].pid)
			continue;
		if (HAS_PERM(PERM_SYSOP))
			return 0;
		if (utmpshm->uinfo[i].uid == uinfo.uid &&
		    utmpshm->uinfo[i].pid != uinfo.pid)
			if (utmpshm->uinfo[i].mode == BBSNET)
				return 1;
		if (!strcmp(utmpshm->uinfo[i].from, uinfo.from))
			if (utmpshm->uinfo[i].mode == BBSNET)
				count++;
/*          if( count>=3 && strcmp ( uinfo.from , "argo.zsu.edu.cn"))
		return 2; */
	}

/*	if (count > get_limit(uinfo.from))
		return 2;
*/
	return 0;
}

int
ent_bnet(void)
{
	int sign;
	char buf[80];

	sign = bbsnet_allow();
	if (sign != 0) {
		clear();

		if (sign == 1) {
			move(12, 28);
			outs("�Բ������Ѿ���һ������ʹ��BBSNET��!\n");
		}

/*
 *		if (sign == 2) {
 *			move(12, 12);
 *			outs("�Բ�������IP�Ѿ�������ʹ�����������ˣ����Ժ�����BBSNET!");
 *		}
 */
		pressanykey();
		return 1;
	}

	snprintf(buf, sizeof(buf), "etc/bbsnet.ini %s %d", currentuser.userid, t_lines - 2);
	exec_cmd(BBSNET, NA, "bin/bbsnet", buf);
	return 0;
}

int
ent_winmine(void)
{
	char buf[80];

	snprintf(buf, sizeof(buf), "%s %s", currentuser.userid, currentuser.lasthost);
	exec_cmd(WINMINE, NA, "bin/winmine", buf);
	return 0;
}

int
ent_worker(void)
{
	char buf[80];

	snprintf(buf, sizeof(buf), "%s %s", currentuser.userid, currentuser.lasthost);
	exec_cmd(WORKER, NA, "bin/worker", buf);
	return 0;
}

int
is_birth(struct userec user)
{
	struct tm *tm;
	time_t now;

	now = time(NULL);
	tm = localtime(&now);

	if (strcasecmp(user.userid, "guest") == 0)
		return NA;

	return (user.birthmonth == (tm->tm_mon + 1) && user.birthday == tm->tm_mday) ? YEA : NA;
}

int fast_cloak(void)
{
	if (!HAS_PERM(PERM_CLOAK)) return 0;
	x_cloak();
	return 1;
}

/* Pudding: ����ָ·����
   Date: 2005-9-23
 */

#define QUERYSTR_LEN		70
#define TOPN			5
#define ZH_POINT	("�������������������������ۣݣ�������������������������������������" \
                        "��������ޣ��������ߣ����ܣ�������������������" \
			"������������������������")
#define MAX_KEY_REPORT		3


#define IS_ZH(ptr)	((*(ptr) & 0x80) != 0)

int
contain_zh(char *word)
{
	for (; *word != '\0' && !IS_ZH(word); word++);
	return (*word != '\0');
}
int
zh_in_set(char *ch, char *set)
{
	for (; *set != '\0'; set += 2)
		if (strncmp(ch, set, 2) == 0) return 1;
	return 0;
}


/* Core Eval Function */
int
keyquery_eval_file(char *filename, int argc, char  **argv, char hit_word[MAX_KEY_REPORT + 1][TITLELEN + 2])
{
	char name[TITLELEN + 2];
	int hit;
	int i, name_len;
	int arg_len[QUERYSTR_LEN];
	int value[QUERYSTR_LEN];
	char *ptr;
	int is_hit;
	int total_top = 0;
	int nhits;
	
	FILE *fp = fopen(filename, "r");
	if (!fp) return 0;

	nhits = 0;
	for (i = 0; i < argc; i++) arg_len[i] = strlen(argv[i]);
	memset(value, 0, sizeof(value));

	while (fscanf(fp, "%s %d", name, &hit) != EOF) {
		total_top += hit;
		name_len = strlen(name);
		is_hit = 0;
		for (i = 0; i < argc; i++) {
			if (!contain_zh(argv[i])) {
				if (strcmp(name, argv[i]) == 0) {
					value[i] = value[i] + hit * name_len;
					is_hit = 1;
				}

				ptr = strstr(name, argv[i]);
				if (ptr && (ptr == name || (ptr - name) + arg_len[i] == name_len)) {
					value[i] = value[i] + ((hit * arg_len[i]) / name_len);
					is_hit = 1;
				}
			} else {			
				if ((ptr = strstr(name, argv[i])) && ((ptr - name) % 2 == 0)) {
					value[i] = value[i] + ((hit * arg_len[i]) / name_len);
					is_hit = 1;
				}
				if ((ptr = strstr(argv[i], name)) && ((ptr - argv[i]) % 2 == 0)) {
					value[i] = value[i] + ((hit * name_len) / arg_len[i]);
//					value[i] = value[i] + hit * arg_len[i];
					is_hit = 1;
				}
			}
		}
		if (is_hit && hit_word && nhits < MAX_KEY_REPORT) {
			strlcpy(hit_word[nhits], name, sizeof(hit_word[nhits]));
			nhits++;
		}
	}
	fclose(fp);
	if (hit_word) hit_word[nhits][0] = '\0';

	double total;
	int total_match = 0;
//	int max_value = 0;
	int ret;
	/* get max value[i] as base */
	for (i = 0; i < argc; i++) {
		if (value[i] > 0) total_match++;
//		if (value[i] > max_value) max_value = value[i];
	}
//	if (max_value > 65535) max_value = 65535; /* -_-||||||||| */
	/* count prop */
	if (total_match > 0) {
		total = 1;
		for (i = 0; i < argc; i++) {
			if (value[i] <= 0) continue;
			total = total * ((double)(value[i]) / (double)total_top);
		}
	} else total = 0;
	/* things to consider
	   1. boards that match more keyword should float up
	   2. boards that has more relate titles should float up
	 */
	ret = total_match * 65536 + (int)(total * total_top);
	
	return ret;
}

char* skip_word[] = {
	"is", "am", "was", "were", "do", "does", "the", "an",
	"you", "he", "she", "we", "us", "it", "that",
	"and", "or", "not",
	"re", "zt", "zz",
	NULL
};

int
x_isskipword(char *word)
{
	int i;	
	if ((*word < 'a' || *word > 'z') &&
	    (*word < 'A' || *word > 'Z'))
		return 0;			/* We don't skip chineses word, and word with Caption start */
	if (strlen(word) < 2) return 1;		/* filter a single char */
	for (i = 0; skip_word[i]; i++)
		if (strcasecmp(word, skip_word[i]) == 0) return 1;
	return 0;
}


int
x_keyquery(void)
{
	struct boardheader *bptr;
	char query[QUERYSTR_LEN * 2];
	char hit_word[MAX_KEY_REPORT + 1][TITLELEN + 2];
	char *query_sect[1000];
	char *ptr;
	int sects, N, top, i, j;
	int index[1024];
	int tmp;
	int len;
	int value[1024];
	char fname[1024];

	/* Input and Pre-Process Part */
	modify_user_mode(DICT);
	clear();
	getdata(1, 0, "��ѯ�ؼ���: ", query, QUERYSTR_LEN - 2, 1, 1);
	
	for (i = 0; query[i] != '\0'; i++)
		if (query[i] >= 'A' && query[i] <= 'Z') query[i] = tolower(query[i]);
	for (ptr = query; *ptr != '\0'; ptr++) {
		if (IS_ZH(ptr)) {
			if (zh_in_set(ptr, ZH_POINT)) {
				*ptr = ' ';
				*(ptr + 1) = ' ';
			}
			ptr++;
			continue;
		}
		if ((*ptr >= 'a' && *ptr <= 'z') ||
		    (*ptr >= 'A' && *ptr <= 'Z') ||
		    (*ptr >= '0' && *ptr <= '9'))
			continue;
		/* Clear special char */
		*ptr = ' ';
	}
	/* �ֿ���ĸ�뺺�� */
	len = strlen(query);
	for (i = 0; i < len; i++) {
		if (query[i] == ' ' || query[i] == '\t') continue;
		if (IS_ZH(query + i)) {
			/* ����abc */
			if (i + 2 < len && !IS_ZH(query + i + 2)) {
				/* Move */
				for (j = len; j >= i + 2; j--)
					query[j + 1] = query[j];
				query[i + 2] = ' ';
				i++;
				len++;
			}
			i++;
		} else {
			if (IS_ZH(query + i + 1)) {
				/* Move */
				for (j = len; j >= i + 1; j--)
					query[j + 1] = query[j];
				query[i + 1] = ' ';
				i++;
				len++;
			}
		}		
	}

	int input_sects = 0;
	sects = 0;
	ptr = strtok(query, " \t\r\n");
	while (ptr && *ptr != '\0') {
		if (!x_isskipword(ptr)) {
			query_sect[sects] = ptr;
			sects++;
		}
		ptr = strtok(NULL, " \t\r\n");
		input_sects++;
	}
	query_sect[sects] = NULL;

	if (sects <= 0) {
		if (input_sects) {
			move(3, 0);
			prints("%s", "����ѯ�Ĺؼ��ֹ��ڳ���, �����޷��ҵ�����Ҫ�İ���\n");
			pressanykey();
		}
		return 0;
	}

	/* Search and Eval Boards */
	N = 0;
	resolve_boards();
	for (i = 0; i < numboards; i++) {
		bptr = &bcache[i];
		if (strcmp(bptr->filename, DEFAULTBOARD) != 0) {
			if (bptr->filename[0] == '\0' ||
			    bptr->flag & BRD_RESTRICT ||
			    bptr->flag & BRD_GROUP ||
//		    bptr->title[0] == '0' ||	/* ����ʾϵͳ������ */
			    bptr->level != 0)
				continue;
		}
		index[N] = i;
		snprintf(fname, sizeof(fname), BBSHOME"/topkey/%s.top", bptr->filename);
		value[N] = keyquery_eval_file(fname, sects, query_sect, NULL);
		N++;
	}

	/* Sort and Output */
	top = (TOPN < N - 1) ? TOPN : (N - 1);
	for (i = 0; i < top; i++) {
		for (j = i + 1; j < N; j++) {
			if (value[i] < value[j]) {
				tmp = value[i];
				value[i] = value[j];
				value[j] = tmp;
				tmp = index[i];
				index[i] = index[j];
				index[j] = tmp;
			}
		}
	}

	if (value[0] <= 0) {
		/* �Ҳ������Ƽ��İ��� */
		move(3, 0);
		prints("�Բ���, ���˲�֪���������Ƽ���Щ����\n");
		prints("����ϵ�ϵ� kyhpudding.bbs@bbs.zsu.edu.cn �Ľ��������\n");
	} else {
		move(3, 0);
		prints("�����Ƽ����°���, ���ȶ��ɸߵ���\n\n");
		for (i = 0; i < top; i++) {
			if (value[i] <= 0) break;
			prints("%2d. %-16s%s\n", i + 1, bcache[index[i]].filename, bcache[index[i]].title + 1);
			snprintf(fname, sizeof(fname), BBSHOME"/topkey/%s.top", bcache[index[i]].filename);
			keyquery_eval_file(fname, sects, query_sect, hit_word);

			prints("%s", "    �ؼ���: ");
			for (j = 0; j < MAX_KEY_REPORT && hit_word[j][0] != '\0'; j++)
				prints("%s ", hit_word[j]);
			prints("\n\n");
		}
		prints("----------------------------------------------------------------------\n");
		prints("                                                   Powered by Argo ^_^\n");
	}

	pressanykey();
	return 0;
}
