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

/* written & mantained by monster (monster@marco.zsu.edu.cn) */

#include "bbs.h"

struct anninfo ainfo;

static int sauthor = 1;
static struct annpath paths[8];

/* wrapper of getdata */
int
a_prompt(char *pmt, char *buf, int len, int clearlabel)
{
	saveline(t_lines - 1, 0);
	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, pmt, buf, len, DOECHO, clearlabel);
	my_ansi_filter(buf);
	saveline(t_lines - 1, 1);
	return (buf[0] == '\0') ? -1 : 0;
}

static int
real_check_annmanager(char *direct)
{
	char *p;
	char cdir[STRLEN];
	struct boardheader *bp;

	/* �����޸Ķ�̬���ɵ���Ŀ */
	if (direct[0] == '@')
		return NA;

	/* SYSOP�;��������ܿ��Ա༭������Ŀ */
	if (HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_ANNOUNCE))
		return YEA;

	/* ����Ƿ�����ļ������� */
	if (HAS_PERM(PERM_PERSONAL)) {
		snprintf(cdir, sizeof(cdir), "0Announce/personal/%c/%s/", mytoupper(currentuser.userid[0]),
			 currentuser.userid);

		if (strncmp(direct, cdir, strlen(cdir)) == 0)
			return YEA;
	}

	/* ����Ƿ񾫻������ڰ�İ��� */
	if (strncmp(direct, "0Announce/boards/", 17) != 0)
		return NA;

	if ((p = strchr(direct + 17, '/')) == NULL)
		return NA;

	strlcpy(cdir, direct + 17, p - direct - 16);
	if ((bp = getbcache(cdir)) == NULL)
		return NA;
	return check_bm(currentuser.userid, bp->BM);
}

int
check_annmanager(char *direct)
{
	char resolved[PATH_MAX + 1];

	if (real_check_annmanager(direct) == YEA)
		return YEA;

	/* ��direct����ʵ·������һ�μ�� */
	if (realpath(direct, resolved) != NULL) {
		return real_check_annmanager(resolved + strlen(BBSHOME) + 1);
	}

	return NA;
}

int
get_annjunk(char *direct)
{
	char *ptr;

	/* ȷ����¼�ļ��� */
	if (strncmp(ainfo.basedir, "0Announce/boards/", 17) == 0) {
		if ((ptr = strchr(ainfo.basedir + 18, '/')) == NULL) {
			strcpy(direct, ainfo.basedir);
		} else {
			strlcpy(direct, ainfo.basedir, ptr - ainfo.basedir + 1);
		}

		strcat(direct, "/junk");
	} else if (strncmp(ainfo.basedir, "0Announce/personal/", 19) == 0) {
		if (!isalpha(ainfo.basedir[19]) || ainfo.basedir[20] != '/')
			return -1;

		if ((ptr = strchr(ainfo.basedir + 21, '/')) == NULL) {
			strcpy(direct, ainfo.basedir);
		} else {
			strlcpy(direct, ainfo.basedir, ptr - ainfo.basedir + 1);
		}

		strcat(direct, "/junk");
	}
	if(dashf(direct))
		return 0;
	if(!dash(direct))
		f_mkdir(direct , 0755);
	return 1;
}


int
get_atracename(char *fname)
{
	char *ptr;

	/* ȷ����¼�ļ��� */
	if (strncmp(ainfo.basedir, "0Announce/boards/", 17) == 0) {
		if ((ptr = strchr(ainfo.basedir + 18, '/')) == NULL) {
			strcpy(fname, ainfo.basedir);
		} else {
			strlcpy(fname, ainfo.basedir, ptr - ainfo.basedir + 1);
		}

		strcat(fname, "/.trace");
		return 0;
	} else if (strncmp(ainfo.basedir, "0Announce/personal/", 19) == 0) {
		if (!isalpha(ainfo.basedir[19]) || ainfo.basedir[20] != '/')
			return -1;

		if ((ptr = strchr(ainfo.basedir + 21, '/')) == NULL) {
			strcpy(fname, ainfo.basedir);
		} else {
			strlcpy(fname, ainfo.basedir, ptr - ainfo.basedir + 1);
		}

		strcat(fname, "/.trace");
		return 0;
	}

	return -1;
}

/* ������������¼ */
void
atrace(int operation, char *info0, char *info1)
{
	int fd;
	char fname[PATH_MAX + 1];
	struct anntrace trace;

	/* ֻ��¼�԰��澫�����͸����ļ��Ĳ�����¼ */
	if (get_atracename(fname) == -1)
		return;

	if ((fd = open(fname, O_WRONLY | O_CREAT | O_APPEND, 0644)) > 0) {
		memset(&trace, 0, sizeof(trace));
		trace.operation = operation;
		trace.otime = time(NULL);
		strlcpy(trace.executive, currentuser.userid, sizeof(trace.executive));
		strlcpy(trace.location, ainfo.title, sizeof(trace.location));
		if (info0 != NULL) strlcpy(trace.info[0], info0, sizeof(trace.info[0]));
		if (info1 != NULL) strlcpy(trace.info[1], info1, sizeof(trace.info[1]));
		write(fd, &trace, sizeof(trace));
		close(fd);
	}
}

void
atracetitle(void)
{
	if (chkmail(NA)) {
		prints("\033[1;33;44m[������]\033[1;37m%*s[�����ż����� M ������]%*s\033[m\n", 19, " ", 29, " ");
	} else {
		prints("\033[1;33;44m[������]\033[1;37m%*s%s%*s\033[m\n", 25, " ", "������������¼", 32, " ");
	}

	clrtoeol();
	prints("�뿪[\033[1;32m��\033[m,\033[1;32mq\033[m] ѡ��[\033[1;32m��\033[m,\033[1;32m��\033[m] �Ķ�[\033[1;32m��\033[m,\033[1;32mRtn\033[m] ���[\033[1;32mE\033[m]\n");
	prints("\033[1;37;44m ���   %-12s %6s %-50s \033[m\n", "ִ �� ��", "��  ��", " ��  ��");
}

static inline char *
get_atracedesc(struct anntrace *ent)
{
	switch (ent->operation) {
	case ANN_COPY:
		return "������Ŀ";
	case ANN_CUT:
		return "������Ŀ";
	case ANN_MOVE:
		return ent->info[0];
	case ANN_EDIT:
		return "�༭����";
	case ANN_CREATE:
		return ent->info[0];
	case ANN_DELETE:
		return "ɾ����Ŀ";
	case ANN_CTITLE:
		return "���ı���";
	case ANN_ENOTES:
		return "�༭����¼";
	case ANN_DNOTES:
		return "ɾ������¼";
	case ANN_INDEX:
		return "���ɾ���������";
	}

	return "";
}

char *
atracedoent(int num, void *ent_ptr)
{
	static char buf[128];
	struct anntrace *ent = (struct anntrace *)ent_ptr;

#ifdef COLOR_POST_DATE
	struct tm *mytm;
	char color[8] = "\033[1;30m";
#endif

	#ifdef COLOR_POST_DATE

	mytm = localtime(&ent->otime);
	color[5] = mytm->tm_wday + 49;

	snprintf(buf, sizeof(buf), " %4d   %-12.12s %s%6.6s\033[m  �� %-.45s ",
		 num, ent->executive, color, ctime(&ent->otime) + 4, get_atracedesc(ent));

	#else

	snprintf(buf, sizeof(buf), " %4d   %-12.12s %6.6s  �� %-.45s ",
		 num, ent->executive, ctime(&ent->otime) + 4, get_atracedesc(ent));

	#endif

	return buf;
}

int
atrace_empty(int ent, struct anntrace *traceinfo, char *direct)
{
	char fname[PATH_MAX + 1];

        /* ֻ��SYSOP�;��������ܿ�����վ�����������¼ */
        if (!HAS_PERM(PERM_SYSOP) && !HAS_PERM(PERM_ANNOUNCE))
                return DONOTHING;

	if (get_atracename(fname) == -1)
		return DONOTHING;

	if (askyn("ȷ��Ҫ��վ�����������¼��", NA, YEA) == NA)
		return PARTUPDATE;

	snprintf(genbuf, sizeof(genbuf), "λ��: %s", fname);
	securityreport2("������������¼�����", NA, genbuf);

	unlink(fname);
	return NEWDIRECT;
}

int
atrace_read(int ent, struct anntrace *traceinfo, char *direct)
{
	clear();

	/* ������Ϣ */
	getdatestring(traceinfo->otime);
	prints("��������: %s\n", get_atracedesc(traceinfo));
	prints("����λ��: %s\n", traceinfo->location);
	prints("��������: %s\n", datestring);
	prints("ִ����:   %s\n", traceinfo->executive);

	/* ������Ϣ */
	if (traceinfo->operation != ANN_DNOTES && traceinfo->operation != ANN_ENOTES)
		prints("\n������Ϣ: \n\n");

	switch (traceinfo->operation) {
	case ANN_COPY:
	case ANN_CUT:
		prints("��Ŀ����: %s\n", traceinfo->info[0]);
		prints("ԭλ��:   %s\n", traceinfo->info[1]);
		break;
	case ANN_EDIT:
	case ANN_DELETE:
		prints("��Ŀ����: %s\n", traceinfo->info[0]);
		break;
	case ANN_MOVE:
	case ANN_CREATE:
		prints("��Ŀ����: %s\n", traceinfo->info[1]);
		break;
	case ANN_CTITLE:
		prints("ԭ����:   %s\n", traceinfo->info[0]);
		prints("�±���:   %s\n", traceinfo->info[1]);
		break;
	}

	clear_line(t_lines - 1);
	outs("\033[1;31;44m[������������¼]  \033[33m �� ���� Q,�� �� ��һ�� U,���� ��һ�� <Space>,�� ��          \033[m");

	switch (egetch()) {
		case KEY_DOWN:
		case ' ':
		case '\n':
			return READ_NEXT;
		case KEY_UP:
		case 'u':
		case 'U':
			return READ_PREV;
	}

	return FULLUPDATE;
}

