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

/*the code below is for post_stat */
struct postrecord {
	char user[IDLEN + 1];
	char title[49];
	unsigned short cnt;
};

struct userrecord {
	char user[IDLEN + 1];
	unsigned num;
	struct userrecord *next;
};

static unsigned short prused = 0;
static unsigned short prsize = 128;
static struct postrecord *precord = NULL;
static struct userrecord *urecord = NULL;

void
init_postrecord(void)
{
	prused = 0;
	prsize = 128;
	precord = NULL;
}

struct postrecord *
countpost(char *user, char *title)
{
	int i, j;

//init precord, prsize, prused
	if (precord == NULL) {
		precord = (struct postrecord *)
		    malloc(prsize * sizeof (struct postrecord));
		if (precord == NULL)
			return NULL;
		memset(precord, 0, prsize * sizeof (struct postrecord));
	}
//search user, title
	if (prused > 0) {
		for (i = 0; i < prused; i++) {
			if ((0 == strcmp(precord[i].user, user)) &&
			    (0 == strncmp(precord[i].title, title, 48))) {
//found, add cnt
				struct postrecord r;

				precord[i].cnt++;
				memcpy(&r, &precord[i], sizeof (r));
				for (j = i - 1; j >= 0; j--) {
					if (precord[j].cnt > r.cnt - 1)
						break;
				}

				if (j + 1 < i) {
					memcpy(&precord[i], &precord[j + 1], sizeof(r));
					memcpy(&precord[j + 1], &r, sizeof(r));
				}

				return precord;
			}
		}
	}
//not found, insert

	if (prused == prsize) {
//full ,need more space
//debug         prints("realloc: %d", prused);pressreturn();
		precord = realloc(precord,
				  (prsize +
				   128) * sizeof (struct postrecord));
		if (precord == NULL)
			return NULL;
		memset(&precord[prsize], 0,
		       128 * sizeof (struct postrecord));
		prsize += 128;
	}
//insert
	strlcpy(precord[prused].user, user, sizeof(precord[prused].user));
	strlcpy(precord[prused].title, title, sizeof(precord[prused].title));
	precord[prused].cnt = 1;
	prused++;
	return precord;
}

/* monster: ͳ���û�������Ŀ */
void
free_urecord(struct userrecord *p)
{
	if (p != NULL) {
		free_urecord(p->next);
		free(p);
		p = NULL;
	}
}

void
countuser(char *user, int num)
{
	struct userrecord *p = urecord, *prev = NULL;

	while (p && strcmp(user, p->user) >= 0) {
		prev = p;
		p = p->next;
	}

	if (prev && !strcmp(user, prev->user)) {
		prev->num += num;
		return;
	}

	if (prev == NULL) {
		if (urecord == NULL) {
			p = urecord = (struct userrecord *) calloc(1, sizeof (struct userrecord));
			p->next = NULL;
		} else {
			p = (struct userrecord *) calloc(1, sizeof (struct userrecord));
			p->next = urecord;
			urecord = p;
		}
	} else {
		p = (struct userrecord *) calloc(1, sizeof (struct userrecord));
		p->next = prev->next;
		prev->next = p;
	}

	strcpy(p->user, user);
	p->num = num;
}

/* Added by betterman 06/11/25 */
int
bm_ann_report(void)
{
        char bname[STRLEN], buf[256], tmpfile[PATH_MAX + 1];
        FILE *ftmp;
        int fd;
        time_t begin, end;
        int year, month, num;
        struct tm time;
        struct anntrace trace;
	struct userrecord *p = NULL;
	int pos, i;
	struct boardheader fh;

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;

	stand_title("ͳ�ƾ�������������");
	move(2, 0);
	make_blist();
	namecomplete("��������������: ", bname);
	if (*bname == '\0') {
		move(3, 0);
		prints("���������������");
		pressreturn();
		clear();
		return -1;
	}
	pos = search_record(BOARDS, &fh, sizeof (fh), cmpbnames, bname);
	if (!pos) {
		move(3, 0);
		prints("���������������");
		pressreturn();
		clear();
		return -1;
	}
        snprintf(tmpfile, 256, "0Announce/boards/%s/.trace", bname);
        if ((fd = open(tmpfile, O_RDONLY)) == -1) {
                prints("��¼�ļ��򿪳���");
                pressreturn();
                return -1;
	}
        getdata(3, 0, "������ʼ���: ", buf, 6, DOECHO, YEA);
        year = atoi(buf);
        if(year < 2000 || year > 2020){
                prints("��������");
                pressreturn();
                close(fd);
                return -1;
        }
        getdata(4, 0, "������ʼ�·�: ", buf, 6, DOECHO, YEA);
        month = atoi(buf) - 1;
        if(month < 0 || month > 11){
                prints("������·�");
                pressreturn();
                close(fd);
                return -1;
        }
        getdata(5, 0, "����ͳ������: ", buf, 6, DOECHO, YEA);
        num = atoi(buf);
        if(num < 1 || num > 12){
                prints("�����ͳ������");
                pressreturn();
                close(fd);
                return -1;
        }
        memset(&time, 0, sizeof(time));
        time.tm_sec = 0;
        time.tm_min = 0;         /* minutes */
        time.tm_hour = 0;        /* hours */
        time.tm_mday = 1;        /* day of the month */
        time.tm_mon = month;         /* month */
        time.tm_year = year - 1900;  /* year */
        time.tm_wday = 0;        /* mktime() ignores this content */
        time.tm_yday = 0;        /* mktime() ignores this content */
        time.tm_isdst = 0;       /* daylight saving time */
        if((begin = mktime(&time)) == -1){
                prints("��ʼ���ڴ���");
                pressreturn();
                close(fd);
                return -1;
        }
        time.tm_mon = month + num; /* mktime() will normalize the outside legal interval ^_^ */
        if((end = mktime(&time)) == -1){
                prints("�������ڴ���");
                pressreturn();
                close(fd);
                return -1;
        }

	free_urecord(urecord);
	urecord = NULL;

	while (read(fd, &trace, sizeof(trace)) == sizeof(trace)) {
               if(trace.otime < begin) continue;
               else if(trace.otime > end )break;
               else countuser(trace.executive, 1);
	}
        close(fd);

	snprintf(tmpfile, sizeof(tmpfile), "tmp/ann_trace.poststat.%d", getpid());
	if ((ftmp = fopen(tmpfile, "w")) == NULL) {
		prints("��¼�ļ��򿪳���");
		pressreturn();
		return -1;
	}
	fprintf(ftmp, "ͳ�ư��棺%s\n", bname);
	fprintf(ftmp, "��ʼʱ�䣺%s", ctime(&begin));
	fprintf(ftmp, "��ֹʱ�䣺%s\n", ctime(&end));
	fprintf(ftmp, "�� �� ��     �� ¼ ��  \n");
	fprintf(ftmp,
		"=========================================================================\n");

	p = urecord;
	i = 0;
	while (p) {
		fprintf(ftmp, "%-12.12s %5d\n", p->user, p->num);
		p = p->next;
	}
	free_urecord(urecord);
	urecord = NULL;

	fclose(ftmp);

	ansimore(tmpfile, NA);
	if (askyn("Ҫ��ͳ�ƽ���Ļص�������", NA, NA) == YEA) {
		char titlebuf[TITLELEN];

		snprintf(titlebuf, sizeof(titlebuf), "%s������������������¼��Ŀͳ�ƽ��", bname);
		mail_sysfile(tmpfile, currentuser.userid, titlebuf);
	}
	unlink(tmpfile);

	return 0;
        
}

