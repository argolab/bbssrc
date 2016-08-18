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

extern time_t now;
#ifdef AUTHHOST
extern unsigned int valid_host_mask;
#endif

#ifdef ALLOWSWITCHCODE
extern int convcode;
#endif

int
bad_user_id(char *userid)
{
	FILE *fp;
	char *ptr;
	char pattern[1024];

	if ((fp = fopen("etc/bad_id", "r")) != NULL) {
#ifdef FNM_CASEFOLD
		while (fgets(pattern, sizeof(pattern), fp)) {
			if (pattern[0] == '#')
				continue;

			if ((ptr = strrchr(pattern, '\n')) != NULL)
				*ptr = '\0';

			if (fnmatch(pattern, userid, FNM_CASEFOLD) == 0) {
				fclose(fp);
				return YEA;
			}	
		}
#else
		char lpattern[1024], luserid[1024];

		strtolower(luserid, userid);
		while (fgets(pattern, sizeof(pattern), fp)) {
			if (pattern[0] == '#')
				continue;

			if ((ptr = strrchr(pattern, '\n')) != NULL)
				*ptr = '\0';

			strtolower(lpattern, pattern);
			if (fnmatch(lpattern, luserid, 0) == 0) {
				fclose(fp);
				return YEA;
			}	
		}		
#endif
		fclose(fp);
	}
	return NA;
}

int
id_with_nonalphabet(char *userid)
{
	char *ptr;

	for (ptr = userid; *ptr != '\0'; ptr++) {
		if ((*ptr >= 'A' && *ptr <= 'Z') || (*ptr >='a' && *ptr <= 'z'))
			continue;
		return YEA;
	}
	return NA;
}

#ifdef ALLOW_CHINESE_ID
int
bad_chinese_id(char *user_id)
{
	int i;
	int len = strlen(user_id);

	if (len % 2)
		return 1;
	if (len == 2)
		return 2;
	for (i = 0; i < len; i += 2)
		if (user_id[i] > 0)
			return 3;
	return 0;
}

int
bad_id(char *user_id)
{
	return (user_id[0] < 0) ?
		(bad_chinese_id(user_id)) : (id_with_nonalphabet(user_id) || bad_user_id(user_id));
}
#endif

int
valid_ident(char *ident)
{
	static char *invalid[] = {
		"unknown@", "root@", "gopher@", "bbs@",
		"guest@", "nobody@", "www@", NULL
	};
	int i;

	if (ident[0] == '@')
		return 0;
	for (i = 0; invalid[i]; i++)
		if (strstr(ident, invalid[i]) != NULL)
			return 0;
	return 1;
}

int
compute_user_value(struct userec *urec)
{
	int value;

	if (urec->userlevel & PERM_XEMPT || !strcmp(urec->userid, "guest") || !strcmp(urec->userid, "SYSOP"))
		return 999;

	if (urec->numlogins == 0)
		return -1;

	value = (time(NULL) - urec->lastlogin) / 60;

	if (urec->userlevel & PERM_SUICIDE) {
		value = (3 * 1440 - value) / 1440;	/* monster: ��ɱ�����ӵ������������ */
	} else if (urec->numlogins <= 3 && !(urec->userlevel & PERM_WELCOME)) {
		value = (15 * 1440 - value) / 1440;
	} else if (!(urec->userlevel & PERM_LOGINOK)) {
		value = (30 * 1440 - value) / 1440;
	} else if (urec->stay > 1000000) {
		value = (365 * 1440 - value) / 1440;
	} else {
		value = (120 * 1440 - value) / 1440;
	}

	if (value >= 0)
		return value;

	if (urec->userlevel & PERM_BOARDS)	/* monster: ������������ڼ����� */
		return 0;
	else
		return (-1);
}

