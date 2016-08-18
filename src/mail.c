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

static int mailid;

int
check_query_mail(char *maildir)
{
	int fd;
	struct fileheader fh;

	if ((fd = open(maildir, O_RDONLY)) == -1)
		return NA;

	if (lseek(fd, (off_t) 0 - sizeof(struct fileheader), SEEK_END) == -1 ||
	    read(fd, &fh, sizeof(struct fileheader)) != sizeof(struct fileheader)) {
		close(fd);
		return NA;
	}

	close(fd);
	return (fh.flag & FILE_READ) ? NA : YEA;
}

int
chkmail(int delay)
{
	static time_t lasttime = 0;
	static int ismail = NA;
	struct stat st;
	char maildir[STRLEN];

	if (!HAS_PERM(PERM_BASIC))
		return 0;

	/* monster: �ڱ༭���»������ʱ�򣬼�����ż�Ƶ�ʿ��ʵ��Ż� */
	if (delay == YEA && time(NULL) - lasttime < 10)
		return ismail;

	setmaildir(maildir, currentuser.userid);
	if (stat(maildir, &st) == -1) {
		ismail = NA;
		return NA;
	}

	if (lasttime >= st.st_mtime)
		return ismail;

	lasttime = time(NULL);
	ismail = check_query_mail(maildir);
	return ismail;
}

const char gdesc[5][40] = {
	"��δͨ�����ȷ�ϵ�ʹ����",
	"����ͨ�����ȷ�ϵ�ʹ����",
	"���еİ���",
	"��վ������",
	"�Զ���Ȩ��",
};

int
mailall(void)
{
        char ans[4], fname[PATH_MAX + 1], buf[STRLEN], buf2[STRLEN];
	int i, ret;

	modify_user_mode(SMAIL);

	clear();
	prints("��Ҫ�ĸ����еģ�\n\n");
	prints("(0) ����\n");
	for (i = 0; i < 5; i++)
		prints("(%d) %s\n", i + 1, gdesc[i]);

	getdata(8, 0, "������ģʽ (0~5)? [0]: ", ans, 2, DOECHO, YEA);
	if (ans[0] - '0' < 1 || ans[0] - '0' > 5) {
		return NA;
	}

	if(ans[0] - '0' == 5){
	        move(10,0);
		outs("����Ȩ���߼����ʽ(��\"+PERM_POST -PERM_WELCOME\"��ʾ��PERM_POST��\n��û��PERM_WELCOMEȨ):");
	        getdata(12, 0, ":", buf2, STRLEN, DOECHO, YEA);
		if(buf2[0] == '\0' || buf2[0] == '\n')
		  return NA;
	}

	move(13, 0);
	if(ans[0] - '0' == 5){
	        snprintf(buf, sizeof(buf), "�Ƿ�ȷ���ĸ�%s", buf2);
	}else{
	        snprintf(buf, sizeof(buf), "�Ƿ�ȷ���ĸ�%s", gdesc[ans[0] - '0' - 1]);
	}
	if (askyn(buf, NA, NA) == NA)
		return NA;

	header.reply_mode = NA;
	strcpy(header.title, "û����");
	strcpy(header.ds, gdesc[ans[0] - '0' - 1]);
	header.postboard = NA;
	if ((i = post_header(&header)) == -1)
		return NA;

	if (i == YEA) {
		snprintf(save_title, TITLELEN, "[Type %c ����] %.40s", ans[0], header.title);
	}

	quote_file[0] = 0;
	snprintf(fname, sizeof(fname), "mail/.tmp/mailall.%s.%d", currentuser.userid, getpid());
	do_quote(fname, header.include_mode);
	if (vedit(fname, EDIT_SAVEHEADER | EDIT_MODIFYHEADER | EDIT_ADDLOGINFO) == -1) {
		unlink(fname);
		clear();
		return -2;
	}

	clear();
	prints("bin/gsend, gsend, %s, %s, %s, %s, %s\n",ans, fname, save_title, currentuser.userid, ans[0] - '0' == 5 ? buf2 : "NULL");
	if(ans[0] - '0' == 5){
	        ret = cmd_exec("bin/gsend", "gsend", ans, fname, save_title, currentuser.userid, buf2, NULL);
	}else{
	        ret = cmd_exec("bin/gsend", "gsend", ans, fname, save_title, currentuser.userid, NULL);
	}
	
	if (ret != 0) {
		prints("����ʧ��!");
	} else {
		prints("���ͳɹ�!");
	}
	pressanykey();
	return 0;
}

#ifdef INTERNET_EMAIL
int
m_internet(void)
{
	char receiver[68];

	modify_user_mode(SMAIL);
	if (check_maxmail()) {
		pressreturn();
		return -1;
	}

	getdata(1, 0, "������E-mail��", receiver, 65, DOECHO, YEA);
	strtolower(genbuf, receiver);
	if (strstr(genbuf, ".bbs@" BBSHOST) || strstr(genbuf, ".bbs@localhost")) {
		move(3, 0);
		prints("վ���ż�, ���� (S)end ָ������\n");
		pressreturn();
	} else if (!invalidaddr(receiver)) {
		*quote_file = '\0';
		clear();
		do_send(receiver, NULL, YEA, 0);
	} else {
		move(3, 0);
		prints("�����˲���ȷ, ������ѡȡָ��\n");
		pressreturn();
	}
	clear();
	refresh();
	return 0;
}
#endif

int
autoreply(char *userid, char *title)
{
	char newtitle[TITLELEN];
	char fname[PATH_MAX + 1];

	/* �����Զ��ظ��ż������� */
	if (strncmp(title, "[�Զ��ظ�]", 10) == 0)
		return 0;

	snprintf(newtitle, sizeof(newtitle), "[�Զ��ظ�] %s", title);
	snprintf(fname, sizeof(fname), "mail/%c/%s/autoreply", mytoupper(userid[0]), userid);
	if (!dashf(fname))
		return 0;

	/* return mail_file(fname, currentuser.userid, newtitle); */

	/* freestyler: autoreplay ������Ӧ���� userid, ����currentuser.userid 
	 * so ���ܵ��� mail_file */
	struct fileheader newmessage;
	char maildir[STRLEN], filename[STRLEN];

	memset(&newmessage, 0, sizeof (newmessage));

	strlcpy(newmessage.owner, userid, sizeof(newmessage.owner));
	strlcpy(newmessage.title, newtitle, sizeof(newmessage.title));
	strlcpy(save_title, newmessage.title, sizeof(save_title));

	create_maildir(currentuser.userid);
	if (getfilename(maildir, filename, GFN_FILE | GFN_UPDATEID, &newmessage.id) == -1)
		return -1;
	strlcpy(newmessage.filename, strrchr(filename, '/') + 1, sizeof(newmessage.filename));
	if (  f_cp(fname, filename, O_CREAT) == -1)
		return -1;

	snprintf(genbuf, sizeof(genbuf), "%s/%s", maildir, DOT_DIR);
	newmessage.filetime = time(NULL);
	if (append_record(genbuf, &newmessage, sizeof(newmessage)) == -1)
		return -1;
	report("mailed %s ", userid);
	return 0;
}