/* monster: this function originally written by windeye, modified by monster */
int
bm_post_stat(void)
{
	char bname[STRLEN], num[8], buf[256], sbrd[STRLEN];
	char sdate[17] = { '\0' }, edate[17] = { '\0' }, suser[20], tmpfile[PATH_MAX + 1];
	int pos, tpos, i, j;
	struct boardheader fh;
	FILE *fp, *ftmp;
	char *phead, *ptail, *ptitle;
	struct postrecord *ppr = NULL;
	struct userrecord *p = NULL;

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;

	stand_title("ͳ�����·�������");
	move(2, 0);
	make_blist();
	namecomplete("��������������: ", bname);
	if (*bname == '\0') {
		move(3, 0);
		prints("���������������");
		pressreturn();
		clear();
		return -1;
	}
	pos = search_record(BOARDS, &fh, sizeof (fh), cmpbnames, bname);
	if (!pos) {
		move(3, 0);
		prints("���������������");
		pressreturn();
		clear();
		return -1;
	}
	if (fh.flag & ANONY_FLAG) {
		move(3, 0);
		prints("�����棬���Ǳ����");
		pressreturn();
		clear();
		return -1;
	}
	getdata(4, 0, "����λ����(һ������ֵ����λΪK): ", num, 6, DOECHO, YEA);
	tpos = atoi(num);
	if (tpos <= 0 || tpos > 64 * 1024) {
		prints("����λ��\n");
		pressreturn();
		return -1;
	}
	tpos *= (-1024);

	if ((fp = fopen("reclog/trace", "r")) == NULL) {
		prints("��¼�ļ��򿪳���");
		pressreturn();
		return -1;
	}

	if (fseek(fp, tpos, SEEK_END) == -1)
		rewind(fp);

	/* skip a line to accurate datetime */
	if (fgets(buf, sizeof(buf), fp) == NULL) {
		prints("��¼�ļ��򿪳���");
		pressreturn();
		return -1;
	}

	while (fgets(buf, sizeof(buf), fp))
		if (sscanf(buf, "%*s %16c", sdate) > 0)
			break;

	if (sdate[0] == '\0') {
		prints("�Ҳ������ļ�¼");
		pressreturn();
		return -1;
	}

	move(5, 0);
	prints("��¼ͳ�ƿ�ʼʱ��Ϊ: %s", sdate);
	move(6, 0);
	if (askyn("�Ƿ����ͳ�ƣ�", NA, NA) != YEA) {
		return -1;
	}
	init_postrecord();
	snprintf(sbrd, sizeof(sbrd), "' on '%s'\n", bname);	//fgets�Ľ�����ܲ�����\n
	while (fgets(buf, 255, fp) != NULL) {
//bufĩβΪsbrd
//debug:        if(strstr(buf,bname) == NULL) continue;
//debug:        if((ptail = strchr(buf,'\n')) != NULL) *ptail = '\0';
		ptail = buf + strlen(buf) - strlen(sbrd);
		if (strcmp(ptail, sbrd)) {
			prints("ptail is :%s\nsbrd is :%s\n", ptail, sbrd);
			continue;
		}

		/* monster: parse author & title of article from trace line */
		j = 0;
		while ((buf[j] != 0) && (buf[j] != ' ') && (j < 255))
			j++;
		if (j == 0)
			continue;
		// snprintf(suser, (size_t)(j + 1), "%s", buf);
		strlcpy(suser, buf, j + 1);
		if ((phead = strstr(buf, " posted '")) == NULL)
			continue;
		ptitle = phead + 9;
		ptail = strchr(ptitle, '\'');
		if (ptail == NULL)
			continue;
		else
			*ptail = '\0';
		if (strncmp(ptitle, "Re: ", 4) == 0)
			ptitle += 4;

		ppr = countpost(suser, ptitle);
		if (ppr == NULL) {
			report("bm_post_stat: memory not enough(used %d of %d)!\n",
				prused, (int)sizeof (struct postrecord));
			fclose(fp);
			return 0;
		}

		/* monster: due to Easterly's request */
		sscanf(buf, "%*s %16c", edate);
	}
	fclose(fp);

	snprintf(tmpfile, sizeof(tmpfile), "tmp/trace.poststat.%d", getpid());
	if ((ftmp = fopen(tmpfile, "w")) == NULL) {
		prints("��¼�ļ��򿪳���");
		pressreturn();
		return -1;
	}

	if (edate[0] == '\0') {
		time_t now = time(NULL);

		snprintf(edate, sizeof(edate), "%16.16s", ctime(&now));
	}

	fprintf(ftmp, "ͳ�ư��棺%s\n", bname);
	fprintf(ftmp, "��ʼʱ�䣺%s\n", sdate);
	fprintf(ftmp, "��ֹʱ�䣺%s\n\n", edate);
	fprintf(ftmp, "ƪ ��  �� �� ��       �� ��\n");
	fprintf(ftmp,
		"=========================================================================\n");

	if (ppr != NULL) {
		for (i = 0; i < prused; i++) {
			fprintf(ftmp, "%5d  %-12.12s   %-48.48s\033[m\n",
				ppr[i].cnt, ppr[i].user, ppr[i].title);
			countuser(ppr[i].user, ppr[i].cnt);
		}
	}

	fprintf(ftmp, "\n\n");
	fprintf(ftmp,
		"�� �� ��     ƪ ��              �� �� ��     ƪ ��\n");
	fprintf(ftmp,
		"=========================================================================\n");

	p = urecord;
	i = 0;
	while (p) {
		fprintf(ftmp, "%-12.12s %s%5d\033[m              ", p->user,
			(p->num < 100) ? "" : "\033[1;31m", p->num);
		if ((++i) % 2 == 0)
			fputc('\n', ftmp);

		p = p->next;
	}
	free_urecord(urecord);
	urecord = NULL;

	fclose(ftmp);

	ansimore(tmpfile, NA);
	if (askyn("Ҫ��ͳ�ƽ���Ļص�������", NA, NA) == YEA) {
		char titlebuf[TITLELEN];

		snprintf(titlebuf, sizeof(titlebuf), "%s������������Ŀͳ�ƽ��", bname);
		mail_sysfile(tmpfile, currentuser.userid, titlebuf);
	}
	if (ppr != NULL)
		free(ppr);
	unlink(tmpfile);

	return 0;
}