int
getnewuserid(void)
{
	struct userec utmp;

	struct stat st;
	int fd, i;

	if ((fd = open(PASSFILE, O_RDWR | O_CREAT, 0600)) == -1)
		return -1;
	f_exlock(fd);
	i = searchnewuser();
	snprintf(genbuf, sizeof(genbuf), "uid %d from %s", i, fromhost);
	log_usies("APPLY", genbuf);
	if (i <= 0 || i > MAXUSERS) {
		int wait_min = 60;
		if (fstat(fd, &st) == 0) {
			wait_min = (st.st_mtime - time(NULL) + 3660) / 60;
		}
		f_unlock(fd);
		close(fd);
		prints("��Ǹ, ʹ�����ʺ��Ѿ�����, �޷�ע���µ��ʺ�.\n\r");
		prints("��ȴ� %d ���Ӻ�����һ��, ף������.\n\r", wait_min);
		sleep(2);
		exit(1);
	}
	memset(&utmp, 0, sizeof(utmp));
	strcpy(utmp.userid, "new");
	utmp.lastlogin = time(NULL);
	if (lseek(fd, (off_t)(sizeof(utmp) * (i - 1)), SEEK_SET) == -1) {
		f_unlock(fd);
		close(fd);
		return -1;
	}
	write(fd, &utmp, sizeof (utmp));
	setuserid(i, utmp.userid);
	f_unlock(fd);
	close(fd);

	return i;
}

void
new_register(void)
{
	struct userec newuser;
	char passbuf[STRLEN];
	int allocid, try, fd;

	if (dashf("etc/NOREGISTER")) {
		ansimore("etc/NOREGISTER", NA);
		pressreturn();
		exit(1);
	}
	ansimore("etc/register", NA);
	try = 0;
	while (1) {
		if (++try >= 9) {
			prints("\n�ݰݣ���̫����  <Enter> ��...\n");
			oflush();
			longjmp(byebye, -1);
		}
		getdata(0, 0, "�������ʺ����� (Enter User ID, leave blank to abort): ",
			passbuf, IDLEN + 1, DOECHO, YEA);
		if (passbuf[0] == '\0') {
			longjmp(byebye, -1);
		}

#ifndef ALLOW_CHINESE_ID
		if (id_with_nonalphabet(passbuf)) {
			prints("�ʺű���ȫΪӢ����ĸ!\n");
			continue;
		} else if (strlen(passbuf) < 2) {
			prints("�ʺ�������������Ӣ����ĸ!\n");
			continue;
		} else if ((*passbuf == '\0') || bad_user_id(passbuf)) {
			prints("��Ǹ, ������ʹ�� %s ��Ϊ�ʺš� �����⡣\n", passbuf);
			continue;
		}
#else
		if (strlen(passbuf) < 2) {
			prints("�ʺ��������������ַ�!\n");
			continue;
		} else if ((*passbuf == '\0') || bad_id(passbuf)) {
			prints("��Ǹ, ������ʹ�� %s ��Ϊ�ʺš� �����⡣\n", passbuf);
			continue;
		}
#endif
		if ((fd = filelock("ucache.lock", YEA)) == -1) {
			prints("ϵͳ�����ڲ�����...\n");
			oflush();
			longjmp(byebye, -1);
		}

		if (dosearchuser(passbuf)) {
			prints("���ʺ��Ѿ�����ʹ��\n");
			close(fd);
			continue;
		}

		close(fd);
		break;
	}
	

	memset(&newuser, 0, sizeof (newuser));
	allocid = getnewuserid();
	if (allocid > MAXUSERS || allocid <= 0) {
		prints("ϵͳ�޷�Ϊ���˺ŷ���ռ�, ����ϵͳά����ϵ\n\r");
		oflush();
		longjmp(byebye, -1);
	}
	strcpy(newuser.userid, passbuf);

	try = 0;
	passbuf[0] = 0;
	while (1) {
		if (!strcmp(newuser.userid, "guest")) {
			guestuser = 1;
			break;
		}

		if (++try > 3) {
			prints("�����������������.\n");
			oflush();
			longjmp(byebye, -1);
		}

		getdata(0, 0, "���趨�������� (Setup Password): ", passbuf, PASSLEN, NOECHO, YEA);
		if (strlen(passbuf) < 4 || !strcmp(passbuf, newuser.userid)) {
			prints("����̫�̻���ʹ���ߴ�����ͬ, ����������\n");
			continue;
		}
		strlcpy(newuser.passwd, passbuf, PASSLEN);
		getdata(0, 0, "��������һ���������� (Reconfirm Password): ",
			passbuf, PASSLEN, NOECHO, YEA);
		if (strncmp(passbuf, newuser.passwd, PASSLEN) != 0) {
			prints("�����������, ��������������.\n");
			continue;
		}

		setpasswd(passbuf, &newuser);
		break;
	}
	strlcpy(newuser.ident, fromhost, sizeof(newuser.ident));	//ע���½�� deardragon 2001/03/16
	strcpy(newuser.termtype, "vt100");
	newuser.gender = 'X';
	newuser.userdefine = -1;
	if (guestuser) {
		newuser.userlevel = 0;
		newuser.userdefine &=
		    ~(DEF_FRIENDCALL | DEF_ALLMSG | DEF_FRIENDMSG |
		      DEF_NOENDLINE | DEF_NOANSI | DEF_QUICKLOGIN);
	} else {
		newuser.userlevel = PERM_BASIC;
		newuser.flags[0] = PAGER_FLAG;
		newuser.userdefine &=
		    ~(DEF_NOLOGINSEND | DEF_NOENDLINE | DEF_NOANSI |
		      DEF_QUICKLOGIN | DEF_RANDSIGN | DEF_NOTHIDEIP |
		      DEF_DELDBLCHAR);
	}

#ifdef ALLOWSWITCHCODE
	if (convcode)
		newuser.userdefine &= ~DEF_USEGB;
#endif
	newuser.flags[1] = 0;
	newuser.firstlogin = newuser.lastlogin = time(NULL);
	if (substitute_record(PASSFILE, &newuser, sizeof (newuser), allocid) == -1) {
		prints("too much, good bye!\n");
		oflush();
		sleep(2);
		exit(1);
	}
	setuserid(allocid, newuser.userid);
	if (!dosearchuser(newuser.userid)) {
		prints("User failed to create\n");
		oflush();
		sleep(2);
		exit(1);
	}

	/* monster: �����������û�����������Ŀ¼ */
	clear_userdir(newuser.userid);

	report("new account");
}