int
do_send(char *userid, char *title, int check_permission, int id)
{
	struct userec lookupuser;
	struct fileheader newmessage;
	struct override fh;
	char save_title2[TITLELEN];
	int internet_mail, result, res;
	char tmp_fname[PATH_MAX + 1], filename[PATH_MAX + 1], maildir[PATH_MAX + 1];

	if (check_permission == YEA && !HAS_PERM(PERM_SENDMAIL))
		return -7;

	if (strchr(userid, '@')) {
		/* internet mail */
		internet_mail = YEA;

		snprintf(tmp_fname, sizeof(tmp_fname), "mail/.tmp/imail.%s.%05d", currentuser.userid, uinfo.pid);
		strcpy(filename, tmp_fname);
	} else {
		/* internal mail */
		internet_mail = NA;

		if (!getuser(userid, &lookupuser))
			return -1;
		if (!(lookupuser.userlevel & PERM_READMAIL))
			return -3;
		if ((lookupuser.userlevel & PERM_SUICIDE) && (!HAS_PERM(PERM_SYSOP)))
			return -6;	/* monster: �κ��˶����ܷ��Ÿ���ɱ���ˣ�����Ա���� */

		/* Rewrite by cancel at 01/09/19 : �����ż� */
		if (strcmp(lookupuser.userid, currentuser.userid)) {
			sethomefile(filename, userid, "maildeny");
			if (search_record(filename, &fh, sizeof (fh), cmpfnames, currentuser.userid))
				return -5;
		}
		/* Rewrite End. */

		if (getmailboxsize(lookupuser.userlevel) * 2 < getmailsize(lookupuser.userid))
			return -4;

		create_maildir(userid);
		memset(&newmessage, 0, sizeof(newmessage));
		if (getfilename(maildir, filename, GFN_FILE | GFN_UPDATEID, &newmessage.id) == -1)
			return -1;
		strlcpy(newmessage.filename, strrchr(filename, '/') + 1, sizeof(newmessage.filename));
		strlcpy(newmessage.owner, currentuser.userid, sizeof(newmessage.owner));
	}

	if (title == NULL) {
		header.reply_mode = NA;
		strcpy(header.title, "û����");
	} else {
		header.reply_mode = YEA;
		strlcpy(header.title, title, sizeof(header.title));
	}
	header.postboard = NA;

	setuserfile(genbuf, "signatures");
	ansimore2(genbuf, NA, 0, 18);
	strcpy(header.ds, userid);
	result = post_header(&header);
	if (result == -1) {
		clear();
		return -2;
	}
	if (result == YEA) {
		strcpy(newmessage.title, header.title);
		strlcpy(save_title, newmessage.title, sizeof(save_title));
		snprintf(save_title2, sizeof(save_title2), "{%.12s} %.40s", userid, newmessage.title);
	}
	do_quote(filename, header.include_mode);

	if (internet_mail) {
#ifndef INTERNET_EMAIL
		prints("�Բ��𣬱�վ�ݲ��ṩ Internet Mail ����");
		pressanykey();
#else
		if (vedit(filename, EDIT_SAVEHEADER | EDIT_MODIFYHEADER | EDIT_ADDLOGINFO) == -1) {
			unlink(filename);
			clear();
			return -2;
		}

		clear();
		prints("�ż������ĸ� %s \n", userid);
		prints("����Ϊ�� %s \n", header.title);
		if (askyn("ȷ��Ҫ�ĳ���", YEA, NA) == NA) {
			prints("\n�ż���ȡ��...\n");
			res = -2;
		} else {
			int filter = YEA;

			if (askyn("�Ƿ����ANSI���Ʒ�", DEFINE(DEF_NOANSI), NA) == NA)
				filter = NA;
			if (askyn("�Ƿ񱸷ݸ��Լ�", NA, NA) == YEA)
				mail_file(tmp_fname, currentuser.userid, save_title2);

			prints("���Ժ�, �ż�������...\n");
			refresh();
			res = bbs_sendmail(tmp_fname, header.title, userid, filter);
		}
		unlink(tmp_fname);
		report("mailed %s", userid);
		return res;
#endif
	} else {
		if (vedit(filename, EDIT_SAVEHEADER | EDIT_MODIFYHEADER | EDIT_ADDLOGINFO) == -1) {
			unlink(filename);
			clear();
			return -2;
		}

		clear();
		if (askyn("�Ƿ񱸷ݸ��Լ�", NA, NA) == YEA)
			mail_file(filename, currentuser.userid, save_title2);

		snprintf(genbuf, sizeof(genbuf), "%s/%s", maildir, DOT_DIR);
		if (id != 0) newmessage.id = id;
		newmessage.filetime = time(NULL);
		if (append_record(genbuf, &newmessage, sizeof(newmessage)) == -1)
			return -1;
		report("mailed %s", userid);
		return autoreply(userid, newmessage.title);
	}
	return 0;
}

void
m_feedback(int code, char *uident, char *defaultmsg)
{
	switch (code) {
	case -1:
		prints("�����߲���ȷ\n");
		break;
	case -2:
		prints("ȡ��\n");
		break;
	case -3:
		prints("[%s] �޷�����\n", uident);
		break;
	case -4:
		prints("[%s] �����������޷�����\n", uident);
		break;
	case -5:
		prints("[%s] �����յ������ż�\n", uident);
		break;
	case -6:
		prints("[%s] ��ɱ�У��޷�����\n", uident);
		break;
	case -7:
		prints("�ܱ�Ǹ����û�з����ż���Ȩ��\n");
		break;
	default:
		if (defaultmsg == NULL) {
			prints("�ż��Ѽĳ�\n");
		} else {
			prints("%s\n", defaultmsg);
		}
		break;
	}
	pressreturn();
}