/*
	 ���/��� ��غ���
*/

int
addtodeny(char *uident, char *msg, int ischange, int flag, struct fileheader *header)
{
	char buf[50], strtosave[256], buf2[50], tmpbuf[1024];
	char tmpfile[PATH_MAX + 1], orgfile[PATH_MAX + 1], direct[PATH_MAX + 1]; 
	struct denyheader dh;
	time_t nowtime;
	int day, seek, err = 0;
	FILE *rec, *org;

	if (uident[0] == '\0' || getuser(uident, NULL) == 0)
		return -1;

	if (flag & D_FULLSITE) {
		strcpy(direct, "boards/.DENYLIST");
	} else {
		setboardfile(direct, currboard, ".DENYLIST");
	}

	seek = search_record(direct, &dh, sizeof(struct denyheader), denynames, uident);

	if (seek && !ischange) {
		move(3, 0);
		prints(" %s �Ѿ��������С�", (flag & D_ANONYMOUS) ? "��������" : uident);
		pressanykey();
		return -1;
	}

	if (ischange && !seek) {
		move(3, 0);
		prints(" %s ���������С�", (flag & D_ANONYMOUS) ? "��������" : uident);
		pressanykey();
		return -1;
	}

	getdata(3, 0, "����˵��: ", buf, 40, DOECHO, YEA);
	if (killwordsp(buf) == 0)
		return -1;

	nowtime = time(NULL);
	getdatestring(nowtime);

	do {
		getdata(4, 0, "������ʱ��[ȱʡΪ 1 ��, 0 Ϊ����]: ", buf2, 4, DOECHO, YEA);
		day = (buf2[0] == '\0') ? 1 : atoi(buf2);

		/* monster: ���ư����������� */
		if (!HAS_PERM(PERM_SYSOP | PERM_OBOARDS) && day > 21)
			continue;

		if (day == 0) {
			move(6, 0);
			outs("ȡ���������");
			pressanykey();
			return -1;
		}
	} while (day < 0 || (!HAS_PERM(PERM_OBOARDS | PERM_SYSOP) && day > 21));

	nowtime += day * 86400;
	getdatestring(nowtime);
	snprintf(strtosave, sizeof(strtosave), "%-12s %-40s %14.14s���", uident, buf, datestring);
	/* Pudding: �ı���ĸ�ʽ */
/* 	if (!ischange) { */
	
		sprintf(msg,
			"\n  \033[1;32m%s\033[m ����: \n\n"
			"    ���Ѿ��� \033[1;32m%s\033[m ȡ���� \033[1;4;36m%s\033[m %s�ķ���Ȩ�� \033[1;35m%d\033[m �졣\n\n"
			"    ���������ԭ����: \033[1;4;33m%s\033[m\n\n"
			"    ������ \033[1;35m%14.14s\033[m ��ý�⡣�������ʣ���鿴\033[1;33mվ����\033[m��ز��֡�\n\n"
			"    �������飬�뵽 \033[1;4;36mComplain\033[m �水��ʽ������ߣ���л������\n\n",
			uident,
			currentuser.userid,
			(flag & D_FULLSITE) ? "��վ" : currboard, (flag & D_FULLSITE) ? "" : "��",
			day, buf,
			datestring);
/*		
	} else {
		sprintf(msg,
			"\n  \033[1;4;32m%s\033[m ����: \n\n"
			"    �������� \033[1;4;36m%s\033[m ��ȡ�� \033[1;4;33m����\033[m Ȩ�����⣬�ֱ�����£�\n\n"
			"    �����ԭ�� \033[1;4;33m%s\033[m\n\n"
			"    �����ڿ�ʼ��ֹͣ��Ȩ��ʱ�䣺 \033[1;4;35m%d\033[m ��\n\n"
			"    ������ \033[1;4;35m%14.14s\033[m �� \033[1;4;32m%s\033[m ���������⡣\n\n",
			uident, (flag & D_FULLSITE) ? "ȫվ" : currboard,
			buf, day, datestring, currentuser.userid);			
	}	
*/
	memset(&dh, 0, sizeof (dh));
	snprintf(dh.filename, sizeof(dh.filename), "D.%d.%c", time(NULL), (getpid() % 26) + 'A');
	setboardfile(tmpfile, currboard, dh.filename);

	if ((rec = fopen(tmpfile, "w")) == NULL) {
		err = 1;
		goto error_process;
	}

	fprintf(rec, "���ԭ��%s\n", buf);
	fprintf(rec, "������ڣ�%14.14s\n", datestring);
	fprintf(rec, "ִ���ߣ�  %s\n", currentuser.userid);

	if ((flag & D_ANONYMOUS) == 0)
		fprintf(rec, "����ߣ�  %s\n", uident);

	fprintf(rec, "\n���ģ�\n\n");

	if (flag & D_NOATTACH) {
		fprintf(rec, "(ֱ�ӷ�����޸���)\n");
	} else {
		if (ischange) {
			setboardfile(orgfile, currboard, header->filename);
			if ((org = fopen(orgfile, "r")) == NULL) {
				err = 1;
				goto error_process;
			}

			while (fgets(tmpbuf, 1024, org) != NULL) {
				if (strncmp(tmpbuf, "����", 4) == 0)
					break;
			}

			if (fgets(tmpbuf, 1024, org)) {
				while (fgets(tmpbuf, 1024, org) != NULL)
					fputs(tmpbuf, rec);
			}

			fclose(org);
			unlink(orgfile);
		} else {
			setboardfile(orgfile, currboard, header->filename);
			if ((org = fopen(orgfile, "r")) == NULL) {
				err = 1;
				goto error_process;
			}

			while (fgets(tmpbuf, 1024, org) != NULL)
				fputs(tmpbuf, rec);
		}
	}

      error_process:
	if (rec)
		fclose(rec);

	if (err) {
		unlink(tmpfile);
		presskeyfor("���ʧ��, ����ϵͳά����ϵ, ��<Enter>����...");
		return (-1);
	}

	strlcpy(dh.executive, currentuser.userid, sizeof(dh.executive));
	strlcpy(dh.blacklist, uident, sizeof(dh.blacklist));
	strlcpy(dh.title, buf, sizeof(dh.title));
	dh.undeny_time = nowtime;
	dh.filetime = time(NULL);

	if (append_record(direct, &dh, sizeof (dh)) == -1) {
		err = 1;
		goto error_process;
	}

	return 1;
}