struct one_key atrace_comms[] = {
	{ 'E',		atrace_empty	},
	{ 'f',          t_friends       },
	{ 'o',          fast_cloak      },
	{ 'r',          atrace_read     },
	{ Ctrl('V'), 	x_lockscreen_silent },
	{ '\0',         NULL }
};

void
anntitle(void)
{
	int len;

	static char *satype[3] = { "��  ��", "��  ��", "      " };

	if (chkmail(NA)) {
		prints("\033[1;33;44m[������]\033[1;37m%*s[�����ż����� M ������]%*s\033[m\n", 19, " ", 28, " ");
	} else {
		len = 32 - strlen(ainfo.title) / 2;
		prints("\033[1;33;44m[������]\033[1;37m%*s%s%*s\033[m\n", len, " ", ainfo.title, 71 - strlen(ainfo.title) - len, " ");
	}

	prints("           \033[1;32m F\033[37m �Ļ��Լ�������  \033[32m����\033[37m �ƶ� \033[32m �� <Enter> \033[37m��ȡ \033[32m ��,q\033[37m �뿪\033[m\n");
	prints("\033[1;44;37m ���   %-20s                         %s          �༭����  \033[m\n",
	       "[���] ��    ��", satype[sauthor]);
}

char *
anndoent(int num, void *ent_ptr)
{
	struct tm *pt;
	static char buf[128];
	struct annheader *ent = (struct annheader *)ent_ptr;
	char itype;
	int tnum;

	static char *ctype[5] = {
		"\033[1;36m�ļ�\033[m", "\033[1;37mĿ¼\033[m",
		"\033[1;32m�ν�\033[m", "\033[1;35m����\033[m",
		"\033[1;31mδ֪\033[m"
	};

	if (ent->flag & ANN_FILE) {
		tnum = 0;
	} else if (ent->flag & ANN_DIR) {
		tnum = 1;
	} else if ((ent->flag & ANN_LINK) || (ent->flag & ANN_RLINK)) {
		tnum = 2;
	} else if (ent->flag & ANN_GUESTBOOK) {
		tnum = 3;
	} else {
		tnum = 4;
	}

	itype = ((ainfo.manager == YEA) && (ent->flag & ANN_SELECTED)) ? '$' : ' ';

	// monster: Here we assume time_t to be 32bit integer. The code may break
	//          if it's not the case. Note standard did not specify type & range
	//          for time_t.
	time_t mtime = ent->mtime;
	pt = localtime(&mtime);
	ent->mtime = mtime;

	if (sauthor != 2 && ent->filename[0] != '@') {
		snprintf(buf, sizeof(buf), "%5d %c [%s] %-37.37s %-12s [\033[1;37m%04d\033[m.\033[1;37m%02d\033[m.\033[1;37m%02d\033[m]",
			  num, itype, ctype[tnum], ent->title, (sauthor == NA) ? ent->editor : ent->owner,
			  pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday);
	} else {
		snprintf(buf, sizeof(buf), "%5d %c [%s] %-50.50s [\033[1;37m%04d\033[m.\033[1;37m%02d\033[m.\033[1;37m%02d\033[m]",
			  num, itype, ctype[tnum], ent->title,
			  pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday);
	}

	return buf;
}

int
ann_switch_sauthor(int ent, struct annheader *anninfo, char *direct)
{
	sauthor = (sauthor + 1) % 3;
	return FULLUPDATE;
}

static int
ann_add_file(char *filename, char *title, char *owner, char *direct, int flag, int remove)
{
	struct annheader fileinfo;
	char fname[PATH_MAX + 1];
	char *ptr;

	/* ���Ȩ�� */
	if (ainfo.manager != YEA && !((ainfo.flag & ANN_GUESTBOOK) && (HAS_ORGPERM(PERM_POST))))
		return -2;

	/* ��ʼ�� */
	memset(&fileinfo, 0, sizeof(fileinfo));

	/* �趨���� */
	if (title == NULL || title[0] == '\0') {
		a_prompt("����: ", fileinfo.title, sizeof(fileinfo.title), YEA);
	} else {
		strlcpy(fileinfo.title, title, sizeof(fileinfo.title));
	}
	if (fileinfo.title[0] == '\0')
		return -2;

	if (getfilename(ainfo.basedir, fname, 0, NULL) == -1)
		return -1;

	if (filename != NULL && filename[0] != '\0') {
		if (f_cp(filename, fname, O_TRUNC) != 0)
			return -1;
	} else {
		/* �༭���� */
		if (vedit(fname, EDIT_MODIFYHEADER) == -1)
			return -2;
	}

	if ((ptr = strrchr(fname, '/')) == NULL)
		return -1;
	strlcpy(fileinfo.filename, ptr + 1, sizeof(fileinfo.filename));

	fileinfo.flag = ANN_FILE | flag;
	fileinfo.mtime = time(NULL);
	strlcpy(fileinfo.owner, owner, sizeof(fileinfo.owner));
	strlcpy(fileinfo.editor, currentuser.userid, sizeof(fileinfo.editor));

	/* ��Ӽ�¼ */
	if (append_record(direct, &fileinfo, sizeof(fileinfo)) == -1)
		return -1;

	if (remove == YEA)
		unlink(filename);

	atrace(ANN_CREATE, "�����ļ�", fileinfo.title);
	return 0;
}

int
ann_import_article(char *fname, char *title, char *owner, int attached, int batch)
{
	char direct[PATH_MAX + 1];

	 /* ��������������װ��˿· */
	if (batch == NA && ann_loadpaths() == -1)
		return -1;

	snprintf(direct, sizeof(direct), "%s/.DIR", paths[0].path);
	ainfo.manager = check_annmanager(direct);
	strlcpy(ainfo.basedir, paths[0].path, sizeof(ainfo.basedir));
	strlcpy(ainfo.title, paths[0].title, sizeof(ainfo.title));	

	return ann_add_file(fname, title, owner, direct, attached ? ANN_ATTACHED : 0, NA);
}

int
ann_add_article(int ent, struct annheader *anninfo, char *direct)
{
	if (ann_add_file(NULL, NULL, currentuser.userid, direct, 0, NA) == -1)
		presskeyfor("����ʧ��...");

	return FULLUPDATE;
}

static int
add_copypaste_record(int fd, struct annheader *anninfo, int copymode)
{
	struct anncopypaste rec;

	/* ��¼�����Ϣ */
	strlcpy(rec.basedir, ainfo.basedir, sizeof(rec.basedir));
	strlcpy(rec.location, ainfo.title, sizeof(rec.location));
	memcpy(&rec.fileinfo, anninfo, sizeof(rec.fileinfo));
	rec.copymode = copymode;
	rec.fileinfo.flag &= ~ANN_SELECTED;

	/* д���¼ */
	return write(fd, &rec, sizeof(rec));
}