int
invalid_email(char *addr)
{
	FILE *fp;
	char temp[STRLEN], tmp2[STRLEN], *p;

	if (strlen(addr) < 3)
		return 1;

	/* monster: �ж�email��ַ���Ƿ�������ʼ���������ַ */
	p = strchr(addr, '@');
	if ((p == NULL) || (*(++p) == '\0'))
		return 1;

	strtolower(tmp2, addr);
	if (strstr(tmp2, "bbs") != NULL)
		return 1;

	if ((fp = fopen("etc/bad_email", "r")) != NULL) {
		while (fgets(temp, STRLEN, fp) != NULL) {
			strtok(temp, "\n");
			strtolower(genbuf, temp);
			if (strstr(tmp2, genbuf) != NULL ||
			    strstr(genbuf, tmp2) != NULL) {
				fclose(fp);
				return 1;
			}
		}
		fclose(fp);
	}
	return 0;
}

int
check_register_ok(void)
{
	FILE *fn;
	char fname[PATH_MAX + 1];

	sethomefile(fname, currentuser.userid, "register");
	if ((fn = fopen(fname, "r")) != NULL) {
		fgets(genbuf, STRLEN, fn);
		fclose(fn);
		strtok(genbuf, "\n");
		if (valid_ident(genbuf) && ((strchr(genbuf, '@') != NULL) || strstr(genbuf, "usernum"))) {
			move(t_lines - 3, 0);
			outs("������!! ����˳����ɱ�վ��ʹ����ע������,\n"
			     "������������ӵ��һ��ʹ���ߵ�Ȩ��������...");
			pressanykey();
			return 1;
		}
	}
	return 0;
}