int
delfromdeny(char *uident, int flag)
{
	int uid;
	char repbuf[STRLEN], msgbuf[1024];
	struct userec lookupuser;

	if (uident[0] == '\0')
		return -1;

	if ((uid = getuser(uident, NULL)) == 0)
		return (flag & D_IGNORENOUSER) ? 1 : -1;

	if (flag & D_FULLSITE) {
		if (get_record(PASSFILE, &lookupuser, sizeof(struct userec), uid) == -1)
			return -1;

		lookupuser.userlevel |= PERM_POST;
		substitute_record(PASSFILE, &lookupuser, sizeof (struct userec), uid);

		snprintf(repbuf, sizeof(repbuf), "�ָ� %s ��ȫվ����Ȩ��", uident);
		securityreport(repbuf);
		snprintf(msgbuf, sizeof(msgbuf), 
				"\n  %s ���ѣ�\n\n"
				"    ����ʱ���ѹ����ָֻ����ڱ�վ�ķ���Ȩ����\n\n",
				uident);
		autoreport(repbuf, msgbuf, NA, uident, NULL);
	} else {
		snprintf(repbuf, sizeof(repbuf), "�ָ� %s �� %s ��ķ���Ȩ��", uident, currboard);
		securityreport(repbuf);
		snprintf(msgbuf, sizeof(msgbuf),
			"\n  \033[1;32m%s\033[m ���ѣ�\n\n"
			"    ����ʱ���ѹ����ָֻ����� \033[1;4;36m%s\033[m ��ķ���Ȩ����\n\n",
			uident, currboard);
		autoreport(repbuf, msgbuf, (flag & D_ANONYMOUS) ? NA : YEA, uident, NULL);
		return 1;
	}
	return 1;
}

/* ȫվ��� */
int
add_deny_fullsite(void)
{
	int uid;
	char msgbuf[1024], repbuf[STRLEN], uident[IDLEN + 2];
	struct userec lookupuser;

	stand_title("���ʹ����");
	move(2, 0);
	usercomplete("����׼��������������ʹ����ID: ", uident);

	if (addtodeny(uident, msgbuf, 0, D_NOATTACH | D_FULLSITE | D_NODENYFILE, NULL) == 1) {
		if ((uid = getuser(uident, &lookupuser)) != 0) {
			lookupuser.userlevel &= ~PERM_POST;
			substitute_record(PASSFILE, &lookupuser, sizeof(lookupuser), uid);

			snprintf(repbuf, sizeof(repbuf), "ȡ�� %s ��ȫվ����Ȩ��", uident);
			securityreport(repbuf);
			if (msgbuf[0] != '\0') {
				       autoreport(repbuf, msgbuf, NA, uident, NULL);
			}
		}
	}
	return FULLUPDATE;
}