int
ann_copy(int ent, struct annheader *anninfo, char *direct)
{
	int fd;
	char fname[PATH_MAX + 1];

	if (ainfo.manager != YEA)
		return DONOTHING;

	if (anninfo->filename[0] == '\0')
		return DONOTHING;

	if (anninfo->flag != ANN_FILE && anninfo->flag != ANN_DIR) {
		presskeyfor("ֻ�ܶ�Ŀ¼/�ļ�����");
		return PARTUPDATE;
	}

	sethomefile(fname, currentuser.userid, "copypaste");
	if ((fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1)
		goto error_process;

	f_exlock(fd);
	if (add_copypaste_record(fd, anninfo, ANN_COPY) == -1) {
		close(fd);
		goto error_process;
	}

	close(fd);
	presskeyfor("������ʶ���. (ע��! ճ�����º���ܽ����� delete!)");
	return PARTUPDATE;

error_process:
	if (fd != -1) close(fd);
	presskeyfor("����ʧ��...");
	return PARTUPDATE;
}

int
ann_cut(int ent, struct annheader *anninfo, char *direct)
{
	int fd;
	char fname[PATH_MAX + 1];

	if (ainfo.manager != YEA)
		return DONOTHING;

	if (anninfo->filename[0] == '\0')
		return DONOTHING;

	if (anninfo->flag != ANN_FILE && anninfo->flag != ANN_DIR) {
		presskeyfor("ֻ�ܶ�Ŀ¼/�ļ�����");
		return PARTUPDATE;
	}

	sethomefile(fname, currentuser.userid, "copypaste");
	if ((fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1)
		goto error_process;

	f_exlock(fd);
	if (add_copypaste_record(fd, anninfo, ANN_CUT) == -1) {
		close(fd);
		goto error_process;
	}

	close(fd);
	presskeyfor("������ʶ���. (ע��! ճ�����º���ܽ����� delete!)");
	return PARTUPDATE;

error_process:
	if (fd != -1) close(fd);
	presskeyfor("����ʧ��...");
	return PARTUPDATE;
}

static int
cmpafilename(void *filename_ptr, void *fileinfo_ptr)
{
	char *filename = (char *)filename_ptr;
	struct annheader *fileinfo = (struct annheader *)fileinfo_ptr;

	return (!strcmp(filename, fileinfo->filename)) ? YEA : NA;
}

int
ann_paste(int ent, struct annheader *anninfo, char *direct)
{
	time_t now;
	int id, fd, count = 0;
	char fname[PATH_MAX + 1], src[PATH_MAX + 1], dst[PATH_MAX + 1], cmdbuf[2 * PATH_MAX + 15];
	char olddirect[PATH_MAX + 1];
	struct anncopypaste rec;
	struct annheader fileinfo;
	struct stat st;

	if (ainfo.manager != YEA)
		return DONOTHING;

	sethomefile(fname, currentuser.userid, "copypaste");
	if ((fd = open(fname, O_RDWR)) == -1)
		return DONOTHING;

	f_exlock(fd);		/* monster: caution, not a wise solution, as the copy process may take long time */
	while (read(fd, &rec, sizeof(rec)) == sizeof(rec)) {
		snprintf(src, sizeof(src), "%s/%s", rec.basedir, rec.fileinfo.filename);
		snprintf(dst, sizeof(dst), "%s/%s", ainfo.basedir, rec.fileinfo.filename);

		/* ���������· */
		if (strstr(dst, src) == dst)
			continue;

		/* ��֤Դ�ļ�/Ŀ¼���� */
		if (!dash(src)) {
			clear();
			continue;
		}

		switch (rec.copymode) {
		case ANN_COPY:  /* ���� */
			if (rec.fileinfo.filename[0] != '@') {
				now = time(NULL);
				while (1) {
					snprintf(rec.fileinfo.filename + 2, sizeof(rec.fileinfo.filename) - 2, "%d.A", (int)(now++));
					snprintf(dst, sizeof(dst), "%s/%s", ainfo.basedir, rec.fileinfo.filename);

					if (stat(dst, &st) == -1) {
						snprintf(cmdbuf, sizeof(cmdbuf), "/bin/cp -p -f -R %s %s", src, dst);
						system(cmdbuf);
						break;
					}
				}
			}

			if (append_record(currdirect, &rec.fileinfo, sizeof(rec.fileinfo)) != -1) {
				atrace(ANN_COPY, rec.fileinfo.title, rec.location);
				++count;
			}
			break;
		case ANN_CUT:   /* ���� */
			if (rec.fileinfo.filename[0] != '@') {
				strlcpy(fname, rec.fileinfo.filename, sizeof(fname));
				if (rename(src, dst) == -1) {
					now = time(NULL);
					while (1) {
						snprintf(rec.fileinfo.filename + 2, sizeof(rec.fileinfo.filename) - 2, "%d.A", (int)(now++));
						snprintf(dst, sizeof(dst), "%s/%s", ainfo.basedir, rec.fileinfo.filename);

						/* ����ǰ���뱣֤Ŀ�겻����, ����Ŀ���ļ��ᱻ���� */
						if (stat(dst, &st) == -1)
							if (rename(src, dst) == 0)
								break;
					}
				}
			}

			if (append_record(currdirect, &rec.fileinfo, sizeof(rec.fileinfo)) != -1) {
				atrace(ANN_CUT, rec.fileinfo.title, rec.location);
				++count;
			}

			snprintf(olddirect, sizeof(olddirect), "%s/.DIR", rec.basedir);
			if ((id = search_record(olddirect, &fileinfo, sizeof(fileinfo), cmpafilename, fname)) > 0)
				delete_record(olddirect, sizeof(fileinfo), id);
			break;
		}
	}

	close(fd);
	unlink(fname);

	if (count == 0)
		presskeyfor("����ʧ��");

	return PARTUPDATE;
}

static int
ann_rangecheck(void *rptr, void *extrarg)
{
	struct annheader *anninfo = (struct annheader *)rptr;
	char src[PATH_MAX + 1];
	char dst[PATH_MAX+1];	
	char cmdbuf[PATH_MAX + 1];

	if (anninfo->flag & (ANN_DIR | ANN_GUESTBOOK))
		return -1;

	get_annjunk(genbuf);
	snprintf(dst, sizeof(dst), "%s/%d_%s_%s", genbuf, time(NULL), currentuser.userid, anninfo->title);
	snprintf(src, sizeof(src), "%s/%s", ainfo.basedir, anninfo->filename);

        if (rename(src, dst) != 0) {
	  snprintf(cmdbuf, sizeof(cmdbuf), "/bin/cp -p -f -R %s %s", src, dst);
	  system(cmdbuf);
	  f_rm(src);
	}

	atrace(ANN_DELETE, anninfo->title, NULL);
	return (dash(src)) ? KEEPRECORD : REMOVERECORD;
}

int
ann_delete_range(int ent, struct annheader *anninfo, char *direct)
{
	char num[8];
	int inum1, inum2, result;
	return -1;
	if (ainfo.manager != YEA)
		return DONOTHING;

	if (anninfo->filename[0] == '\0' && anninfo->title[0] == '\0')
		return DONOTHING;

	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, "��ƪ���±��: ", num, 7, DOECHO, YEA);
	inum1 = atoi(num);
	if (inum1 <= 0)
		goto error_range;

	getdata(t_lines - 1, 25, "ĩƪ���±��: ", num, 7, DOECHO, YEA);
	inum2 = atoi(num);
	if (inum2 < inum1 + 1)
		goto error_range;

	move(t_lines - 1, 50);
	if (askyn("ȷ��ɾ��", NA, NA) == YEA) {
		result = process_records(currdirect, 128, inum1, inum2, ann_rangecheck, NULL);
		fixkeep(direct, inum1, inum2);

		if (result == -1)
			goto error_process;
	}

	return NEWDIRECT;

error_range:
	move(t_lines - 1, 50);
	clrtoeol();
	prints("�������...");
	egetch();
	return NEWDIRECT;

error_process:
	presskeyfor("ɾ��ʧ��...");
	return NEWDIRECT;
}

int
ann_delete_item(int ent, struct annheader *anninfo, char *direct)
{
	char src[PATH_MAX + 1];
	char dst[PATH_MAX+1];	
	char cmdbuf[PATH_MAX + 1];

	if (ainfo.manager != YEA)
		return DONOTHING;

	if (anninfo->filename[0] == '\0' && anninfo->title[0] == '\0')
		return DONOTHING;

	clear_line(t_lines - 1);
	if (anninfo->flag & ANN_DIR) {
		if (askyn("ɾ��������Ŀ¼, ����ЦŶ, ȷ����", NA, YEA) == NA)
			return NEWDIRECT;
	} else {
		if (askyn("ɾ���˵���, ȷ����", NA, YEA) == NA)
			return NEWDIRECT;
	}

	get_annjunk(genbuf);
	snprintf(dst, sizeof(dst), "%s/%d_%s_%s", genbuf, time(NULL), currentuser.userid, anninfo->title);
	snprintf(src, sizeof(src), "%s/%s", ainfo.basedir, anninfo->filename);
	snprintf(cmdbuf, sizeof(cmdbuf), "/bin/cp -p -f -R %s %s", src, dst);
	system(cmdbuf);
	f_rm(src);
	if (delete_record(direct, sizeof(struct annheader), ent) == -1)
		presskeyfor("ɾ��ʧ��...");

	atrace(ANN_DELETE, anninfo->title, NULL);
	return NEWDIRECT;
}

int
ann_edit_article(int ent, struct annheader *anninfo, char *direct)
{
	char fname[PATH_MAX + 1];

	/* ���Ȩ�� */
	if (ainfo.manager != YEA)
		return DONOTHING;

	if (anninfo->filename[0] == '\0' || anninfo->filename[0] == '@' || !(anninfo->flag & ANN_FILE))
		return DONOTHING;

	/* �༭���� */
	snprintf(fname, sizeof(fname), "%s/%s", ainfo.basedir, anninfo->filename);
	if (vedit(fname, EDIT_MODIFYHEADER) != -1) {
		anninfo->mtime = time(NULL);
		strlcpy(anninfo->editor, currentuser.userid, sizeof(anninfo->editor));
		safe_substitute_record(direct, (struct fileheader *)anninfo, ent, NA);
		atrace(ANN_EDIT, anninfo->title, NULL);
	}

	return FULLUPDATE;
}

int
ann_select_item(int ent, struct annheader *anninfo, char *direct)
{
	if (ainfo.manager != YEA)
		return DONOTHING;

	if (anninfo->filename[0] == '\0')
		return DONOTHING;

	if (anninfo->flag & ANN_SELECTED) {
		anninfo->flag &= ~ANN_SELECTED;
	} else {
		anninfo->flag |= ANN_SELECTED;
	}

	safe_substitute_record(direct, (struct fileheader *)anninfo, ent, NA);
	return PARTUPDATE;
}

static int
ann_delete_selected(void *rptr, void *extrarg)
{
	struct annheader *anninfo = (struct annheader *)rptr;
	char src[PATH_MAX + 1];
	char dst[PATH_MAX+1];	
	char cmdbuf[PATH_MAX + 1];

	if (!(anninfo->flag & ANN_SELECTED))
		return KEEPRECORD;

	anninfo->flag &= ~ANN_SELECTED;
	get_annjunk(genbuf);
	snprintf(dst, sizeof(dst), "%s/%d_%s_%s", genbuf, time(NULL), currentuser.userid, anninfo->title);
	snprintf(src, sizeof(src), "%s/%s", ainfo.basedir, anninfo->filename);
	snprintf(cmdbuf, sizeof(cmdbuf), "/bin/cp -p -f -R %s %s", src, dst);
	system(cmdbuf);
	f_rm(src);
	atrace(ANN_DELETE, anninfo->title, NULL);
	return (dash(src)) ? KEEPRECORD : REMOVERECORD;
}

int
ann_copycut_selected(void *rptr, void *extrarg)
{
	struct annheader *anninfo = (struct annheader *)rptr;
	int *flags = (int *)extrarg;

	if (!(anninfo->flag & ANN_SELECTED))
		return KEEPRECORD;

	anninfo->flag &= ~ANN_SELECTED;
	if (add_copypaste_record(flags[0], anninfo, flags[1]) != -1)
		flags[3]++;

	return KEEPRECORD;
}

int
ann_process_selected(int ent, struct annheader *anninfo, char *direct)
{
	char ch[2] = { '\0' }, fname[PATH_MAX + 1];
	int fd, result, flags[2];

	if (ainfo.manager != YEA)
		return DONOTHING;

	saveline(t_lines - 1, 0);
	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, "ִ��: 1) ����  2) ����  3) ɾ��  0) ȡ�� [0]: ", ch, 2, DOECHO, YEA);
	if (ch[0] < '1' || ch[0] > '3') {
		saveline(t_lines - 1, 1);
		return DONOTHING;
	}

	switch (ch[0]) {
	case '1':
	case '2':
		sethomefile(fname, currentuser.userid, "copypaste");
		if ((fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1)
			goto error_process;

		flags[0] = fd;
		flags[1] = (ch[0] == '1') ? ANN_COPY : ANN_CUT;
		f_exlock(fd);
		if (process_records(direct, sizeof(struct annheader), 1, 999999, ann_copycut_selected, &flags) == -1) {
			close(fd);
			goto error_process;
		}		

		close(fd);
		presskeyfor("������ʶ���. (ע��! ճ�����º���ܽ����� delete!)");
		break;
	case '3':
		clear_line(t_lines - 1);
		if (askyn("ȷ��ɾ��", NA, NA) == YEA) {
			/* monster: here we choose a big enough number to avoid an extra
			 *          get_num_records call */
			result = process_records(direct, sizeof(struct annheader), 1, 999999, ann_delete_selected, NULL);
			fixkeep(direct, 1, 999999);

			if (result == -1)
				goto error_process;
		}
	}

	return (ch[0] == '1' || ch[0] == '2') ? NEWDIRECT : PARTUPDATE;

error_process:
	presskeyfor("����ʧ��...");
	return (ch[0] == '1' || ch[0] == '2') ? NEWDIRECT : PARTUPDATE;
}

static
int create_folder(int flag)
{
	struct annheader fileinfo;
	char dname[PATH_MAX + 1];
	char *ptr;

	/* ���Ȩ�� */
	if (ainfo.manager == NA)
		return -2;

	/* ��ʼ�� */
	memset(&fileinfo, 0, sizeof(fileinfo));

	/* �趨����������Ŀ¼ */
	if (a_prompt("����: ", fileinfo.title, sizeof(fileinfo.title), YEA) == -1)
		return -2;

	if (getdirname(ainfo.basedir, dname) == -1)
		return -1;

	if ((ptr = strrchr(dname, '/')) == NULL)
		return -1;
	strlcpy(fileinfo.filename, ptr + 1, sizeof(fileinfo.filename));

	fileinfo.flag = flag;
	fileinfo.mtime = time(NULL);

	/* ��Ӽ�¼ */
	if (append_record(currdirect, &fileinfo, sizeof(fileinfo)) == -1)
		return -1;

	return 0;
}

int
ann_create_guestbook(int ent, struct annheader *anninfo, char *direct)
{
	if (ainfo.manager == NA)
		return DONOTHING;

	if (create_folder(ANN_GUESTBOOK) == -1) {
		presskeyfor("����ʧ��...");
	} else {
		atrace(ANN_CREATE, "�������Բ�", anninfo->title);
	}
	return PARTUPDATE;
}

int
ann_create_folder(int ent, struct annheader *anninfo, char *direct)
{
	if (ainfo.manager == NA)
		return DONOTHING;

	if (create_folder(ANN_DIR) == -1) {
		presskeyfor("����ʧ��...");
	} else {
		atrace(ANN_CREATE, "����Ŀ¼", anninfo->title);
	}
	return PARTUPDATE;
}

int
ann_create_special(int ent, struct annheader *anninfo, char *direct)
{
	char ch[2], bname[BFNAMELEN];
	struct annheader fileinfo;

	/* ֻ��SYSOP�;��������ܿ��Դ���������Ŀ */
	if (!HAS_PERM(PERM_SYSOP) && !HAS_PERM(PERM_ANNOUNCE))
		return DONOTHING;

	stand_title("����������Ŀ");
	move(2, 0);
	prints("[\033[1;32m1\033[m] ����Ŀ\n");
	prints("[\033[1;32m2\033[m] ����������\n");
	prints("[\033[1;32m3\033[m] ��������������\n");
	getdata(6, 0, "��Ҫ������һ��������Ŀ: ", ch, 2, DOECHO, YEA);

	/* ��ʼ��fileinfo */
	memset(&fileinfo, 0, sizeof(fileinfo));
	fileinfo.flag = ANN_DIR;
	fileinfo.mtime = time(NULL);

	switch (ch[0]) {
	case '1':
		strcpy(fileinfo.filename, "@NULL");
		if (append_record(direct, &fileinfo, sizeof(fileinfo)) == -1)
			goto error_process;
		break;
	case '2':
		move(7, 0);
		prints("������������ (���հ׼��Զ���Ѱ): ");

		make_blist();
		namecomplete(NULL, bname);

		if (bname[0] == '\0')
			return FULLUPDATE;

		getdata(9, 0, "����: ", fileinfo.title, sizeof(fileinfo.title), DOECHO, YEA);
		my_ansi_filter(fileinfo.title);
		if (fileinfo.title[0] == '\0')
			return FULLUPDATE;

		strcpy(fileinfo.filename, "@BOARDS");
		strlcpy(fileinfo.owner, bname, sizeof(fileinfo.owner) + sizeof(fileinfo.editor));
		if (append_record(direct, &fileinfo, sizeof(fileinfo)) == -1)
			goto error_process;
		break;
	case '3':
		do {
			getdata(8, 0, "�������������(0-9, A-Z, *): ", ch, 2, DOECHO, YEA);
		} while (!isdigit(ch[0]) && !isalpha(ch[0]) && ch[0] != '*');

		getdata(9, 0, "����: ", fileinfo.title, sizeof(fileinfo.title), DOECHO, YEA);
		my_ansi_filter(fileinfo.title);
		if (fileinfo.title[0] == '\0')
			return FULLUPDATE;

		snprintf(fileinfo.filename, sizeof(fileinfo.filename), "@GROUP:%c", ch[0]);
		if (append_record(direct, &fileinfo, sizeof(fileinfo)) == -1)
			goto error_process;
		break;
	}

	return FULLUPDATE;

error_process:
	presskeyfor("����ʧ��...");
	return FULLUPDATE;
}

int
ann_loadpaths(void)
{
	int fd, retval;
	char fname[PATH_MAX + 1];

	sethomefile(fname, currentuser.userid, "apaths");
	if ((fd = open(fname, O_RDONLY)) == -1)
		return -1;

	memset(paths, 0, sizeof(paths));
	retval = (read(fd, paths, sizeof(paths)) == sizeof(paths)) ? 0 : -1;
	close(fd);

	return retval;
}

int
ann_savepaths(void)
{
	int fd;
	char fname[PATH_MAX + 1];

	sethomefile(fname, currentuser.userid, "apaths");
	if ((fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
		return -1;

	if (write(fd, paths, sizeof(paths)) != sizeof(paths)) {
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

static int
ann_select_hpath(void)
{
	int i;
	char ch[2] = { '\0' };

	stand_title("��ʷ·��");
	move(3, 0);
	for (i = 1; i < 8; i++) {
		if (paths[i].title[0] == '\0' || paths[i].path[0] == '\0')
			continue;

		prints("%d. λ��: ", i);
		if (paths[i].board[0] != '\0') {
			prints("%s\n", paths[i].board);
		} else {
			if (paths[i].board[1] != '\0') {
				prints("%s (�����ļ�)\n", &paths[i].board[1]);
			} else {
				outs("������������\n");
			}
		}
		prints("   ����: %s\n\n", paths[i].title);
	}

	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, "��ѡ��˿·�ı�� (1~7): ", ch, 2, DOECHO, YEA);

	if (ch[0] >= '1' && ch[0] <= '7') {
		i = ch[0] - '0';
		if (paths[i].board[0] != '\0' || paths[i].board[1] != '\0') {
			memcpy(&paths[0], &paths[i], sizeof(paths[0]));
			memset(&paths[i], 0, sizeof(paths[i]));
			ann_savepaths();
			presskeyfor("�ѽ���·����Ϊ˿·, �밴���������...");
			return FULLUPDATE;
		}
	}

	presskeyfor("������ȡ��, �밴���������...");
	return FULLUPDATE;
}

int
ann_setpath(int ent, struct annheader *anninfo, char *direct)
{
	struct annpath paths2[8];

	char *ptr, ch[2] = { '\0' };
	int i, j;

	if (ainfo.manager != YEA)
		return DONOTHING;

	ann_loadpaths();
	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0,
		"�趨˿·: 1) ��ǰ·��  2) ��ʷ·��  0) ȡ�� [1]: ",
		ch, 2, DOECHO, YEA);

	if (ch[0] == '0')
		return PARTUPDATE;

	if (ch[0] == '2')
		return ann_select_hpath();

	memset(paths2, 0, sizeof(paths2));
	for (i = 0, j = 1; i <= 6; i++) {
		if (paths[i].path[0] == '\0' || strcmp(paths[i].path, ainfo.basedir) == 0)
			continue;
		memmove(&paths2[j], &paths[i], sizeof(paths[i]));
		++j;
	}
	memmove(paths, paths2, sizeof(paths));

	memset(&paths[0], 0, sizeof(paths[0]));
	strlcpy(paths[0].title, ainfo.title, sizeof(paths[0].title));
	strlcpy(paths[0].path, ainfo.basedir, sizeof(paths[0].path));
	if (strncmp(ainfo.basedir, "0Announce/boards/", 17) == 0) {
		strlcpy(paths[0].board, ainfo.basedir + 17, sizeof(paths[0].board));
		if ((ptr = strchr(paths[0].board, '/')) != NULL)
			*ptr = '\0';
	} else if (strncmp(ainfo.basedir, "0Announce/personal/", 19) == 0) {
		if (strlen(ainfo.basedir) < 23 || ainfo.basedir[20] != '/') {
			paths[0].board[0] = '\0';
			paths[0].board[1] = '\0';
		} else {
			strlcpy(paths[0].board + 1, ainfo.basedir + 20, sizeof(paths[0].board));
			if ((ptr = strchr(paths[0].board + 1, '/')) != NULL)
				*ptr = '\0';
			paths[0].board[0] = '\0';
		}
	}

	ann_savepaths();
	presskeyfor("�ѽ���·����Ϊ˿·, �밴���������...");
	return FULLUPDATE;
}

int
ann_help(int ent, struct annheader *anninfo, char *direct)
{
	show_help("help/announcereadhelp");
	return FULLUPDATE;
}

int
ann_savepost(char *key, struct fileheader *fileinfo, int nomsg)
{
	char fname[PATH_MAX + 1];
	int ans = NA;

	if (nomsg == NA) {
		clear_line(t_lines - 1);
		snprintf(genbuf, sizeof(genbuf), "ȷ���� [%-.40s] �����ݴ浵��", fileinfo->title);
		if (askyn(genbuf, NA, YEA) == NA)
			return FULLUPDATE;
	}

	sethomefile(fname, currentuser.userid, "savepost");
	if (dashf(fname)) {
		ans = (nomsg) ? YEA : askyn("Ҫ�����ھ��ݴ浵֮����", NA, YEA);
	}

	if (INMAIL(uinfo.mode)) {
		snprintf(genbuf, sizeof(genbuf), "mail/%c/%s/%s", mytoupper(currentuser.userid[0]),
			 currentuser.userid, fileinfo->filename);
	} else {
		snprintf(genbuf, sizeof(genbuf), "boards/%s/%s", key, fileinfo->filename);
	}

	if (!ans) unlink(fname);
	f_cp(genbuf, fname, O_APPEND | O_CREAT);

	if (nomsg == NA) {
		presskeyfor("�ѽ������´����ݴ浵, �밴<Enter>����...");
	}

	return FULLUPDATE;
}

int
ann_export_savepost(int ent, struct annheader *anninfo, char *direct)
{
	char fname[PATH_MAX + 1];

	sethomefile(fname, currentuser.userid, "savepost");
	if (!dashf(fname))
		return DONOTHING;

	if (ann_add_file(fname, NULL, currentuser.userid, direct, 0, YEA) == -1)
		presskeyfor("����ʧ��...");

	return PARTUPDATE;
}

int
ann_make_symbolink(int ent, struct annheader *anninfo, char *direct)
{
	FILE *fp;
	time_t now;
	char ch[2];
	char type_char;
	char lpath[PATH_MAX + 2], resolved[PATH_MAX + 1];
	char spath[PATH_MAX + 1], fname[PATH_MAX + 1];
	struct annheader fileinfo;

	/* ֻ��SYSOP�;��������ܿ��Դ����������� */
	if (!HAS_PERM(PERM_SYSOP) && !HAS_PERM(PERM_ANNOUNCE))
		return DONOTHING;

	sethomefile(fname, currentuser.userid, "slink");
	if ((fp = fopen(fname, "r")) == NULL)
		goto set_link;

	if (fgets(lpath, sizeof(lpath), fp) == NULL) {
		fclose(fp);
		goto set_link;
	}

	fclose(fp);

	getdata(t_lines - 1, 0,
		"��������: 1) ��������  2) ����Դ·��  0) ȡ�� [0]: ",
		ch, 2, DOECHO, YEA);

	if (ch[0] == '1') goto create_link;
	if (ch[0] == '2') goto set_link;

	return PARTUPDATE;

set_link:               /* ��¼Դ·�� */
	if ((fp = fopen(fname, "w")) == NULL)
		goto error_set_link;
	type_char = anninfo->filename[0];
	if (type_char != 'G' && type_char != 'D' && type_char != 'M')
		type_char = 'D';		/* Pudding: It must be a personal-announce-directory */
/*
	snprintf(lpath, sizeof(lpath), "%c%s/%s/%s", anninfo->filename[0], BBSHOME,
		 ainfo.basedir, anninfo->filename);
*/
	snprintf(lpath, sizeof(lpath), "%c%s/%s/%s", type_char, BBSHOME,
		 ainfo.basedir, anninfo->filename);

	fputs(lpath, fp);
	fclose(fp);
	presskeyfor("·�����óɹ�������Ŀ��·������ L ��������...");
	return PARTUPDATE;

create_link:            /* ������������ */
	memset(&fileinfo, 0, sizeof(fileinfo));
	if (a_prompt("����: ", fileinfo.title, sizeof(fileinfo.title), NA) == -1)
		return PARTUPDATE;

	if (realpath(lpath + 1, resolved) == NULL)
		goto error_create_link;

	/* ���Դ·���Ƿ���BBSĿ¼���� */
	if (strncmp(resolved, BBSHOME"/", strlen(BBSHOME) + 1) != 0)
		goto error_create_link;

	/* �������� */
	now = time(NULL);
	while (1) {
		snprintf(spath, sizeof(spath), "%s/%c.%d.L", ainfo.basedir, lpath[0], (int)now);
		if (symlink(lpath + 1, spath) == 0)
			break;
		if (errno != EEXIST)
			goto error_create_link;
	}

	fileinfo.flag = ANN_LINK;
	fileinfo.mtime = time(NULL);
	strlcpy(fileinfo.owner, currentuser.userid, sizeof(fileinfo.owner));
	strlcpy(fileinfo.editor, currentuser.userid, sizeof(fileinfo.editor));
	strlcpy(fileinfo.filename, strrchr(spath, '/') + 1, sizeof(fileinfo.filename));

	/* ��Ӽ�¼ */
	if (append_record(currdirect, &fileinfo, sizeof(fileinfo)) == -1)
		goto error_create_link;

	unlink(fname);
	atrace(ANN_CREATE, "�����ν�", fileinfo.title);
	return PARTUPDATE;

error_set_link:         /* ��¼Դ·���Ĵ����� */
	presskeyfor("����·��ʧ��...");
	return PARTUPDATE;

error_create_link:      /* �����������ӵĴ����� */
	presskeyfor("������������ʧ��...");
	return PARTUPDATE;
}

int
ann_move_item(int ent, struct annheader *anninfo, char *direct)
{
	char buf[TITLELEN];
	int dst;

	/* ���Ȩ�� */
	if (ainfo.manager != YEA)
		return DONOTHING;

	snprintf(genbuf, sizeof(genbuf), "������� %d ����´���: ", ent);
	a_prompt(genbuf, buf, 6, YEA);
	if ((dst = atoi(buf)) <= 0)
		return DONOTHING;

	if (move_record(direct, sizeof(struct annheader), ent, dst) != -1) {
		snprintf(buf, sizeof(buf), "����Ŀ�ӵ�%d���ƶ���%d��", ent, dst);
		atrace(ANN_MOVE, buf, anninfo->title);
	}

	return DIRCHANGED;
}

int
ann_read(int ent, struct annheader *anninfo, char *direct)
{
	char *ptr, fname[PATH_MAX + 1];
	char attach_info[1024];
	int flag;

	if (anninfo->filename[0] == '\0')
		return FULLUPDATE;

	/* ����Ƿ�Ϊ����Ŀ */
	if (strcmp(anninfo->filename, "@NULL") == 0)
		return FULLUPDATE;

	/* ����Ƿ�Ϊ���澫���� */
	if (strcmp(anninfo->filename, "@BOARDS") == 0)
		return show_board_announce(anninfo->owner);

	/* ����Ƿ�Ϊ������Ŀ */
	if (anninfo->filename[0] == '@')
		return show_announce(anninfo->filename, anninfo->title, 0);

	/* ��ȡ�ļ�/Ŀ¼�� */
	strlcpy(fname, direct, sizeof(fname));
	if ((ptr = strrchr(fname, '/')) == NULL)
		return FULLUPDATE;
	*ptr = '\0';
	snprintf(fname, sizeof(fname), "%s/%s", fname, anninfo->filename);

	/* �ж���Ŀ���� */
	if (anninfo->flag & ANN_LINK) {
		switch (anninfo->filename[0]) {
		case 'G':       /* ���� */
			flag = ANN_GUESTBOOK;
			break;
		case 'D':       /* Ŀ¼ */
			flag = ANN_DIR;
			break;
		case 'M':       /* �ļ� */
			flag = ANN_FILE;
			break;
		default:
			return FULLUPDATE;
		}
	} else {
		flag = anninfo->flag;
	}

	if (flag & ANN_FILE) {
		if (anninfo->flag & ANN_ATTACHED) {
	                snprintf(attach_info, sizeof(attach_info),
                	         "http://%s/bbsanc?path=%s",
				 BBSHOST, fname + 10);
		}
		
		if (ansimore4(fname, NULL, NULL, (anninfo->flag & ANN_ATTACHED) ? attach_info : NULL, NA) == -1) {
			clear();
			move(10, 29);
			prints("�Բ����������ݶ�ʧ!");
			pressanykey();
			return FULLUPDATE;
		}

		clear_line(t_lines - 1);
		outs("\033[1;31;44m[�Ķ�����������]  \033[33m �� ���� Q,�� �� ��һ�� U,���� ��һ�� <Space>,�� ��          \033[m");

		switch (egetch()) {
			case KEY_DOWN:
			case ' ':
			case '\n':
				return READ_NEXT;
			case KEY_UP:
			case 'u':
			case 'U':
				return READ_PREV;
		}
	} else if ((flag & ANN_DIR) || (flag & ANN_GUESTBOOK)) {
		strlcat(fname, "/.DIR", sizeof(fname));
		return show_announce(fname, anninfo->title, flag);
	}
	return FULLUPDATE;
}

int
ann_select_board(int ent, struct annheader *anninfo, char *direct)
{
	char bname[BFNAMELEN];

	move(0, 0);
	clrtoeol();
	prints("ѡ��һ�������� (Ӣ����ĸ��Сд�Կ�)\n");
	prints("������������ (���հ׼��Զ���Ѱ): ");
	clrtoeol();

	make_blist();
	namecomplete(NULL, bname);
	return show_board_announce(bname);
}

int
ann_change_title(int ent, struct annheader *anninfo, char *direct)
{
	char oldtitle[TITLELEN];

	/* ���Ȩ�� */
	if (ainfo.manager != YEA)
		return DONOTHING;
	
	strlcpy(oldtitle, anninfo->title, sizeof(oldtitle));
	a_prompt("����: ", anninfo->title, sizeof(anninfo->title), NA);
	if (anninfo->title[0] == '\0')
		return DONOTHING;
	if (safe_substitute_record(direct, (struct fileheader *)anninfo, ent, NA) != -1)
		atrace(ANN_CTITLE, oldtitle, anninfo->title);

	return PARTUPDATE;
}

int
ann_edit_notes(int ent, struct annheader *anninfo, char *direct)
{
	char ans[4], fname[PATH_MAX + 1];
	int aborted;

	if (!HAS_PERM(PERM_ACBOARD) && ainfo.manager == NA)     /* monster: suggested by MidautumnDay */
		return DONOTHING;

	snprintf(fname, sizeof(fname), "%s/welcome", ainfo.basedir);

	clear();
	move(1, 0);
	prints("�༭/ɾ������¼");
	getdata(3, 0, "(E)�༭ (D)ɾ�� (A)ȡ�� [E]: ", ans, 2, DOECHO, YEA);
	if (ans[0] == 'A' || ans[0] == 'a') {
		aborted = -1;
	} else if (ans[0] == 'D' || ans[0] == 'd') {
		move(4, 0);
		if (askyn("���Ҫɾ������¼", NA, NA) == YEA) {
			move(5, 0);
			unlink(fname);
			atrace(ANN_DNOTES, NULL, NULL);
			prints("����¼�Ѿ�ɾ��...\n");
		}
	} else {
		if (vedit(fname, EDIT_MODIFYHEADER) != -1) {
			atrace(ANN_ENOTES, NULL, NULL);
			prints("����¼�Ѹ���\n");
		}
	}
	pressreturn();
	return FULLUPDATE;
}

int
ann_show_notes(int ent, struct annheader *anninfo, char *direct)
{
	char fname[PATH_MAX + 1];

	snprintf(fname, sizeof(fname), "%s/welcome", ainfo.basedir);
	if (dashf(fname)) show_help(fname);

	return FULLUPDATE;
}

int
ann_crosspost(int ent, struct annheader *anninfo, char *direct)
{
	char fname[PATH_MAX + 1], tname[PATH_MAX + 1];
	char bname[BFNAMELEN], title[TITLELEN], buf[8192];
	FILE *fin, *fout;
	int len;

	if ((!HAS_PERM(PERM_POST) && !HAS_PERM(PERM_WELCOME)) || !(anninfo->flag & ANN_FILE))
		return DONOTHING;

	snprintf(fname, sizeof(fname), "%s/%s", ainfo.basedir, anninfo->filename);
	snprintf(tname, sizeof(tname), "%s/cross.%5d", ainfo.basedir, getpid());

	clear();
	if (get_a_boardname(bname, "������Ҫת��������������: ")) {
		move(1, 0);
		if (deny_me(bname) || !haspostperm(bname) || check_readonly(bname)) {
			prints("\n\n����������Ψ����, ����������Ȩ���ڴ˷������¡�");
		} else {
			snprintf(genbuf, sizeof(genbuf), "��ȷ��Ҫת���� %s ����", bname);
			if (askyn(genbuf, NA, NA) == 1) {
				move(3, 0);

				if ((fin = fopen(fname, "r")) == NULL)
					goto error_process;

				if ((fout = fopen(tname, "w")) == NULL) {
					fclose(fin);
					goto error_process;
				}

				while ((len = fread(buf, 1, sizeof(buf), fin)) > 0)
					fwrite(buf, 1, len, fout);

				fprintf(fout, "\n--\n\033[m\033[1;%2dm�� ת��:��%s %s��[FROM: %s]\033[m\n",
					(currentuser.numlogins % 7) + 31, BoardName, BBSHOST, fromhost);

				fclose(fin);
				fclose(fout);

				set_safe_record();
				strlcpy(quote_user, anninfo->owner, sizeof(quote_user));
				snprintf(title, sizeof(title), "[ת��] %s", anninfo->title);
				postfile(tname, bname, title, 3);
				unlink(tname);
				prints("�Ѿ�����ת���� %s ����", bname);
			}
		}
	}

	pressreturn();
	return FULLUPDATE;

error_process:
	prints("ת��ʧ��: ϵͳ��������");
	pressreturn();
	return FULLUPDATE;
}

#ifdef INTERNET_EMAIL

int
ann_forward(int ent, struct annheader *anninfo, char *direct, int uuencode)
{
	struct fileheader fileinfo;
	char fname[PATH_MAX + 1], tname[PATH_MAX + 1], buf[8192];
	FILE *fin, *fout;
	int len;

	if (!HAS_PERM(PERM_FORWARD) || !(anninfo->flag & ANN_FILE))
		return DONOTHING;

	snprintf(fname, sizeof(fname), "%s/%s", ainfo.basedir, anninfo->filename);
	snprintf(tname, sizeof(tname), "%s/forward.%5d", ainfo.basedir, getpid());

	if ((fin = fopen(fname, "r")) == NULL)
		return DONOTHING;

	if ((fout = fopen(tname, "w")) == NULL) {
		fclose(fin);
		return DONOTHING;
	}

	getdatestring(time(NULL));
	fprintf(fout, "������: %s (%s)\n", currentuser.userid, currentuser.username);
	fprintf(fout, "��  ��: %s\033[m\n", anninfo->title);
	fprintf(fout, "����վ: %s (%s)\n", BoardName, datestring);
	fprintf(fout, "��  Դ: %s\n\n", currentuser.lasthost);

	while ((len = fread(buf, 1, sizeof(buf), fin)) > 0)
		fwrite(buf, 1, len, fout);

	fclose(fin);
	fclose(fout);

	memcpy(&fileinfo, anninfo, sizeof(fileinfo));
	snprintf(fileinfo.filename, sizeof(fileinfo.filename), "forward.%5d", getpid());

	switch (doforward(ainfo.basedir, &fileinfo, uuencode)) {
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
	unlink(tname);
	pressreturn();
	clear();
	return FULLUPDATE;
}

int
ann_mail_forward(int ent, struct annheader *anninfo, char *direct)
{
	return ann_forward(ent, anninfo, direct, NA);
}

int
ann_mail_u_forward(int ent, struct annheader *anninfo, char *direct)
{
	return ann_forward(ent, anninfo, direct, YEA);
}

#endif

int
ann_view_atrace(int ent, struct annheader *anninfo, char *direct)
{
	char fname[PATH_MAX + 1], olddirect[PATH_MAX + 1];

	/* monster: ���������ܿ��Բ鿴���澫������¼, ������ζ�����������ܿ��Ը��ľ��������� */
	if ((!strncmp(direct, "0Announce/boards/", 17) && HAS_PERM(PERM_OBOARDS)) || ainfo.manager == YEA) {
 		if (get_atracename(fname) == -1)
			return DONOTHING;
	} else {
		return DONOTHING;
	}

	strlcpy(olddirect, currdirect, sizeof(olddirect));
	i_read(DIGESTRACE, fname, NULL, NULL, atracetitle, atracedoent, update_atraceendline, &atrace_comms[0],
	       get_records, get_num_records, sizeof(struct anntrace));
	strlcpy(currdirect, olddirect, sizeof(currdirect));
	return NEWDIRECT;
}

/* �������������� */
void
ann_process_item(FILE *fp, char *directory, struct annheader *header, int *last, int level,int onlydir) 
{
	char buf[256] = { '\0' }, tdir[PATH_MAX + 1];
	int traverse, color;
	int i, maxlen;

	if ((header->flag & ANN_LINK) || (header->flag & ANN_RLINK)) {	// �ν�
		traverse = NA;
		color = 32;			
	} else if (header->flag & ANN_DIR) {				// Ŀ¼
		traverse = (level < 15) ? YEA : NA;
		color = 37;
	} else if (header->flag & ANN_GUESTBOOK) {			// ���Բ�
		traverse = (level < 15) ? YEA : NA;
		color = 35;
	} else {							// �ļ� - ����
		traverse = NA;
		color = 0;
	}

	for (i = 0; i < level - 1; i++)
		strlcat(buf, (last[i] == NA) ? "��   " : "     ", sizeof(buf));
	strlcat(buf, (last[level - 1] == NA) ? "���� " : "���� ", sizeof(buf));

	if (color > 0)
		snprintf(buf, sizeof(buf), "%s\033[1;%dm", buf, color);

	maxlen = 78 - level * 5;
	if (strlen(header->title) > maxlen) {
		char title[TITLELEN];
		
		strlcpy(title, header->title, sizeof(title));
		title[maxlen] = '\0';
		title[maxlen - 1] = '.';
		title[maxlen - 2] = '.';
		title[maxlen - 3] = '.';
		strlcat(buf, title, sizeof(buf));
	} else {
		strlcat(buf, header->title, sizeof(buf));
	}
	strlcat(buf, (color > 0) ? "\033[m\n" : "\n", sizeof(buf));
	fputs(buf, fp);
	
	if (traverse == YEA) {
		snprintf(tdir, sizeof(tdir), "%s/%s", directory, header->filename);
		ann_index_traverse(fp, tdir, last, level + 1,onlydir);
	}
}

void
ann_index_traverse(FILE *fp, char *directory, int *last, int level,int onlydir) /*SuperDog��������ֻ��ʾĿ¼������ʱΪYEA,����ΪNA*/
{
	char filename[PATH_MAX + 1];
	struct annheader header, rheader;
	FILE *fdir;

	snprintf(filename, sizeof(filename), "%s/.DIR", directory);
	if ((fdir = fopen(filename, "r")) == NULL)
		return;

	if (fread(&header, 1, sizeof(header), fdir) != sizeof(header)) {
		fclose(fdir);
		return;
	}
	if (onlydir==YEA && !((header.flag & ANN_DIR) || (header.flag & ANN_GUESTBOOK)))
	{
		while (fread(&header,1,sizeof(header),fdir) == sizeof(header))
			if ((header.flag & ANN_DIR) || (header.flag & ANN_GUESTBOOK)) break;
	}

	last[level - 1] = NA;
	while (fread(&rheader, 1, sizeof(rheader), fdir) == sizeof(rheader)) {
		if (onlydir==YEA) {
			if ((rheader.flag & ANN_DIR) || (rheader.flag & ANN_GUESTBOOK))
			{
				ann_process_item(fp, directory, &header, last, level, onlydir);
				memcpy(&header, &rheader, sizeof(header));
			}
		}
		else {
			ann_process_item(fp, directory, &header, last, level, onlydir);
                        memcpy(&header, &rheader, sizeof(header));
		}
	}
	last[level - 1] = YEA;
	if (onlydir==YEA) {
		if ((header.flag & ANN_DIR) || (header.flag & ANN_GUESTBOOK))
			ann_process_item(fp, directory, &header, last, level, onlydir);
	}
	else ann_process_item(fp, directory, &header, last, level, onlydir);
	fclose(fdir);
}

int
ann_make_index(int ent, struct annheader *anninfo, char *direct)
{
	FILE *fp;
	int last[16];
	char filename[PATH_MAX + 1];

	if (ainfo.manager != YEA)
		return DONOTHING;

	snprintf(filename, sizeof(filename), "tmp/annindex.%5d", getpid());
	if ((fp = fopen(filename, "w")) == NULL)
		return DONOTHING;

	clear_line(t_lines - 1);
	prints("�������ɾ��������������Ժ򡣡���");
	refresh();

	last[0] = NA;
	fprintf(fp, "\033[1;41;33m������������%s%*s\033[m\n", ainfo.title, (int)(67 - strlen(ainfo.title)), " ");
	ann_index_traverse(fp, ainfo.basedir, last, 1, NA);
	fclose(fp);
	ann_add_file(filename, "����������", "", currdirect, 0, YEA);
	atrace(ANN_INDEX, NULL, NULL);		

	return PARTUPDATE;
}

int 
ann_make_dirindex(int ent,struct annheader *anninfo,char *direct)
{
	FILE *fp;
	int last[16];
	char filename[PATH_MAX + 1];
	if (ainfo.manager != YEA)
		return DONOTHING;
	snprintf(filename, sizeof(filename), "tmp/anndirindex.%5d", getpid());
	if ((fp = fopen(filename, "w")) == NULL)
		return DONOTHING;

	clear_line(t_lines - 1);
	prints("�������ɾ�����Ŀ¼���������Ժ򡣡���");
	refresh();

	last[0] = NA;
	fprintf(fp, "\033[1;41;33m������Ŀ¼������%s%*s\033[m\n", ainfo.title, (int)(67 - strlen(ainfo.title)), " ");
	ann_index_traverse(fp, ainfo.basedir, last, 1, YEA);
	fclose(fp);
	ann_add_file(filename, "������Ŀ¼����", "", currdirect, 0, YEA);
	atrace(ANN_INDEX, NULL, NULL);		

	return PARTUPDATE;

}
struct one_key ann_comms[] = {
	{ 'A',          ann_switch_sauthor   },
	{ 'a',          ann_add_article      },
	{ 'c',          ann_copy             },
	{ 'D',          ann_delete_range     },
	{ 'd',          ann_delete_item      },
	{ 'E',          ann_edit_article     },
	{ 'e',          ann_select_item      },
	{ 'G',          ann_create_guestbook },
	{ 'g',          ann_create_folder    },
	{ 'f',          ann_setpath          },
	{ 'h',          ann_help             },
	{ 'i',          ann_export_savepost  },
	{ 'I',		ann_make_index	     },
        { Ctrl('D'),    ann_make_dirindex    }, 
	{ 'L',          ann_make_symbolink   },
	{ 'm',          ann_move_item        },
	{ 'o',          fast_cloak           },
	{ 'p',          ann_paste            },
	{ 'r',          ann_read             },
	{ 's',          ann_select_board     },
	{ 'T',          ann_change_title     },
	{ 'x',          ann_cut              },
	{ 'W',          ann_edit_notes       },
	{ '.',          ann_create_special   },
	{ ',',          ann_view_atrace      },
        { '/', 		title_search_down    },         
        { '?',		title_search_up      },           
#ifdef INTERNET_EMAIL
	{ 'F',          ann_mail_forward     },
	{ 'U',          ann_mail_u_forward   },
#endif
	{ KEY_TAB,      ann_show_notes       },
	{ Ctrl('C'),    ann_crosspost        },
	{ Ctrl('E'),    ann_process_selected },
	{ Ctrl('P'),    ann_add_article      },
	{ Ctrl('V'), 	x_lockscreen_silent },
	{ '\0',         NULL }
};

struct one_key ann_comms_readonly[] = {
	{ 'A',          ann_switch_sauthor   },
	{ 'h',          ann_help             },
	{ 'o',          fast_cloak           },
	{ 'r',          ann_read             },
	{ 's',          ann_select_board     },
	{ Ctrl('V'), 	x_lockscreen_silent },
	{ '\0',         NULL }
};

static struct boardheader *blist;       /* ���ڰ����б� */
static int bcount;                      /* ���ڰ�����Ŀ */
static char groupid;                    /* ����ʾ */

static int
get_num_records_blist(char *filename, int size)
{
	return bcount;
}

static int
get_records_blist(char *filename, void *rptr, int size, int id, int number)
{
	struct annheader fileinfo;
	void *ptr = rptr;
	int count = 0;

	memset(&fileinfo, 0, sizeof(fileinfo));
	strlcpy(fileinfo.filename, "@BOARDS", sizeof(fileinfo.filename));
	fileinfo.flag = ANN_DIR;
	fileinfo.mtime = time(NULL);

	while ((number--) && (id <= bcount)) {
		strlcpy(fileinfo.title, blist[id - 1].title + 8, sizeof(fileinfo.title));
		strlcpy(fileinfo.owner, blist[id - 1].filename, sizeof(fileinfo.owner) + sizeof(fileinfo.editor));
		memcpy(ptr, &fileinfo, sizeof(fileinfo));
		ptr += size;
		++id;
		++count;
	}
	return count;
}

static void
init_blist()
{
	int i, size = 25;
	char *prefix = NULL, buf[STRLEN] = "EGROUP*";

	bcount = 0;

	if (groupid != '*') {
		buf[6] = groupid;
		if ((prefix = sysconf_str(buf)) == NULL || prefix[0] == '\0')   /* ��ȡ��ǰ׺ */
			return;
	}

	/* �����ڴ� */
	if ((blist = malloc(sizeof(struct boardheader) * size)) == NULL)
		return;

	/* �������ڰ����б� */
	for (i = 0; i < numboards; i++) {
		/* ���ز��ɼ�������������� */
		if (bcache[i].filename[0] == '\0')
			continue;

		if (!(bcache[i].level & PERM_POSTMASK) && !HAS_PERM(bcache[i].level) && !(bcache[i].level & PERM_NOZAP))
			continue;

		if (groupid != '*' && strchr(prefix, bcache[i].title[0]) == NULL)
			continue;

		if (bcache[i].flag & BRD_RESTRICT)
			if (restrict_boards == NULL || strsect(restrict_boards, bcache[i].filename, "\n\t ") == NULL)
				continue;

		/* ���ư���������blist����, �粻���ڴ������·���, ����Ϊ25 */
		if (bcount == size - 1) {
			size += 25;
			if ((blist = realloc(blist, sizeof(struct boardheader) * size)) == NULL) {
				bcount = 0;
				return;
			}
		}

		memcpy(&blist[bcount], &bcache[i], sizeof(struct boardheader));
		++bcount;
	}
}

static void
free_blist()
{
	if (blist != NULL) {
		free(blist);
		blist = NULL;
		bcount = 0;
	}
}

int
show_announce_special(char *direct)
{
	if (strncmp(direct, "@GROUP:", 7) == 0) {               // �������������� (�����б�)
		if (bcount > 0)                                 // ��Ϊblist, bcount�ȱ�������, �ʲ�������
			return DONOTHING;

		groupid = direct[7];
		i_read(DIGEST, direct, init_blist, free_blist, anntitle, anndoent, update_annendline, &ann_comms_readonly[0],
		       get_records_blist, get_num_records_blist, sizeof(struct annheader));

		return NEWDIRECT;
	} else if (strncmp(direct, "@PERSONAL:", 10) == 0) {    // �����ļ�
		return show_personal_announce(direct + 10);
	} else if (strcmp(direct, "@UP") == 0) {                // ������һ��Ŀ¼
		return DOQUIT;
	}

	return DONOTHING;
}

int
show_announce(char *direct, char *title, int flag)
{
	int result = NEWDIRECT;
	struct anninfo sinfo;
	char *ptr;

	memcpy(&sinfo, &ainfo, sizeof(sinfo));                  // ������һ�����
	ainfo.manager = check_annmanager(direct);               // ����Ƿ���Թ������Ŀ¼
	ainfo.flag = flag;                                      // ���ñ�־
	strlcpy(ainfo.direct, direct, sizeof(ainfo.direct));    // ���������ļ���
	strlcpy(ainfo.title, title, sizeof(ainfo.title));       // ���ñ���
	strlcpy(ainfo.basedir, direct, sizeof(ainfo.basedir));  // �������������Ŀ¼
	if ((ptr = strrchr(ainfo.basedir, '/')) != NULL)
		*ptr = '\0';

	if (direct[0] == '@') {
		result = show_announce_special(direct);
	} else {
		/* ��ʾ����¼ */
		ann_show_notes(0, NULL, NULL);

		i_read(DIGEST, direct, NULL, NULL, anntitle, anndoent, update_annendline, &ann_comms[0],
			      get_records, get_num_records, sizeof(struct annheader));
	}

	memcpy(&ainfo, &sinfo, sizeof(ainfo));                  // �ָ���һ�����
	strlcpy(currdirect, ainfo.direct, sizeof(currdirect));  // �ָ������ļ���

	return result;
}

/* �鿴ָ������ľ����� */
int
show_board_announce(char *bname)
{
	struct boardheader *bp;
	char direct[PATH_MAX + 1];

	if ((bname[0] == '\0') || ((bp = getbcache(bname)) == NULL))
		return FULLUPDATE;

	snprintf(direct, sizeof(direct), "0Announce/boards/%s/.DIR", bname);
	return show_announce(direct, bp->title + 11, ANN_DIR);
}

/* �鿴ָ���û��ĸ����ļ� */
int
show_personal_announce(char *userid)
{
	char direct[PATH_MAX + 1];
	struct annheader fileinfo;

	strlcpy(ainfo.direct, currdirect, sizeof(ainfo.direct));
	if (userid == NULL || userid[0] == '\0' || userid[0] == '*')
		return show_announce("0Announce/personal/.DIR", BBSNAME" �����ļ�", ANN_DIR);

	if (isalpha(userid[0]) && userid[1] == '\0') {
		snprintf(direct, sizeof(direct), "0Announce/personal/%c/.DIR", mytoupper(userid[0]));
		return show_announce(direct, BBSNAME" �����ļ�", ANN_DIR);
	}

	snprintf(direct, sizeof(direct), "0Announce/personal/%c/.DIR", mytoupper(userid[0]));
	if (search_record(direct, &fileinfo, sizeof(fileinfo), cmpafilename, userid) <= 0)
		return DONOTHING;

	snprintf(direct, sizeof(direct), "0Announce/personal/%c/%s/.DIR", mytoupper(userid[0]), userid);
	return show_announce(direct, fileinfo.title, ANN_DIR);
}

/* �鿴ָ���û��ĸ����ļ� */
int
pannounce()
{
	char userid[IDLEN + 2];

	clear();
	move(2, 0);
	usercomplete("���뿴˭�ĸ����ļ�: ", userid);

	if (userid[0] != '\0') {
		show_personal_announce(userid);
	}

	return FULLUPDATE;
}

/* �����������ߵĸ����ļ� */
int
author_announce(int ent, struct fileheader *fileinfo, char *direct)
{
	return show_personal_announce(fileinfo->owner);
}

/* ���뵱ǰ�澫���� */
int
currboard_announce()
{
	strlcpy(ainfo.direct, currdirect, sizeof(ainfo.direct));
	return show_board_announce(currboard);
}

/* ���뾫���������� */
int
announce()
{
	strlcpy(ainfo.direct, currdirect, sizeof(ainfo.direct));
	return show_announce("0Announce/.DIR", "������������", ANN_DIR);
}

/* ���������ļ� */
int
add_personalcorpus()
{
	int id;
	char userid[IDLEN + 2];
	char direct[PATH_MAX + 1], title[TITLELEN];
	struct annheader fileinfo;
	struct userec lookupuser;

	/* SYSOP�;��������ܲſ��Դ��������ļ� */
	if (!HAS_PERM(PERM_SYSOP) && !HAS_PERM(PERM_ANNOUNCE))
		return DONOTHING;

	modify_user_mode(ADMIN);
	stand_title("���������ļ�");

	move(2, 0);
	usercomplete("����Ϊ˭���������ļ�: ", userid);
	clear_line(2);
	if (userid[0] == '\0' || (id = getuser(userid, &lookupuser)) == 0)
		return 0;

	snprintf(direct, sizeof(direct), "0Announce/personal/%c/.DIR", mytoupper(userid[0]));
	if (search_record(direct, &fileinfo, sizeof(fileinfo), cmpafilename, userid) <= 0) {
		/* ��ʼ�� fileinfo */
		memset(&fileinfo, 0, sizeof(fileinfo));
		strlcpy(fileinfo.filename, userid, sizeof(fileinfo.filename));
		fileinfo.flag = ANN_DIR;
		fileinfo.mtime = time(NULL);

		/* �趨�ļ����� */
		getdata(2, 0, "����: ", title, sizeof(fileinfo.title), DOECHO, YEA);
		my_ansi_filter(title);
		if (title[0] == '\0')
			return 0;

		memset(fileinfo.title, '-', sizeof(fileinfo.title));
		memcpy(fileinfo.title, userid, strlen(userid));
		strlcpy(fileinfo.title + 19, title, sizeof(fileinfo.title) - 19);

		/* ��Ӽ�¼ */
		if (append_record(direct, &fileinfo, sizeof(fileinfo)) == -1) {
			move(4, 0);
			outs("����ʧ��...");
			pressanykey();
			return 0;
		}
	}

	/* �����ļ�����Ȩ�� */
	if (!(lookupuser.userlevel & PERM_PERSONAL)) {
		lookupuser.userlevel |= PERM_PERSONAL;
		if (substitute_record(PASSFILE, &lookupuser, sizeof(lookupuser), id) == -1) {
			move(4, 0);
			outs("�޷�����ʹ�����ļ�����Ȩ��...");
			pressanykey();
			return 0;
		}
	}

	/* ���������ļ�Ŀ¼ */
	snprintf(direct, sizeof(direct), "0Announce/personal/%c/%s", mytoupper(userid[0]), userid);
	if (f_mkdir(direct, 0755) == -1) {
		move(4, 0);
		outs("����ʧ��...");
		pressanykey();
		return 0;
	}

	move(4, 0);
	prints("%s�ĸ����ļ������ɹ�", userid);
	snprintf(genbuf, sizeof(genbuf), "Ϊ%s���������ļ�", userid);
	securityreport(genbuf);
	pressanykey();

	return 0;
}