#ifdef MAILCHECK
#ifdef CODE_VALID
char *
genrandpwd(int seed)
{
	static char panel[] =
	    "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char *result;
	int i, rnd;

	result = (char *)malloc(RNDPASSLEN + 1);
	srand((unsigned)(time(NULL) * seed));
	memset(result, 0, RNDPASSLEN + 1);
	for (i = 0; i < RNDPASSLEN; i++) {
		rnd = rand() % sizeof(panel);
		if (panel[rnd] == '\0') {
			i--;
			continue;
		}
		result[i] = panel[rnd];
	}
	sethomefile(genbuf, currentuser.userid, ".regpass");
	unlink(genbuf);
	file_append(genbuf, result);
	return result;
}
#endif

void
send_regmail(struct userec *trec)
{
	FILE *fin, *fout/*, *dp*/;
	time_t code = time(NULL);
	pid_t pid = getpid();

#ifdef CODE_VALID
	char *buf;
#endif

/*
	sethomefile(genbuf, trec->userid, "mailcheck");
	if ((dp = fopen(genbuf, "w")) == NULL)
		return;
	fprintf(dp, "%9.9d:%d\n", (int)code, (int)pid);
	fclose(dp);
*//* Canceled by betterman 06.07 */
	if ((fin = fopen("etc/mailcheck", "r")) == NULL)
		return;

	snprintf(genbuf, sizeof(genbuf), "%s -f new.bbs@%s %s", SENDMAIL, BBSHOST, trec->email);
	if ((fout = popen(genbuf, "w")) == NULL) {
		fclose(fin);
		return;
	}

#ifdef CODE_VALID
	buf = genrandpwd(pid);
#endif

	fprintf(fout, "Reply-To: SYSOP.bbs@%s"CRLF, BBSHOST);
	fprintf(fout, "From: SYSOP.bbs@%s"CRLF, BBSHOST);
	fprintf(fout, "To: %s"CRLF, trec->email);
	fprintf(fout, "Subject: @%s@[-%9.9d:%d-]%s mail check."CRLF, trec->userid, (int)code, (int)pid, BBSID);
	fprintf(fout, "X-Purpose: %s registration mail."CRLF, BBSNAME);
	fprintf(fout, "X-Priority: 1 (Highest)"CRLF);
	fprintf(fout, "X-MSMail-Priority: High"CRLF);
	fputs(CRLF, fout);

	fprintf(fout, "[����]"CRLF);
	fprintf(fout, "BBS ��ַ           : %s (%s)"CRLF, BBSHOST, BBSIP);
	fprintf(fout, "��ע��� BBS ID    : %s"CRLF, trec->userid);
	fprintf(fout, "��������           : %s", ctime(&trec->firstlogin));
	fprintf(fout, "������Դ           : %s"CRLF, fromhost);
	fprintf(fout, "������ʵ����/�ǳ�  : %s (%s)"CRLF, trec->realname, trec->username);
#ifdef CODE_VALID
	fprintf(fout, "��֤����           : %s (��ע���Сд)"CRLF, buf);
#endif
	fprintf(fout, "��֤�ŷ�������     : %s"CRLF, ctime(&code));

	fprintf(fout, "[English]"CRLF);
	fprintf(fout, "BBS LOCATION       : %s (%s)"CRLF, BBSHOST, BBSIP);
	fprintf(fout, "YOUR BBS USER ID   : %s"CRLF, trec->userid);
	fprintf(fout, "APPLICATION DATE   : %s", ctime(&trec->firstlogin));
	fprintf(fout, "LOGIN HOST         : %s"CRLF, fromhost);
	fprintf(fout, "YOUR NICK NAME     : %s"CRLF, trec->username);
	fprintf(fout, "YOUR NAME          : %s"CRLF, trec->realname);
#ifdef CODE_VALID
	fprintf(fout, "AUTHENTICATION CODE: %s (case sensitive)"CRLF, buf);
#endif
	fprintf(fout, "THIS MAIL SENT ON  : %s"CRLF, ctime(&code));

	while (fgets(genbuf, 255, fin) != NULL) {
		if (genbuf[0] == '.' && genbuf[1] == '\n') {
			fputs(". "CRLF, fout);
		} else {
			char *ptr;

			/* monster: convert tailing '\n' into '\r\n' (CRLF) */
			if ((ptr = strrchr(genbuf, '\n')) != NULL) {
				*ptr = '\0';
				fprintf(fout, "%s"CRLF, genbuf);
			} else {
				fputs(genbuf, fout);
			}
		}
	}
	fprintf(fout, ".\n");
	fclose(fin);
	fclose(fout);
}
#endif