int
del_deny_fullsite(int ent, struct denyheader *fileinfo, char *direct)
{
	int i;

	move(t_lines - 1, 0);
	if (askyn("ȷ���ָ�����ߵķ���Ȩ��", NA, NA) == YEA) {
		if ((i = delfromdeny(fileinfo->blacklist, D_FULLSITE | D_IGNORENOUSER)) == 1) {
			delete_record("boards/.DENYLIST", sizeof (struct denyheader), ent);

			if (get_num_records("boards/.DENYLIST", sizeof (struct denyheader)) == 0)
				return DOQUIT;
		}
	}

	return PARTUPDATE;
}

int
change_deny_fullsite(int ent, struct denyheader *fileinfo, char *direct)
{
	char msgbuf[1024], repbuf[STRLEN], uident[IDLEN + 2];

	stand_title("���ʹ���� (�޸�)");

	if (addtodeny(fileinfo->blacklist, msgbuf, 1,
		      ((digestmode == 10) ? D_ANONYMOUS : 0) | D_FULLSITE | D_NODENYFILE, (struct fileheader *)fileinfo) == 1) {
		delete_record(currdirect, sizeof(struct denyheader), ent);
		snprintf(repbuf, sizeof(repbuf), "�޸Ķ� %s ��ȡ��ȫվ����Ȩ���Ĵ���", fileinfo->blacklist);
		securityreport(repbuf);
		if (msgbuf[0] != '\0') {
			autoreport(repbuf, msgbuf, NA, uident, NULL);
		}
	}
	return FULLUPDATE;
}

int
read_denyinfo(int ent, struct denyheader *fileinfo, char *direct)
{
	clear();
	getdatestring(fileinfo->undeny_time);
	prints("���ԭ��%s\n", fileinfo->title);
	prints("������ڣ�%14.14s\n", datestring);
	prints("ִ���ߣ�  %s\n", fileinfo->executive);
	prints("����ߣ�  %s\n", fileinfo->blacklist);
	prints("\n���ģ�\n\n");
	prints("(ֱ�ӷ�����޸���)\n");
	pressanykey();

	return FULLUPDATE;
}

struct one_key deny_comms[] = {
	{ 'a',		add_deny_fullsite },
	{ 'd', 		del_deny_fullsite },
	{ 'E',		change_deny_fullsite },
	{ 'r',		read_denyinfo },
	{ '\0', 	NULL }
};

void
denytitle(void)
{
	strcpy(genbuf, (chkmail(NA) == YEA) ? "[�����ż�]" : BoardName);
	showtitle("ȫվ�������", genbuf);
	prints("�뿪[\033[1;32m��\033[m,\033[1;32mq\033[m] ѡ��[\033[1;32m��\033[m,\033[1;32m��\033[m] �Ķ�[\033[1;32m��\033[m,\033[1;32mRtn\033[m] ���[\033[1;32ma\033[m] �޸ķ��[\033[1;32mE\033[m] ������[\033[1;32md\033[m] ����[\033[1;32mh\033[m] \n");
	prints("\033[1;37;44m ���   %-12s %6s %-40s[����б�] \033[m\n", "�� �� ��", "��  ��", " �� �� ԭ ��");
}

int
bm_post_perm(void)
{
	digestmode = 11;
	if (get_num_records("boards/.DENYLIST", sizeof(struct denyheader)) > 0) {
		i_read(ADMIN, "boards/.DENYLIST", NULL, NULL, denytitle, readdoent, update_endline, &deny_comms[0],
		       get_records, get_num_records, sizeof(struct denyheader));
	} else {
		modify_user_mode(ADMIN);
		add_deny_fullsite();
	}
	digestmode = NA;
	return 0;
}

int
update_boardlist(int action, slist *list, char *uident)
{
	char boardctl[PATH_MAX + 1];

	sethomefile(boardctl, uident, "board.ctl");
	switch (action) {
	case LE_ADD:
		file_appendline(boardctl, currboard);
		break;
	case LE_REMOVE:
		del_from_file(boardctl, currboard);
		break;
	}

	/* monster: ���²�������������ư��б� */
	if (!strcmp(uident, currentuser.userid)) {
		load_restrict_boards();
	}
	return YEA;
}

int
control_user(void)
{
	struct boardheader *bp;
	char boardctl[STRLEN];

	bp = getbcache(currboard);
	if ((bp->flag & BRD_RESTRICT) == 0)
		return DONOTHING;

	setboardfile(boardctl, currboard, "board.ctl");
	if (!HAS_PERM(PERM_SYSOP) && (!current_bm || !dashf(boardctl)))
		return DONOTHING;

	listedit(boardctl, "�༭�������Ա������", update_boardlist);
	return FULLUPDATE;
}

/* monster: ��ѯָ���û�������� */

static int post_count;

void
poststat(FILE * fp, char *board, char *owner, time_t dt)
{
	time_t at;
	int fd, low, high, mid, stated = NA;
	char filename[PATH_MAX + 1];
	struct fileheader header;
	struct stat st;

	dt = time(NULL) - 86400 * dt;
	snprintf(filename, sizeof(filename), "boards/%s/.DIR", board);

	if ((fd = open(filename, O_RDONLY)) == -1)
		return;

	if (fstat(fd, &st) == -1) {
		close(fd);
		return;
	}

	/* monster: ���ö��ּ����ӿ��ٶ� */
	low = 0;
	high = st.st_size / sizeof(header) - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		if (lseek(fd, (off_t)(mid * sizeof(header)), SEEK_SET) == -1)
			return;
		if (read(fd, &header, sizeof (header)) != sizeof (header))
			return;
		at = header.filetime;

		if (dt > at) {
			low = mid + 1;
		} else if (dt < at) {
			high = mid - 1;
		} else {
			break;
		}
	}

	do {
		at = header.filetime;
		if (!strcmp(header.owner, owner) && (at > dt)) {
			if (stated == NA) {
				stated = YEA;
				fprintf(fp, "\n%s: \n", board);
			}
			fprintf(fp, "%5d.  %24.24s  %s\n", ++post_count, ctime(&at), header.title);
		}
	} while (read(fd, &header, sizeof (header)) == sizeof (header));
	close(fd);
}