int
m_send(char *userid)
{
	char uident[IDLEN + 2] = { '\0' };

	if (check_maxmail()) {
		pressreturn();
		return 0;
	}

	if (userid == NULL || (uinfo.mode != LUSERS && uinfo.mode != LAUSERS &&
	    uinfo.mode != QUERY && uinfo.mode != FRIEND && uinfo.mode != GMENU)) {
		clear_line(1);
		modify_user_mode(SMAIL);
		usercomplete("�����ˣ� ", uident);

		if (uident[0] == '\0')
			return FULLUPDATE;
	}

	modify_user_mode(SMAIL);
	clear();
	*quote_file = '\0';
	m_feedback(do_send((uident[0] == '\0') ? userid : uident, NULL, YEA, 0), uident, NULL);
	return FULLUPDATE;
}

int
read_mail(struct fileheader *fptr)
{
	snprintf(genbuf, sizeof(genbuf), "mail/%c/%s/%s", mytoupper(currentuser.userid[0]),
		currentuser.userid, fptr->filename);
	ansimore(genbuf, NA);
	fptr->flag |= FILE_READ;
	return 0;
}

static int mrd;
static int delmsgs[1024];
static int delcnt;

int
read_new_mail(void *ptr, int unused)
{
	static int idc;
	char done = NA, delete_it;
	char fname[STRLEN], maildir[STRLEN];
	struct fileheader *fptr = (struct fileheader *)ptr, hdr;

	if (fptr == NULL) {
		delcnt = 0;
		idc = 0;
		return 0;
	}
	idc++;

	/* monster: remove invalid mail */

	if (fptr->filename[0] == 0 && delcnt < 1024) {
		delmsgs[delcnt++] = idc;
		return 0;
	}

	if (fptr->flag & FILE_READ)
		return 0;

	mrd = 1;
	prints("��ȡ %s ������ '%s' ?\n", fptr->owner, fptr->title);
	getdata(1, 0, "(Y)��ȡ (N)���� (Q)�뿪 [Y]: ", genbuf, 3, DOECHO, YEA);
	if (genbuf[0] == 'q' || genbuf[0] == 'Q') {
		clear();
		return QUIT;
	}
	if (genbuf[0] != 'y' && genbuf[0] != 'Y' && genbuf[0] != '\0') {
		clear();
		return 0;
	}

	/* monster: copy on write */
	memcpy(&hdr, ptr, sizeof(struct fileheader));
	fptr = &hdr;

	// mrd = 1;
	read_mail(fptr);
	strlcpy(fname, genbuf, sizeof(fname));
	setmaildir(maildir, currentuser.userid);
	if (substitute_record(maildir, fptr, sizeof (*fptr), idc))
		return -1;
	delete_it = NA;
	while (!done) {
		move(t_lines - 1, 0);
		prints("(R)����, (D)ɾ��, (G)���� ? [G]: ");
		switch (egetch()) {
		case 'R':
		case 'r':
			mail_reply(idc, fptr, maildir);
			break;
		case 'D':
		case 'd':
			if (delcnt < 1024) {
				delete_it = YEA;
			} else {
				/* monster: ��ֹdelmsgs��� */
				move(t_lines - 1, 0);
				clrtoeol();
				prints("�޷�ɾ���ʼ��������������");
				egetch();
			}
		default:
			done = YEA;
		}
		if (!done)
			ansimore(fname, NA);	/* re-read */
	}
	if (delete_it) {
		clear();
		snprintf(genbuf, sizeof(genbuf), "ɾ���ż� [%-.55s]", fptr->title);
		if (askyn(genbuf, NA, NA) == YEA) {
			snprintf(genbuf, sizeof(genbuf), "mail/%c/%s/%s",
				mytoupper(currentuser.userid[0]),
				currentuser.userid, fptr->filename);
			unlink(genbuf);
			delmsgs[delcnt++] = idc;
		}
	}
	clear();
	return 0;
}

int
m_new(void)
{
	char maildir[STRLEN];

	if (guestuser)
		return 0;

	clear();
	mrd = 0;
	modify_user_mode(RMAIL);
	read_new_mail(NULL, 0);
	setmaildir(maildir, currentuser.userid);
	apply_record(maildir, read_new_mail, sizeof(struct fileheader));
	while (delcnt--)
		delete_record(maildir, sizeof(struct fileheader),
			      delmsgs[delcnt]);
	if (!mrd) {
		clear();
		move(10, 30);
		prints("������û�����ż�!");
		pressanykey();
	}
	return -1;
}

void
mailtitle(void)
{
	int total, used;

	total = getmailboxsize(currentuser.userlevel);
	used = getmailsize(currentuser.userid);

	showtitle("�ʼ�ѡ��", BoardName);
	prints("�뿪[\033[1;32m��\033[m,\033[1;32mq\033[m] ѡ��[\033[1;32m��\033[m, \033[1;32m��\033[m] �Ķ��ż�[\033[1;32m��\033[m,\033[1;32mRtn\033[m] �� ��[\033[1;32mR\033[m] ���ţ��������[\033[1;32md\033[m,\033[1;32mD\033[m] ����[\033[1;32mh\033[m]\033[m\n");
	prints("\033[1;44m ���  ������       �� ��      ����  (\033[33m������������Ϊ[%4dK]����ǰ����[%4dK]\033[37m) \033[m\n",
		     total, used);
	clrtobot();
}

int
getmailboxsize(unsigned int userlevel)
{
	if (userlevel & (PERM_SYSOP))
		return 5000;
	if (userlevel & (PERM_LARGEMAIL))
		return 3600;
	if (userlevel & (PERM_BOARDS | PERM_INTERNAL))
		return 3600;
	if (userlevel & (PERM_LOGINOK))
		return 1200;
	return 5;
}