#ifndef AUTHHOST
void
check_register_info(void)
{
	struct userec *urec = &currentuser;
	char buf[PATH_MAX + 1];

#if defined(NEWCOMERREPORT) || defined(CODE_VALID)
	FILE *fp;
#endif

#ifdef MAILCHECK
	char ans[4];
#endif

	if (!(urec->userlevel & PERM_BASIC)) {
		urec->userlevel = 0;
		return;
	}

#ifdef NEWCOMERREPORT
	if (urec->numlogins == 1 && abs(time(NULL) - urec->firstlogin) < 86400) {
		snprintf(buf, sizeof(buf), "tmp/newcomer.%s", currentuser.userid);
		if ((fp = fopen(buf, "w")) != NULL) {
			int i;

			fprintf(fp, "��Һ�,\n\n");
			fprintf(fp, "���� %s (%s), ���� %s\n", currentuser.userid, urec->username, fromhost);
			fprintf(fp, "����%s������վ����, ���Ҷ��ָ�̡�\n", (urec->gender == 'M') ? "С��" : "СŮ��");

			clear();
			move(2, 0);
			prints("�ǳ���ӭ %s ���ٱ�վ��ϣ�������ڱ�վ�ҵ������Լ���һƬ��գ�\n\n", currentuser.userid);
			outs("����������̵ĸ��˼��, ��վ����ʹ���ߴ���к�\n");
			outs("(����������, д���ֱ�Ӱ� <Enter> ����)....");

			for (i = 6; i < 9; i++) {
				getdata(i, 0, ":", genbuf, 75, DOECHO, YEA);
				if (genbuf[0] == '\0') break;
				fprintf(fp, "%s\n", genbuf);
			}
			fclose(fp);

			snprintf(genbuf, sizeof(genbuf), "������·: %s", urec->username);
			postfile(buf, "newcomers", genbuf, 2);
			unlink(buf);
		}
	}
#endif
#ifdef PASSAFTERTHREEDAYS
	if (urec->lastlogin - urec->firstlogin < 3 * 86400) {
		if (!HAS_PERM(PERM_SYSOP)) {
			set_safe_record();
			urec->userlevel = PERM_BASIC;
			substitute_record(PASSFILE, urec, sizeof(struct userec), usernum);
			ansimore("etc/newregister", YEA);
			return;
		}
	}
#endif

	clear();
	if (HAS_PERM(PERM_LOGINOK))
		return;
#ifndef AUTOGETPERM
	if (check_register_ok()) {
#endif
		set_safe_record();
		urec->lastjustify = time(NULL);
		urec->userlevel |= PERM_DEFAULT;
		if (deny_me_fullsite()) urec->userlevel &= ~PERM_POST;
		substitute_record(PASSFILE, urec, sizeof(struct userec), usernum);
		return;
#ifndef AUTOGETPERM
	}
#endif

#ifdef MAILCHECK
#ifdef CODE_VALID
	sethomefile(buf, currentuser.userid, ".regpass");
	if (dashf(buf)) {
		int i = 3;

		move(13, 0);
		outs("����δͨ�����ȷ��... \n");
		outs("�����ڱ�������ע��ȷ������, \"��֤����\"����Ϊ���ȷ��\n");
		prints("һ���� %d ���ַ�, ��Сд���в���, ��ע��.\n", RNDPASSLEN);
		outs("����ȡ�������������� [Enter] ��.\n");
		outs("\033[1;33m��ע��, ����������һ����֤�������������������룡\033[m\n");
		if ((fp = fopen(buf, "r")) != NULL) {
			char code[1024], *ptr;

			if (fgets(code, sizeof(code), fp) != NULL) {
				if ((ptr = strrchr(code, '\n')) != NULL)
					*ptr = '\0';

				for (i = 0; i < 3; i++) {
					move(18, 0);
					prints("������ %d �λ���\n", 3 - i);
					getdata(19, 0, "��������֤����: ", genbuf, (RNDPASSLEN + 1), DOECHO, YEA);
					if (strcmp(genbuf, code) == 0)
						break;
				}
			}
			fclose(fp);
		}

		if (i == 3) {
			outs("������֤ʧ��! ����Ҫ��дע�ᵥ�����ȷ������ȷ���������");
			getdata(t_lines - 2, 0,
				"��ѡ��1.��ע�ᵥ 2.����ȷ���� [1]:", ans, 2,
				DOECHO, YEA);
			if (ans[0] == '2') {
				send_regmail(&currentuser);
				pressanykey();
			} else {
				unlink(buf);	/* Pudding: ѡ����дע�ᵥ������֤���� */
				x_fillform();
			}
		} else {
			set_safe_record();
			urec->lastjustify = time(NULL);
			urec->userlevel |= PERM_DEFAULT;
			if (deny_me_fullsite()) urec->userlevel &= ~PERM_POST;
			strlcpy(urec->reginfo, urec->email, sizeof(urec->reginfo));
			
			if (substitute_record(PASSFILE, urec, sizeof (struct userec), usernum) == -1) {
				outs("�ܱ�Ǹ��ϵͳ���ϣ�������˳�����ע������������ϵ��վϵͳά����");
			} else {
				outs("������!! ����˳����ɱ�վ��ʹ����ע������,\n");
				outs("������������ӵ��һ��ʹ���ߵ�Ȩ��������...");
				mail_sysfile("etc/smail", currentuser.userid, "��ӭ���뱾վ����");
			}
			unlink(buf);
			pressanykey();
		}
		return;
	}	
#endif
	if ((!strstr(urec->email, BBSHOST)) && (!invalidaddr(urec->email)) && (!invalid_email(urec->email))) {

#ifdef CHECK_ZSUMAIL
		if (strstr(urec->email, "@student.sysu.edu.cn") || strstr(urec->email, "@mail.sysu.edu.cn") ||
		    strstr(urec->email, "@mail2.sysu.edu.cn") || strstr(urec->email, "@mail3.sysu.edu.cn")) {

#endif
			move(13, 0);			
			outs("���ĵ������� ����ͨ��������֤...  \n");
			outs("    ��վ�����ϼ�һ����֤�Ÿ���,\n");			
			prints("    ��ֻҪ�� %s ����, �Ϳ��Գ�Ϊ��վ�ϸ���.\n\n", urec->email);
			outs("    ��Ϊ��վ�ϸ���, �������и����Ȩ���!\n");			
			outs("    ��Ҳ����ֱ����дע�ᵥ��Ȼ��ȴ�վ�����ֹ���֤��\n");
			getdata(t_lines - 3, 0, "��ѡ��1.��ע�ᵥ 2.��ȷ���� [1]: ", ans, 2, DOECHO, YEA);
			if (ans[0] == '2') {
				send_regmail(&currentuser);
				getdata(21, 0, "ȷ�����Ѽĳ�, ��������Ŷ!! ", ans, 2, DOECHO, YEA);
				return;
			}
#ifdef CHECK_ZSUMAIL
		}
#endif
	}
#endif
	x_fillform();
}
#endif