int
user_poststat(void)
{
	char buf[PATH_MAX + 1], id[IDLEN + 2];
	FILE *fp;
	int i, day;

	stand_title("ͳ���û��������");
	modify_user_mode(ADMIN);

	move(2, 0);
	usercomplete("����������ѯ��ʹ���ߴ���: ", id);
	if (id[0] == '\0') {
		clear();
		return 0;
	}

	getdata(3, 0, "ͳ�����������ķ������(1~7): ", buf, 2, DOECHO, YEA);
	move(5, 0);

	day = buf[0] - '0';
	if (day < 1 || day > 7) {
		prints("�ܱ�Ǹ��ϵͳֻ��ͳ��1~7���ڵķ������");
		pressanykey();
		return 0;
	}

	snprintf(buf, sizeof(buf), "tmp/user_poststat.%s.%d", id, getpid());
	if ((fp = fopen(buf, "w")) == NULL) {
		prints("ͳ��ʧ��");
		pressanykey();
		return 0;
	}

	post_count = 0;
	prints("ͳ���У����Ժ�...");
	refresh();

	fprintf(fp, "������%s���%d��ķ��������ͳ�ƽ��������\n", id, day);
	fprintf(fp, "�����ư��Լ�ʹ��������������£�\n");

	resolve_boards();
	for (i = 0; i < numboards; i++) {
		/* monster: ��ͳ�����ư������� */
		if ((bcache[i].level & PERM_POSTMASK) ||
		    ((bcache[i].level & ~PERM_POSTMASK) != 0))
			continue;

		poststat(fp, bcache[i].filename, id, day);
	}
	fclose(fp);

	ansimore(buf, NA);
	if (askyn("Ҫ��ͳ�ƽ���Ļص�������", NA, NA) == YEA) {
		char titlebuf[TITLELEN];

		snprintf(titlebuf, sizeof(titlebuf), "%s���%d��ķ������", id, day);
		mail_sysfile(buf, currentuser.userid, titlebuf);
	}
	unlink(buf);
	return 0;
}

/* monster: �������������� */

inline int
bmfunc_match(struct fileheader *fileinfo, struct bmfuncarg *arg)
{
	if (arg->flag & LOCATE_THREAD) {
		return (fileinfo->id == arg->id) ? YEA : NA;
	} else if (arg->flag & LOCATE_AUTHOR) {
		return (!strcasecmp(fileinfo->owner, arg->author)) ? YEA : NA;
	} else if (arg->flag & LOCATE_TITLE) {
		return (strstr2(fileinfo->title, arg->title) != NULL) ? YEA : NA;
	} else if ((arg->flag & LOCATE_SELECTED) && (fileinfo->flag & FILE_SELECTED)) {
		fileinfo->flag &= ~FILE_SELECTED;	// remove selection mark
		return YEA;
	} else if (arg->flag & LOCATE_ANY) {
		return YEA;
	}
	return NA;
}

int
bmfunc_del(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg;
	char *ptr, filename[PATH_MAX + 1];

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	if (INMAIL(uinfo.mode)) {           // ɾ���ʼ�
		if (fileinfo->flag & FILE_MARKED)
			return KEEPRECORD;

		strlcpy(filename, currdirect, sizeof(filename));
		if ((ptr = strrchr(filename, '/')) == NULL)
			return KEEPRECORD;
		strcpy(ptr + 1, fileinfo->filename);
		unlink(filename);
	} else {                        // ɾ����������
		if (fileinfo->flag & (FILE_MARKED | FILE_DIGEST))
			return KEEPRECORD;

		cancelpost(currboard, currentuser.userid, fileinfo, 0, YEA);
	}

	return REMOVERECORD;
}

int
bmfunc_underline(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg;

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	if (fileinfo->flag & FILE_NOREPLY) {
		fileinfo->flag &= ~FILE_NOREPLY;
	} else {
		fileinfo->flag |= FILE_NOREPLY;
	}
	return KEEPRECORD;
}

int
bmfunc_mark(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg;

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	if (fileinfo->flag & FILE_MARKED) {
		fileinfo->flag &= ~FILE_MARKED;
	} else {
		fileinfo->flag |= FILE_MARKED;
	}
	return KEEPRECORD;
}

int
bmfunc_cleanmark(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg;

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	if (fileinfo->flag & FILE_DIGEST)
		dodigest(0, fileinfo, currdirect, YEA, NA);
	fileinfo->flag &= FILE_ATTACHED;
	return KEEPRECORD;
}

int
bmfunc_import(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg; /* arg->extrarg[0] ==> import_visited */
	char *ptr, filename[PATH_MAX + 1];

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	strlcpy(filename, currdirect, sizeof(filename));
	if ((ptr = strrchr(filename, '/')) == NULL)
		return KEEPRECORD;
	strcpy(ptr + 1, fileinfo->filename);

	if ((*(int *)arg->extrarg[0] == YEA) || !(fileinfo->flag & FILE_VISIT))
		if (ann_import_article(filename, fileinfo->title, fileinfo->owner, fileinfo->flag & FILE_ATTACHED, YEA) == 0)
			fileinfo->flag |= FILE_VISIT;

	return KEEPRECORD;
}