int
getmailsize(char *userid)
{
	struct fileheader fcache;
	struct stat st;
	char dirfile[PATH_MAX + 1], mailfile[PATH_MAX + 1];
	int fd, total = 0, i, count;
	char *p;

	setmaildir(dirfile, userid);
	if ((fd = open(dirfile, O_RDWR)) != -1) {
		f_exlock(fd);
		fstat(fd, &st);
		count = st.st_size / sizeof (fcache);
		snprintf(mailfile, sizeof(mailfile), "mail/%c/%s/", mytoupper(userid[0]), userid);
		p = strrchr(mailfile, '/') + 1;

		for (i = 0; i < count; i++) {
			if (lseek(fd, (off_t) (sizeof (fcache) * i), SEEK_SET) == -1)
				break;

			if (read(fd, &fcache, sizeof (fcache)) != sizeof (fcache))
				break;

			if (fcache.size <= 0) {
				strcpy(p, fcache.filename);
				if (stat(mailfile, &st) != -1) {
					fcache.size = (st.st_size > 0) ? st.st_size : 1;
				} else {
					fcache.size = 1;
				}

				if (lseek(fd, (off_t) (sizeof (fcache) * i), SEEK_SET) == -1)
					break;

				if (safewrite(fd, &fcache, sizeof (fcache)) != sizeof (fcache))
					break;
			}
			total += fcache.size;
		}
		f_unlock(fd);
		close(fd);
	}

	return total / 1024 + 1;
}

char *
maildoent(int num, void *ent_ptr)
{
	static char buf[512];
	char b2[512];
	char status;
	char *date, *t;
	struct fileheader *ent = (struct fileheader *)ent_ptr;

#ifdef COLOR_POST_DATE
	struct tm *mytm;
	char color[8] = "\033[1;30m";
#endif

	date = ctime(&ent->filetime) + 4;

	strcpy(b2, ent->owner);
	if ((b2[strlen(b2) - 1] == '>') && strchr(b2, '<')) {
		t = strtok(b2, "<>");
		if (invalidaddr(t))
			t = strtok(NULL, "<>");
		if (t != NULL)
			strcpy(b2, t);
	}
	if ((t = strchr(b2, ' ')) != NULL)
		*t = '\0';
	if (ent->flag & FILE_READ) {
		if ((ent->flag & FILE_MARKED) &&
		    (ent->flag & MAIL_REPLY))
			status = 'b';
		else if (ent->flag & FILE_MARKED)
			status = 'm';
		else if (ent->flag & MAIL_REPLY)
			status = 'r';
		else
			status = ' ';
	} else {
		status = (ent->flag & FILE_MARKED) ? 'M' : 'N';
	}
#ifdef COLOR_POST_DATE
	mytm = localtime(&ent->filetime);
	color[5] = mytm->tm_wday + 49;

	if (!strncmp("Re: ", ent->title, 4)) {
		snprintf(buf, sizeof(buf), " %s%3d\033[m %c %-12.12s %s%6.6s\033[m  %s%.50s\033[m",
			(mailid == ent->id) ? "\033[1;33m" : "", num, status, b2, color, date,
			(mailid == ent->id) ? "\033[1;33m" : "", ent->title);
	} else {
		snprintf(buf, sizeof(buf), " %s%3d\033[m %c %-12.12s %s%6.6s\033[m  �� %s%.47s\033[m",
			(mailid == ent->id) ? "\033[1;36m" : "", num, status, b2, color, date,
			(mailid == ent->id) ? "\033[1;36m" : "", ent->title);
	}
#else
	if (!strncmp("Re: ", ent->title, 4)) {
		snprintf(buf, sizeof(buf), " %s%3d\033[m %c %-12.12s %6.6s\033[m  %s%.50s\033[m",
			(mailid == ent->id) ? "\033[1;33m" : "", num, status, b2, date, (mailid == ent->id) ? "\033[1;33m" : "",
			ent->title);
	} else {
		snprintf(buf, sizeof(buf), " %s%3d\033[m %c %-12.12s %6.6s\033[m  �� %s%.47s\033[m",
			(mailid == ent->id) ? "\033[1;36m" : "", num, status, b2, date, (mailid == ent->id) ? "\033[1;36m" : "",
			ent->title);
	}
#endif
	return buf;
}

int
mail_read(int ent, struct fileheader *fileinfo, char *direct)
{
	char buf[PATH_MAX + 1], notgenbuf[PATH_MAX + 1];
	char *t;
	int readnext = NA, done = NA, delete_it = NA, replied = NA;

	clear();
	mailid = fileinfo->id;
	strlcpy(buf, direct, sizeof(buf));
	if ((t = strrchr(buf, '/')) != NULL)
		*t = '\0';
	snprintf(notgenbuf, sizeof(notgenbuf), "%s/%s", buf, fileinfo->filename);

	while (!done) {
		ansimore(notgenbuf, NA);
		move(t_lines - 1, 0);
		prints("(R)����, (D)ɾ��, (G)����? [G]: ");
		switch (egetch()) {
		case 'R':
		case 'r':
			replied = YEA;
			mail_reply(ent, fileinfo, direct);
			break;
		case KEY_UP:
		case KEY_PGUP:
			done = YEA;
			readnext = READ_PREV;
			break;
		case ' ':
		case 'j':
		case KEY_RIGHT:
		case KEY_DOWN:
		case KEY_PGDN:
			done = YEA;
			readnext = READ_NEXT;
			break;
		case 'D':
		case 'd':
			delete_it = YEA;
		default:
			done = YEA;
		}
	}

	fileinfo->flag |= FILE_READ;
	substitute_record(direct, fileinfo, sizeof (*fileinfo), ent);
	if (delete_it) {
		return mail_del(ent, fileinfo, direct);	/* �޸��ż�֮bug */
	}

	return (readnext == NA) ? FULLUPDATE : readnext;
}

int
mail_reply(int ent, struct fileheader *fileinfo, char *direct)
{
	char uid[STRLEN];
	char title[STRLEN];
	char *t;
	int old_mode, code;

	old_mode = uinfo.mode;
	modify_user_mode(SMAIL);
	snprintf(genbuf, sizeof(genbuf), "MAILER-DAEMON@%s", BBSHOST);
	if (strstr(fileinfo->owner, genbuf)) {
		ansimore("help/mailerror-explain", YEA);
		modify_user_mode(old_mode);
		return FULLUPDATE;
	}
	if (check_maxmail()) {
		pressreturn();
		modify_user_mode(old_mode);
		return DONOTHING;
	}
	clear();
	strlcpy(uid, fileinfo->owner, sizeof(uid));
	if ((uid[strlen(uid) - 1] == '>') && strchr(uid, '<')) {
		t = strtok(uid, "<>");
		if (invalidaddr(t))
			t = strtok(NULL, "<>");
		if (t != NULL)
			strcpy(uid, t);
		else {
			prints("�޷�Ͷ��\n");
			pressreturn();
			modify_user_mode(old_mode);
			return FULLUPDATE;
		}
	}

	if ((t = strchr(uid, ' ')) != NULL)
		*t = '\0';

	if ((fileinfo->title[0] != 'R' && fileinfo->title[0] != 'r') ||
	    (fileinfo->title[1] != 'E' && fileinfo->title[1] != 'e') ||
	     fileinfo->title[2] != ':') {
		snprintf(title, TITLELEN, "Re: %s", fileinfo->title);
	} else {
		strlcpy(title, fileinfo->title, TITLELEN);
	}

	snprintf(quote_file, sizeof(quote_file), "mail/%c/%s/%s", mytoupper(currentuser.userid[0]),
		currentuser.userid, fileinfo->filename);
	strcpy(quote_user, fileinfo->owner);

	if ((code = do_send(uid, title, YEA, fileinfo->id)) == 0) {
		fileinfo->flag |= MAIL_REPLY;
		substitute_record(direct, fileinfo, sizeof (*fileinfo), ent);
	}
	m_feedback(code, uid, NULL);
	modify_user_mode(old_mode);
	return FULLUPDATE;
}