/* monster: ɾ���û��������ʼ�Ŀ¼ */
void
clear_userdir(char *userid)
{
	char tmpstr[PATH_MAX + 1];

	if (userid == NULL || userid[0] == '\0')
		return;

#ifndef ALLOW_CHINESE_ID
	if (!isalpha(userid[0]))
		return;
#endif

	setmailpath(tmpstr, userid);
	f_rm(tmpstr);
	sethomepath(tmpstr, userid);
	f_rm(tmpstr);
}

/* <- Added by betterman 06.07 -> */
int
countmails(void *uentp_ptr, int unused)
{
	static int totalusers;
	struct userec *uentp = (struct userec *)uentp_ptr;

	if (uentp == NULL) {
		int c = totalusers;

		totalusers = 0;
		return c;
	}
	if (uentp->userid[0] != '\0' && 
	    uentp->userlevel & (PERM_WELCOME) && 
	    strcmp(uentp->reginfo, genbuf) == 0)
		totalusers++;
	return 0;
}
/* gcc: 2011 */
int
countnetids(void *uentp_ptr, int unused)
{
	static int totalusers;
	struct userec *uentp = (struct userec *)uentp_ptr;

	if (uentp == NULL) {
		int c = totalusers;

		totalusers = 0;
		return c;
	}
	char *at = strchr(uentp->reginfo, '@');
	int n = (at == NULL) ? strlen(genbuf) : at - uentp->reginfo;

	if (uentp->userid[0] != '\0' && 
	    uentp->userlevel & (PERM_WELCOME) && 
	    strncmp(uentp->reginfo, genbuf, n) == 0)
		totalusers++;
	return 0;
}


