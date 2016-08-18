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

struct BCACHE *brdshm = NULL;
struct boardheader *bcache = NULL;
int numboards = -1;	/* ��������Ŀ */

/* monster: adopted from ytht */
/* ��ȡ�������һƪ���·���ʱ�䵽 lastpost, ����������total */
int
get_lastpost(char *board, unsigned int *lastpost, unsigned int *total)
{
	struct fileheader fh;
	struct stat st;
	char filename[PATH_MAX + 1];
	int fd, atotal;

	snprintf(filename, sizeof(filename), "boards/%s/.DIR", board);

	if ((fd = open(filename, O_RDONLY)) == -1)
		return -1;
	if (fstat(fd, &st) == -1 || st.st_size == 0) {
		close(fd);
		return -1;
	}

	atotal = st.st_size / sizeof (fh);
	*total = atotal;
	if (lseek(fd, (off_t) (atotal - 1) * sizeof (fh), SEEK_SET) != -1) /* seek�����һƪ����header */
		if (read(fd, &fh, sizeof(fh)) == sizeof(fh))
			*lastpost = fh.filetime;
	close(fd);
	return 0;
}

int
update_lastpost(char *board)
{
	struct boardheader *bptr;

	if ((bptr = getbcache(board)) == NULL)
		return -1;
	unsigned lastpost = bptr->lastpost;
	get_lastpost(bptr->filename, &bptr->lastpost, &bptr->total);

	return 0;
}

int
update_total_today(char *board)
{
	struct boardheader *bptr;

	if ((bptr = getbcache(board)) == NULL)
		return -1;
	
	bptr->total_today++;
	return 0;
}

int
fillbcache(void *fptr, int unused)
{
	struct boardheader *bptr;

	if (numboards >= MAXBOARD)
		return 0;

	bptr = &bcache[numboards++];
	memcpy(bptr, fptr, sizeof(struct boardheader));
	get_lastpost(bptr->filename, &bptr->lastpost, &bptr->total);
	return 0;
}

void
resolve_boards(void)
{
	int fd;
	struct stat st;

	if (brdshm == NULL) {
		brdshm = attach_shm("BCACHE_SHMKEY", sizeof(*brdshm));
	}
	numboards = brdshm->number;
	bcache = brdshm->bcache;
	if (brdshm->updating != YEA && brdshm->uptime + 300 < time(NULL)) {
		if (stat(BOARDS, &st) == -1)
			return;

		if (brdshm->uptime > st.st_mtime)
			return;


		if ((fd = filelock("bcache.lock", NA)) > 0) {
			brdshm->updating = YEA;
			log_usies("CACHE", "reload bcache");
			numboards = 0;
			apply_record(BOARDS, fillbcache, sizeof (struct boardheader));
			brdshm->number = numboards;
			brdshm->uptime = time(NULL);
			close(fd);
			brdshm->updating = NA;
		} 
	}
}

int
apply_boards(int (*func)(struct boardheader *bptr))
{
	int i;

	resolve_boards();
	for (i = 0; i < numboards; i++)
		if (bcache[i].level & PERM_POSTMASK || HAS_PERM(bcache[i].level)
		    || (bcache[i].level & PERM_NOZAP))
			if ((*func) (&bcache[i]) == QUIT)
				return QUIT;
	return 0;
}

struct boardheader *
getbcache(char *bname)
{
	int i;
	static char last_bname[STRLEN];
	static int last_bindex;


	/* monster: a cache specific to getbcache function for faster lookup */
	if (!strncasecmp(bname, last_bname, STRLEN)) {
		return &bcache[last_bindex];
	} else {
		strcpy(last_bname, bname);
	}

	resolve_boards();
	for (i = 0; i < numboards; i++) {
		if (!strncasecmp(bname, bcache[i].filename, STRLEN)) {
			last_bindex = i;
			return &bcache[i];
		}
	}
	return NULL;
}

int
getbnum(char *bname)
{
	int i;

	resolve_boards();
	for (i = 0; i < numboards; i++) {
		if (bcache[i].level & PERM_POSTMASK || HAS_PERM(bcache[i].level)
		    || (bcache[i].level & PERM_NOZAP)) {
			if (!strncasecmp(bname, bcache[i].filename, STRLEN))
				return i + 1;
		}
	}
	return 0;
}