int
bmfunc_savepost(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg;

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	ann_savepost(currboard, fileinfo, YEA);
	return KEEPRECORD;
}

/*
   monster: ��ָ�����¼��뵽�ϼ���

      �÷�: addtocombine(���¾��, �ϼ����, ������Ϣ);
*/

int
addtocombine(FILE * fp1, FILE * fp, struct fileheader *fileinfo)
{
	int i, len, blankline = 0;
	char temp[LINELEN];

	/* ���ɷָ��� */
	snprintf(temp, sizeof(temp), "(%24.24s) ", ctime(&fileinfo->filetime));
	len = strlen(fileinfo->owner) + strlen(temp) + 1;
	fprintf(fp, "\033[1;36m�������� \033[37m%s %s\033[36m ", fileinfo->owner, temp);

	if (len % 2 != 0) {
		fputc(' ', fp);
		++len;
	}
	len = 30 - len / 2;
	for (i = 0; i < len; i++)
		fprintf(fp, "��");
	fprintf(fp, "\033[m\n\n");

	/* ������ͷ */
	while (fgets(temp, sizeof(temp), fp1) && temp[0] != '\n');

	/* ����������(�ļ�ͷ�͵�һ�� '--\n' ֮��)����ϼ��ļ��� */
	while (fgets(temp, sizeof(temp), fp1)) {
		if (temp[0] == '\n' || temp[0] == '\r') {
			blankline = 1;
			continue;
		} else {
			if (blankline) fputc('\n', fp);
			blankline = 0;
		}

		if (temp[0] == '-' && temp[1] == '-' && temp[2] == '\n')
			break;
		fputs(temp, fp);
	}
	fputc('\n', fp);

	return 0;
}

int
bmfunc_combine(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg; /* arg->extrarg[0] ==> count, [1] ==> fp, [2] ==> dele_orig */
	char *ptr, filename[PATH_MAX + 1];
	FILE *fp;

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	strlcpy(filename, currdirect, sizeof(filename));
	if ((ptr = strrchr(filename, '/')) == NULL)
		return KEEPRECORD;
	strcpy(ptr + 1, fileinfo->filename);

	if ((fp = fopen(filename, "r")) != NULL) {
		addtocombine(fp, (FILE *)arg->extrarg[1], fileinfo);
		fclose(fp);
		++(*(int *)arg->extrarg[0]);
	}

	if ((*(int *)arg->extrarg[2] == YEA) && !(fileinfo->flag & FILE_MARKED)) {
		if (INMAIL(uinfo.mode)) {
			unlink(filename);
		} else {
			if (!(fileinfo->flag & FILE_DIGEST)) {
				cancelpost(currboard, currentuser.userid, fileinfo, NA, YEA);
			} else {
				return KEEPRECORD;
			}
		}
		return REMOVERECORD;
	}

	return KEEPRECORD;
}