int
mail_del(int ent, struct fileheader *fileinfo, char *direct)
{
	snprintf(genbuf, sizeof(genbuf), "ɾ���ż� [%-.55s]", fileinfo->title);
	if (askyn(genbuf, NA, YEA) == NA) {
		presskeyfor("����ɾ���ż�...");
		return FULLUPDATE;
	}

	if (delete_file(direct, ent, fileinfo->filename, YEA) == 0) {
		check_maxmail();
		return DIRCHANGED;
	}
	presskeyfor("ɾ��ʧ��...");
	check_maxmail();
	return FULLUPDATE;
}

#ifdef INTERNET_EMAIL
int
mail_forward(int ent, struct fileheader *fileinfo, char *direct)
{
	char buf[STRLEN];
	char *p;

	if (!HAS_PERM(PERM_FORWARD)) {
		return DONOTHING;
	}
	strlcpy(buf, direct, sizeof(buf));
	buf[STRLEN - 1] = '\0';
	if ((p = strrchr(buf, '/')) != NULL)
		*p = '\0';
	switch (doforward(buf, fileinfo, 0)) {
	case 0:
		prints("����ת�����!\n");
		break;
	case -1:
		prints("ת��ʧ��: ϵͳ��������.\n");
		break;
	case -2:
		prints("ת��ʧ��: ����ȷ�����ŵ�ַ.\n");
		break;
	case -3:
		prints("�������䳬�ޣ���ʱ�޷�ʹ���ʼ�����.\n");
		break;
	default:
		prints("ȡ��ת��...\n");
	}
	pressreturn();
	clear();
	return FULLUPDATE;
}

int
mail_u_forward(int ent, struct fileheader *fileinfo, char *direct)
{
	char buf[PATH_MAX + 1];
	char *p;

	if (!HAS_PERM(PERM_FORWARD))
		return DONOTHING;
	strlcpy(buf, direct, sizeof(buf));

	if ((p = strrchr(buf, '/')) != NULL)
		*p = '\0';
	switch (doforward(buf, fileinfo, 1)) {
	case 0:
		prints("����ת�����!\n");
		break;
	case -1:
		prints("ת��ʧ��: ϵͳ��������.\n");
		break;
	case -2:
		prints("ת��ʧ��: ����ȷ�����ŵ�ַ.\n");
		break;
	case -3:
		prints("�������䳬�ޣ���ʱ�޷�ʹ���ʼ�����.\n");
		break;
	default:
		prints("ȡ��ת��...\n");
	}
	pressreturn();
	clear();
	return FULLUPDATE;
}
#endif

int
mail_del_range(int ent, struct fileheader *fileinfo, char *direct)
{
	int result;

	result = del_range(ent, fileinfo, direct);
	check_maxmail();	/* monster: ����ϵͳ���󱨸���������Ĵ��� */
	return result;
}

int
mail_mark(int ent, struct fileheader *fileinfo, char *direct)
{
	if (fileinfo->flag & FILE_MARKED)
		fileinfo->flag &= ~FILE_MARKED;
	else
		fileinfo->flag |= FILE_MARKED;
	substitute_record(direct, fileinfo, sizeof (*fileinfo), ent);
	return (PARTUPDATE);
}

struct one_key mail_comms[] = {
	{ 'd', 		mail_del },
	{ 'D', 		mail_del_range },
	{ Ctrl('P'), 	m_send },
	{ 'E', 		edit_post },
	{ 'r', 		mail_read },
	{ 'R', 		mail_reply },
	{ 'm', 		mail_mark },
	{ 'c', 		mail_clear },
	{ 'T', 		edit_title },
	{ 'i', 		save_post },
	{ 'I', 		import_post },
	{ 'X', 		author_announce },
	{ 'x', 		currboard_announce },
	{ KEY_TAB, 	show_user_notes },
#ifdef INTERNET_EMAIL
	{ 'F', 		mail_forward },
	{ 'U', 		mail_u_forward },
#endif
	{ 'a', 		auth_search_down },
	{ 'A', 		auth_search_up },
	{ '/', 		title_search_down },
	{ '?', 		title_search_up },
	{ '\'', 	post_search_down },
	{ '\"', 	post_search_up },
	{ ']', 		thread_search_down },
	{ '[', 		thread_search_up },
	{ Ctrl('A'), 	show_author },
	{ Ctrl('N'), 	SR_first_new },
	{ '\\', 	SR_last },
	{ '=', 		SR_first },
	{ 'l', 		show_allmsgs },
	{ Ctrl('C'), 	do_cross },
	{ Ctrl('S'), 	SR_read },
	{ 'n', 		SR_first_new },
	{ 'p', 		SR_read },
	{ Ctrl('X'), 	pannounce },
	{ Ctrl('U'), 	SR_author },
	{ 'h', 		mailreadhelp },
	{ Ctrl('J'), 	mailreadhelp },
	{ Ctrl('V'), 	x_lockscreen_silent },
	{ '!', 		Q_Goodbye },
	{ 'S', 		s_msg },
	{ 'f', 		t_friends },
	{ 'o',		fast_cloak },
	{ 'b', 		m_func },
	{ 'L', 		show_allmsgs },
	{ '\0', 	NULL }
};