void
setoboard(char *bname)
{
	int i;

	resolve_boards();
	for (i = 0; i < numboards; i++) {
		if (bcache[i].level & PERM_POSTMASK || HAS_PERM(bcache[i].level)
		    || (bcache[i].level & PERM_NOZAP)) {
			if (bcache[i].filename[0] != 0 &&
			    bcache[i].filename[0] != ' ') {
				strcpy(bname, bcache[i].filename);
				return;
			}
		}
	}
}


int
haspostperm(char *bname)
{
	int i;
	if ((i = getbnum(bname)) == 0)
		return 0;

#ifdef AUTHHOST
	/* û��post���Ƶİ�, ��post���Ƶİ�������ó���δ�����û�id����,����freshmen */
	if ( !( bcache[i-1].level & PERM_POSTMASK ) ) 
		/* δ����id �� SYSOP ���ɷ��� */
		if ( !HAS_ORGPERM(PERM_WELCOME) && !HAS_ORGPERM(PERM_SYSOP)) 
			return 0;
#endif
	/* freestyler: �Ǳ�׼ģʽ���Է����� 
	if (digestmode)
		return 0;  */
	if (!strcmp(currentuser.userid, "guest"))
		return 0;
	if (deny_me(bname) && !HAS_PERM(PERM_SYSOP))
		return 0;
	if (!strcmp(bname, "syssecurity"))
		return 0;
#ifdef AUTHHOST
	if (HAS_PERM(PERM_WELCOME))
#endif
	if (strcmp(bname, DEFAULTBOARD) == 0) /* complain */
		return 1;
	set_safe_record();

	// 2007.07.09 Change by Henry:
	// �ȼ��������ԣ��ټ��POSTȨ����һЩδ���POSTȨ���û�
	// ��Ȼ������ĳЩ�ض��淢�ģ������û�����δͨ��ע��ĺͱ�
	// ��ȫվ�ġ�Ŀǰ������������û���������
	if (bcache[i - 1].level & PERM_POSTMASK)
		return HAS_PERM((bcache[i - 1].level & ~PERM_NOZAP) & ~PERM_POSTMASK);

	return HAS_ORGPERM(PERM_POST);
}


/* int */
/* haspostperm(char *bname) */
/* { */
/* 	int i; */

/* #ifdef AUTHHOST */
/* 	if (!valid_host_mask && !(perm_unauth & PERM_POST)) */
/* 		return 0; */
/* #endif */
/* 	if (digestmode) */
/* 		return 0; */
/* 	if (!strcmp(currentuser.userid, "guest")) */
/* 		return 0; */
/* 	if (deny_me(bname) && !HAS_PERM(PERM_SYSOP)) */
/* 		return 0; */
/* 	if (!strcmp(bname, "syssecurity")) */
/* 		return 0; */
/* 	if (strcmp(bname, DEFAULTBOARD) == 0) */
/* 		return 1; */
/* 	if ((i = getbnum(bname)) == 0) */
/* 		return 0; */
/* 	set_safe_record(); */
/* 	if (!HAS_PERM(PERM_POST)) */
/* 		return 0; */
/* 	return (HAS_PERM((bcache[i - 1].level & ~PERM_NOZAP) & ~PERM_POSTMASK)); */
/* } */

int
normalboard(char *bname)
{
	struct boardheader *bp;

	if ((bp = getbcache(bname)) == NULL)
		return NA;
	if (bp->flag & (ANONY_FLAG | JUNK_FLAG | BRD_RESTRICT | BRD_NOPOSTVOTE))
		return NA;
	
	return (bp->level == 0) ? YEA : NA;
}

int 
junkboard(void)
{
        struct boardheader *bp;

        bp = getbcache(currboard);
        return (bp->flag & JUNK_FLAG) ? YEA : NA;
}


#ifdef	INBOARDCOUNT
/* freestyler: */
int 
board_setcurrentuser(int idx, int num)
{
	if(idx < 0) return 0;
	if(num > 0) {
		int fd = filelock("inboarduser.lock", 1);
		if (fd > 0) {
			brdshm->inboard[idx]++;
			fileunlock(fd);
		} 
	} else if(num < 0) {
		 int fd = filelock("inboarduser.lock", 1);
		 if(fd > 0) {
			 brdshm->inboard[idx] = brdshm->inboard[idx] > 0 ? brdshm->inboard[idx] - 1 : 0;
			 fileunlock(fd);
		 }
	} 
	
	return brdshm->inboard[idx];
}
#endif