int
bmfunc(int ent, struct fileheader *fileinfo, char *direct, int dotype)
{
	FILE *fp;
	char ch[7] = { '\0' }, filename[PATH_MAX + 1];
	int dotype2, dele_orig, count = 0, first = 1, end = 999999;
	struct bmfuncarg arg;

	static char *items[5] =
		{ "��ͬ����", "��ͬ����", "�������" , "����", "ѡ������"};

	static char *items2[6] =
		{ "ɾ��", "����", "������", "������", "�ݴ浵", "�ϼ�" };

	if (!strcmp(currboard, "syssecurity") || !current_bm || digestmode)
		return DONOTHING;

	saveline(t_lines - 1, 0);
	clear_line(t_lines - 1);

	if (dotype < 0) {
		getdata(t_lines - 1, 0,	"ִ��: 1) ��ͬ����  2) ��ͬ����  3) �������  4) ����  0) ȡ�� [0]: ",
			ch, 2, DOECHO, YEA);
		dotype = ch[0] - '1';
	}

	switch (dotype) {
	case 0:		/* ��ͬ���� */
		arg.id = fileinfo->id;
		arg.flag = LOCATE_THREAD;
		break;
	case 1:		/* ��ͬ���� */
		strlcpy(arg.author, fileinfo->owner, sizeof(arg.author));
		arg.flag = LOCATE_AUTHOR;
		break;
	case 2:		/* ������� */
		arg.flag = LOCATE_TITLE;
		break;
	case 3:		/* ���� */
		arg.flag = LOCATE_ANY;
		break;
	case 4:		/* ѡ������ */
		arg.flag = LOCATE_SELECTED;
		break;
	default:
		goto quit;
	}

	snprintf(genbuf, sizeof(genbuf), "%s (1)ɾ�� (2)���� (3)������ (4)������ (5)�ݴ浵 (6)�ϼ� ? [0]: ",
		 items[dotype]);
	getdata(t_lines - 1, 0,	genbuf, ch, 2, DOECHO, YEA);
	dotype2 = ch[0] - '1';

	if (dotype2 < 0 || dotype2 > 5)
		goto quit;

	move(t_lines - 1, 0);
	snprintf(genbuf, sizeof(genbuf), "ȷ��Ҫִ��%s[%s]��", items[dotype], items2[dotype2]);
	if (askyn(genbuf, NA, NA) == NA)
		goto quit;

	switch (dotype) {
	case 0:		/* ��ͬ���� */
		move(t_lines - 1, 0);
		snprintf(genbuf, sizeof(genbuf), "�Ƿ�Ӵ������һƪ��ʼ%s (Y)��һƪ (N)Ŀǰ��һƪ", items2[dotype2]);
		if (askyn(genbuf, YEA, NA) == YEA) {
			if ((first = locate_article(direct, fileinfo, ent, LOCATE_THREAD | LOCATE_FIRST, &fileinfo->id)) == -1)
				first = ent;
		} else {
			first = ent;
		}
		break;
	case 1:         /* ��ͬ���� */
		move(t_lines - 1, 0);
		snprintf(genbuf, sizeof(genbuf), "�Ƿ�Ӵ����ߵ�һƪ��ʼ%s (Y)��һƪ (N)Ŀǰ��һƪ", items2[dotype2]);
		if (askyn(genbuf, YEA, NA) == YEA) {
			if ((first = locate_article(direct, fileinfo, ent, LOCATE_AUTHOR | LOCATE_FIRST, fileinfo->owner)) == -1)
				first = ent;
		} else {
			first = ent;
		}
		break;
	case 2:         /* ������� */
		clear_line(t_lines - 1);
		getdata(t_lines - 1, 0, "����������ؼ���: ", arg.title, sizeof(arg.title), DOECHO, YEA);
		if (arg.title[0] == '\0') {
			presskeyfor("����ؼ��ֲ���Ϊ��, �����������...");
			goto quit;
		}
		break;
	case 3:         /* ���� */
		clear_line(t_lines - 1);
		getdata(t_lines - 1, 0, "��ƪ���±��: ", ch, 7, DOECHO, YEA);
		if ((first = atoi(ch)) <= 0) {
			move(t_lines - 1, 50);
			prints("������...");
			egetch();
			goto quit;
		}
		getdata(t_lines - 1, 25, "ĩƪ���±��: ", ch, 7, DOECHO, YEA);
		if ((end = atoi(ch)) <= first) {
			move(t_lines - 1, 50);
			prints("������...");
			egetch();
			goto quit;
		}
		break;
	case 4:		/* ѡ������ */
		first = 1;
		end = 999999;
		break;
	}

	switch (dotype2) {
	case 0:         /* ɾ�� */
		process_records(direct, sizeof(struct fileheader), first, end, bmfunc_del, &arg);
		update_lastpost(currboard);
		break;
	case 1:         /* ���� (ʹ���²��ɻظ�) */
		process_records(direct, sizeof(struct fileheader), first, end, bmfunc_underline, &arg);
		break;
	case 2:		/* ������ */
		process_records(direct, sizeof(struct fileheader), first, end, bmfunc_cleanmark, &arg);
		break;
	case 3:		/* ������ */
		if (ann_loadpaths() == -1) {
			presskeyfor("�Բ���, ��û���趨˿·��˿·�趨����. ���� f �趨˿·.");
		} else {
			int import_visited;

			import_visited = askyn("�Ƿ���¼�����뾫����������", YEA, YEA);
			arg.extrarg[0] = &import_visited;

			update_ainfo_title(YEA);
			if (process_records(direct, sizeof(struct fileheader), first, end, bmfunc_import, &arg) == -1)
				presskeyfor("����ʧ��, ����ϵͳά����ϵ, �����������...");
			update_ainfo_title(NA);
		}
		break;
	case 4:		/* �ݴ浵 */
		process_records(direct, sizeof(struct fileheader), first, end, bmfunc_savepost, &arg);
		break;
	case 5:		/* �ϼ� */
		clear_line(t_lines - 1);
		dele_orig = askyn("�Ƿ�Ҫɾ��ԭ�ģ�", NA, NA);

		snprintf(filename, sizeof(filename), "boards/.tmp/combine.%d.A", time(NULL));
		if ((fp = fopen(filename, "w")) == NULL) {
			presskeyfor("����ʧ��, ����ϵͳά����ϵ, �����������...");
			goto quit;
		}

		switch (dotype) {
		case 0:		/* ��ͬ���� */
			if ((fileinfo->title[0] == 'R' || fileinfo->title[0] == 'r') &&
			     fileinfo->title[1] == 'e' && fileinfo->title[2] == ':' &&
			     fileinfo->title[3] == ' ') {
				snprintf(save_title, sizeof(save_title), "���ϼ���%s", fileinfo->title + 4);
			} else {
				snprintf(save_title, sizeof(save_title), "���ϼ���%s", fileinfo->title);
			}
			break;
		case 1:         /* ��ͬ���� */
			snprintf(save_title, sizeof(save_title), "���ϼ���%s������", arg.author);
			break;
		case 2:         /* ������� */
			snprintf(save_title, sizeof(save_title), "���ϼ���%s", arg.title);
			break;
		case 3:         /* ���� */
			snprintf(save_title, sizeof(save_title), "���ϼ���%d - %d", first, end);
			break;
		case 4:		/* ѡ������ */
			snprintf(save_title, sizeof(save_title), "���ϼ���%s��ѡ������", currboard);
			break;
		}

		arg.extrarg[0] = &count;
		arg.extrarg[1] = fp;
		arg.extrarg[2] = &dele_orig;
		process_records(direct, sizeof(struct fileheader), first, end, bmfunc_combine, &arg);

		fprintf(fp, "\033[m\n--\n\033[1;%dm�� ��Դ:. %s %s. [FROM: %s]\033[m\n",
			(currentuser.numlogins % 7) + 31, BoardName, BBSHOST, currentuser.lasthost);
		fclose(fp);

		if (count > 0) {
			if (strncmp(save_title, "���ϼ������ϼ���", 16) == 0) {
				postfile(filename, currboard, save_title + 8, 2);
				report("posted '%s' on '%s'", save_title + 8, currboard);
			} else {
				postfile(filename, currboard, save_title, 2);
				report("posted '%s' on '%s'", save_title, currboard);
			}
		}
		update_lastpost(currboard);
		update_total_today(currboard);
		unlink(filename);
		break;
	}

	return DIRCHANGED;

quit:
	saveline(t_lines - 1, 1);
	return DONOTHING;
}

int
bmfuncs(int ent, struct fileheader *fileinfo, char *direct)
{
	return bmfunc(ent, fileinfo, direct, -1);
}