/* monster: ͬ������� */
int
m_func(int ent, struct fileheader *fileinfo, char *direct)
{
	FILE *fp;
	int first, dele_orig, count = 0;
	char ch[2], buf[80], filename[PATH_MAX + 1];
	struct bmfuncarg arg;
	static char *func_items[4] = { "ɾ��", "����", "�ϼ�" };

	saveline(t_lines - 1, 0);
	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, "��ͬ���� (1)ɾ�� (2)���� (3)�ϼ� ? [0]: ", ch, 2, DOECHO, YEA);

	if (ch[0] < '1' || ch[0] > '3') {
		saveline(t_lines - 1, 1);
		return DONOTHING;
	}
	ch[0] -= '1';

	move(t_lines - 1, 0);
	snprintf(buf, sizeof(buf), "ȷ��Ҫִ����ͬ����[%s]��", func_items[(int)ch[0]]);
	if (askyn(buf, NA, NA) == NA) {
		saveline(t_lines - 1, 1);
		return DONOTHING;
	}

	move(t_lines - 1, 0);
	snprintf(buf, sizeof(buf), "�Ƿ�Ӵ������һƪ��ʼ%s (Y)��һƪ (N)Ŀǰ��һƪ", func_items[(int)ch[0]]);
	if (askyn(buf, YEA, NA) == YEA) {
		if ((first = locate_article(direct, fileinfo, ent, LOCATE_THREAD | LOCATE_FIRST, &fileinfo->id)) == -1)
			first = ent;
	} else {
		first = ent;
	}

	arg.id = fileinfo->id;
	arg.flag = LOCATE_THREAD;
	switch (ch[0]) {
	case 0:		/* ͬ����ɾ�� */
		process_records(direct, sizeof(struct fileheader), first, 99999, bmfunc_del, &arg);
		break;
	case 1:		/* ͬ���Ᵽ�� */
		process_records(direct, sizeof(struct fileheader), first, 99999, bmfunc_mark, &arg);
		break;
	case 2:		/* �ϼ� */
		clear_line(t_lines - 1);
		dele_orig = askyn("�Ƿ�Ҫɾ��ԭ�ģ�", NA, NA);

		snprintf(filename, sizeof(filename), "mail/.tmp/mail_combine.%d.A", time(NULL));
		if ((fp = fopen(filename, "w")) == NULL) {
			presskeyfor("����ʧ�ܣ�����վ����ӳ��\n");
			return PARTUPDATE;
		}

		if ((fileinfo->title[0] == 'R' || fileinfo->title[0] == 'r') &&
			    fileinfo->title[1] == 'e' && fileinfo->title[2] == ':' &&
			    fileinfo->title[3] == ' ') {
				snprintf(save_title, sizeof(save_title), "���ϼ���%s", fileinfo->title + 4);
		} else {
			snprintf(save_title, sizeof(save_title), "���ϼ���%s", fileinfo->title);
		}

		fprintf(fp, "������: %s (%s)\n", currentuser.userid, currentuser.username);
		fprintf(fp, "��  ��: %s\033[m\n", save_title);
		fprintf(fp, "����վ: %s (%s)\n", BoardName, datestring);
		fprintf(fp, "��  Դ: %s\n\n", currentuser.lasthost);

		arg.extrarg[0] = &count;
		arg.extrarg[1] = fp;
		arg.extrarg[2] = &dele_orig;
		process_records(direct, sizeof(struct fileheader), first, 99999, bmfunc_combine, &arg);

		fprintf(fp, "\n\033[m--\n\033[1;%dm�� ��Դ:. %s %s. [FROM: %s]\033[m\n",
			(currentuser.numlogins % 7) + 31, BoardName, BBSHOST, currentuser.lasthost);
		fclose(fp);

		if (count > 0) {
			mail_file(filename, currentuser.userid, save_title);
		}
		unlink(filename);
		break;
	}

	check_maxmail();
	fixkeep(direct, ent + 1, 99999);
	return DIRCHANGED;
}

int
m_read(void)
{
	char maildir[STRLEN];

	if (guestuser || !HAS_PERM(PERM_READMAIL))
		return 0;

	setmaildir(maildir, currentuser.userid);
	i_read(RMAIL, maildir, NULL, NULL, mailtitle, maildoent, update_endline, &mail_comms[0],
	       get_records, get_num_records, sizeof(struct fileheader));
	return 0;
}

int
invalidaddr(char *addr)
{
	if (*addr == '\0' || !strchr(addr, '@'))
		return 1;
	while (*addr) {
		if (!isalnum((unsigned int)(*addr)) && !strchr(".!@:-_", *addr))
			return 1;
		addr++;
	}
	return 0;
}

int
g_send(void)
{
	char maillist[STRLEN], fname[STRLEN];
	int i;

	if (!HAS_PERM(PERM_SENDMAIL)) {
		clear();
		prints("�ܱ�Ǹ����û�з����ż���Ȩ��.\n");
		pressreturn();
		return 0;
	}

	if (check_maxmail()) {
		clear();
		prints("�������䳬�ޣ���ʱ�޷�ʹ���ʼ�����.\n");
		pressreturn();
		return 0;
	}

	modify_user_mode(SMAIL);
	*quote_file = '\0';
	sethomefile(maillist, currentuser.userid, "maillist");
	if (listedit(maillist, "�༭��Ⱥ���ż�������", NULL) == 0) {
		prints("\nȺ���ż�����Ϊ��\n");
	} else {
		if (askyn("��ȷ��Ҫ��Ⱥ���ż���", NA, NA) == NA)
			return 0;

		header.reply_mode = NA;
		strcpy(header.title, "û����");
		strcpy(header.ds, "���Ÿ�һȺ��");
		header.postboard = NA;

		if ((i = post_header(&header)) == -1)
			return NA;

		if (i == YEA) {
			snprintf(save_title, TITLELEN, "[Ⱥ���ż�] %.40s", header.title);
		}

		quote_file[0] = 0;
		snprintf(fname, sizeof(fname), "mail/.tmp/gmail.%s.%d", currentuser.userid, getpid());
		do_quote(fname, header.include_mode);
		if (vedit(fname, EDIT_SAVEHEADER | EDIT_MODIFYHEADER | EDIT_ADDLOGINFO) == -1) {
			unlink(fname);
			clear();
			return -2;
		}

		clear();
		if (cmd_exec("bin/gsend", "gsend", "0", fname, save_title, currentuser.userid, maillist, NULL) != 0) {
			prints("����ʧ��!");
		} else {
			report("mailed %s", save_title);
			prints("���ͳɹ�!");
		}
	}
	pressreturn();
	return 0;
}