int multi_mail_check(char *email)
{
	strcpy(genbuf,email);
	countmails(NULL, 0);
	if (apply_record(PASSFILE, countmails, sizeof (struct userec)) == -1) {
		return 0;
	}
	return countmails(NULL, 0);	
}

int multi_netid_check(char *netid)
{
	strcpy(genbuf, netid);
	countnetids(NULL, 0);
	if (apply_record(PASSFILE, countnetids, sizeof (struct userec)) == -1) {
		return 0;
	}
	return countnetids(NULL, 0);
}



#ifdef AUTHHOST
int
get_code(struct userec *trec)
{
	char ans[4];
	char username[NAMELEN]; /* because I don��t know the most length of mail name */
	char email[STRLEN - 12];
	int ch, num;

	static char *mail[] =
	{
		"mail.sysu.edu.cn",
		"mail2.sysu.edu.cn",
		"mail3.sysu.edu.cn",
		"student.sysu.edu.cn",
		NULL
	};

	clear();
	outs("����ѡ��������ĺ�׺��Ȼ��ֻ�����û�������\n���磺mn07xz@��������mn07xz����\n");
	move(2,0);
	for (num = 0; mail[num] != NULL ;
	     num++) {
		prints("[\033[1;32m%2d\033[m] %s", num + 1,
		       mail[num]);
		if (num < 17)
			move(3 + num, 0);
		else
			move(num - 15, 50);
	}
	getdata(7, 0,
          	"��ѡ��: ", ans, 2,
         	DOECHO, YEA);
	ch = atoi(ans);
	if (!isdigit(ans[0]) || ch <= 0 || ch > num || ans[0] == '\n' ||
	    ans[0] == '\0')
	{
		presskeyfor("����ѡ��");
		return -1;
	}
	ch -= 1;

	do {
		getdata(8, 0, "����������û���: ", username, NAMELEN, DOECHO, YEA);
		sprintf(email, "%s@%s", username,mail[ch]);
	} while (username[0] & invalid_email(email));

	if (!username[0]) {
		presskeyfor("ȡ��...");
		sleep(1);
		return 0;		
	}

	prints("���������� %s \n",email);
       	if (askyn("ȷ��Ҫ�ĳ���", NA, YEA) == NA) {
          		pressanykey();
		return 0;
        }	

	clear();
	set_safe_record();
	strlcpy(trec->email,email,sizeof(email));
	if (substitute_record(PASSFILE, trec, sizeof (struct userec), usernum) == -1) {
		presskeyfor("ϵͳ��������ϵϵͳά��Ա\n");
		return -1;
	}

	if(multi_netid_check(username) >= MAXMAIL){
		presskeyfor("\n\033[1;32m ����: ��ǰ�����Ѿ��������! ��������!\033[m\n");
		return 0;
	}

	send_regmail(trec);
	presskeyfor("���ļ������Ѿ��ĳ�\n");
	return 1;	
}