int
mail_sysfile(char *tmpfile, char *userid, char *title)
{
	FILE *fpr, *fpw;
	char tmpfile2[STRLEN], buf[256];
	int result;

	/* monster: guest��Ӧ���յ�ϵͳ�ż� */
	if (!strcmp(userid, "guest"))
		return -1;

	if ((fpr = fopen(tmpfile, "r")) == NULL)
		return -1;

	snprintf(tmpfile2, sizeof(tmpfile2), "mail/.tmp/mail_sysfile.%d", getpid());
	if ((fpw = fopen(tmpfile2, "w")) == NULL) {
		fclose(fpr);
		return -1;
	}

	add_sysheader(fpw, NULL, title);
	while (fgets(buf, 256, fpr)) {
		if (buf[0] == '-' && buf[1] == '-')
			break;
		fputs(buf, fpw);
	}
	add_syssign(fpw);
	fclose(fpw);
	fclose(fpr);

	result = mail_file(tmpfile2, userid, title);
	unlink(tmpfile2);
	return result;
}

int
mail_file(char *tmpfile, char *userid, char *title)
{
	struct fileheader newmessage;
	extern int anonymousmail;
	char maildir[STRLEN], filename[STRLEN];

	if (userid == NULL || userid[0] == '\0')
		return -1;

	memset(&newmessage, 0, sizeof (newmessage));
	strlcpy(newmessage.owner, anonymousmail ? currboard : currentuser.userid, sizeof(newmessage.owner));
	strlcpy(newmessage.title, title, sizeof(newmessage.title));
	strlcpy(save_title, newmessage.title, sizeof(save_title));

	create_maildir(userid);
	if (getfilename(maildir, filename, GFN_FILE | GFN_UPDATEID, &newmessage.id) == -1)
		return -1;
	strlcpy(newmessage.filename, strrchr(filename, '/') + 1, sizeof(newmessage.filename));
	if (f_cp(tmpfile, filename, O_CREAT) == -1) {
		return -1;
	}

	snprintf(genbuf, sizeof(genbuf), "%s/%s", maildir, DOT_DIR);
	newmessage.filetime = time(NULL);
	if (append_record(genbuf, &newmessage, sizeof(newmessage)) == -1)
		return -1;
	report("mailed %s ", userid);
	return 0;
}

#ifdef INTERNET_EMAIL
int
doforward(char *direct, struct fileheader *fh, int mode)
{
	struct userec lookupuser;
	static char address[STRLEN];
	char fname[STRLEN], tmpfname[STRLEN];
	char receiver[STRLEN];
	char title[STRLEN];
	int return_no, internet_mail = 0;
	int filter = YEA;

	clear();
	if (INMAIL(uinfo.mode))
		if (check_maxmail())
			return -3;
	if (address[0] == '\0') {
		strlcpy(address, currentuser.email, sizeof(address));
	}

	/* Check fh */
	/* Pudding: check whether the post is valid */
	if (fh->title[0] == '\0') {
		prints("���²�����, ת��ʧ��\n");
		return -1;
	}
	
#ifndef NOCHECK_PERM_SETADDR
	if (HAS_PERM(PERM_SETADDR)) {
#endif
		prints("��ֱ�Ӱ� Enter ������������ʾ�ĵ�ַ, ��������������ַ\n");
		prints("���ż�ת�ĸ� [%s]\n", address);
		getdata(2, 0, "==> ", receiver, 70, DOECHO, YEA);
#ifndef NOCHECK_PERM_SETADDR
	} else
		strcpy(receiver, currentuser.userid);
#endif
	if (receiver[0] != '\0') {
		strlcpy(address, receiver, sizeof(address));
	} else {
		strlcpy(receiver, address, sizeof(receiver));
	}
	snprintf(genbuf, sizeof(genbuf), ".bbs@%s", BBSHOST);
	if (strstr(receiver, genbuf) || strstr(receiver, ".bbs@localhost") || strstr(receiver, ".bbs@"BBSHOST)) {
		char *pos;

		pos = strchr(address, '.');
		*pos = '\0';
	}
	if (strpbrk(address, "@."))
		internet_mail = YEA;
	if (!internet_mail) {
		/* Added by cancel at 01/09/19 : �����ż� */
		char filepath[STRLEN];
		struct override fh;

		/* Added End. */
		if (!getuser(address, &lookupuser))
			return -1;
		/* Added by cancel at 01/09/19 : �����ż� */
		if (strcmp(lookupuser.userid, currentuser.userid)) {
			sethomefile(filepath, lookupuser.userid, "maildeny");
			if (search_record
			    (filepath, &fh, sizeof (fh), cmpfnames,
			     currentuser.userid)) {
				prints("[%s] �����յ�����š�\n", lookupuser.userid);
				return -5;
			}
		}
		/* Added End. */
		if (getmailboxsize(lookupuser.userlevel) * 2 <
		    getmailsize(lookupuser.userid)) {
			prints("[%s] �����������޷����š�\n", address);
			return -4;
		}
	}
	snprintf(genbuf, sizeof(genbuf), "ȷ�������¼ĸ� %s ��", address);
	if (askyn(genbuf, YEA, NA) == 0)
		return 1;
	if (invalidaddr(address) && internet_mail)
		return -2;
	snprintf(tmpfname, sizeof(tmpfname), "mail/.tmp/forward.%s.%05d", currentuser.userid, uinfo.pid);
	snprintf(genbuf, sizeof(genbuf), "%s/%s", direct, fh->filename);

	if (askyn("�Ƿ��޸���������", NA, NA) == YEA) {
		FILE *fp;

		if (f_cp(genbuf, tmpfname, O_CREAT) == -1)
			return -1;

		if (vedit(tmpfname, EDIT_NONE) == -1) {
			if (askyn("�Ƿ�ĳ�δ�޸ĵ�����", YEA, NA) == 0) {
				unlink(tmpfname);
				clear();
				return 1;
			}
		} else {
			add_edit_mark(tmpfname, 3, NULL);
		}

		if ((fp = fopen(tmpfname, "a")) == NULL)
			return -1;

		fprintf(fp, "--\n\033[m\033[1;%2dm�� ת��:��%s %s��[FROM: %s]\033[m\n",
			(currentuser.numlogins % 7) + 31, BoardName, BBSHOST, fromhost);

		fclose(fp);
		clear();
	} else {
		FILE *fin, *fout;
		char buf[8192];
		int count;

		if ((fin = fopen(genbuf, "r")) == NULL)
			return -1;

		if ((fout = fopen(tmpfname, "w")) == NULL) {
			fclose(fin);
			return -1;
		}

		while ((count = fread(buf, 1, sizeof(buf), fin)) > 0)
			fwrite(buf, 1, count, fout);

		fprintf(fout, "--\n\033[m\033[1;%2dm�� ת��:��%s %s��[FROM: %s]\033[m\n",
			(currentuser.numlogins % 7) + 31, BoardName, BBSHOST, fromhost);

		fclose(fin);
		fclose(fout);
	}

	if (internet_mail) {
		if (askyn("�Ƿ����ANSI���Ʒ�", DEFINE(DEF_NOANSI), NA) == NA)
			filter = NA;
	}
	prints("ת���ż��� %s, ���Ժ�....\n", address);
	refresh();

//	/* Pudding: check whether the post is valid */
//	if (fh->title[0] == '\0') {
//		prints("���²�����, ת��ʧ��\n");
//		return -1;
//	}

	
	if (mode == 0)
		strcpy(fname, tmpfname);
	else if (mode == 1) {
		snprintf(fname, sizeof(fname), "mail/.tmp/file.uu%05d", uinfo.pid);
		snprintf(genbuf, sizeof(genbuf), "/bin/uuencode %s fb-bbs.%05d.txt > %s",
			tmpfname, uinfo.pid, fname);
		system(genbuf);
	}

	if (!strstr(fh->title, "[ת��]")) {
		snprintf(title, sizeof(title), "[ת��] %s", fh->title);
	} else {
		strlcpy(title, fh->title, sizeof(title));
	}

	if (!internet_mail) {
		return_no = mail_file(fname, lookupuser.userid, title);
	} else {
		return_no = bbs_sendmail(fname, title, address, filter);
	}
	if (mode == 1) {
		unlink(fname);
	}
	unlink(tmpfname);
	return (return_no);
}
#endif

int
check_maxmail(void)
{
	int maxsize, mailsize, oldnum;
	char maildir[STRLEN];

/*
 *	int maxmail;
 *
 *	maxmail = (HAS_PERM(PERM_SYSOP)) ?
 *	    	   MAX_SYSOPMAIL_HOLD : (HAS_PERM(PERM_BOARDS)) ? MAX_BMMAIL_HOLD : MAX_MAIL_HOLD;
 */

	set_safe_record();
	oldnum = currentuser.nummails;
	setmaildir(maildir, currentuser.userid);
	currentuser.nummails = get_num_records(maildir, sizeof (struct fileheader));
	if (oldnum != currentuser.nummails)
		substitute_record(PASSFILE, &currentuser, sizeof (currentuser),
				  usernum);
	maxsize = getmailboxsize(currentuser.userlevel);
	mailsize = getmailsize(currentuser.userid);
	if ( /* currentuser.nummails > maxmail || */ mailsize > maxsize) {
		clear();
		move(4, 0);

/*
 *		if(currentuser.nummails > maxmail)
 *			      prints("����˽���ż��ߴ� %d ��, �����ʼ�����: %d ��\n",
 *				    currentuser.nummails, maxmail);
 *
 *		if (currentuser.nummails > maxmail + 100) {
 *			snprintf(genbuf, sizeof(genbuf), "˽���ż�����: %d ��", currentuser.nummails);
 *			securityreport(genbuf);
 *		}
 *		if (mailsize > maxsize + 1000) {
 *			snprintf(genbuf, sizeof(genbuf), "˽���ż�����: %d K", mailsize);
 *			securityreport(genbuf);
 *		}
 */
		prints("�����ż������ߴ� %d K��������������: %d K\n", mailsize, maxsize);
		prints("����˽���ż��Ѿ�����, ���������䣬�����޷�ʹ�ñ�վ�����Ź��ܡ�\n");
		return YEA;
	} 
	return NA;
}

#ifdef INTERNET_EMAIL
int
bbs_sendmail(char *fname, char *title, char *receiver, int filter)
{
	/* Pudding: û��sendmailʱ��ת��internet�ʼ� */
#ifndef SENDMAIL
	return -1;
#else
	FILE *fin, *fout;

	if ((fin = fopen(fname, "r")) == NULL)
		return -1;

	snprintf(genbuf, sizeof(genbuf), "%s -f %s.bbs@%s %s", SENDMAIL, currentuser.userid, BBSHOST, receiver);
	if ((fout = popen(genbuf, "w")) == NULL) {
		fclose(fin);
		return -1;
	}

	/* write mail header */
	fprintf(fout, "MIME-Version: 1.0"CRLF);
	fprintf(fout, "Content-Type: text/plain; charset=gb2312"CRLF);
	fprintf(fout, "Content-Transfer-Encoding: 8bit"CRLF);

	fprintf(fout, "Return-Path: %s.bbs@%s"CRLF, currentuser.userid, BBSHOST);
	fprintf(fout, "Reply-To: %s.bbs@%s"CRLF, currentuser.userid, BBSHOST);
	fprintf(fout, "From: %s.bbs@%s"CRLF, currentuser.userid, BBSHOST);
	fprintf(fout, "To: %s"CRLF, receiver);
	fprintf(fout, "Subject: %s"CRLF, title);
	fprintf(fout, "X-Forwarded-By: %s (%s)"CRLF, currentuser.userid,
#ifdef MAIL_REALNAMES
		currentuser.realname);
#else
		currentuser.username);
#endif

	fprintf(fout, "X-Disclaimer: %s �Ա�������ˡ������"CRLF, BoardName);
//	fprintf(fout, "Precedence: junk"CRLF);
	fputs(CRLF, fout);

	while (fgets(genbuf, 255, fin) != NULL) {
		if (filter) {
			my_ansi_filter(genbuf);
		}

		/* avoid smtp server terminating session before mail complete sent */
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
	pclose(fout);
	return 0;
#endif
}
#endif

int
mfunc_cleanmark(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;

	fileinfo->flag |= FILE_READ;
	return KEEPRECORD;
}

int
mail_clear(int ent, struct fileheader *fileinfo, char *direct)
{
	process_records(direct, sizeof(struct fileheader), 1, 999999, mfunc_cleanmark, NULL);

	return NEWDIRECT;
}