int m_activation()
{
  	struct userec *urec = &currentuser;
	char ans[4];
  	FILE *fp;
  	char buf[PATH_MAX + 1], secu[STRLEN];
  	int i = 0;
	int fore_user = (urec->userlevel != PERM_BASIC); /* �����¾��û� */
  	if (guestuser)
    		return -1;

  	modify_user_mode(NEW);
	if (dashf("etc/Activation"))
		ansimore("etc/Activation", YEA);
  	clear();
  	move(2, 0);
  	clrtobot();
  	outs("���������ʺ�\n");
  	if(HAS_PERM(PERM_WELCOME))
    	{
      		presskeyfor("�����ʺ��Ѿ�����");
      		return 0;
    	}
	getdata(3, 0,
		"��ѡ��: 1.��ȡ��֤�� 2.������֤�� 3.У�������Զ���֤ : ", 
		ans, 2, DOECHO, YEA);
		
	switch (ans[0]) {
	case '1':
		sethomefile(buf, currentuser.userid, ".regpass");
		unlink(buf);
    		get_code(&currentuser);
		break;
	case '2':
		sethomefile(buf, currentuser.userid, ".regpass");
    		if (dashf(buf)) {			
      			if ((fp = fopen(buf, "r")) != NULL) {
        			char code[1024], *ptr;
		
        			if (fgets(code, sizeof(code), fp) != NULL) {
          				if ((ptr = strrchr(code, '\n')) != NULL)
            					*ptr = '\0';

          				for (i = 0; i < 3; i++) {
            				move(4, 0);
            				prints("���� %d �λ���ȥ����",3 - i);
            				getdata(5, 0, "������֤��: ", genbuf, (RNDPASSLEN + 1), DOECHO, YEA);
					trim(genbuf);
            				if (strcmp(genbuf, code) == 0)
             					break;
         				}
        			}
        			fclose(fp);
      			}
      			if(i == 3){
						clear();
        				outs("\033[1;31m������֤�����\033[0m");
	        			if (askyn("���»�ȡ��֤��", NA, YEA) == YEA) {
					unlink(buf);
          					get_code(&currentuser);
          					pressanykey();
        				} else {
          					pressanykey();
        			}

      			}else{
				if(multi_mail_check(currentuser.email) >= MAXMAIL){
					presskeyfor("\n\033[1;32m ����: ��ǰ�����Ѿ��������! ��������!\033[m\n");
					break;
				}
        		set_safe_record();
        		urec->lastjustify = time(NULL);
        		urec->userlevel |= (PERM_WELCOME | PERM_DEFAULT);
        		if (deny_me_fullsite()) urec->userlevel &= ~PERM_POST;
        		strlcpy(urec->reginfo, urec->email, sizeof(urec->reginfo)); /* ͬʱд��ע����Ϣ���� */
        		if (substitute_record(PASSFILE, urec, sizeof (struct userec), usernum) == -1) {
          		outs("ϵͳ��������ϵϵͳά��Ա\n");
        		}else{
          		outs("������!! �����ʺ���˳������.\n");
					if(fore_user){ /* ���û� */
						mail_sysfile("etc/Activa_fore_users", currentuser.userid, "��ϲ����������ڸ��س���Argo");
					}else{ /* ���û� */
						mail_sysfile("etc/smail", currentuser.userid, "��ӭ���뱾վ����");
					}
					snprintf(secu, sizeof(secu), "���� %s ���ʺ�", urec->userid);
					securityreport2(secu, YEA, NULL);
        			}
        				unlink(buf);
        				pressanykey();
      			}


		}else{
      			presskeyfor("���Ȼ�ȡ��֤��\n");
    		}
		break;
	case '3':
		auth_fillform(&currentuser, usernum);
		break;
	default:
		break;
	}

    	return 0;
}
#endif
