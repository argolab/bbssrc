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

    Adms Bulletin Board System
    Copyright (C) 2013, Mo Norman, LTaoist6@gmail.com

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

struct postheader header;
extern int child_pid;
int digestmode;
int local_article;
struct userec currentuser;	
int usernum = 0;
char currboard[BFNAMELEN + 1];
int current_bm;	/* is current bm ? */
char someoneID[31];
int FFLL = 0;
int ReadID = 0;
int noreply = NA;
int anonymousmail = 0;
static int postandmail = 0;
extern char *restrict_boards;

unsigned int posts_article_id;	/* for write_posts() */


#ifdef  INBOARDCOUNT 
int 	is_inboard = 0;		/* �û��������?  */
#endif 
char	attach_link[1024];	/* �������� */
char	attach_info[1024];	/* ������Ϣ */



#ifdef MARK_X_FLAG
int markXflag = 0;
#endif
int mailtoauthor = 0;

char genbuf[BUFLEN];
char save_title[TITLELEN];
char quote_title[TITLELEN], quote_board[BFNAMELEN + 1];
char quote_file[PATH_MAX + 1], quote_user[IDLEN + 2];
struct	fileheader* quote_fh;	/* freestyler: for ת������ */

#ifndef NOREPLY
char replytitle[STRLEN];
#endif

int totalusers, usercounter;

extern int friendflag;

/* monster: �������Ƿ�ֻ�� */
inline int
check_readonly(char *board)
{
	struct boardheader *bp;

	bp = getbcache(board);
	return (bp->flag & BRD_READONLY) ? YEA : NA;
}
/* Cypress:  ������flag�Ƿ���ĳ��attr */
inline int 
check_board_attr(char *board, unsigned int attr)
{
	struct boardheader *bp;

	bp = getbcache(board);
	return (bp->flag & attr) ? YEA : NA;
}

inline int
check_bm(char *userid, char *bm)
{
	char *p, ch;

	if ((p = strcasestr(bm, userid)) == NULL) /* ignore case search */
		return NA;

	if (p != bm) {
		while (1) {
			ch = *(p - 1);
			p += strlen(userid);
			if (ch == ' ' || ch == '\t' || ch == '(')
				break;
			if (*p == '\0' || (p = strcasestr(p, userid)) == NULL)
				return NA;
		}
	}  else {
		p += strlen(userid);
	}

	return (*p == '\0' || *p == ' ' || *p == '\t' || *p == ')');
}

/* Added by cancel at 2001.09.10 */
/* ����Ƿ��������ޣ����򷵻�0��ʾ���ܷ��ģ����򷵻�1��ʾ���Է��� */
/* monster: ���Ӱ����־�ļ�� */

inline int
check_max_post(char *board)
{
	int num;
	char filename[PATH_MAX + 1];
	struct boardheader *bp;

	bp = getbcache(board);
	if ((HAS_PERM(PERM_BOARDS | PERM_PERSONAL) &&
	     check_bm(currentuser.userid, bp->BM)) ||
	    HAS_PERM(PERM_BLEVELS))
		return YEA;
	
	if (bp->flag & NOPLIMIT_FLAG)
		return YEA;

	snprintf(filename, sizeof(filename), "boards/%s/%s", board, DOT_DIR);
	num = get_num_records(filename, sizeof (struct fileheader));

	if ((!(bp->flag & BRD_MAXII_FLAG) && num >= MAX_BOARD_POST) ||
	    ((bp->flag & BRD_MAXII_FLAG) && num >= MAX_BOARD_POST_II)) {
		clear();
		outs("\n\n           �Բ��𣬸ð���������Ѿ��������ޣ���ʱ�޷�����   \n");
		outs("                        �����ĵȺ�վ���Ĵ���");
		pressreturn();
		clear();
		return NA;
	}
	return YEA;
}

/* Added End */

/* �Ƿ��滻 ���� `$userid' ���ַ��� */
inline int
check_stuffmode(void)
{
	return (uinfo.mode == RMAIL) ? YEA : NA;
}

inline int
isowner(struct userec *user, struct fileheader *fileinfo)
{
	if ((strcmp(fileinfo->owner, user->userid)) &&
	     (strcmp(fileinfo->realowner, user->userid)))
		return NA;

	return (fileinfo->filetime < user->firstlogin) ? NA : YEA;
}

void
setrid(int id)
{
	FFLL = 1;
	ReadID = (id == 0) ? -1 : id;
}

void
setbdir(char *buf, char *boardname)
{
	switch (digestmode) {
	case 0:
		sprintf(buf, "boards/%s/%s", boardname, DOT_DIR);
		break;
	case 1:
		sprintf(buf, "boards/%s/%s", boardname, DIGEST_DIR);
		break;
	case 2:
		sprintf(buf, "boards/%s/%s", boardname, THREAD_DIR);
		break;
	case 3:
		sprintf(buf, "boards/%s/%s", boardname, MARKED_DIR);
		break;
	case 4:
		sprintf(buf, "boards/%s/%s", boardname, AUTHOR_DIR);
		break;
	case 5:         /* ͬ���� */
	case 6:         /* ͬ���� */
		sprintf(buf, "boards/%s/SOMEONE.%s.DIR.%d", boardname,
			someoneID, digestmode - 5);
		break;
	case 7:         /* ����ؼ��� */
		sprintf(buf, "boards/%s/KEY.%s.DIR", boardname,
			currentuser.userid);
		break;
	case 8:         /* ����վ */
		sprintf(buf, "boards/%s/%s", boardname, DELETED_DIR);
		break;
	case 9:         /* ��ֽ¨ */
		sprintf(buf, "boards/%s/%s", boardname, JUNK_DIR);
		break;
	case 10:        /* ����б� */
	case 11:
		sprintf(buf, "boards/%s/%s", boardname, DENY_DIR);
		break;
	}
}

int
denynames(void *userid, void *dh_ptr)
{
	struct denyheader *dh = (struct denyheader *)dh_ptr;

	return (strcmp((char *)userid, dh->blacklist)) ? NA : YEA;
}

int
deny_me(char *bname)
{
	char buf[PATH_MAX + 1];
	struct denyheader dh;

	setboardfile(buf, bname, DENY_DIR);
	return search_record(buf, &dh, sizeof(struct denyheader), denynames, currentuser.userid);
}

int
deny_me_fullsite(void)
{
	struct denyheader dh;

	return search_record("boards/.DENYLIST", &dh, sizeof(struct denyheader), denynames, currentuser.userid);
}

int
shownotepad(void)
{
	modify_user_mode(NOTEPAD);
	ansimore("etc/notepad", YEA);
	return 0;
}

int
g_board_names(struct boardheader *bptr)
{
	if (bptr->flag & BRD_RESTRICT) {
		if (restrict_boards == NULL || strsect(restrict_boards, bptr->filename, "\n\t ") == NULL)
			return 0;
	}

	/* freestyler: У�ڰ�������������namecomplete�� */
	if (valid_host_mask == HOST_AUTH_NA) {
		if (bptr->flag & BRD_INTERN) { /* У�ڰ��� */ 
			if (!HAS_PERM(PERM_SYSOP) && 
			    !HAS_PERM(PERM_OBOARDS) &&
			    !HAS_PERM(PERM_INTERNAL) &&
			    !HAS_PERM(PERM_BOARDS) )
				return 0;
		}
		if (bptr->flag & BRD_HALFOPEN) { /* У�⼤��ſɿ� */
			if (!HAS_PERM(PERM_SYSOP) &&
			    !HAS_PERM(PERM_WELCOME))
				return 0;
		}
	} 

	if ((bptr->level & PERM_POSTMASK) || HAS_PERM(bptr->level)
	    || (bptr->level & PERM_NOZAP)) {
		AddToNameList(bptr->filename);
	}
	return 0;
}

void
make_blist(void)
{
	CreateNameList();
	apply_boards(g_board_names);
}

int
Select(void)
{
	modify_user_mode(SELECT);
	do_select(0, NULL, genbuf);
	return 0;
}

int
Post(void)
{
	if (currboard[0] == 0) {
		prints("\n\n���� (S)elect ȥѡ��һ����������\n");
		pressreturn();
		clear();
		return 0;
	}
#ifndef NOREPLY
	*replytitle = '\0';
#endif
	do_post();
	return 0;
}

int
postfile(char *filename, char *nboard, char *posttitle, int mode)
{
	int result;
	char dbname[BFNAMELEN + 1];

/*
	monster: �Ҳ�������ֻ���ܷ����������

		 1. ����һ�����ļ�
		 2. ��post_cross�д��ļ�ʧ�ܷ���

		 ��û�б�Ҫ����������post֮ǰ�������Ƿ���ڣ�
		 ��Ϊ�������Ŀ����Լ�С

	struct boardheader fh;

	if (search_record(BOARDS, &fh, sizeof(fh), cmpbnames, nboard) <= 0) {
		report("%s �������Ҳ���", nboard);
		return -1;
	}
*/
	strlcpy(quote_board, nboard, sizeof(quote_board));
	strlcpy(dbname, currboard, sizeof(dbname));
	strlcpy(currboard, nboard, sizeof(currboard));
	strlcpy(quote_file, filename, sizeof(quote_file));
	strlcpy(quote_title, posttitle, sizeof(quote_title));
	result = post_cross('l', mode);
	strlcpy(currboard, dbname, sizeof(currboard));
	return result;
}

int
postfile_cross(char *filename, char *qboard, char *nboard, char *posttitle)
{
	char dbname[BFNAMELEN + 1];

	strlcpy(quote_board, qboard, sizeof(quote_board));
	strlcpy(dbname, currboard, sizeof(dbname));
	strlcpy(currboard, nboard, sizeof(currboard));
	strlcpy(quote_file, filename, sizeof(quote_file));
	strlcpy(quote_title, posttitle, sizeof(quote_title));
	post_cross('l', 0);
	strlcpy(currboard, dbname, sizeof(currboard));
	return 0;
}

int
get_a_boardname(char *bname, char *prompt)
{
	struct boardheader fh;

	make_blist();
	namecomplete(prompt, bname);
	if (*bname == '\0') {
		return 0;
	}
	if (search_record(BOARDS, &fh, sizeof (fh), cmpbnames, bname) <= 0) {
		move(1, 0);
		prints("���������������\n");
		pressreturn();
		move(1, 0);
		return 0;
	}
	return 1;
}

#ifdef RECOMMEND
/* Add by Betterman 070423 */

int search_record_by_filename(void *filename, void *buf){
	// ���ڵ�ʱ�䶼��10λ�˵İ�....
	return strcmp((char *)filename, ((struct fileheader *)buf)->filename)  ;
}

int get_origin_info(struct fileheader *fileinfo, int *origin_ent, 
								char *origin_bname, char *origin_fname, 
								struct fileheader *origin_fileinfo){
	FILE *inf;
	char filepath[STRLEN];
	char buf[8192];
	char *ptr, *ptr2;

	snprintf(filepath, sizeof(filepath), "boards/%s/%s", currboard, fileinfo->filename);
	if ((inf = fopen(filepath, "r")) == NULL) {
		report("get_origin_info: cannot open %bnames for reading", filepath);
		return 0;
	}

	while (fgets(buf, 256, inf) != NULL && buf[0] != '\n');
	fgets(buf, 256, inf);
	if ( (ptr = strstr(buf, "�� ���������Ƽ��� ") ) && 
			(ptr2 = strstr(buf, "������ ��") )  ) {
		ptr += 23;
		ptr2 -= 6;
		if(ptr >= ptr2 || ptr - ptr2 >= STRLEN)
			return 0;
		memcpy(origin_bname, ptr, ptr2 - ptr);
		origin_bname[ptr2 - ptr] = '\0';

		fgets(buf, 256, inf);
		if(strlen(buf) < 47)
			return 0;
		if ((ptr = strstr(buf, "�� ԭ���ļ���")) != NULL) {
			ptr += 19;
			memcpy(origin_fname, ptr, FNAMELEN - 1);
			origin_fname[FNAMELEN - 1] = '\0';
			* strchr(origin_fname, ' ') = '\0';
		} else {
			return 0;
		}
	} else {
		return 0;
	}
	
	snprintf(buf, sizeof(buf), "boards/%s/.DIR", origin_bname);
	*origin_ent = search_record_bin(buf, origin_fileinfo, sizeof(*origin_fileinfo), 1, search_record_by_filename, origin_fname);

	if(*origin_ent == 0)
		return 0;
	return 1;
	
}

void
getrecommend(char *filepath, int mode)
{
	FILE *inf, *of;
	char buf[8192];
	char owner[248], *ptr;
	char filename[FNAMELEN];
	int count, owner_found = 0;

	modify_user_mode(POSTING);
	if ((inf = fopen(quote_file, "r")) == NULL) {
		report("getrecommend: cannot open %s for reading", quote_file);
		return;
	}

	if ((of = fopen(filepath, "w")) == NULL) {
		report("getrecommend: cannot open %s for writing", filepath);
		fclose(inf);
		return;
	}

	if (mode == 0) {
		write_header(of, 1 /* ��д�� .posts */, NA);
		if (fgets(buf, 256, inf) != NULL) {
			if ((ptr = strstr(buf, "����: ")) == NULL) {
				owner_found = 0;
				strcpy(owner, "Unkown User");
			} else {
				ptr += 6;
				for (count = 0; ptr[count] != 0 && ptr[count] != ' ' && ptr[count] != '\n'; count++);

				if (count <= 1) {
					owner_found = 0;
					strcpy(owner, "Unkown User");
				} else {
					if (buf[0] == 27) {
						strcpy(owner, "�Զ�����ϵͳ");
					} else {
						strlcpy(owner, ptr, count + 1);
					}
					owner_found = 1;
				}
			}
		}

		strcpy(filename, strrchr(quote_file, '/') + 1);

		fprintf(of, "\033[1;37m�� ���������Ƽ��� \033[32m%s \033[37m������ ��\n", quote_board);
		fprintf(of, "\033[1;37m�� ԭ���ļ��� \033[32m%s \033[37m ��\n", filename);
		fprintf(of, "\033[1;37m�� ���������� \033[32m%s \033[37m�Ƽ� ��\n",currentuser.userid);
	



		if (owner_found) {
			/* skip file header */
			while (fgets(buf, 256, inf) != NULL && buf[0] != '\n');
			fgets(buf, 256, inf);
			if ((strstr(buf, "�� ��������ת���� ") && strstr(buf, "������ ��"))) {
				fgets(buf, 256, inf); 
				if (strstr(buf, "�� ԭ����") && strstr(buf, "������ ��")) {
					fputs(buf, of);
				} else {
					fprintf(of, "�� ԭ����\033[32m %s\033[37m ������ ��\033[m\n\n", owner);
				}
			} else {
				fprintf(of, "�� ԭ����\033[32m %s\033[37m ������ ��\033[m\n\n", owner);
				fputs(buf, of);
			}
		} else {
			fprintf(of, "\033[m");
			fseek(inf, 0, SEEK_SET);
		}
	}
	while ((count = fread(buf, 1, sizeof(buf), inf)) > 0)
		fwrite(buf, 1, count, of);

	if (!mode) {			// �Ƽ���¼
		fprintf(of, "--\n\033[m\033[1;%2dm�� �Ƽ�:.%s %s.[FROM: %s]\033[m\n",
			(currentuser.numlogins % 7) + 31, BoardName, BBSHOST, fromhost);
	}

	fclose(of);
	fclose(inf);
	quote_file[0] = '\0';
}

int post_recommend(struct fileheader *fileinfo, int mode){
	struct fileheader postfile;
	struct boardheader *bp;
	char bdir[STRLEN], filepath[STRLEN];
	char buf[256], whopost[IDLEN + 2];
	if (!haspostperm(currboard) && !mode) {
		outs("\n\n������Ȩ���Ƽ����£�ȡ���Ƽ�\n");
		return -1;
	}

	memset(&postfile, 0, sizeof (postfile));

	if (!mode && strncmp(quote_title, "[�Ƽ�]", 6)) {
		snprintf(save_title, sizeof(save_title), "[�Ƽ�] %s", quote_title);
	} else {
		strlcpy(save_title, quote_title, sizeof(save_title));
	}

	snprintf(bdir, sizeof(bdir), "boards/%s", currboard);
	if ((getfilename(bdir, filepath, GFN_FILE | GFN_UPDATEID, &postfile.id)) == -1)
		return -1;
	strcpy(postfile.filename, strrchr(filepath, '/') + 1);

	if (mode == 1) {
		strcpy(whopost, BBSID);
		postfile.flag |= FILE_MARKED;
	} else {
		FILE* inf;
		char *ptr;
		int owner_found = 0, count = 0;
		if ((inf = fopen(quote_file, "r")) == NULL) {
			report("getrecommend: cannot open %s for reading", quote_file);
		} else {
			if (fgets(buf, 256, inf) != NULL) {
				if ((ptr = strstr(buf, "����: ")) != NULL) {
					ptr += 6;
					for (count = 0; ptr[count] != 0 && ptr[count] != ' ' && ptr[count] != '\n'; count++);
					if (count > 1) 
						owner_found = 1;
				}
			}
		}
		if (owner_found) { /* ��ת�����Ƽ�ʱ��ȡԭ���� */
			/* skip file header */
			while (fgets(buf, 256, inf) != NULL && buf[0] != '\n');
			fgets(buf, 256, inf);
			if ((strstr(buf, "�� ��������ת���� ") && strstr(buf, "������ ��"))) {
				fgets(buf, 256, inf); 
				if ((ptr = strstr(buf, "�� ԭ����")) && strstr(buf, "������ ��")) {
					ptr = ptr + 15;
					for (count = 0; ptr[count] != '\033'; count++);
					strlcpy(whopost, ptr, count + 1);
				} else strcpy(whopost, fileinfo->owner);
			} else strcpy(whopost, fileinfo->owner);
		} else strcpy(whopost, fileinfo->owner);
	}

	strlcpy(postfile.owner, whopost, sizeof(postfile.owner));
	strlcpy(postfile.title, save_title, sizeof(postfile.title));
//      setboardfile(filepath, currboard, postfile.filename);

	bp = getbcache(currboard);
	local_article = YEA;

	getrecommend(filepath, mode);

	postfile.flag &= ~FILE_OUTPOST;

	setbdir(buf, currboard);
	postfile.filetime = time(NULL);
	if (append_record(buf, &postfile, sizeof(postfile)) == -1) {
		unlink(postfile.filename);      // monster: remove file on failure
		if (!mode) {
			report("recommend_posting '%s' on '%s': append_record failed!",
				postfile.title, quote_board);
		} else {
			report("Recommend '%s' on '%s': append_record failed!",
				postfile.title, quote_board);
		}
		pressreturn();
		clear();
		return -1;
	}

	if (!mode) {
		report("recommend_posted '%s' on '%s'", postfile.title,
			currboard);
	}

	update_lastpost(currboard);
	update_total_today(currboard);	
	return 1;
}


int do_recommend(int ent, struct fileheader *fileinfo, char *direct)
{
	char dbname[STRLEN];
	char isrecommend[10];	
	int old_digestmode;
	int origin_ent;
	char origin_bname[STRLEN];
	char origin_fname[FNAMELEN];
	struct fileheader origin_fileinfo;
	struct boardheader *bp, *recommend_bp;

	if (digestmode > 1)
		return DONOTHING;
	old_digestmode = digestmode;
	digestmode = 0;

	set_safe_record();

#ifdef AUTHHOST
	if ( !HAS_ORGPERM(PERM_WELCOME) && !HAS_ORGPERM(PERM_SYSOP) &&
             !HAS_ORGPERM(PERM_OBOARDS) && !HAS_ORGPERM(PERM_ACBOARD) &&
             !HAS_ORGPERM(PERM_ACHATROOM) )
		return DONOTHING;
#endif

	/* ��鵱ǰ�û�ģʽ */
	if (uinfo.mode != RMAIL) {
		snprintf(genbuf, sizeof(genbuf), "boards/%s/%s", currboard, fileinfo->filename);
	} else {
		move(7, 0);
		outs("ֻ���Ƽ������ϵ�����");
		pressreturn();
		return FULLUPDATE;
	}
	strlcpy(quote_file, genbuf, sizeof(quote_file));
	strlcpy(quote_title, fileinfo->title, sizeof(quote_title));

	if ((recommend_bp = getbcache(DEFAULTRECOMMENDBOARD)) == NULL)
	    return FULLUPDATE;

        if (strcmp(currboard, DEFAULTRECOMMENDBOARD) != 0 && !current_bm && 
	    !check_bm(currentuser.userid, recommend_bp->BM)) {
		clear();
                move(7,22);
	        outs("�Բ�����û��Ȩ�޽����Ƽ���");
	        pressreturn();
	        return FULLUPDATE;
        }
    

	if (!strcmp(currboard, DEFAULTRECOMMENDBOARD)) {
        	if(strncmp(quote_title, "[�Ƽ�]", 6)){
			return DONOTHING;
		}
		if(get_origin_info(fileinfo, &origin_ent, 
							origin_bname, origin_fname, &origin_fileinfo) == 0){
			presskeyfor("�Բ���ԭ���Ѷ�ʧ��û��ԭ�ġ�\n");
			return FULLUPDATE;
		}

		if ((bp = getbcache(origin_bname)) == NULL)
			return FULLUPDATE;
		if (bp->flag & (BRD_RESTRICT | BRD_NOPOSTVOTE)) /* ��ֹ���������ܿ����İ���  */
			return FULLUPDATE;

		return do_select2(ent, fileinfo, direct, origin_bname, origin_ent);
	}

	clear();
        outs("\n\n");
	outs("\033[1;35m          ��ӭʹ����ҳ�Ƽ����ܣ�\033[m\n\n");
	outs("\033[1;37m    ���Ĳ������������Ƽ���\033[m\033[1;33mRecommend\033[m\033[1;37m��\n\n");
	outs("    ��\033[m\033[1;33mRecommend\033[m�汻������ժ�������½���ʾ��web��ҳ��\n\n");
	outs("    ����\033[m\033[1;31m����\033[m\033[1;37m�Ƽ�Ȩ�����Ұ�������Ľ������ȡ���Ƽ�Ȩ��\n\n");
	outs("    �Ƿ������\033[m\n\n");

	if ((bp = getbcache(currboard)) == NULL)
		return DONOTHING;
	if (bp->flag & (BRD_RESTRICT | BRD_NOPOSTVOTE)){
	        clear();
		move(12,21);
		outs("�Բ��𣬲����Ƽ����ư�������\n");
		pressreturn();
		return FULLUPDATE;
	}
        if (bp->flag & BRD_INTERN) {
	        clear();
	        move(12,22);
	        outs("�Բ��𣬲����Ƽ��������ΰ�������\n");
	        pressreturn();
	        return FULLUPDATE;
        }

	if (!check_max_post(DEFAULTRECOMMENDBOARD))
		return FULLUPDATE;

	#ifdef FILTER
	if (!has_filter_inited()) {
		init_filter();
	}
	if (!regex_strstr(currentuser.username))
	{
		move(12,0); 
		outs("�Բ�������ǳ��к��в����ʵ����ݣ������Ƽ������Ƚ����޸�\n");
		pressreturn();
		return FULLUPDATE;
	}
	#endif

	move(12, 0);
	if (fileinfo->flag & FILE_RECOMMENDED) 
		prints("\033[1;32m  �����ѱ��Ƽ��� %s ��, �Ƿ�����Ƽ� \033[m",  DEFAULTRECOMMENDBOARD);
	else
		prints("\033[1m    �Ƽ� ' %s ' �� %s �� \033[m",  quote_title, DEFAULTRECOMMENDBOARD);
	getdata(14, 0, "\033[1m    (S)ȷ��  (A)ȡ��? [S]\033[m: ", isrecommend, 9, DOECHO, YEA);
	if (isrecommend[0] == 'a' || isrecommend[0] == 'A') {
		outs("ȡ��");
	} else {
		strlcpy(quote_board, currboard, sizeof(quote_board));
		strlcpy(dbname, currboard, sizeof(dbname));
		strlcpy(currboard, DEFAULTRECOMMENDBOARD, sizeof(currboard));
		if (post_recommend(fileinfo, 0) == -1) {
			pressreturn();
			strlcpy(currboard, dbname, sizeof(currboard));
			return FULLUPDATE;
		}
		strlcpy(currboard, dbname, sizeof(currboard));
		fileinfo->flag |= FILE_RECOMMENDED;
		safe_substitute_record(direct, fileinfo, ent, (digestmode == 2) ? NA : YEA );
		prints("\n�Ѱ����� \'%s\' �Ƽ��� %s ��\n", quote_title, DEFAULTRECOMMENDBOARD);
	}

	digestmode = old_digestmode;
	pressreturn();
	return FULLUPDATE;
}
#endif

/* Add by SmallPig */
int
do_cross(int ent, struct fileheader *fileinfo, char *direct)
{
	char bname[STRLEN];
	char dbname[STRLEN];
	char ispost[10];
	int old_digestmode;

#ifdef BM_CROSSPOST
	if (uinfo.mode != RMAIL) {
		if (!HAS_PERM(PERM_BOARDS) && strcmp(currboard, "Post"))
#ifdef INTERNET_EMAIL
			return forward_post(ent, fileinfo, direct);
#else
			return DONOTHING;
#endif
	}
#endif

	if (digestmode > 7)
		return DONOTHING;
	old_digestmode = digestmode;
	digestmode = 0;

	set_safe_record();

#ifdef AUTHHOST
	if ( !HAS_ORGPERM(PERM_WELCOME) && !HAS_ORGPERM(PERM_SYSOP) &&
             !HAS_ORGPERM(PERM_OBOARDS) && !HAS_ORGPERM(PERM_ACBOARD) &&
             !HAS_ORGPERM(PERM_ACHATROOM) )
		return DONOTHING;
#endif

	if (uinfo.mode != RMAIL) {
		snprintf(genbuf, sizeof(genbuf), "boards/%s/%s", currboard, fileinfo->filename);
	} else {
		snprintf(genbuf, sizeof(genbuf), "mail/%c/%s/%s",
			mytoupper(currentuser.userid[0]), currentuser.userid,
			fileinfo->filename);
	}
	strlcpy(quote_file, genbuf, sizeof(quote_file));
	strlcpy(quote_title, fileinfo->title, sizeof(quote_title));

	clear();

	outs("����ϧ��Դ��ͬ�����ݵ�������������������ת�أ�\033[1;31m4\033[mƪ�����ϡ�\n");
	outs("��ȷ����Ҫ���뵽\033[1;33msuggest\033[m�淢�����룬��ͬ������һ�Ķ෢��\n");
	outs("\033[1;32mΥ�߽��ܵ����ȫվ����Ȩ�Ĵ�������������߽��ᱻɾ���ʺš�\033[m\n");
	outs("��ϸ�涨��������վ�档 лл����!\n\n");

	if (!get_a_boardname(bname, "�����ȷ��Ҫת�صĻ���������Ҫת��������������(ȡ��ת���밴�س�): "))
		return FULLUPDATE;
	
	if (YEA == check_readonly(bname)) {
		move(7, 0);
		outs("�Բ���������ת�����µ�ֻ�������ϡ�");
		pressreturn();
		return FULLUPDATE;
	}
	
	if (!strcmp(bname, DEFAULTRECOMMENDBOARD)) {
		move(7, 0);
		outs("�Բ����㲻��ת�����µ��Ƽ����档");
		pressreturn();
		return FULLUPDATE;
	}

	if (!strcmp(bname, currboard) && uinfo.mode != RMAIL) {
		move(7, 0);
		outs("�Բ��𣬱��ľ�����Ҫת�صİ����ϣ���������ת�ء�");
		pressreturn();
		return FULLUPDATE;
	}

	if (!check_max_post(bname))
		return FULLUPDATE;

	#ifdef FILTER
	if (!has_filter_inited()) {
		init_filter();
	}
	if (!regex_strstr(currentuser.username))
	{
		move(7,0);
		outs("�Բ�������ǳ��к��в����ʵ����ݣ����ܷ��ģ����Ƚ����޸�\n");
		pressreturn();
		return FULLUPDATE;
	}
	#endif

	move(7, 0);
	prints("ת�� ' %s ' �� %s �� ", quote_title, bname);
	getdata(9, 0, "(S)ת�� (L)��վ (A)ȡ��? [L]: ", ispost, 9, DOECHO, YEA);
	if (ispost[0] == 'a' || ispost[0] == 'A') {
		outs("ȡ��");
	} else {
		quote_fh = fileinfo;
		strlcpy(quote_board, currboard, sizeof(quote_board));
		strlcpy(dbname, currboard, sizeof(dbname));
		strlcpy(currboard, bname, sizeof(currboard));
		if (ispost[0] != 's' && ispost[0] != 'S')
			ispost[0] = 'L';
		if (post_cross(ispost[0], 0) == -1) {
			pressreturn();
			strlcpy(currboard, dbname, sizeof(currboard));
			return FULLUPDATE;
		}
		strlcpy(currboard, dbname, sizeof(currboard));
		prints("\n�Ѱ����� \'%s\' ת���� %s ��\n", quote_title, bname);
        
	}
	digestmode = old_digestmode;
	pressreturn();
	return FULLUPDATE;
}

/* show the first 3 lines */
void
readtitle(void)
{
	struct boardheader *bp;
	int i, j, bnum, tuid;
	struct user_info uin;
	char *currBM;
	char tmp[40], bmlists[3][IDLEN + 2];
	char header[STRLEN], title[STRLEN];
	char readmode[11];

	bp = getbcache(currboard);
	current_bm = (HAS_PERM(PERM_BOARDS | PERM_PERSONAL) &&
		      check_bm(currentuser.userid, bp->BM)) ||
		      HAS_PERM(PERM_BLEVELS);

	currBM = bp->BM;
	for (i = 0, j = 0, bnum = 0; currBM[i] != '\0' && bnum < 3; i++) {
		if (currBM[i] == ' ') {
			bmlists[bnum][j] = '\0';
			bnum++;
			j = 0;
		} else {
			bmlists[bnum][j++] = currBM[i];
		}
	}
	bmlists[bnum][j] = '\0';
	if (currBM[0] == '\0' || currBM[0] == ' ') {
		strcpy(header, "����������");
	} else {
		strcpy(header, "����: ");
		for (i = 0; i <= bnum; i++) {
			tmp[0] = '\0';
			tuid = getuser(bmlists[i], NULL);
			search_ulist(&uin, t_cmpuids, tuid);			
			if (uin.active && uin.pid && !uin.invisible) {
				snprintf(tmp, sizeof(tmp), "\033[32m%s\033[33m ", bmlists[i]); /* green bm */
			} else if (uin.active && uin.pid && uin.invisible && (HAS_PERM(PERM_SEECLOAK) || usernum == uin.uid)) {
				snprintf(tmp, sizeof(tmp), "\033[36m%s\033[33m ", bmlists[i]); /* blue bm */
			} else {
				snprintf(tmp, sizeof(tmp), "%s ", bmlists[i]);
			}
			strlcat(header, tmp, sizeof(header));
		}
	}

	if (chkmail(NA)) {
		strcpy(title, "[�����ż����� M ������]");
	} else if ((bp->flag & VOTE_FLAG)) {
		snprintf(title, sizeof(title), "��ͶƱ��,�� v ����ͶƱ��");
	} else {
		strcpy(title, bp->title + 8);
	}

	showtitle(header, title);
	if (digestmode == 8 || digestmode == 9) {
		prints("�뿪[\033[1;32m��\033[m,\033[1;32mq\033[m] ѡ��[\033[1;32m��\033[m,\033[1;32m��\033[m] �Ķ�[\033[1;32m��\033[m,\033[1;32mRtn\033[m] �ָ�����[\033[1;32my\033[m] ���[\033[1;32mE\033[m] ����¼[\033[1;32mTAB\033[m] ����[\033[1;32mh\033[m]     \n");
	} else if (digestmode == 10 || digestmode == 11) {
		prints("�뿪[\033[1;32m��\033[m,\033[1;32mq\033[m] ѡ��[\033[1;32m��\033[m,\033[1;32m��\033[m] �Ķ�[\033[1;32m��\033[m,\033[1;32mRtn\033[m] ���[\033[1;32ma\033[m] �޸ķ��[\033[1;32mE\033[m] ������[\033[1;32md\033[m] ����[\033[1;32mh\033[m] \n");
	} else {
		prints("�뿪[\033[1;32m��\033[m,\033[1;32mq\033[m] ѡ��[\033[1;32m��\033[m,\033[1;32m��\033[m] �Ķ�[\033[1;32m��\033[m,\033[1;32mRtn\033[m] ��������[\033[1;32mCtrl-P\033[m] ����[\033[1;32md\033[m] ����¼[\033[1;32mTAB\033[m] ����[\033[1;32mh\033[m]\n");
	}

	if (digestmode == 0) {
		if (DEFINE(DEF_THESIS)) { /* youzi 1997.7.8 */
			strcpy(readmode, "����");
		} else {
			strcpy(readmode, "һ��");
		}
	} else if (digestmode == 1) {
		strcpy(readmode, "��ժ");
	} else if (digestmode == 2) {
		strcpy(readmode, "����");
	} else if (digestmode == 3) {
		strcpy(readmode, "MARK");
	} else if (digestmode == 4) {
		strcpy(readmode, "ԭ��");
	} else if (digestmode == 7) {
		strcpy(readmode, "����ؼ���");
	}


#ifdef INBOARDCOUNT
	int idx = getbnum(currboard);
	int inboard = board_setcurrentuser(idx-1, 0);

	if (DEFINE(DEF_THESIS) && digestmode == 0) {
		prints("\033[1;37;44m ���   %-12s %6s %-18s %14s%4d [%4sʽ����] \033[m\n",
		       "�� �� ��", "��  ��", " ��  ��", "����:", inboard, readmode);
	} else if (digestmode == 5 || digestmode == 6) { /* ģ�� or ��ȷͬ�����Ķ� */
		prints("\033[1;37;44m ���   %-12s %6s %-10s (�ؼ���: \033[32m%-12s\033[37m) [\033[33m%s\033[37mͬ�����Ķ�] \033[m\n", "�� �� ��", "��  ��", " ��  ��", someoneID, (digestmode == 5) ? "ģ��" : "��ȷ");
	} else if (digestmode == 7) {
		prints("\033[1;37;44m ���   %-12s %6s %-18s %9s%4d [%10sʽ����]\033[m\n",
		       "�� �� ��", "��  ��", " ��  ��", "����:", inboard, readmode);
	} else if (digestmode == 8) {
		prints("\033[1;37;44m ���   %-12s %6s %-42s[����վ] \033[m\n",
		       "�� �� ��", "��  ��", " ��  ��");
	} else if (digestmode == 9) {
		prints("\033[1;37;44m ���   %-12s %6s %-42s[��ֽ¨] \033[m\n",
		       "�� �� ��", "��  ��", " ��  ��");
	} else if (digestmode == 10) {
		prints("\033[1;37;44m ���   %-12s %6s %-40s[����б�] \033[m\n",
		       "ִ �� ��", "��  ��", " �� �� ԭ ��");
	} else if (digestmode == 11) {
		prints("\033[1;37;44m ���   %-12s %6s %-40s[����б�] \033[m\n",
		       "�� �� ��", "��  ��", " �� �� ԭ ��");
	} else {
		prints("\033[1;37;44m ���   %-12s %6s %-18s %14s%4d [%4sģʽ]   \033[m\n",
		       "�� �� ��", "��  ��", " ��  ��", "����:", inboard, readmode);
	}
#else


	if (DEFINE(DEF_THESIS) && digestmode == 0) {
		prints("\033[1;37;44m ���   %-12s %6s %-38s[%4sʽ����] \033[m\n",
		       "�� �� ��", "��  ��", " ��  ��", readmode);
	} else if (digestmode == 5 || digestmode == 6) {
		prints("\033[1;37;44m ���   %-12s %6s %-10s (�ؼ���: \033[32m%-12s\033[37m) [\033[33m%s\033[37mͬ�����Ķ�] \033[m\n",
		       "�� �� ��", "��  ��", " ��  ��", someoneID,
		       (digestmode == 5) ? "ģ��" : "��ȷ");
	} else if (digestmode == 7) {
		prints("\033[1;37;44m ���   %-12s %6s %-32s[%10sʽ����] \033[m\n",
		       "�� �� ��", "��  ��", " ��  ��", readmode);
	} else if (digestmode == 8) {
		prints("\033[1;37;44m ���   %-12s %6s %-42s[����վ] \033[m\n",
		       "�� �� ��", "��  ��", " ��  ��");
	} else if (digestmode == 9) {
		prints("\033[1;37;44m ���   %-12s %6s %-42s[��ֽ¨] \033[m\n",
		       "�� �� ��", "��  ��", " ��  ��");
	} else if (digestmode == 10) {
		prints("\033[1;37;44m ���   %-12s %6s %-40s[����б�] \033[m\n",
		       "ִ �� ��", "��  ��", " �� �� ԭ ��");
	} else if (digestmode == 11) {
		prints("\033[1;37;44m ���   %-12s %6s %-40s[����б�] \033[m\n",
		       "�� �� ��", "��  ��", " �� �� ԭ ��");
	} else {
		prints("\033[1;37;44m ���   %-12s %6s %-40s[%4sģʽ] \033[m\n",
		       "�� �� ��", "��  ��", " ��  ��", readmode);
	}
#endif
	clrtobot();
}

/* monster: �����б� (ͬ����ģʽ�� */
char *
readdoent_thread(int num, struct fileheader *ent, char mark2[], char *date, time_t filetime, int type)
{
	static char buf[128];
	const char tchar[3][3] = { "��", "��", "��" };
	char *title = NULL;

#ifdef COLOR_POST_DATE
	struct tm *mytm;
	char color[8] = "\033[1;30m";
#endif

#ifdef COLOR_POST_DATE
	mytm = localtime(&filetime);
	color[5] = mytm->tm_wday + 49;

	if (ent->title[0] == '\0') {
		snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s�� %-.45s\033[m ",
			num, type, BBSID, color, date, mark2, "< ������ȱʧ����ɾ�� >");

		return buf;
	}

	if (FFLL == 0) {
		if (!strncmp("Re: ", ent->title, 4)) {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s%s %-.46s\033[m ",
				num, type, ent->owner, color, date, mark2,
				tchar[(int)ent->reserved[0]], ent->title + 4);
		} else {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s%s %-.43s\033[m ",
				num, type, ent->owner, color, date, mark2,
				tchar[(int)ent->reserved[0]], ent->title);
		}
	} else {
		if (ReadID == ent->id) {
			if (ent->reserved[0] == THREAD_BEGIN) {
				snprintf(buf, sizeof(buf), " \033[1;32m%4d\033[m %c %-12.12s %s%6.6s.%s\033[1;32m%s %-.44s\033[m ",
					num, type, ent->owner, color, date, mark2,
					tchar[(int)ent->reserved[0]], ent->title);
			} else {
				if (!strncmp(ent->title, "Re: ", 4)) {
					title = ent->title + 4;
				} else {
					title = ent->title;
				}

				snprintf(buf, sizeof(buf), " \033[1;36m%4d\033[m %c %-12.12s %s%6.6s.%s\033[1;36m%s %-.44s\033[m ",
					num, type, ent->owner, color, date, mark2,
					tchar[(int)ent->reserved[0]], title);
			}
		} else {
			if (strncmp(ent->title, "Re: ", 4) || ent->reserved[0] == THREAD_BEGIN) {
				title = ent->title;
			} else {
				title = ent->title + 4;
			}

			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s%s %-.44s ",
				num, type, ent->owner, color, date, mark2,
				tchar[(int)ent->reserved[0]], title);
		}
	}
#else
	if (ent->title[0] == '\0') {
		snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s�� %-.45s\033[m ",
			num, type, BBSID, date, mark2, "< ������ȱʧ����ɾ�� >");

		return buf;
	}

	if (FFLL == 0) {
		if (!strncmp("Re: ", ent->title, 4)) {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s%s %-.46s\033[m ",
				num, type, ent->owner, date, mark2, tchar[(int)ent->reserved[0]], ent->title + 4);
		} else {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s%s %-.43s\033[m ",
				num, type, ent->owner, date, mark2, tchar[(int)ent->reserved[0]], ent->title);
		}
	} else {
		if (ReadID == ent->id) {
			if (ent->reserved[0] == THREAD_BEGIN) {
				snprintf(buf, sizeof(buf), " \033[1;32m%4d\033[m %c %-12.12s %6.6s.%s\033[1;32m%s %-.44s\033[m ",
					num, type, ent->owner, date, mark2, tchar[(int)ent->reserved[0]],
					ent->title);
			} else {
				if (!strncmp(ent->title, "Re: ", 4)) {
					title = ent->title + 4;
				} else {
					title = ent->title;
				}

				snprintf(buf, sizeof(buf), " \033[1;36m%4d\033[m %c %-12.12s %6.6s.%s\033[1;36m%s %-.44s\033[m ",
					num, type, ent->owner, date, mark2, tchar[(int)ent->reserved[0]], title);
			}
		} else {
			if (strncmp(ent->title, "Re: ", 4) || ent->reserved[0] == THREAD_BEGIN) {
				title = ent->title;
			} else {
				title = ent->title + 4;
			}

			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s%s %-.44s ",
				num, type, ent->owner, date, mark2, tchar[(int)ent->reserved[0]], title);
		}
	}
#endif

	return buf;
}

char *
readdoent(int num, void *ent_ptr)             // �����б�
{
	static char buf[128], mark2[16];
	struct boardheader *bp;
	char *date, *owner;
	int type;
	struct fileheader *ent = (struct fileheader *)ent_ptr;

#ifdef COLOR_POST_DATE
	struct tm *mytm;
	char color[8] = "\033[1;30m";
#endif

	type = brc_unread(ent->filetime) ? 'N' : ' ';
	if ((ent->flag & FILE_SELECTED) && (current_bm || HAS_PERM(PERM_ANNOUNCE))) {
		type = '$';
		goto skip;
	}
	if ((ent->flag & FILE_DIGEST) /* && HAS_PERM(PERM_MARKPOST) */ ) {
		if (type == ' ')
			type = 'g';
		else
			type = 'G';
	}
	if (ent->flag & FILE_MARKED) {
		switch (type) {
		case ' ':
			type = 'm';
			break;
		case 'N':
			type = 'M';
			break;
		case 'g':
			type = 'b';
			break;
		case 'G':
			type = 'B';
			break;
		}
	}

	/* monster: disable x-mark display
	 *  if(ent->flag & FILE_DELETED  && current_bm)
	 *          type = (brc_unread(ent->filename) ? 'X' ? 'x';
	 */

      skip:
	date = ctime(&ent->filetime) + 4;

	/* monster: ���ں����ǰ�ı������Ȩ��@ (��������x �����ɻظ��� */
	if (ent->flag & FILE_ATTACHED) {
		strcpy(mark2, "\033[1;33m@\033[m");
	} else {
		bp = getbcache(currboard);

		if (ent->flag & FILE_NOREPLY || bp->flag & NOREPLY_FLAG) {
			strcpy(mark2, "\033[0;1;4;33mx\033[m");
		} else {
			mark2[0] = ' ';
			mark2[1] = 0;
		}
	}

	if (digestmode == 2) { /* ����ģʽ */
		return readdoent_thread(num, ent, mark2, date, ent->filetime, type);
	}

	/* monster: here 'owner' means 'blacklist', 'realowner' means 'executive' when in denylist */
	owner = (digestmode != 11) ? ent->owner : ent->realowner;

#ifdef COLOR_POST_DATE
	mytm = localtime(&ent->filetime);
	color[5] = mytm->tm_wday + 49;

	if (ent->title[0] == '\0') {
		snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s�� %-.45s\033[m ",
			num, type, BBSID, color, date, mark2, "< ������ȱʧ����ɾ�� >");

		return buf;
	}

	if (FFLL == 0) {
		if (!strncmp("Re: ", ent->title, 4)) {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s%-.48s\033[m ",
				num, type, owner, color, date, mark2, ent->title);
		} else {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s�� %-.45s\033[m ",
				num, type, owner, color, date, mark2, ent->title);
		}
	} else {
		if (!strncmp("Re: ", ent->title, 4)) {
			if (ReadID == ent->id) { /* ��ǰ�ڶ����� */
				snprintf(buf, sizeof(buf), " \033[1;36m%4d\033[m %c %-12.12s %s%6.6s.%s\033[1;36m%-.48s\033[m ",
					num, type, owner, color, date, mark2, ent->title);
			} else {
				snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s%-.48s ",
					num, type, owner, color, date, mark2, ent->title);
			}
		} else {
			if (ReadID == ent->id) {
				snprintf(buf, sizeof(buf), " \033[1;32m%4d\033[m %c %-12.12s %s%6.6s.%s\033[1;32m�� %-.45s\033[m ",
					num, type, owner, color, date, mark2, ent->title);
			} else {
				snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s�� %-.45s\033[m ",
					num, type, owner, color, date, mark2, ent->title);
			}
		}
	}
#else
	if (ent->title[0] == '\0') {
		snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s�� %-.45s\033[m ",
			num, type, BBSID, date, mark2, "< ������ȱʧ����ɾ�� >");

		return buf;
	}

	if (FFLL == 0) {
		if (!strncmp("Re: ", ent->title, 4)) {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s%-.48s\033[m ",
				num, type, owner, date, mark2, ent->title);
		} else {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s�� %-.45s\033[m ",
				num, type, owner, date, mark2, ent->title);
		}
	} else {
		if (!strncmp("Re: ", ent->title, 4)) {
			if (ReadID == ent->id) {
				snprintf(buf, sizeof(buf), " \033[1;36m%4d\033[m %c %-12.12s %6.6s.%s\033[1;36m%-.48s\033[m ",
					num, type, owner, date, mark2, ent->title);
			} else {
				snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s%-.48s ",
					num, type, owner, date, mark2, ent->title);
			}
		} else {
			if (ReadID == ent->id) {
				snprintf(buf, sizeof(buf), " \033[1;32m%4d\033[m %c %-12.12s %6.6s.%s\033[1;32m�� %-.45s\033[m ",
					num, type, owner, date, mark2, ent->title);
			} else {
				snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s�� %-.45s\033[m ",
					num, type, owner, date, mark2, ent->title);
			}
		}
	}
#endif

	return buf;
}

int
cmpfilename(struct fileheader *fhdr, char *filename)
{
	return (!strncmp(fhdr->filename, filename, sizeof(fhdr->filename))) ? YEA : NA;
}

int
cmpfilename2(void *filename, void *fhdr_ptr)
{
	struct fileheader *fhdr = (struct fileheader *)fhdr_ptr;

	return (!strcmp(fhdr->filename, (char *)filename)) ? YEA : NA;
}

/* freestyler:  ��ȡ������Ϣ��ȫ�ֱ��� attach_info, attach_link
 * return 0 ���û���� */
int
getattachinfo(struct fileheader *fileinfo)
{
	struct boardheader* bh = getbcache(quote_board);
	if (bh && (bh->flag & BRD_ATTACH))  { /* ������ϴ����� */
		if (fileinfo->flag & FILE_ATTACHED) { /* �и��� */
			struct attacheader ah;
			char	afname[BFNAMELEN];
			strcpy(afname, fileinfo->filename);
			afname[0] = 'A';
			if (getattach(quote_board, afname, &ah)) {
				char 	buf[40];
				struct  stat st;
				snprintf(buf, sizeof(buf), "attach/%s/%s", quote_board, ah.filename);
				if (lstat(buf, &st) == -1)  return 0;
				snprintf(attach_info, sizeof(attach_info),
					"����: %s (%d KB)", ah.origname, (int)st.st_size/1024);
				
				snprintf(attach_link, sizeof(attach_link), 
					 "http://%s/attach/%s/%d.%s", BBSHOST, quote_board, atoi(ah.filename + 2), ah.filetype );
				return 1;
			}
		}
	}
	return 0;
}

int
read_post(int ent, struct fileheader *fileinfo, char *direct)
{
	char *t;
	char buf[512], article_link[1024];
	int ch, result;
	struct fileheader header;

	clear();
	brc_addlist(fileinfo->filetime);
	strcpy(buf, direct); /*  buf is like "boards/Joke/.DIR" */
	if ((t = strrchr(buf, '/')) != NULL)
		*t = '\0';
	snprintf(genbuf, sizeof(genbuf), "%s/%s", buf, fileinfo->filename);
	if (!dashf(genbuf)) {
		clear();
		move(10, 30);
		prints("�Բ��𣬱������ݶ�ʧ��");
		pressanykey();
		return FULLUPDATE;      //deardragon 0729
	}
	strlcpy(quote_file, genbuf, sizeof(quote_file));
	strcpy(quote_board, currboard);
	strcpy(quote_title, fileinfo->title);
	strcpy(quote_user, fileinfo->owner);


	snprintf(article_link, sizeof(article_link), 	/* ȫ������ */
				"http://%s/bbscon?board=%s&file=%s",
				BBSHOST, currboard, fileinfo->filename);

	if (getattachinfo(fileinfo)) {
#ifndef NOREPLY
		ch = ansimore4(genbuf, attach_info, attach_link, article_link, NA);
#else
		ch = ansimore4(genbuf, attach_info, attach_link, article_link, YEA);
#endif
	} else {
#ifndef NOREPLY
		ch = ansimore4(genbuf, NULL, NULL, article_link, NA);
#else
		ch = ansimore4(genbuf, NULL, NULL, article_link, YEA);
#endif
	}

#ifndef NOREPLY
	clear_line(t_lines - 1);
	if (haspostperm(currboard)) {
		prints("\033[1;44;31m[�Ķ�����]  \033[33m���� R �� ���� Q,�� ����һ�� ������һ�� <Space>,���������Ķ� ^X��p \033[m");
	} else {
		prints("\033[1;44;31m[�Ķ�����]  \033[33m���� Q,�� ����һ�� ������һ�� <Space>,<Enter>,���������Ķ� ^X �� p \033[m");
	}

	/* Re-Write By Excellent */

	FFLL = 1;
	ReadID = fileinfo->id;

	refresh();
	if (!(ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_PGUP))
		ch = egetch();

	switch (ch) {
	case 'N':
	case 'Q':
	case 'n':
	case 'q':
	case KEY_LEFT:
		break;
	case ' ':
	case 'j':
	case KEY_RIGHT:
		if (DEFINE(DEF_THESIS)) {       /* youzi */
			sread(NA, NA, ent, fileinfo);
			break;
		} else
			return READ_NEXT;
	case KEY_DOWN:
	case KEY_PGDN:
		return READ_NEXT;
	case KEY_UP:
	case KEY_PGUP:
		return READ_PREV;
	case 'Y':
	case 'R':
	case 'y':
	case 'r':
		{
			struct boardheader *bp;

			bp = getbcache(currboard);
			noreply = (fileinfo->flag & FILE_NOREPLY) || (bp->flag & NOREPLY_FLAG);

			if (!noreply || HAS_PERM(PERM_SYSOP) || current_bm || isowner(&currentuser, fileinfo)) {
				local_article = (fileinfo->flag & FILE_OUTPOST) ? NA : YEA;
				postandmail = (fileinfo->flag & FILE_MAIL) ? 1 : 0;
				do_reply(fileinfo->title, fileinfo->owner, fileinfo->id);
			} else {
				clear();
				prints("\n\n    �Բ���, �������в��� RE ����, �㲻�ܻظ�(RE) ��ƪ����.    ");
				pressreturn();
				clear();
			}
		}
		break;
	case Ctrl('R'):
		post_reply(ent, fileinfo, direct);
		break;
	case 'g':
		digest_post(ent, fileinfo, direct);
		break;
	case Ctrl('U'):
		sread(YEA, YEA, ent, fileinfo);
		break;
	case Ctrl('N'):
		if ((result = locate_article(direct, &header, ent, LOCATE_THREAD | LOCATE_NEW | LOCATE_NEXT, &fileinfo->id)) != -1)
			sread(YEA, NA, ent, &header);
		break;
	case Ctrl('S'):
	case Ctrl('X'):
	case 'p':               /* Add by SmallPig */
		sread(NA, NA, ent, fileinfo);
		break;
	case Ctrl('A'): /* Add by SmallPig */
		clear();
		show_author(0, fileinfo, '\0');
		return READ_NEXT;
		break;
	case 'S':               /* by youzi */
		if (!HAS_PERM(PERM_MESSAGE))
			break;
		clear();
		s_msg();
		break;
	default:
		break;
	}
#endif
	return FULLUPDATE;
}

int
skip_post(int ent, struct fileheader *fileinfo, char *direct)
{
	brc_addlist(fileinfo->filetime);
	return GOTO_NEXT;
}

int
do_select(int ent, struct fileheader *fileinfo, char *direct)
{
	char bname[BFNAMELEN];
	struct boardheader *bp;
	int page, tmp, oldlevel;

	clear_line(0);
	prints("ѡ��һ�������� (Ӣ����ĸ��Сд�Կ�)\n");
	prints("������������ (���հ׼��Զ���Ѱ): ");
	clrtoeol();

	make_blist();
	namecomplete(NULL, bname);
	if (*bname == '\0')
		return FULLUPDATE;

	if ((bp = getbcache(bname)) == NULL)
		goto error;

	if (bp->flag & BRD_GROUP) {
		if ((oldlevel = setboardlevel(bname)) == -1)
			goto error;
#ifdef INBOARDCOUNT
		/* inboard user count  by freestyler */
		int idx = getbnum(currboard);
		board_setcurrentuser(idx-1, -1);
#endif
		brdnum = -1;
		choose_board(YEA);	/* monster: bug here, we did not use original value of newflag here */
		boardlevel = oldlevel;
		brdnum = -1;		/* monster: refresh board list */
		return DOQUIT;
	} else {
		if (digestmode > 1) 
			digestmode = 0;
#ifdef INBOARDCOUNT
		int idx = getbnum(currboard);
		board_setcurrentuser(idx-1, -1);
#endif
		brc_initial(bname);

#ifdef INBOARDCOUNT
		idx = getbnum(currboard);
		board_setcurrentuser(idx-1, 1);
#endif
		setbdir(direct, currboard);
		current_bm = (HAS_PERM(PERM_BOARDS | PERM_PERSONAL) &&
			      check_bm(currentuser.userid, bp->BM)) ||
			      HAS_PERM(PERM_BLEVELS);

		if (DEFINE(DEF_FIRSTNEW)) {
			tmp = unread_position(direct, bp);
			page = tmp - t_lines / 2;
			getkeep(direct, page > 1 ? page : 1, tmp + 1);
		}

		return NEWDIRECT;
	}

error:
	clear_line(2);
	outs("����ȷ��������.");
	pressreturn();
	return FULLUPDATE;
}

/* Add by betterman 07/06/07 */
int
do_select2(int ent, struct fileheader *fileinfo, char *direct, char *bname, int newent)
{
	struct boardheader *bp;
	int page, tmp, oldlevel;

	if (*bname == '\0')
		return FULLUPDATE;

	if ((bp = getbcache(bname)) == NULL)
		goto error;

	if (bp->flag & BRD_GROUP) {
		if ((oldlevel = setboardlevel(bname)) == -1)
			goto error;
#ifdef INBOARDCOUNT
		/* inboard user count  by freestyler */
		int idx = getbnum(currboard);
		board_setcurrentuser(idx-1, -1);
#endif
		brdnum = -1;
		choose_board(YEA);	/* monster: bug here, we did not use original value of newflag here */
		boardlevel = oldlevel;
		brdnum = -1;		/* monster: refresh board list */
		return DOQUIT;
	} else {
		if (digestmode > 1) 
			digestmode = 0;

#ifdef INBOARDCOUNT
		int idx = getbnum(currboard);
		board_setcurrentuser(idx-1, -1);
#endif
		brc_initial(bname);

#ifdef INBOARDCOUNT
		idx = getbnum(currboard);
		board_setcurrentuser(idx-1, 1);
#endif

		setbdir(direct, currboard);
		current_bm = (HAS_PERM(PERM_BOARDS | PERM_PERSONAL) &&
			      check_bm(currentuser.userid, bp->BM)) ||
			      HAS_PERM(PERM_BLEVELS);

		if (DEFINE(DEF_FIRSTNEW)) {
			tmp = unread_position(direct, bp);
			page = tmp - t_lines / 2;
			getkeep(direct, page > 1 ? page : 1, tmp + 1);
		}
		
		sprintf(genbuf, "%d", newent);
		return NEWDIRECT2;
	}

error:
	return FULLUPDATE;
}


/*
int
read_letter(int ent, struct fileheader *fileinfo, char *direct)
{
	setmaildir(direct,currentuser.userid);
	return NEWDIRECT;
}
*/

int
do_acction(int type)
{
	clear_line(t_lines - 1);
	outs("\033[1;5mϵͳ���������, ���Ժ�...\033[m");
	refresh();
	
	switch (type) {
	case 2:
		return make_thread(currboard, NA);
	case 3:		/* marked */
	case 4:		/* ԭ�� */
	case 5:         /* ͬ���� */
	case 6:         /* ͬ����  ��ȷ */
	case 7:         /* ����ؼ��� */
		return marked_all(type - 3);
	}

	return 0;
}

int
acction_mode(int ent, struct fileheader *fileinfo, char *direct)
{
	int type;
	char ch[4] = { '\0' };

	if (digestmode != NA) {
		if (digestmode == 5 || digestmode == 6) {
			snprintf(genbuf, sizeof(genbuf), "boards/%s/SOMEONE.%s.DIR.%d", currboard, someoneID, digestmode - 5);
			unlink(genbuf);
		} else if (digestmode == 7) {
			snprintf(genbuf, sizeof(genbuf), "boards/%s/KEY.%s.DIR", currboard, currentuser.userid);
			unlink(genbuf);
		}
		digestmode = NA;
		setbdir(currdirect, currboard);
	} else {
		saveline(t_lines - 1, 0);
		clear_line(t_lines - 1);
		getdata(t_lines - 1, 0,
			"�л�ģʽ��: 1)��ժ 2)ͬ���� 3)�� m ���� 4)ԭ�� 5)ͬ���� 6)����ؼ��� [2]: ",
			ch, 3, DOECHO, YEA);
		if (ch[0] == '\0')
			ch[0] = '2';
		type = atoi(ch);

		if (type < 1 || type > 6) {
			saveline(t_lines - 1, 1);
			return PARTUPDATE;
		} else if (type == 6) {
			getdata(t_lines - 1, 0, "������ҵ����±���ؼ���: ",
				someoneID, 30, DOECHO, YEA);
			if (someoneID[0] == '\0') {
				saveline(t_lines - 1, 1);
				return PARTUPDATE;
			}
			type = 7;
		} else if (type == 5) {
			strlcpy(someoneID, fileinfo->owner, sizeof(someoneID));
			getdata(t_lines - 1, 0, "���������λ���ѵ�����? ",
				someoneID, 13, DOECHO, NA);
			if (someoneID[0] == '\0') {
				saveline(t_lines - 1, 1);
				return PARTUPDATE;
			}
			getdata(t_lines - 1, 37,
				"��ȷ���Ұ� Y�� ģ��������س�[Enter]", ch, 2,
				DOECHO, YEA);
			if (ch[0] == 'y' || ch[0] == 'Y')
				type = 6;
		}

		if (do_acction(type) != -1) {
			digestmode = type;
			setbdir(currdirect, currboard);
			if (!dashf(currdirect)) {
				digestmode = NA;
				setbdir(currdirect, currboard);
				return PARTUPDATE;
			}
		}
	}
	return NEWDIRECT;
}

int
pure_mode(void)
{
	if (do_acction(4) != -1) {
		digestmode = 4;
		setbdir(currdirect, currboard);
		do_acction(4);

		if (!dashf(currdirect)) {
			digestmode = NA;
			setbdir(currdirect, currboard);
			return PARTUPDATE;
		}
	}
	return NEWDIRECT;
}

int
deny_mode(void)
{
	if (!current_bm)
		return DONOTHING;

	if (digestmode) {
		digestmode = NA;
		setbdir(currdirect, currboard);
	} else {
		struct boardheader *bp;

		bp = getbcache(currboard);  //Added by cancel At 02.05.20
		digestmode = (bp->flag & ANONY_FLAG) ? 10 : 11;
		setbdir(currdirect, currboard);
		if (get_num_records(currdirect, sizeof(struct denyheader)) == 0) {
			denyuser();
			digestmode = NA;
			setbdir(currdirect, currboard);
		}
	}
	return NEWDIRECT;
}

int
digest_mode(void)
{                               /* ��ժģʽ �л� */
	if (digestmode == YEA) {
		digestmode = NA;
		setbdir(currdirect, currboard);
	} else {
		digestmode = YEA;
		setbdir(currdirect, currboard);
		if (!dashf(currdirect)) { /* ����ժ�б��ļ� */
			digestmode = NA;
			setbdir(currdirect, currboard);
			return DONOTHING;
		}
	}
	return NEWDIRECT;
}

int
deleted_mode(void)
{
	if (!current_bm && !HAS_PERM(PERM_JUDGE))
		return DONOTHING;

	if (digestmode == 8) {
		digestmode = NA;
		setbdir(currdirect, currboard);
	} else {
		digestmode = 8;
		setbdir(currdirect, currboard);
		if (!dashf(currdirect)) {
			digestmode = NA;
			setbdir(currdirect, currboard);
			return DONOTHING;
		}
	}
	return NEWDIRECT;
}

int
junk_mode(void)
{

	if (!HAS_PERM(PERM_SYSOP) && !HAS_PERM(PERM_JUDGE))
		return DONOTHING;

	if (digestmode == 9) {
		digestmode = NA;
		setbdir(currdirect, currboard);
	} else {
		digestmode = 9;
		setbdir(currdirect, currboard);
		if (!dashf(currdirect)) {
			digestmode = NA;
			setbdir(currdirect, currboard);
			return DONOTHING;
		}
	}
	return NEWDIRECT;
}

int
dodigest(int ent, struct fileheader *fileinfo, char *direct, int delete, int update)
{
	char ndirect[PATH_MAX + 1]; // normal direct
	char ddirect[PATH_MAX + 1]; // digest direct
	char filename[PATH_MAX + 1], filename2[PATH_MAX + 1];
	char path[PATH_MAX + 1], *ptr;
	struct fileheader header;
	int pos;

	strlcpy(path, direct, sizeof(path));
	if ((ptr = strrchr(path, '/')) == NULL)
		return DONOTHING;
	*ptr = '\0';

	snprintf(ndirect, sizeof(ndirect), "%s/.DIR", path);
	snprintf(ddirect, sizeof(ddirect), "%s/.DIGEST", path);

	if (digestmode == 1) {
		if (!delete)
			return DONOTHING;

		snprintf(filename, sizeof(filename), "%s/%s", path, fileinfo->filename);
		if (delete_record(ddirect, sizeof(struct fileheader), ent) == -1)
			return PARTUPDATE;
		unlink(filename);

		snprintf(filename, sizeof(filename), "M%s", fileinfo->filename + 1);
		if ((pos = search_record(ndirect, &header, sizeof(struct fileheader), cmpfilename2, filename)) > 0) {
			header.flag &= ~FILE_DIGEST;
			safe_substitute_record(ndirect, &header, pos, NA);

		}

		return DIRCHANGED;
	}

	if (delete) {
		snprintf(filename, sizeof(filename), "G%s", fileinfo->filename + 1);
		delete_file(ddirect, 1, filename, YEA);
		fileinfo->flag &= ~FILE_DIGEST;
	} else {
		if (fileinfo->filename[0] != 'G') {
			snprintf(filename, sizeof(filename), "%s/%s", path, fileinfo->filename);
			snprintf(filename2, sizeof(filename2), "%s/G%s", path, fileinfo->filename + 1);
			if (link(filename, filename2) == -1) {
				if (errno != EEXIST) {
					return DONOTHING;
				} else {
					char gfilename[PATH_MAX + 1];

					// monster: ��� g ��ͬ������
					snprintf(gfilename, sizeof(gfilename), "G%s", fileinfo->filename + 1);
					if (search_record(ddirect, &header, sizeof(struct fileheader), 
					    cmpfilename2, gfilename) > 0) {
						fileinfo->flag |= FILE_DIGEST;
						return DONOTHING;
					}
				}
			}
		}

		memcpy(&header, fileinfo, sizeof(header));
		header.flag = fileinfo->flag & FILE_ATTACHED;
		snprintf(header.filename, sizeof(header.filename), "G%s", fileinfo->filename + 1);
		if (append_record(ddirect, &header, sizeof(struct fileheader)) == -1) {
			unlink(filename2);
			return DONOTHING;
		}

		fileinfo->flag |= FILE_DIGEST;
	}

	if (update) {
		safe_substitute_record(ndirect, fileinfo, ent, YEA /* digestmode == 0 */);
		if ( digestmode > 1 )  
			safe_substitute_record(direct, fileinfo, ent, (digestmode == 2) ? NA:YEA );

	}

	return DIRCHANGED;
}

int
digest_post(int ent, struct fileheader *fileinfo, char *direct)
{
	if (digestmode > 7 || !current_bm)
		return DONOTHING;

	dodigest(ent, fileinfo, direct, (fileinfo->flag & FILE_DIGEST), YEA);
	return PARTUPDATE;
}

#ifndef NOREPLY
int
do_reply(char *title, char *replyid, int id)
{
	strcpy(replytitle, title);
	post_article(currboard, replyid, id);
	replytitle[0] = '\0';
	return FULLUPDATE;
}
#endif

int
garbage_line(char *str)
{
	int qlevel = 0;

	while (*str == ':' || *str == '>') {
		str++;
		if (*str == ' ')
			str++;
		if (qlevel++ >= 1)
			return 1;
	}
	while (*str == ' ' || *str == '\t')
		str++;
	if (qlevel >= 1)
		if (strstr(str, "�ᵽ:\n") || strstr(str, ": ��\n") ||
		    strncmp(str, "==>", 3) == 0 || strstr(str, "������ ��"))
			return 1;
	return (*str == '\n');
}

/* this is a ���� for bad people to cover my program to his */
int
Origin2(char *text)
{
	char tmp[STRLEN];

	snprintf(tmp, sizeof(tmp), ":��%s %s��[FROM:", BoardName, BBSHOST);
	return (strstr(text, tmp) != NULL) ? YEA : NA;
}

void
do_quote(char *filepath, char quote_mode)
{
	FILE *inf, *outf;
	char *qfile, *quser;
	char buf[256], *ptr;
	char op;
	int bflag, i;

	qfile = quote_file;
	quser = quote_user;
	bflag = strncmp(qfile, "mail", 4);

	if ((outf = fopen(filepath, "w")) == NULL)
		return;

	if (quote_mode != '\0' && *qfile != '\0' && (inf = fopen(qfile, "r")) != NULL) {
		op = quote_mode;
		if (op != 'N' && fgets(buf, sizeof(buf), inf) != NULL) {
			if ((ptr = strrchr(buf, ')')) != NULL) {
				ptr[1] = '\0';
				if ((ptr = strchr(buf, ':')) != NULL) {
					quser = ptr + 1;
					while (*quser == ' ')
						quser++;
				}
			}

			if (bflag) {
				fprintf(outf, "\n�� �� %-.55s �Ĵ������ᵽ: ��\n", quser);
			} else {
				fprintf(outf, "\n�� �� %-.55s ���������ᵽ: ��\n", quser);
			}

			if (op == 'A') {
				while (fgets(buf, 256, inf) != NULL)
					fprintf(outf, ": %s", buf);
			} else if (op == 'R') {
				while (fgets(buf, 256, inf) != NULL)
					if (buf[0] == '\n')
						break;

				while (fgets(buf, 256, inf) != NULL) {
					if (Origin2(buf))
						continue;
					fputs(buf, outf);
				}
			} else {
				while (fgets(buf, 256, inf) != NULL)
					if (buf[0] == '\n')
						break;

				i = 0;
				while (fgets(buf, 256, inf) != NULL) {
					if (strcmp(buf, "--\n") == 0)
						break;
					if (buf[250] != '\0')
						strcpy(buf + 250, "\n");
					if (!garbage_line(buf)) {
						if (op == 'S' && i >= 10) {
							fprintf(outf, ": .................������ʡ�ԣ�");
							break;
						}
						i++;
						fprintf(outf, ": %s", buf);
					}
				}
			}
		}
		fputc('\n', outf);
		fclose(inf);
	}
	*quote_file = '\0';
	*quote_user = '\0';
	if (currentuser.signature == 0 || header.chk_anony) {
		fputs("\n--", outf);
	} else {
		addsignature(outf, 1);
	}
	fclose(outf);
}

/* Add by SmallPig */
void
getcross(char *filepath, int mode)
{
	FILE *inf, *of;
	char buf[8192];
	char owner[248], *ptr;
	int count, owner_found = 0, inmail = INMAIL(uinfo.mode);

	modify_user_mode(POSTING);
	if ((inf = fopen(quote_file, "r")) == NULL) {
		report("getcross: cannot open %s for reading", quote_file);
		return;
	}

	if ((of = fopen(filepath, "w")) == NULL) {
		report("getcross: cannot open %s for writing", filepath);
		fclose(inf);
		return;
	}
	if (mode == 0) {
		write_header(of, 0 /* д�� .post */, NA); /* freestyler: ת��д�� .post, ʮ���������� */ 
		if (fgets(buf, 256, inf) != NULL) {
			if ((ptr = strstr(buf, "����: ")) == NULL) {
				owner_found = 0;
				strcpy(owner, "Unkown User");
			} else {
				ptr += 6;
				for (count = 0; ptr[count] != 0 && ptr[count] != ' ' && ptr[count] != '\n'; count++);

				if (count <= 1) {
					owner_found = 0;
					strcpy(owner, "Unkown User");
				} else {
					if (buf[0] == 27) {
						strcpy(owner, "�Զ�����ϵͳ");
					} else {
						strlcpy(owner, ptr, count + 1);
					}
					owner_found = 1;
				}
			}
		}

		if (inmail) {
			fprintf(of, "\033[1;37m�� ��������ת���� \033[32m%s \033[37m������ ��\n", currentuser.userid);
		} else {
			fprintf(of, "\033[1;37m�� ��������ת���� \033[32m%s \033[37m������ ��\n", quote_board);
		}

		if (owner_found) {
			/* skip file header */
			while (fgets(buf, 256, inf) != NULL && buf[0] != '\n');

			fgets(buf, 256, inf);
			if ((strstr(buf, "�� ��������ת���� ") && strstr(buf, "������ ��"))) {
				fgets(buf, 256, inf);
				if (strstr(buf, "�� ԭ����") && strstr(buf, "������ ��")) {
					fputs(buf, of);
				} else {
					fprintf(of, "�� ԭ����\033[32m %s\033[37m ������ ��\033[m\n\n", owner);
				}
			} else {
				fprintf(of, "�� ԭ����\033[32m %s\033[37m ������ ��\033[m\n\n", owner);
				fputs(buf, of);
			}
		} else {
			fprintf(of, "\033[m");
			fseek(inf, 0, SEEK_SET);
		}
	} else if (mode == 1) {
		add_sysheader(of, quote_board, quote_title);
	} else if (mode == 2) {
		write_header(of, 0 /* д�� .posts */, NA);
	} else if (mode == 3) {
		/* ������ת�������� */
		write_header(of, 0 /* д�� .posts */, NA);
		fprintf(of, "\033[1;37m�� ��������ת���� \033[32m%s\033[37m ��\n", ainfo.title);
		if (quote_user[0] != '\0') {
			fprintf(of, "�� ԭ����\033[32m %s\033[37m ������ ��\033[m\n\n", quote_user);
			quote_user[0] = '\0';
		}
	}

	while ((count = fread(buf, 1, sizeof(buf), inf)) > 0)
		fwrite(buf, 1, count, of);


	if (!mode) {			// ת�ؼ�¼
		if(getattachinfo(quote_fh)) {	/* freestyler: ����������Ϣ��*/
			fprintf(of, "\033[m\n%s ����:\n", attach_info);
			fprintf(of, "\033[4m%s\033[m\n", attach_link);
		}
		fprintf(of, "--\n\033[m\033[1;%2dm�� ת��:.%s %s.[FROM: %s]\033[m\n",
			(currentuser.numlogins % 7) + 31, BoardName, BBSHOST, fromhost);
	}

	fclose(of);
	fclose(inf);
	quote_file[0] = '\0';
}

int
do_post(void)
{
	struct boardheader *bp;
	bp = getbcache(currboard);
	noreply = bp->flag & NOREPLY_FLAG;
	*quote_file = '\0';
	*quote_user = '\0';
	local_article = YEA;
	return post_article(currboard, NULL, 0);
}

int
post_reply(int ent, struct fileheader *fileinfo, char *direct)
{
	char uid[STRLEN];
	char title[TITLELEN], buf[STRLEN];
	char *t;
	FILE *fp;

	if (guestuser || !HAS_PERM(PERM_LOGINOK))
		return DONOTHING;

	clear();
	if (check_maxmail()) {
		pressreturn();
		return FULLUPDATE;
	}
	modify_user_mode(SMAIL);

	/* indicate the quote file/user */
	setboardfile(quote_file, currboard, fileinfo->filename);
	strlcpy(quote_user, fileinfo->owner, sizeof(quote_user));
	
	/* find the author */
	if (getuser(quote_user, NULL) == 0) {
		genbuf[0] = '\0';
		if ((fp = fopen(quote_file, "r")) != NULL) {
			fgets(genbuf, 255, fp);
			fclose(fp);
		}
		t = strtok(genbuf, ":");
		if (strncmp(t, "������", 6) == 0 ||
		    strncmp(t, "������", 6) == 0 ||
		    strncmp(t, "Posted By", 9) == 0 ||
		    strncmp(t, "��  ��", 6) == 0) {
			while (t != NULL) {
				t = (char *) strtok(NULL, " \r\t\n<>");
				if (t == NULL)
					break;
				if (!invalidaddr(t))
					break;
			}
			if (t != NULL) {
				strlcpy(uid, t, sizeof(uid));
			}
		}
		
		if (t == NULL || (strchr(t, '@') == NULL && getuser(t, NULL) == 0)) {
			prints("�Բ��𣬸��ʺ��Ѿ������ڡ�\n");
			pressreturn();
			return FULLUPDATE;
		}
	} else {
		strlcpy(uid, quote_user, sizeof(uid));
	}

	/* make the title */
	if ((fileinfo->title[0] != 'R' && fileinfo->title[0] != 'r') || fileinfo->title[1] != 'e' || fileinfo->title[2] != ':') {
		snprintf(title, sizeof(title), "Re: %s", fileinfo->title);
	} else {
		strlcpy(title, fileinfo->title, sizeof(title));
	}

	/* edit, then send the mail */
	snprintf(buf, sizeof(buf), "�ż��ѳɹ��ؼĸ�ԭ���� %s\n", uid);
	m_feedback(do_send(uid, title, YEA, 0), uid, buf);
	return FULLUPDATE;
}

/* Add by SmallPig */
int
post_cross(char islocal, int mode)
{
	struct fileheader postfile;
	struct boardheader *bp;
	char bdir[STRLEN], filepath[STRLEN];
	char buf[256], whopost[IDLEN + 2];

	if (!haspostperm(currboard) && !mode) {
		prints("\n\n������Ȩ���� %s �淢�����£�ȡ��ת��\n", currboard);
		return -1;
	}

	memset(&postfile, 0, sizeof (postfile));

	if (!mode && strncmp(quote_title, "[ת��]", 6)) {
		snprintf(save_title, sizeof(save_title), "[ת��] %s", quote_title);
	} else {
		strlcpy(save_title, quote_title, sizeof(save_title));
	}

	snprintf(bdir, sizeof(bdir), "boards/%s", currboard);
	if ((getfilename(bdir, filepath, GFN_FILE | GFN_UPDATEID, &postfile.id)) == -1)
		return -1;
	strcpy(postfile.filename, strrchr(filepath, '/') + 1);

	if (mode == 1) {
		strcpy(whopost, BBSID);
		postfile.flag |= FILE_MARKED;
	} else {
		strcpy(whopost, currentuser.userid);
	}

	strlcpy(postfile.owner, whopost, sizeof(postfile.owner));
	strlcpy(postfile.title, save_title, sizeof(postfile.title));
//      setboardfile(filepath, currboard, postfile.filename);

	bp = getbcache(currboard);
	if ((islocal == 'S' || islocal == 's') && (bp->flag & OUT_FLAG))
		local_article = NA;
	else
		local_article = YEA;

	posts_article_id = postfile.id;
	getcross(filepath, mode);

	if (local_article == YEA || !(bp->flag & OUT_FLAG)) {
		postfile.flag &= ~FILE_OUTPOST;
	} else {
		postfile.flag |= FILE_OUTPOST;
		outgo_post(&postfile, currboard);
	}

	setbdir(buf, currboard);
	postfile.filetime = time(NULL);
	if (append_record(buf, &postfile, sizeof(postfile)) == -1) {
		unlink(postfile.filename);      // monster: remove file on failure
		if (!mode) {
			report("cross_posting '%s' on '%s': append_record failed!",
				postfile.title, quote_board);
		} else {
			report("Posting '%s' on '%s': append_record failed!",
				postfile.title, quote_board);
		}
		pressreturn();
		clear();
		return -1;
	}

	if (!mode) {
		report("cross_posted '%s' on '%s'", postfile.title,
			currboard);
	}

    BBS_SINGAL("/post/cross",
               "f", postfile.filename,
               "b", currboard,
               "h", fromhost,
               NULL);

	update_lastpost(currboard);
	update_total_today(currboard);
	return 1;
}

int
show_board_notes(char *bname, int promptend)
{
	char buf[PATH_MAX + 1];

	snprintf(buf, sizeof(buf), "vote/%s/notes", bname);
	if (dashf(buf)) {
		ansimore2(buf, promptend, 0, (promptend == YEA) ? 0 : t_lines - 5);
		return 1;
	} else if (dashf("vote/notes")) {
		ansimore2("vote/notes", promptend, 0, (promptend == YEA) ? 0 : t_lines - 5);
		return 1;
	}
	return -1;
}

int
show_user_notes(void)
{
	char buf[256];

	setuserfile(buf, "notes");
	if (dashf(buf)) {
		ansimore(buf, YEA);
		return FULLUPDATE;
	}
	clear();
	move(10, 15);
	prints("����δ�� InfoEdit->WriteFile �༭���˱���¼��\n");
	pressanykey();
	return FULLUPDATE;
}

int
outgo_post(struct fileheader *fh, char *board)
{
	char buf[PATH_MAX + 1];

	snprintf(buf, sizeof(buf), "%s\t%s\t%s\t%s\t%s\n", board,
		fh->filename, header.chk_anony ? board : currentuser.userid,
		header.chk_anony ? "����������ʹ" : currentuser.username,
		save_title);

	return file_append("innd/out.bntp", buf);
}

void
punish(void)
{
	char keybuf[3];
	static int count;

	if (count == 3) {
		clear();
		prints("������û�취������������ȥ���ˡ�����");
		refresh();
		sleep(3);
		abort_bbs();
	}

	++count;
	clear();
	outc('\n');
	prints("\033[1;37;40m \033[0;37;40m�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{�{ \033[1m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                     \033[0;37;40m  \033[1m                   \033[47m  \033[0;37;40m \033[1m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[0;37;40m \033[1m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[0;37;47m  \033[40m \033[1m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[0;37;40m \033[1m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[0;37;40m \033[1m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[0;37;40m \033[1m\n");
	prints("\033[1;37;40m \033[0;33;47m  \033[1;37;40m                                                          \033[0;33;47m  \033[1;37;40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[0;33;47m  \033[1;37;40m                                        \033[0;1;37;40m                  \033[0;33;47m  \033[1;37;40m\n");
	prints("\033[1;37;40m \033[0;33;47m  \033[1;37;40m                                  \033[0;37;40m  \033[1m                      \033[0;33;47m  \033[37;40m \033[1m\n");
	prints("\033[1;37;40m \033[0;33;47m                              \033[1;37m    \033[0;33;47m                            \033[1;37;40m\n");
	prints("\033[1;37;40m   \033[34m����������������������������������������������������������\033[37m\n");
	prints("\033[1;37;40m                                                                 \033[0;1m\n");
	prints("\033[m\n");
	refresh();
	sleep(1);
	move(4, 6);
	prints("\033[1;37mϵͳ�������ù�ˮ��������\033[m");
	refresh();
	sleep(1);
	move(6, 6);
	prints("\033[1;37m��˵�����ǲ������˹�ˮ����\033[m");
	sleep(1);
	refresh();
	move(8, 6);
	prints("\033[1;37m�����ϣ���ô������ xx �������� yy ƪ���£�\033[m");
	sleep(1);
	refresh();
	move(10, 6);
	prints("\033[1;37mʲô���ֶ��ģ���ƭ˭��������ô�죿\033[m");
	sleep(3);
	refresh();
	move(12, 6);
	sleep(4);
	refresh();
	prints("\033[1;37mŶ�����û�ù�ˮ�����ǿ�����ϵͳ����ˣ���������^_^\033[m");
	refresh();
	sleep(15);
	move(19, 6);
	prints("\033[1;37;5m                (press return)\033[m");
	move(t_lines - 1, 0);
	getdata(t_lines - 1, 0, "", keybuf, 2, NOECHO, YEA);
	clear();
}

static unsigned  int 
ip_str2int(const char *ip)
{
	/* Assume ip is ipv4 : a.b.c.d */
	int dotcnt = 0, i;
	for (i = 0; ip[i]; i++) {
		if ( ip[i] == '.' ) dotcnt++;
	}
	if ( dotcnt != 3 ) return 0;
	unsigned int ret = 0;
	char buf[256];
	strlcpy(buf, ip, sizeof( buf ));
	char *tk = strtok(buf, ".");
	while ( tk != NULL ) {
		ret = ret * 256 + atoi(tk);
		tk = strtok(NULL, ".");
	}
	return ret;
}

int 
check_outcampus_ip()
{
	if (access(SYSU_IP_LIST, F_OK) == -1) {
		return NA;
	}
	
	FILE *fp = fopen(SYSU_IP_LIST, "r");
	char ip_start[64], ip_end[64];
	unsigned int ip_s, ip_e; 
	unsigned int ip_now = ip_str2int(raw_fromhost);

	if ( fp == NULL ) return NA;
	while ( fscanf( fp, "%s%s", ip_start, ip_end ) != EOF ) {
		ip_s = ip_str2int(ip_start);
		ip_e = ip_str2int(ip_end);
		if ( ip_now >= ip_s && ip_now <= ip_e ) {
			fclose(fp);
			return NA;
		}
	}
	fclose(fp);
	return YEA;
}
int
post_article(char *postboard, char *mailid, unsigned int id)
{
	struct fileheader postfile;
	struct boardheader *bp;
	char bdir[STRLEN], filepath[STRLEN], buf[STRLEN], replyfilename[STRLEN];
	int interval;
	static time_t lastposttime = 0;
	static int postcount = 0;
	
	int bm = (current_bm || HAS_PERM(PERM_SYSOP));

    /* LTaoist : backup the quote_file name for siganl the replyfilename */
    if(*quote_file)
    {
        strcpy(replyfilename, basename(quote_file));
    }
	
	if ( check_board_attr(currboard, BRD_RESTRICT) == NA 
			//&& !bm
			&& YEA == check_outcampus_ip() ) { 
		/* 
		 * Cypress 2012.10.17 ѧУ֪ͨ��ʮ�˴��ڼ�У��ip��ʱȡ���������� 
		 * �༭ SYSU_IP_LIST �ļ�(consts.h)����¼У��ip��
		 * �ָ�����ʱֻ�轫���ļ���������ɾ�����ɡ�
		 */
		clear();
		move(8, 1);
		prints("   �Բ���, �������ڸ��������Ϸ�������, ���ܵ�ԭ������:\n\n");
		prints("       1. ����ʱ�ս���2012��10��25����11��30��֮�����ά�����ڼ佫��ͣУ��IP��������.\n");
		prints("       2. ��Уѧ������У�����ͨ��ѧУVPNͨ�����ӷ���������μ���\n");
	    prints("            a. ��Ϣ�����������̨ http://helpdesk.sysu.edu.cn \n");
	    prints("            b. ��� IT����FAQ\n");
	    prints("            c. ��� VPN��� �鿴���˵��\n");
		prints("       3. �����Ĳ�����վ���Ǽ���!\n");
		pressreturn();
		clear();
		return FULLUPDATE;
	}

	if (YEA == check_readonly(currboard)) { /* Leeward 98.03.28 */
		clear();
		move(8, 1);
		prints("   �Բ���, �������ڸ��������Ϸ�������, ���ܵ�ԭ������:\n\n");
		prints("       1. ����δ��÷������µ�Ȩ��;\n");
		prints("       2. ����������Աȡ���˸��������ķ���Ȩ��;\n");
		prints("       3. ��������ֻ��.\n");
		pressreturn();
		clear();
		return FULLUPDATE;
	}

	if (!strcmp(currboard, DEFAULTRECOMMENDBOARD) && !current_bm) {
		clear();
		move(8, 1);
		prints("                �Բ���, ���������Ƽ������Ϸ������¡�\n\n");
		pressreturn();
		clear();
		return FULLUPDATE;
	}

	if (!check_max_post(currboard))
		return FULLUPDATE;
	#ifdef FILTER
	if (!has_filter_inited()) {
		init_filter();
	}
	if (!regex_strstr(currentuser.username))
	{
		clear();
		move(8,1);
		prints("       �Բ�������ǳ��к��в����ʵ����ݣ����ܷ��ģ����Ƚ����޸�\n");
		pressreturn();
		clear();
		return FULLUPDATE;
	}
	#endif
	if ( digestmode > 7 ) {
		clear();
		move(5, 10);
		prints("Ŀǰ�ǻ���վ�����б�, ���ܷ�������\n          (�� ENTER ���ٰ� LEFT ���ɷ���һ��ģʽ)��");
		pressreturn();
		clear();
		return FULLUPDATE;
	} 

	if (!haspostperm(postboard)) {
		clear();
		move(5, 10);
		prints("����������Ψ����, ����������Ȩ���ڴ˷������¡�");
		pressreturn();
		clear();
		return FULLUPDATE;
	}
	
	modify_user_mode(POSTING);
	interval = abs(time(NULL) - lastposttime);
	if (interval < 80) {
		if (postcount >= 20) {
			report("%s �� %s ��ˮ���ѱ�ϵͳ��ֹ", currentuser.userid, currboard);
			punish();
			postcount = 8;
			return FULLUPDATE;
		} else {
			postcount++;
		}
	} else {
		lastposttime = time(NULL);
		postcount = 0;
	}

	memset(&postfile, 0, sizeof (postfile));
	clear();
	show_board_notes(postboard, NA);
	bp = getbcache(postboard);
	if (bp->flag & OUT_FLAG && replytitle[0] == '\0')
		local_article = NA;
#ifndef NOREPLY
	if (replytitle[0] != '\0') {
		if (strncmp(replytitle, "Re: ", 4) == 0) {
			strlcpy(header.title, replytitle, TITLELEN);
		} else {
			snprintf(header.title, sizeof(header.title), "Re: %s", replytitle);
			header.title[TITLELEN - 1] = '\0';
		}
		header.reply_mode = 1;
	} else
#endif
	{
		header.title[0] = '\0';
		header.reply_mode = 0;

	}
	strcpy(header.ds, postboard);
	header.postboard = YEA;
	if (post_header(&header) == YEA) {
		strlcpy(postfile.title, header.title, sizeof(postfile.title));
		strlcpy(save_title, postfile.title, sizeof(save_title));
	} else {
		return FULLUPDATE;
	}

	snprintf(bdir, sizeof(bdir), "boards/%s", postboard);
	if ((getfilename(bdir, filepath, GFN_FILE | ((mailid == NULL) ? GFN_UPDATEID : 0), &id)) == -1)
		return DONOTHING;
	postfile.id = id;
	strcpy(postfile.filename, strrchr(filepath, '/') + 1);

	strlcpy(postfile.owner, (header.chk_anony) ? postboard : currentuser.userid, sizeof(postfile.owner));
	setboardfile(filepath, postboard, postfile.filename);
	modify_user_mode(POSTING);
	do_quote(filepath, header.include_mode);

	posts_article_id = id;
	if (vedit(filepath, EDIT_SAVEHEADER | EDIT_MODIFYHEADER | EDIT_ADDLOGINFO) == -1) {
		unlink(filepath);
		clear();
		return FULLUPDATE;
	}

	strlcpy(postfile.title, save_title, sizeof(postfile.title));

	/* monster: ����ָ���ļ��� */
	#ifdef RENAME_AFTERPOST
	if ((getfilename(bdir, filepath, GFN_LINK | ((mailid == NULL) ? GFN_UPDATEID : 0), &postfile.id)) == 0)
		strcpy(postfile.filename, strrchr(filepath, '/') + 1);
	#endif

	if ((local_article == YEA) || !(bp->flag & OUT_FLAG)) {
		postfile.flag &= ~FILE_OUTPOST;
	} else {
		postfile.flag |= FILE_OUTPOST;
		outgo_post(&postfile, postboard);
	}

	sprintf(buf, "boards/%s/%s", postboard, DOT_DIR);

	if (noreply) {
		postfile.flag |= FILE_NOREPLY;
		noreply = NA;
	}
#ifdef MARK_X_FLAG
	if (markXflag) {
		postfile.flag |= FILE_DELETED;
		markXflag = 0;
	} else {
		postfile.flag &= ~FILE_DELETED;
	}
#endif
	if (mailtoauthor) {
		//Added by cancel At 02.05.22: �����ض����ļ��������bug
		setboardfile(filepath, postboard, postfile.filename);
		if (header.chk_anony)
			prints("�Բ�����������������ʹ�ü��Ÿ�ԭ���߹��ܡ�");
		else if (!mail_file(filepath, mailid, postfile.title))
			prints("�ż��ѳɹ��ؼĸ�ԭ���� %s", mailid);
		else
			prints("�ż��ʼ�ʧ�ܣ�%s �޷����š�", mailid);
		pressanykey();
	}
	else if (postandmail) {
		if (header.chk_anony) anonymousmail = YEA;
		mail_file(filepath, mailid, postfile.title);
		anonymousmail = NA;
	}
	mailtoauthor = 0;
	postandmail = 0;

	/* monster: ��¼�������µ���ʵID */
	if (bp->flag & ANONY_FLAG) {
		strlcpy(postfile.realowner, currentuser.userid, sizeof(postfile.realowner));
	}

	postfile.filetime = time(NULL);
	if (append_record(buf, &postfile, sizeof(postfile)) == -1) {
		unlink(postfile.filename);
		report("posting '%s' on '%s': append_record failed!",
			postfile.title, currboard);
		pressreturn();
		clear();
		return FULLUPDATE;
	}

	update_lastpost(currboard);
	update_total_today(currboard);
	brc_addlist(postfile.filetime);

    if(replytitle[0] == '\0')
    {
        BBS_SINGAL("/post/newtopic",
                   "b", postboard,
                   "f", postfile.filename,
                   "h", fromhost,
                   NULL);
    }
    else
    {
        BBS_SINGAL("/post/reply",
                   "b", postboard,
                   "f", postfile.filename,
                   "f0", replyfilename,
                   "h", fromhost,
                   NULL);
    }

	report("posted '%s' on '%s'", postfile.title, currboard);

	if (!junkboard()) {
		set_safe_record();
		currentuser.numposts++;
		substitute_record(PASSFILE, &currentuser, sizeof (currentuser),
				  usernum);
	}

	if( digestmode == 2 ||  /* ͬ���� */
	    digestmode == 4 ||  /* ԭ�� */
	    digestmode == 5 ||  /* ͬ����*/
	    digestmode == 6 ||  /* ͬ���� ��ȷ */
	    digestmode == 7 )  	/* ����ؼ��� */
		do_acction(digestmode);

	return FULLUPDATE;
}

int
change_title(char *fname, char *title)
{
	FILE *fp, *out;
	char buf[256], outname[PATH_MAX + 1];
	int newtitle = 0;

	if ((fp = fopen(fname, "r")) == NULL)
		return -1;
	snprintf(outname, sizeof(outname), "%s.%s.%05d", fname, currentuser.userid, uinfo.pid);
	if ((out = fopen(outname, "w")) == NULL) {
		fclose(fp);     /* add by quickmouse 01/03/09 */
		return -1;
	}
	while ((fgets(buf, sizeof(buf), fp)) != NULL) {
		if (!strncmp(buf, "��  ��: ", 8) && newtitle == 0) {
			fprintf(out, "��  ��: %s\033[m\n", title);
			newtitle = 1;
			continue;
		}
		fputs(buf, out);
	}
	fclose(fp);
	fclose(out);
	f_mv(outname, fname);
	return 0;
}

int
edit_post(int ent, struct fileheader *fileinfo, char *direct)
{
	char buf[PATH_MAX + 1];
	struct boardheader *bp;

	if( !INMAIL(uinfo.mode) && YEA == check_readonly(currboard)) { /* Leeward 98.03.28 */
		clear();
		move(8, 8);
		prints("�Բ�����������ֻ�������ϱ༭���¡�");
		pressreturn();
		clear();
		return FULLUPDATE;
	}

	/* Pudding: û����ȨҲ�����޸����� */
	if (!INMAIL(uinfo.mode) && !haspostperm(currboard))
		return DONOTHING;

	if (!INMAIL(uinfo.mode) && !current_bm && !isowner(&currentuser, fileinfo))
		return DONOTHING;


	if (INMAIL(uinfo.mode)) {
		setmailfile(buf, fileinfo->filename);
	} else {
		setboardfile(buf, currboard, fileinfo->filename);
	}

	modify_user_mode(EDIT);
	posts_article_id = fileinfo->id;
	if (vedit(buf, EDIT_NONE) == -1) {
		return FULLUPDATE;
	} else {
		bp = getbcache(currboard);
		if ((bp->flag & ANONY_FLAG) && !(strcmp(fileinfo->owner, currboard))) {
			add_edit_mark(buf, 4, NULL);
		} else {
			add_edit_mark(buf, 1, NULL);
		}
	}

#ifdef MARK_X_FLAG
	if (markXflag) {
		fileinfo->flag |= FILE_DELETED;
		markXflag = 0;
	} else {
		fileinfo->flag &= ~FILE_DELETED;
	}

	safe_substitute_record(direct, fileinfo, ent, (digestmode == 0) ? YEA : NA);
#endif

	if (!INMAIL(uinfo.mode))
    {
		report("edited post '%s' on %s", fileinfo->title, currboard);
        BBS_SINGAL("/post/updatepost",
                   "f", fileinfo->filename,
                   "b", currboard,
                   "h", fromhost,
                   NULL);
    }
        
	return FULLUPDATE;
}

int
edit_title(int ent, struct fileheader *fileinfo, char *direct)
{
	struct boardheader *bp;
	char buf[TITLELEN], tmp[PATH_MAX + 1];
	char *ptr;

	if (!INMAIL(uinfo.mode) && YEA == check_readonly(currboard))   /* Leeward 98.03.28 */
		return DONOTHING;

	if (!INMAIL(uinfo.mode) && digestmode > 7 )
		return DONOTHING;
	/* Pudding: û����ȨҲ�����޸����� */
	if (!INMAIL(uinfo.mode) && !haspostperm(currboard))
		return DONOTHING;
	
	if (!INMAIL(uinfo.mode) && !current_bm && !isowner(&currentuser, fileinfo))
		return DONOTHING;

	strlcpy(buf, fileinfo->title, sizeof(buf));
	my_ansi_filter(buf);
	getdata(t_lines - 1, 0, "�����±���: ", buf, 55, DOECHO, NA);

	strlcpy(save_title, buf, sizeof(save_title));
	if (check_text() == 0) {
		return PARTUPDATE;
	}
	
	if (!strcmp(buf, fileinfo->title))
		return PARTUPDATE;
	if (!INMAIL(uinfo.mode))
		check_title(buf);

	int savedigestmode = digestmode;
	if (!INMAIL(uinfo.mode)) {
		if ( digestmode != 1) {
			digestmode = 0;
			setbdir(direct, currboard);
		}
	}
	if (buf[0] != '\0') {
		strlcpy(fileinfo->title, buf, sizeof(fileinfo->title));
		strlcpy(tmp, direct, sizeof(tmp));
		if ((ptr = strrchr(tmp, '/')) != NULL)
			*ptr = '\0';
		snprintf(genbuf, sizeof(genbuf), "%s/%s", tmp, fileinfo->filename);

		bp = getbcache(currboard);
		if ((bp->flag & ANONY_FLAG) && !(strcmp(fileinfo->owner, currboard))) {
			add_edit_mark(genbuf, 5, buf);
		} else {
			add_edit_mark(genbuf, 2, buf);
		}
		safe_substitute_record(direct, fileinfo, ent, (digestmode == 0) ? YEA : NA);
        if(!INMAIL(uinfo.mode))
        {
            BBS_SINGAL("/post/changetitle",
                       "f", fileinfo->filename,
                       "b", currboard,
                       "h", fromhost,
                       NULL);
        }
	}
	if ( INMAIL(uinfo.mode) || savedigestmode == 0 || digestmode == 1 )
		return PARTUPDATE;
	else {
		digestmode = savedigestmode;
		setbdir(direct, currboard);
		do_acction(digestmode);
		return FULLUPDATE;
	}
}

int
underline_post(int ent, struct fileheader *fileinfo, char *direct)
{
	if (digestmode == 1 || digestmode > 7)
		return DONOTHING;

	if (!current_bm && !isowner(&currentuser, fileinfo))
		return DONOTHING;

	if (fileinfo->flag & FILE_NOREPLY) {
		fileinfo->flag &= ~FILE_NOREPLY;
	} else {
		fileinfo->flag |= FILE_NOREPLY;
	}
	/* freestyler:�Ǳ�׼�Ķ�ģʽ�ò��ɻظ���� */
	int savedigestmode = digestmode;
	digestmode = 0;
	setbdir(direct, currboard);

	safe_substitute_record(direct, fileinfo, ent, YEA);

	if( savedigestmode != 0 ) {
		digestmode = savedigestmode;
		setbdir(direct, currboard);
		safe_substitute_record(direct, fileinfo, ent, (digestmode == 2) ? NA: YEA);
	}
	return PARTUPDATE;
}

int
mark_post(int ent, struct fileheader *fileinfo, char *direct)
{
	if (!current_bm || digestmode==1 || digestmode > 7)
		return DONOTHING;

	int flag;
	if (fileinfo->flag & FILE_MARKED) {
		fileinfo->flag &= ~FILE_MARKED;
		flag = 0;
	} else {
		fileinfo->flag |= FILE_MARKED;
		flag = 1;
	/*	fileinfo->flag &= ~FILE_DELETED; */
	}
	/* freestyler: �Ǳ�׼�Ķ�ģʽ mark post */
	int savedigestmode = digestmode;
	digestmode = 0;
	setbdir(direct, currboard);
	safe_substitute_record(direct, fileinfo, ent, YEA );
	if ( savedigestmode != 0 ) { 
		digestmode = savedigestmode;
		setbdir(direct, currboard); 
		safe_substitute_record(direct, fileinfo, ent, (digestmode == 2 ) ? NA : YEA );
	}
    
	return PARTUPDATE;
}

static int
del_rangecheck(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;

	/* ����m��g������� */
	if (fileinfo->flag & (FILE_MARKED | FILE_DIGEST))
		return KEEPRECORD;

	cancelpost(currboard, currentuser.userid, fileinfo, 0, YEA);
	return REMOVERECORD;
}

int
del_range(int ent, struct fileheader *fileinfo, char *direct)
{
	char num[8];
	int inum1, inum2, result;

	if (!INMAIL(uinfo.mode) && !current_bm)
		return DONOTHING;

	if (digestmode || !strcmp(currboard, "syssecurity"))
		return DONOTHING;

	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, "��ƪ���±��: ", num, 7, DOECHO, YEA);
	if ((inum1 = atoi(num)) <= 0) {
		move(t_lines - 1, 50);
		prints("������...");
		egetch();
		return PARTUPDATE;
	}
	getdata(t_lines - 1, 25, "ĩƪ���±��: ", num, 7, DOECHO, YEA);
	if ((inum2 = atoi(num)) <= inum1) {
		move(t_lines - 1, 50);
		prints("��������...");
		egetch();
		return PARTUPDATE;
	}
	move(t_lines - 1, 50);
	if (askyn("ȷ��ɾ��", NA, NA) == YEA) {
		result = process_records(direct, sizeof(struct fileheader), inum1, inum2, del_rangecheck, NULL);
		fixkeep(direct, inum1, inum2);

		if (INMAIL(uinfo.mode)) {
			snprintf(genbuf, sizeof(genbuf), "Range delete %d-%d in mailbox", inum1, inum2);
		} else {
			update_lastpost(currboard);
			snprintf(genbuf, sizeof(genbuf), "Range delete %d-%d on %s", inum1, inum2, currboard);
			securityreport(genbuf);
		}
		report("%s", genbuf);
		return DIRCHANGED;
	}
	move(t_lines - 1, 50);
	clrtoeol();
	prints("����ɾ��...");
	egetch();
	return PARTUPDATE;
}

/*
 *   undelete һƪ���� Leeward 98.05.18
 *   modified by ylsdd, monster
 */

static int
cmp_fname(const void *hdr1_ptr, const void *hdr2_ptr)
{
	const struct fileheader *hdr1 = (const struct fileheader *)hdr1_ptr;
	const struct fileheader *hdr2 = (const struct fileheader *)hdr2_ptr;

	return (hdr1->filetime - hdr2->filetime);
}

int
undelete_article(int ent, struct fileheader *fileinfo, char *direct, int update)
{
	char *ptr, *ptr2, buf[LINELEN];
	struct fileheader header;
	FILE *fp;
	int valid = NA;

	setboardfile(buf, currboard, fileinfo->filename);
	if ((fp = fopen(buf, "r")) == NULL)
		return -1;

	while (fgets(buf, sizeof(buf), fp)) {
		if (!strncmp(buf, "��  ��: ", 8)) {
			valid = YEA;
			break;
		}
	}
	fclose(fp);

	memcpy(&header, fileinfo, sizeof(header));
	header.flag = 0;
	if (valid == YEA) {
		strlcpy(header.title, buf + 8, sizeof(header.title));
		if ((ptr = strrchr(header.title, '\n')) != NULL)
			*ptr = '\0';
	} else {
		/* article header is not valid, use default title */
		strlcpy(header.title, fileinfo->title, sizeof(header.title));
		if ((ptr = strrchr(header.title, '-')) != NULL) {
			*ptr = '\0';
			for (ptr2 = ptr - 1; ptr >= header.title; ptr2--) {
				if (*ptr2 != ' ') {
					*(ptr2 + 1) = '\0';
					break;
				}
			}
		}
	}

	snprintf(buf, sizeof(buf), "boards/%s/.DIR", currboard);
	if (append_record(buf, &header, sizeof(struct fileheader)) == -1)
		return -1;

	fileinfo->flag |= FILE_FORWARDED;
	snprintf(fileinfo->title, sizeof(fileinfo->title), "<< �����ѱ� %s �ָ������� >>", currentuser.userid);

	if (update == YEA) {
		if (safe_substitute_record(direct, fileinfo, ent, NA) == -1)
			return -1;
		report("undeleted %s's '%s' on %s", header.owner, header.title, currboard);
	}
	return 0;
}

int
undel_post(int ent, struct fileheader *fileinfo, char *direct)
{
	char buf[PATH_MAX + 1];

	if ((digestmode != 8 && digestmode != 9) || !current_bm)
		return DONOTHING;

	if (fileinfo->flag & FILE_FORWARDED) {
		presskeyfor("�����ѻָ�����");
	} else if (undelete_article(ent, fileinfo, direct, YEA) == -1) {
		presskeyfor("�����²����ڣ��ѱ��ָ�, ɾ�����б����");
	} else {
		snprintf(buf, sizeof(buf), "boards/%s/.DIR", currboard);
		sort_records(buf, sizeof(struct fileheader), cmp_fname);
		update_lastpost(currboard);
	}

	return PARTUPDATE;
}

static int
undel_rangecheck(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;

	if (!(fileinfo->flag & FILE_FORWARDED))
		undelete_article(0, fileinfo, currdirect, NA);
	return KEEPRECORD;
}

int
undel_range(int ent, struct fileheader *fileinfo, char *direct)
{
	int inum1, inum2, old_digestmode;
	char num[8], buf[PATH_MAX + 1];

	if ((digestmode != 8 && digestmode != 9) || !current_bm)
		return DONOTHING;

	saveline(t_lines - 1, 0);
	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, "��ƪ���±��: ", num, 7, DOECHO, YEA);
	if ((inum1 = atoi(num)) <= 0) {
		move(t_lines - 1, 50);
		prints("������...");
		egetch();
		saveline(t_lines - 1, 1);
		return DONOTHING;
	}

	getdata(t_lines - 1, 25, "ĩƪ���±��: ", num, 7, DOECHO, YEA);
	if ((inum2 = atoi(num)) <= inum1) {
		move(t_lines - 1, 50);
		prints("��������...");
		egetch();
		saveline(t_lines - 1, 1);
		return DONOTHING;
	}

	move(t_lines - 1, 50);
	if (askyn("�ָ�����", NA, NA) == YEA) {
		process_records(direct, sizeof(struct fileheader), inum1, inum2, undel_rangecheck, NULL);

		snprintf(buf, sizeof(buf), "boards/%s/.DIR", currboard);
		sort_records(buf, sizeof(struct fileheader), cmp_fname);
		update_lastpost(currboard);

		old_digestmode = digestmode;
		digestmode = NA;
		snprintf(genbuf, sizeof(genbuf), "Range undelete %d-%d on %s", inum1, inum2, currboard);
		securityreport(genbuf);
		digestmode = old_digestmode;

		return DIRCHANGED;
	}

	move(t_lines - 1, 50);
	clrtoeol();
	prints("�����ָ�...");
	egetch();
	return PARTUPDATE;
}

int
empty_recyclebin(int ent, struct fileheader *fileinfo, char *direct)
{
	char buf[STRLEN];

	if (!HAS_PERM(PERM_SYSOP))
		return DONOTHING;
//      if (digestmode != 8 && digestmode != 9)
//              return DONOTHING;

	snprintf(buf, sizeof(buf), "ȷ��Ҫ���%s��", (digestmode == 8) ? "����վ" : "��ֽ¨");
	if (askyn(buf, NA, YEA) == NA)
		return PARTUPDATE;

	snprintf(buf, sizeof(buf), "boards/%s/.removing.%d", currboard, getpid());
	f_mv(direct, buf);

	snprintf(buf, sizeof(buf), "%s��%s�����", currboard, (digestmode == 8) ? "����վ" : "��ֽ¨");
	digestmode = NA;
	securityreport(buf);
	setbdir(currdirect, currboard);
	return NEWDIRECT;
}

int
del_post(int ent, struct fileheader *fileinfo, char *direct)
{
	char buf[PATH_MAX + 1], *ptr;
	int owned, IScombine;
	struct userec lookupuser;

	if (digestmode > 7 || !strcmp(currboard, "syssecurity"))
		return DONOTHING;

	/* freestyler: �Ǳ�׼ģʽɾ�� */
	if (digestmode >= 2 ) {
		int savedigestmode = digestmode ;
		digestmode = NA;
		setbdir(direct, currboard);
		ent = get_dir_index(direct, fileinfo) + 1;
		if (del_post(ent, fileinfo, direct) == DIRCHANGED) { /* �ݹ�,ɾ���ɹ� */
			digestmode = savedigestmode;
			setbdir(direct, currboard);
			do_acction(digestmode);
			return DIRCHANGED;
		} else {
			digestmode = savedigestmode;
			setbdir(direct, currboard);
			return PARTUPDATE;
		}
	}

	if (guestuser)
		return DONOTHING;       /* monster: guest ���ܿ�����, �������Լ����� ... */

	owned = isowner(&currentuser, fileinfo);
	if (!current_bm && !owned)
		return DONOTHING;

	snprintf(genbuf, sizeof(genbuf), "ɾ������ [%-.55s]", fileinfo->title);
	if (askyn(genbuf, NA, YEA) == NA) {
		presskeyfor("����ɾ������...");
		return PARTUPDATE;
	}

	if (digestmode == YEA) {
		dodigest(ent, fileinfo, direct, YEA, YEA);
		return DIRCHANGED;
	}

	if (delete_file(direct, ent, fileinfo->filename, NA) == 0) {
		struct boardheader *bp;

		bp = getbcache(currboard);

		strlcpy(buf, direct, sizeof(buf));
		if ((ptr = strrchr(buf, '/')) != NULL)
			*ptr = '\0';

		report("Del '%s' on '%s'", fileinfo->title, currboard);

        BBS_SINGAL("/post/del",
                   "b", currboard,
                   "f", fileinfo->filename,
                   NULL);
        
		IScombine = (!strncmp(fileinfo->title, "���ϼ���", 8) || 
			     !strncmp(fileinfo->title, "[�ϼ�]", 6) ) ;
		cancelpost(currboard, currentuser.userid, fileinfo, owned && (!IScombine), ((bp->flag & ANONY_FLAG) && owned) ? NA : YEA);

		if (!junkboard() && !digestmode && !IScombine) {
			if (owned) {
				set_safe_record();
				if (currentuser.numposts > 0) {
					--currentuser.numposts;
					substitute_record(PASSFILE, &currentuser, sizeof (currentuser), usernum);
				}
			} else {
				if ((owned = getuser(fileinfo->owner, &lookupuser))) {
					if (lookupuser.numposts > 0) {
						--lookupuser.numposts;
						substitute_record(PASSFILE, &lookupuser, sizeof (struct userec), owned);
					}
				}
			}
		}

		update_lastpost(currboard);
		return DIRCHANGED;
	}

	update_lastpost(currboard);
	presskeyfor("ɾ��ʧ��...");
	return PARTUPDATE;
}

#ifdef QCLEARNEWFLAG
int
flag_clearto(int ent, char *direct, int clearall)
{
	int i, fd;
	struct fileheader f_info;

	if (uinfo.mode != READING)
		return DONOTHING;

	if ((fd = open(direct, O_RDONLY, 0)) == -1)
		return DONOTHING;

	if (clearall) {
		lseek(fd, (off_t) ((-sizeof(struct fileheader)) * (BRC_MAXNUM + 1)), SEEK_END);
		while (read(fd, &f_info, sizeof(struct fileheader)) == sizeof(struct fileheader))
			brc_addlist(f_info.filetime);
	} else {
		lseek(fd, (off_t) ((ent - 1) * sizeof(struct fileheader)), SEEK_SET);
		read(fd, &f_info, sizeof(struct fileheader));
		for (i = f_info.filetime - BRC_MAXNUM; i <= f_info.filetime; i++)
			brc_addlist(i);
	}

	close(fd);
	return PARTUPDATE;
}
#else
int
flag_clearto(int ent, char *direct, int clearall)
{
	int fd, i;
	struct fileheader f_info;

	if (uinfo.mode != READING)
		return DONOTHING;
	if ((fd = open(direct, O_RDONLY, 0)) == -1)
		return DONOTHING;
	for (i = 0; clearall || i < ent; i++) {
		if (read(fd, &f_info, sizeof(struct fileheader)) != (struct fileheader))
			break;
		brc_addlist(f_info.filetime);
	}
	close(fd);
	return PARTUPDATE;
}
#endif

int
new_flag_clearto(int ent, struct fileheader *fileinfo, char *direct)
{
	return flag_clearto(ent, direct, NA);
}

int
new_flag_clear(int ent, struct fileheader *fileinfo, char *direct)
{
	extern int brc_num, brc_cur;

	brc_num = 0;
	brc_cur = 0;
	brc_insert(time(NULL));

	return PARTUPDATE;      /* return flag_clearto(ent, direct, YEA); */
}

int
save_post(int ent, struct fileheader *fileinfo, char *direct)
{
	if (!current_bm && !HAS_PERM(PERM_ANNOUNCE))
		return DONOTHING;

	return ann_savepost(currboard, fileinfo, NA);
}

/* monster: 
 *
 *	����ainfo.title, ʹatrace�ܼ�¼��ȷ�Ĳ���λ�� 
 *
 *	ע�⣬update_ainfo_title(YEA) �� update_ainfo_title(NA)������ԣ�
 *	�Ҵ����ܵߵ���������������update_ainfo_title(YEA)����update_ainfo_title(NA)��
 */
void
update_ainfo_title(int import)
{
	static char title[TITLELEN];

	if (import == YEA && INMAIL(uinfo.mode)) {
		strlcpy(title, ainfo.title, sizeof(title));
		snprintf(ainfo.title, sizeof(ainfo.title), "%s������", currentuser.userid);
	} else {
		strlcpy(ainfo.title, title, sizeof(ainfo.title));
	}
}

int
import_post(int ent, struct fileheader *fileinfo, char *direct)
{
	char fname[PATH_MAX + 1];
	char *ptr;

	if (!current_bm && !HAS_PERM(PERM_ANNOUNCE) && !HAS_PERM(PERM_PERSONAL))
		return DONOTHING;

	if (fileinfo->flag & FILE_VISIT) {
		if (askyn("�����������뾫����, ���ڻ�Ҫ�ٷ�����", YEA, YEA) == NA)
			return PARTUPDATE;
	}

	strlcpy(fname, direct, sizeof(fname));
	if ((ptr = strrchr(fname, '/')) == NULL)
		return DONOTHING;
	strcpy(ptr + 1, fileinfo->filename);

	update_ainfo_title(YEA);
	if (ann_import_article(fname, fileinfo->title, fileinfo->owner, fileinfo->flag & FILE_ATTACHED, NA) == 0) {
		fileinfo->flag |= FILE_VISIT;    /* ����־��λ */
		safe_substitute_record(direct, fileinfo, ent, (digestmode == 0) ? YEA : NA);
		presskeyfor("�����ѷ��뾫����, �����������...");
	} else {
		presskeyfor("�Բ���, ��û���趨˿·��˿·�趨����. ���� f �趨˿·.");
	}
	update_ainfo_title(NA);

	return PARTUPDATE;
}

int
select_post(int ent, struct fileheader *fileinfo, char *direct)
{
	if ((!current_bm && !HAS_PERM(PERM_ANNOUNCE)) || digestmode == 1 || digestmode > 7)
		return DONOTHING;

	if (fileinfo->flag & FILE_SELECTED) {
		fileinfo->flag &= ~FILE_SELECTED;
	} else {
		fileinfo->flag |= FILE_SELECTED;
	}

	/* freestyler: �Ǳ�׼�Ķ�ģʽѡ������ */
	int savedigestmode = digestmode;
	digestmode = 0;
	setbdir(direct, currboard);
	safe_substitute_record(direct, fileinfo, ent, (digestmode == 0) ? YEA : NA);
	if ( savedigestmode != 0 ) {
		digestmode = savedigestmode;
		setbdir(direct, currboard);
		safe_substitute_record(direct, fileinfo, ent, (digestmode==2) ? NA : YEA);
	}
	return PARTUPDATE;
}

int
process_select_post(int ent, struct fileheader *fileinfo, char *direct)
{
	return bmfunc(ent, fileinfo, direct, 4);
}

int
author_operate(int ent, struct fileheader *fileinfo, char *direct)
{
	int bm, anony, action;
	char ans[3], msgbuf[1024], repbuf[STRLEN], fname[PATH_MAX + 1];
	int deny_attach;
	struct user_info *uin;
	struct boardheader *bp;

	if (guestuser || !strcmp(currentuser.userid, fileinfo->owner) ||
	    strchr(fileinfo->owner, '.'))
		return DONOTHING;

	bm = (current_bm || HAS_PERM(PERM_SYSOP));

	/* monster: ������������Ƿ�Ϊ�����˺� */
	bp = getbcache(currboard);
	anony = (bp->flag & ANONY_FLAG);
	if (anony && !strcmp(fileinfo->owner, currboard)) {
		if (bm) {
			getdata(t_lines - 1, 0, "��������: 1) ��� 0) ȡ�� [0]: ", ans, 2, DOECHO, YEA);
			if (ans[0] == '1') {
				action = 4;
				goto process_actions;
			}
		}
		return PARTUPDATE;
	}

	getdata(t_lines - 1, 0, bm ?
		"��������: 1) ������Ϣ  2) ��Ϊ���� 3) ��Ϊ���� 4) ��� 0) ȡ�� [0]: "
		:
		"��������: 1) ������Ϣ  2) ��Ϊ���� 3) ��Ϊ���� 0) ȡ�� [0]: ",
		ans, 2, DOECHO, YEA);

	action = ans[0] - '0';
	if (action < 1 || (bm && action > 4) || (!bm && action > 3))
		return PARTUPDATE;

      process_actions:

	switch (action) {
	case 1:         /* ������Ϣ */
		uin = (struct user_info *) t_search(fileinfo->owner, NA);
		if (uinfo.invisible && !HAS_PERM(PERM_SYSOP)) {
			presskeyfor("��Ǹ, �˹���������״̬�²���ִ�У��밴<Enter>����...");
			break;
		}
		if (!HAS_PERM(PERM_PAGE) || !uin || !canmsg(uin) || uin->mode == BBSNET || INBBSGAME(uin->mode)) {
			presskeyfor("���޷�������Ϣ����������, �밴<Enter>����...");
			break;
		}
		/*
		getdata(t_lines - 1, 0, "���� : ", msgbuf, 55, DOECHO, YEA);
		*/
		multi_getdata(0, 0, "���� : ", msgbuf, MSGLEN, 80, MSGLINE, YEA); /* Pudding: �����ѶϢ���� */
		if (msgbuf[0] != '\0') {
			do_sendmsg(uin, msgbuf, 2, uin->pid);
		}
		break;
	case 2:         /* ��Ϊ���� */
		friendflag = 1;
		addtooverride(fileinfo->owner);
		break;
	case 3:         /* ��Ϊ���� */
		friendflag = 0;
		addtooverride(fileinfo->owner);
		break;
	case 4:         /* ��� */
		if (!(anony && !strcmp(fileinfo->owner, currboard)) && !getuser(fileinfo->owner, NULL)) {
			presskeyfor("�޷������������, �밴<Enter>����...");
			break;
		}
		/* Pudding: �ð���ѡ���Ƿ񹫿����� */
		/*
		deny_attach = askyn("�Ƿ񹫿�����(��������Ч)", YEA, YEA);
		*/
		deny_attach = YEA;
		stand_title("�����������");
		if (anony) {
			if (addtodeny(strcmp(fileinfo->owner, currboard) ? fileinfo->owner : fileinfo->realowner, msgbuf, 0, D_ANONYMOUS, fileinfo) == 1) {
				snprintf(repbuf, sizeof(repbuf), "%s ��ȡ���� %s ��ķ���Ȩ��", fileinfo->realowner, currboard);
				securityreport(repbuf);
				if (msgbuf[0] != '\0') {
//					strlcat(msgbuf, "���ģ�\n\n", sizeof(msgbuf));
					setboardfile(fname, currboard, fileinfo->filename);
					autoreport(repbuf, msgbuf, YEA, fileinfo->realowner, NULL);
				}
			}
		} else {
			if (addtodeny(fileinfo->owner, msgbuf, 0, 0, fileinfo) == 1) {
				snprintf(repbuf, sizeof(repbuf), "%s ��ȡ���� %s ��ķ���Ȩ��", fileinfo->owner, currboard);
				securityreport(repbuf);
				if (msgbuf[0] != '\0') {
					if (deny_attach) strlcat(msgbuf, "���ģ�\n\n", sizeof(msgbuf));
					setboardfile(fname, currboard, fileinfo->filename);
					autoreport(repbuf, msgbuf, YEA, fileinfo->owner, deny_attach ? fname : NULL);
				}
			}
		}
		break;
	}
	return FULLUPDATE;
}

int
denyuser(void)
{
	char msgbuf[1024], repbuf[STRLEN], uident[IDLEN + 2];

	stand_title("���ʹ����");
	move(2, 0);
	usercomplete("����׼��������������ʹ����ID: ", uident);

	if (uident[0] && getuser(uident, NULL) && addtodeny(uident, msgbuf, 0, D_NOATTACH, NULL) == 1) {
		snprintf(repbuf, sizeof(repbuf), "%s ��ȡ���� %s ��ķ���Ȩ��", uident, currboard);
		securityreport(repbuf);
		if (msgbuf[0] != '\0') {
			autoreport(repbuf, msgbuf, YEA, uident, NULL);
		}
	}
	return FULLUPDATE;
}

int
A_action(int ent, struct fileheader *fileinfo, char *direct)
{
	if (digestmode == 10 || digestmode == 11) {
		return denyuser();
	} else {
		return auth_search_up(ent, fileinfo, direct);
	}
}

int
a_action(int ent, struct fileheader *fileinfo, char *direct)
{
	if (digestmode == 10 || digestmode == 11) {
		return denyuser();
	} else {
		return auth_search_down(ent, fileinfo, direct);
	}
}

int
del_deny(int ent, struct denyheader *fileinfo, char *direct)
{
	char fname[STRLEN];

	move(t_lines - 1, 0);
	if (askyn("ȷ���ָ�����ߵķ���Ȩ��", NA, NA) == YEA &&
		 (delfromdeny(fileinfo->blacklist, (digestmode == 10) ?
		  D_ANONYMOUS | D_IGNORENOUSER : D_IGNORENOUSER) == 1)) {
		setboardfile(fname, currboard, fileinfo->filename);
		unlink(fname);
		delete_record(currdirect, sizeof (struct denyheader), ent);

		if (get_num_records(currdirect, sizeof (struct denyheader)) == 0) {
			digestmode = NA;
			setbdir(currdirect, currboard);
		}
	}
	return NEWDIRECT;
}

int
d_action(int ent, struct fileheader *fileinfo, char *direct)
{
	return (digestmode == 10 || digestmode == 11) ?
		del_deny(ent, (struct denyheader *)fileinfo, direct) :
		del_post(ent, fileinfo, direct);
}

int
change_deny(int ent, struct denyheader *fileinfo, char *direct)
{
	char repbuf[STRLEN], msgbuf[1024];

	stand_title("����������� (�޸�)");
	if (addtodeny(fileinfo->blacklist, msgbuf, 1, (digestmode == 10) ? D_ANONYMOUS : 0, (struct fileheader *)fileinfo) == 1) {
		delete_record(currdirect, sizeof (struct denyheader), ent);
		snprintf(repbuf, sizeof(repbuf), "�޸Ķ� %s ��ȡ�� %s �淢��Ȩ���Ĵ���",
			fileinfo->blacklist, currboard);
		securityreport(repbuf);
		if (msgbuf[0] != '\0') {
			autoreport(repbuf, msgbuf, (digestmode == 10) ? NA : YEA, fileinfo->blacklist, NULL);
		}
	}
	return NEWDIRECT;
}

int
E_action(int ent, struct fileheader *fileinfo, char *direct)
{
	switch (digestmode) {
	case 0:
		return edit_post(ent, fileinfo, direct);
		break;
		/* freestyler: �Ǳ�׼�Ķ�ģʽ�༭����, 
		 * û����MARK_X_FLAG, edit_post�������õ�ent, direct������digestmodeȫ�ֱ��� 
		 * ����Ҫ��Ӧ����ent, direct, digestmode */
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		return edit_post(ent, fileinfo, direct);
		break;
	case 8:
	case 9:
		return empty_recyclebin(ent, fileinfo, direct);
		break;
	case 10:
	case 11:
		return change_deny(ent, (struct denyheader *)fileinfo, direct);
		break;
	}
	return DONOTHING;
}

/* gcc:�༭�������� */
unsigned int
showfileinfo(unsigned int pbits, int i, int flag)
{
	move (i + 6, 0);
	prints("%s%c. %-30s %2s\033[m",
	       1 ? "\033[m" : "\033[1;30m",
	       'A' + i,
	       fileinfo_strings[i],
	       ((pbits >> i) & 1 ? "��" : "��"));
	refresh();
	return YEA;
}

/* gcc���༭�������ԣ���������ʱע��Ȩ�� */
int
edit_property(int ent, struct fileheader *fileinfo, char *direct)
{
	if (digestmode == 1 || digestmode > 7)
		return DONOTHING;
	/* if (!isowner(&currentuser, fileinfo)) */
	if (strcmp(fileinfo->owner, currentuser.userid) ||
	    fileinfo->filetime < currentuser.firstlogin
	    )
		return DONOTHING;
    
	unsigned int oldflag = 0;
	unsigned int newflag = 0;
	move(1, 0);
	clrtobot();
	move(2, 0);

	if (fileinfo->flag & FILE_MAIL)
		oldflag |= 1;

	newflag = setperms(oldflag, "��������", NUMFILEINFO, showfileinfo);
	move(2, 0);
	if (newflag == oldflag)
		outs("����û���޸�...\n");
	else {
		if ((newflag ^ oldflag) & 1) {
			if (fileinfo->flag & FILE_MAIL)
				fileinfo->flag &= ~FILE_MAIL;
			else
				fileinfo->flag |= FILE_MAIL;
		}
		int savedigestmode = digestmode;
		digestmode = 0;
		setbdir(direct, currboard);
		safe_substitute_record(direct, fileinfo, ent, (digestmode == 0) ? YEA : NA);
		if ( savedigestmode != 0 ) {
			digestmode = savedigestmode;
			setbdir(direct, currboard);
			safe_substitute_record(direct, fileinfo, ent, (digestmode == 2) ? NA : YEA);
		}

		outs("�µĲ����趨���...\n\n");
	}

	pressreturn();
	return FULLUPDATE;
}


int
toggle_previewmode(int ent, struct fileheader *fileinfo, char *direct)
{
	extern int screen_len;
	if (uinfo.mode == READING) {
		screen_len = (screen_len == t_lines - 4) ? (t_lines - 4) / 2 : t_lines - 4;
		previewmode = !previewmode;
		return MODECHANGED;
	}
	return DONOTHING;
	
}
#ifdef ZMODEM

int
zmodem_transferfile(char *localfile, char *remotefile)
{
	char fname[STRLEN];

	move(2, 0);
	clrtobot();

	outs("��ֱ�Ӱ� Enter ������������ʾ���ļ���, �������������ļ���;\n�� * ��ֹ��ǰ���µĴ���.\n\n");
	prints("�����±���Ϊ [%s]\n", remotefile);
	getdata(6, 0, "==> ", fname, sizeof(fname), YEA, YEA);

	if (fname[0] == '*')
		return -2;

	if (fname[0] == '\0')
		strlcpy(fname, remotefile, sizeof(fname));
	fixstr(fname, "\\/:*?\"<>|", ' ');

	if (bbs_zsendfile(localfile, fname) == -1) {
		clear_line(7);
		outs("����ʧ�ܣ���ȷ���ն��Ƿ�֧�� Zmodem ����Э��");
		redoscr();
		pressanykey();
		return -1;
	}

	return 0;
}

int
zmodem_transfer(int ent, struct fileheader *fileinfo, char *direct)
{
	char localfile[PATH_MAX + 1], remotefile[PATH_MAX + 1];

	stand_title("�������� (Zmodem)");

	setboardfile(localfile, currboard, fileinfo->filename);
	snprintf(remotefile, sizeof(remotefile), "%s.txt", fileinfo->title);

	do {
		if (zmodem_transferfile(localfile, remotefile) == -1)
			return FULLUPDATE;
	} while (0); /* monster: todo - attachment support */

	pressanykey();
	return FULLUPDATE;
}

int
Y_action(int ent, struct fileheader *fileinfo, char *direct)
{
	switch(digestmode) {
	case 8:
	case 9:
		return undel_range(ent, fileinfo, direct);
		break;
	case 10:
	case 11:
		break;
	default:
		return zmodem_transfer(ent, fileinfo, direct);
		break;
	}
	return DONOTHING;
}
#endif

int
show_b_secnote(void)
{
	char buf[256];

	clear();
	setvotefile(buf, currboard, "secnotes");
	if (dashf(buf)) {
		if (!check_notespasswd())
			return FULLUPDATE;
		clear();
		ansimore(buf, NA);
	} else {
		move(3, 25);
		prints("�����������ޡ����ܱ���¼����");
	}
	pressanykey();
	return FULLUPDATE;
}

int
show_b_note(void)
{
	clear();
	if (show_board_notes(currboard, YEA) == -1) {
		move(3, 30);
		prints("�����������ޡ�����¼����");
		pressanykey();
	}
	return FULLUPDATE;
}

#ifdef INTERNET_EMAIL
int
forward_post(int ent, struct fileheader *fileinfo, char *direct)
{
	return (mail_forward(ent, fileinfo, direct));
}

int
forward_u_post(int ent, struct fileheader *fileinfo, char *direct)
{
	return (mail_u_forward(ent, fileinfo, direct));
}
#endif

int
show_fileinfo(int ent, struct fileheader *fileinfo, char *direct)
{
	/* get file size */
	struct stat st;
	char buf[512], filepath[512];
	char *t;
	
	strlcpy(buf, direct, sizeof(buf));
	if ((t = strrchr(buf, '/')) != NULL)
		*t = '\0';
	
	snprintf(filepath, sizeof(filepath), "%s/%s", buf, fileinfo->filename);
	if (stat(filepath, &st) != -1)
		fileinfo->size = st.st_size;
	
	clear();
	move(5, 0);
	prints("��������:\n");
	prints("http://%s/bbscon?board=%s&file=%s\n\n",
	       BBSHOST, currboard, fileinfo->filename);
	if (HAS_PERM(PERM_SYSOP))
		prints("����ʱ��: %d\n\n", fileinfo->filetime);
	prints("���´�С: %d �ֽ�\n\n", fileinfo->size);
	strcpy(quote_board, currboard);
	if (getattachinfo(fileinfo))
		prints("%s\n%s\n", attach_info, attach_link);
	pressanykey();
	return FULLUPDATE;
}


struct one_key read_comms[] = {
	{ '_', underline_post },            /* ���ò��ɻظ���� */
//      { 'w', makeDELETEDflag },           /* ����ˮ�ı�� */
#ifdef ZMODEM
	{ 'Y', Y_action },                  /* ͨ��ZmodemЭ���������� (monster) */
#else
	{ 'Y', undel_range },               /* �����ָ��ļ� */
#endif
	{ 'y', undel_post },                /* �ָ����� */
	{ 'r', read_post },                 /* �Ķ����� */
	{ 'K', skip_post },                 /* �������� */
	{ 'd', d_action },                  /* ɾ������/���������� (monster) */
	{ 'E', E_action },                  /* �༭����/��ջ���վ (monster) */
	{ ':', edit_property },             /* �༭��������(gcc) */
	{ 'D', del_range },                 /* ����ɾ������ */
	{ 'm', mark_post },                 /* ������� (m) */
	{ 'g', digest_post },               /* ������� (g) */
	{ 'e', select_post },               /* ������� ($) (monster) */
	{ Ctrl('G'), digest_mode },         /* ��ժģʽ */
	{ '`', digest_mode },               /* ��ժģʽ */
	{ Ctrl('Y'), pure_mode },           /* ԭ��ģʽ (monster) */
	{ Ctrl('T'), acction_mode },        /* ����ģʽ�л� */
	{ 't', thesis_mode },               /* ����ģʽ */
	{ '.', deleted_mode },              /* ����վ */
	{ '>', junk_mode },                 /* ��ֽ¨ */
	{ 'T', edit_title },                /* �������±��� */
	{ 's', do_select },                 /* ѡ����� */
	{ Ctrl('C'), do_cross },            /* ת������ */
	{ Ctrl('P'), do_post },             /* �������� */
	{ 'C', new_flag_clearto },          /* ���δ����ǵ���ǰλ�� */
	{ 'c', new_flag_clear },            /* ��ȫ��δ����� */
#ifdef INTERNET_EMAIL
	{ 'F', mail_forward },              /* �Ļ����� */
	{ 'U', mail_u_forward },            /* �Ļ����� (UUENCODE) */
	{ Ctrl('R'), post_reply },          /* ���Ÿ�ԭ���� */
#endif
	{ 'i', save_post },            	    /* �����´����ݴ浵 */
	{ 'I', import_post },               /* �����·��뾫���� */
	{ Ctrl('E'), process_select_post }, /* ����ѡ������ (monster) */
	{ Ctrl('V'), x_lockscreen_silent }, /* ���� (monster) */
	{ Ctrl('O'), author_operate },      /* �Ե�ǰ�������߲��� (monster) */
	{ 'R', b_results },                 /* �鿴ǰ��ͶƱ��� */
	{ 'v', b_vote },                    /* ͶƱ */
	{ 'V', b_vote_maintain },           /* ͶƱ���� */
	{ 'W', b_notes_edit },              /* �༭����¼ */
	{ Ctrl('W'), b_notes_passwd },      /* �趨���ܱ���¼���� */
	{ 'h', mainreadhelp },              /* ��ʾ���� */
	{ Ctrl('J'), mainreadhelp },        /* ��ʾ���� */
	{ KEY_TAB, show_b_note },           /* �鿴����¼ */
	{ 'z', show_b_secnote },            /* �鿴���ܱ���¼ */
	{ 'x', currboard_announce },	    /* �鿴������ */
	{ 'X', author_announce },           /* �鿴���˾����� */
	{ Ctrl('X'), pannounce },           /* �鿴ָ���û��ĸ��˾����� */
	{ 'a', a_action },                  /* �����������/��� */
	{ 'A', A_action },                  /* ��ǰ��������/��� */
	{ '/', title_search_down },         /* ����������� */
	{ '?', title_search_up },           /* ��ǰ�������� */
	{ '\'', post_search_down },         /* ����������� */
	{ '\"', post_search_up },           /* ��ǰ�������� */
	{ ']', thread_search_down },        /* ����������� */
	{ '[', thread_search_up },          /* ��ǰ�������� */
	{ Ctrl('D'), deny_mode },           /* ����û� */
	{ Ctrl('K'), control_user },        /* �༭���ư����� (monster) */
	{ Ctrl('A'), show_author },         /* ��ѯ�������� */
	{ Ctrl('N'), SR_first_new },
	{ 'n', SR_first_new },
	{ '\\', SR_last },
	{ '=', SR_first },
	{ '%', jump_to_reply },	 	    /* �����ظ� (freestyler) */
	{ Ctrl('S'), SR_read },
	{ 'p', SR_read },
	{ Ctrl('U'), SR_author },
	{ 'b', bmfuncs },                   /* �������⹦�� */
	{ '!', Q_Goodbye },                 /* ������վ */
	{ 'S', s_msg },                     /* ����ѶϢ */
	{ 'f', t_friends },                 /* Ѱ�Һ���/�����ķ� */
	{ 'o', fast_cloak },                /* ������������л� */
	{ 'L', show_allmsgs },		    /* ��ʾ����ѶϢ */
	{ ',', toggle_previewmode },	    /* �л�Ԥ��ģʽ */
#ifdef RECOMMEND
	{ '<', do_recommend },		    /* �Ƽ����� */
#endif
	{ '*', show_fileinfo },		    /* ��ʾȫ�����ӵ� */
	{ '\0', NULL }
};

int
Read(void)
{
	char buf[STRLEN];
	char notename[STRLEN];
	time_t usetime;
	struct stat st;

	if (currboard[0] == 0) {
		move(2, 0);
		prints("����ѡ��������\n");
		pressreturn();
		clear_line(2);
		return -1;
	}
	brc_initial(currboard);
	setbdir(buf, currboard);
	setvotefile(notename, currboard, "notes");
	if (stat(notename, &st) != -1) {
		if (st.st_mtime < (time(NULL) - 7 * 86400)) {
			utimes(notename, NULL);
			setvotefile(genbuf, currboard, "noterec");
			unlink(genbuf);
		}
	}
#ifdef ALWAYS_SHOW_BRDNOTE
	if (dashf(notename))
		ansimore3(notename, YEA);
#else
	if (vote_flag(currboard, '\0', 1 /* �������µı���¼û */ ) ==
	    0) {
		if (dashf(notename)) {
			ansimore3(notename, YEA);
			vote_flag(currboard, 'R', 1 /* д������µı���¼ */ );
		}
	}
#endif

	usetime = time(NULL);
#ifdef INBOARDCOUNT
	int idx = getbnum(currboard);
	board_setcurrentuser(idx-1, 1);
	is_inboard = 1;
#endif 
	
	i_read(READING, buf, NULL, NULL, readtitle, readdoent, update_endline,
	       &read_comms[0], get_records, get_num_records, sizeof(struct fileheader));
	board_usage(currboard, time(NULL) - usetime);
	brc_update();

#ifdef 	INBOARDCOUNT  
	idx = getbnum(currboard);
	board_setcurrentuser(idx-1, -1);
	is_inboard = 0;
#endif 

	return 0;
}

/*Add by SmallPig*/
void
notepad(void)
{
	char tmpname[PATH_MAX + 1], note1[4];
	char note[3][STRLEN - 4]; /* 3������ */
	char tmp[STRLEN];
	FILE *in;
	int i, n;
	time_t thetime = time(NULL);

	clear();
	move(0, 0);
	prints("��ʼ������԰ɣ��������Ŀ�Դ�....\n");
	prints("���������԰淢��Υ��վ�������(��ֿ�, ��ҵ���), Υ����������\n");
	modify_user_mode(WNOTEPAD);
	snprintf(tmpname, sizeof(tmpname), "tmp/notepad.%s.%05d", currentuser.userid, uinfo.pid);
	if ((in = fopen(tmpname, "w")) != NULL) {
		for (i = 0; i < 3; i++)
			memset(note[i], 0, STRLEN - 4);
		while (1) {
			for (i = 0; i < 3; i++) {
				getdata(2 + i, 0, ": ", note[i],
					STRLEN - 5, DOECHO, NA);
				if (note[i][0] == '\0')
					break;
			}
			if (i == 0) {
				fclose(in);
				unlink(tmpname);
				return;
			}
			getdata(5, 0,
				"�Ƿ����Ĵ����������԰� (Y)�ǵ� (N)��Ҫ (E)�ٱ༭ [Y]: ",
				note1, 3, DOECHO, YEA);
			if (note1[0] == 'e' || note1[0] == 'E')
				continue;
			else
				break;
		}
		if (note1[0] != 'N' && note1[0] != 'n') {
			snprintf(tmp, sizeof(tmp), "\033[1;32m%s\033[37m��%.18s��",
				currentuser.userid, currentuser.username);
			fprintf(in,
				"\033[1;34m��\033[44m����������������������������������\033[36m��\033[32m��\033[33m��\033[31m��\033[37m��\033[34m������������������������������\033[44m��\033[m\n");
			getdatestring(thetime);
			fprintf(in,
				"\033[1;34m��\033[32;44m %-44s\033[32m�� \033[36m%23.23s\033[32m �뿪ʱ���µĻ�  \033[m\n",
				tmp, datestring + 6);
			for (n = 0; n < i; n++) {
				if (note[n][0] == '\0')
					break;
				fprintf(in,
					"\033[1;34m��\033[33;44m %-75.75s\033[1;34m\033[m \n",
					note[n]);
			}
			fprintf(in,
				"\033[1;34m��\033[44m �������������������������������������������������������������������������� \033[m \n");
			catnotepad(in, "etc/notepad"); /* ��'etc/notepad'��2�п�ʼ��������ӵ� in ���� */

			fclose(in);
			f_mv(tmpname, "etc/notepad"); 
		} else {
			fclose(in);
			unlink(tmpname);
		}
	}
	if (talkrequest) {
		talkreply();
	}
	clear();
	return;
}

/* youzi quick goodbye */
int
Q_Goodbye(void)
{
	extern int started;
	extern int *zapbuf;
	char fname[STRLEN];
	int logouts;

	/* freestyler: �����Ķ���� */
	brc_update();
	
	free(zapbuf);
	setuserfile(fname, "msgfile");
#ifdef LOG_MY_MESG
	if (count_self() == 1) {
		unlink(fname);
		setuserfile(fname, "allmsgfile");
	}
#endif

	/* edwardc.990423 ѶϢ����� */
	if (dashf(fname) && DEFINE(DEF_MAILMSG) && count_self() == 1)
		mesgmore(fname);

	clear();
	prints("\n\n\n\n");
	setuserfile(fname, "notes");
	if (dashf(fname))
		ansimore(fname, YEA);
	setuserfile(fname, "logout");
	if (dashf(fname)) {
		logouts = countlogouts(fname);
		if (logouts >= 1) {
			user_display(fname,
				     (logouts ==
				      1) ? 1 : (currentuser.numlogins %
						(logouts)) + 1, YEA);
		}
	} else {
		if (fill_shmfile(2, "etc/logout", "GOODBYE_SHMKEY"))
			show_goodbyeshm();
	}
	pressreturn();          // sunner �Ľ���
	clear();
	refresh();
	report("exit");
	if (started) {
		time_t stay;

		stay = time(NULL) - login_start_time;
		snprintf(genbuf, sizeof(genbuf), "Stay:%3d (%s)", stay / 60, currentuser.username);
		log_usies("EXIT ", genbuf);
		u_exit();
	}


#ifdef CHK_FRIEND_BOOK
	if (num_user_logins(currentuser.userid) == 0 && !guestuser) {
		FILE *fp;
		char buf[STRLEN], *ptr;

		if ((fp = fopen("friendbook", "r")) != NULL) {
			while (fgets(buf, sizeof (buf), fp) != NULL) {
				char uid[14];

				ptr = strstr(buf, "@");
				if (ptr == NULL) {
					del_from_file("friendbook", buf);
					continue;
				}
				ptr++;
				strcpy(uid, ptr);
				ptr = strstr(uid, "\n");
				*ptr = '\0';
				if (!strcmp(uid, currentuser.userid))
					del_from_file("friendbook", buf);
			}
			fclose(fp);
		}
	}
#endif
	deattach_shm();
	sleep(1);
	exit(0);
	return -1;
}

int
Goodbye(void)
{
	char sysoplist[20][41], syswork[20][41], buf[STRLEN];
	int i, num_sysop, choose;
	FILE *sysops;
	char *ptr;

	*quote_file = '\0';
	i = 0;
	if ((sysops = fopen("etc/sysops", "r")) != NULL) {
		while (fgets(buf, STRLEN, sysops) != NULL && i <= 19) {
			if (buf[0] == '#')
				continue;
			ptr = strtok(buf, " \n\r\t");
			if (ptr) {
				strlcpy(sysoplist[i], ptr, sizeof(sysoplist[i]));
				ptr = strtok(NULL, " \n\r\t");
				strlcpy(syswork[i], (ptr == NULL) ?  "[ְ����]" : ptr, sizeof(syswork[i]));
				i++;
			}
		}
		fclose(sysops);
	}

	num_sysop = i;
	move(1, 0);
	alarm(0);
	clear();
	move(0, 0);
	prints("���Ҫ�뿪 %s ������ʲô������\n", BoardName);
	prints("[\033[1;33m1\033[m] ���Ÿ�������Ա\n");
	prints("[\033[1;33m2\033[m] �����������һ�Ҫ��\n");
#ifdef USE_NOTEPAD
#ifdef AUTHHOST
	if (HAS_PERM(PERM_WELCOME)) {
#endif
	if (!guestuser) {
		prints("[\033[1;33m3\033[m] дд\033[1;32m��\033[33m��\033[35m��\033[m��\n");
	}
#ifdef AUTHHOST
	}
#endif
#endif
	if (!guestuser && HAS_PERM(PERM_MESSAGE)) {
		prints("[\033[1;33m4\033[m]\033[1;32m �����ѷ�����Ϣ :)\033[m\n");
	}

	prints("[\033[1;33m5\033[m] �����ޣ�Ҫ�뿪��\n");
	strlcpy(buf, "���ѡ���� [\033[1;32m5\033[m]��", sizeof(buf));
	getdata(8, 0, buf, genbuf, 4, DOECHO, YEA);
	clear();
	choose = genbuf[0] - '0';

	switch (choose) {
	case 1:
		outs("     վ���� ID    �� �� �� ְ ��\n");
		outs("     ============ =====================\n");
		for (i = 1; i <= num_sysop; i++) {
			prints("[\033[1;33m%2d\033[m] %-12s %s\n", i,
			       sysoplist[i - 1], syswork[i - 1]);
		}
		prints("[\033[1;33m%2d\033[m] ���������ޣ�\n", num_sysop + 1);
		snprintf(buf, sizeof(buf), "���ѡ���� [\033[1;32m%d\033[m]��", num_sysop + 1);
		getdata(num_sysop + 5, 0, buf, genbuf, 4, DOECHO, YEA);
		choose = atoi(genbuf);
		if (choose >= 1 && choose <= num_sysop)
			do_send(sysoplist[choose - 1], "ʹ���߼����Ľ�����", NA, 0);
		break;
	case 2:
		return FULLUPDATE;
#ifdef USE_NOTEPAD
	case 3:
		if (!guestuser && HAS_PERM(PERM_WELCOME) && choose == 3)
			notepad();
		break;
#endif
	case 4:
		if (HAS_PERM(PERM_MESSAGE) && choose == 4)
			friend_wall();
		break;
	}
	return Q_Goodbye();
}

void
board_usage(char *mode, time_t usetime)
{
	time_t now;
	char buf[256];

	now = time(NULL);
	snprintf(buf, sizeof(buf), "%24.24s USE %-20.20s Stay: %5d (%s)\n",
		ctime(&now), mode, usetime, currentuser.userid);
	do_report(LOG_BOARD, buf);
}

int
Info(void)
{
	modify_user_mode(XMENU);
	ansimore("Version.Info", YEA);
	clear();
	return 0;
}

int
Conditions(void)
{
	modify_user_mode(XMENU);
	ansimore("COPYING", YEA);
	clear();
	return 0;
}

int
Welcome(void)
{
	char ans[3];

	modify_user_mode(XMENU);
	if (!dashf("etc/Welcome2"))
		ansimore("etc/Welcome", YEA);
	else {
		clear();
		stand_title("�ۿ���վ����");
		for (;;) {
			getdata(1, 0,
				"(1)�����վ������  (2)��վ��վ���� ? : ", ans,
				2, DOECHO, YEA);
			/* skyo.990427 modify  �� Enter ����  */
			if (ans[0] == '\0') {
				clear();
				return 0;
			}
			if (ans[0] == '1' || ans[0] == '2')
				break;
		}
		if (ans[0] == '1')
			ansimore("etc/Welcome", YEA);
		else
			ansimore("etc/Welcome2", YEA);
	}
	clear();
	return 0;
}

int
cmpbnames(void *bname_ptr, void *brec_ptr)
{
	char *bname = (char *)bname_ptr;
	struct fileheader *brec = (struct fileheader *)brec_ptr;

	return (!strncasecmp(bname, brec->filename, sizeof(brec->filename))) ? YEA : NA;
}

/*
 *  by ylsdd, modified by monster
 *
 *  unlink action is taked within cancelpost if in mail mode,
 *  otherwise this item is added to the file '.DELETED' under
 *  the board's directory, the filename is not changed.
 *  Unlike the fb code which moves the file to the deleted
 *  board.
 */
void
cancelpost(char *board, char *userid, struct fileheader *fh, int owned, int keepid)
{
	struct fileheader postfile;
	char oldpath[PATH_MAX + 1];
	int tmpdigestmode;

	if (uinfo.mode == RMAIL) {
		snprintf(oldpath, sizeof(oldpath), "mail/%c/%s/%s",
			mytoupper(currentuser.userid[0]),
			currentuser.userid, fh->filename);
		unlink(oldpath);
		return;
	}

/*
 *	memset(&postfile, 0, sizeof (postfile));
 *	strcpy(postfile.filename, fh->filename);
 *	strlcpy(postfile.owner, fh->owner, IDLEN + 2);
 *	postfile.owner[IDLEN + 1] = 0;
 */

	memcpy(&postfile, fh, sizeof(postfile));
	if (keepid == YEA)
		snprintf(postfile.title, sizeof(postfile.title), "%-32.32s - %s", fh->title, userid);
	postfile.flag = 0;
	tmpdigestmode = digestmode;
	digestmode = (owned) ? 9 : 8;
	setbdir(genbuf, board);
	append_record(genbuf, &postfile, sizeof(postfile));

	digestmode = tmpdigestmode;
}

int
thesis_mode(void)
{
	int id;
	struct userec lookupuser;

	id = getuser(currentuser.userid, &lookupuser);
	lookupuser.userdefine ^= DEF_THESIS;
	currentuser.userdefine ^= DEF_THESIS;
	substitute_record(PASSFILE, &lookupuser, sizeof(lookupuser), id);
	update_utmp();
	return FULLUPDATE;
}

int
marked_all(int type)
{
	struct fileheader post;
	int fd, fd2;
	char fname[PATH_MAX + 1], tname[PATH_MAX + 1];
	char tempname1[TITLELEN + 1], tempname2[51];

	snprintf(fname, sizeof(fname), "boards/%s/%s", currboard, DOT_DIR);
	switch (type) {
	case 0:
		snprintf(tname, sizeof(tname), "boards/%s/%s", currboard, MARKED_DIR);
		break;
	case 1:
		snprintf(tname, sizeof(tname), "boards/%s/%s", currboard, AUTHOR_DIR);
		break;
	case 2:
	case 3:
		snprintf(tname, sizeof(tname), "boards/%s/SOMEONE.%s.DIR.%d", currboard,
			someoneID, type - 2);
		break;
	case 4:
		snprintf(tname, sizeof(tname), "boards/%s/KEY.%s.DIR", currboard,
			currentuser.userid);
		break;
	}

	if ((fd = open(fname, O_RDONLY, 0)) == -1)
		return -1;

	if ((fd2 = open(tname, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
		close(fd);
		return -1;
	}

	while (read(fd, &post, sizeof (struct fileheader)) == sizeof(struct fileheader)) {
		switch (type) {
		case 0:
			if (post.flag & FILE_MARKED) {
				safewrite(fd2, &post, sizeof(post));
			}
			break;
		case 1:
			if (strncmp(post.title, "Re: ", 4) || post.flag & (FILE_MARKED | FILE_DIGEST)) {
				safewrite(fd2, &post, sizeof (post));
			}
			break;
		case 2:
			strtolower(tempname1, post.owner);
			strtolower(tempname2, someoneID);
			if (strstr(tempname1, tempname2)) {
				safewrite(fd2, &post, sizeof (post));
			}
			break;
		case 3:
			if (!strcasecmp(post.owner, someoneID)) {
				safewrite(fd2, &post, sizeof (post));
			}
			break;
		case 4:
			strtolower(tempname1, post.title);
			strtolower(tempname2, someoneID);
			if (strstr(tempname1, tempname2)) {
				safewrite(fd2, &post, sizeof (post));
			}
			break;
		}
	}
	close(fd);
	close(fd2);

	return 0;
}

void
add_edit_mark(char *fname, int mode, char *title)
{
	FILE *fp, *out;
	time_t now;
	char buf[LINELEN], outname[PATH_MAX + 1];
	int step = 0, signature = 0, anonymous = NA, color;


	if ((fp = fopen(fname, "r")) == NULL)
		return;

	/* monster: ����ת������ʱ������ȷ��ӱ�Ǻ������༭����й©ID�Ĵ��� */
	switch (mode) {
	case 3:
		mode = 1;
		snprintf(outname, sizeof(outname), "mail/.tmp/editpost.%s.%d", currentuser.userid, uinfo.pid);
		break;
	case 4:
	case 5:
		mode -= 3;
		anonymous = YEA;
	default:
		if (INMAIL(uinfo.mode)) {
			snprintf(outname, sizeof(outname), "mail/.tmp/editpost.%s.%d", currentuser.userid, uinfo.pid);
		} else {
			snprintf(outname, sizeof(outname), "boards/.tmp/editpost.%s.%d", currentuser.userid, uinfo.pid);
		}
	}

	if ((out = fopen(outname, "w")) == NULL)
		return;

	color = (currentuser.numlogins % 7) + 31;
	while ((fgets(buf, sizeof(buf), fp)) != NULL) {
		if (!strncmp(buf, "--\n", 3))
			signature = 1;
		if (mode == 1) {
			if (buf[0] == 27 && buf[1] == '[' &&
			    buf[2] == '1' && buf[3] == ';' &&
			    !strncmp(buf + 6, "m�� �޸�:��", 11))
				continue;
			if (Origin2(buf) && (step != 3)) {
				now = time(NULL);
				fprintf(out,
					"%s\033[1;%2dm�� �޸�:��%s �� %15.15s �޸ı��ģ�[FROM: %s]\033[m\n",
					(signature) ? "" : "--\n", color,
					(anonymous == YEA) ? currboard : currentuser.userid, ctime(&now) + 4,
					(anonymous == YEA) ? "������ʹ�ļ�" : fromhost);
				step = 3;
			}
			fputs(buf, out);
		} else {
			if (!strncmp(buf, "��  ��: ", 8)) {
				fprintf(out, "��  ��: %s\n", title);
				mode = 1;
				continue;
			}
			fputs(buf, out);
		}
	}
	if ((step != 3) && (mode == 1)) {
		now = time(NULL);
		fprintf(out, "%s\033[1;%2dm�� �޸�:��%s �� %15.15s �޸ı��ģ�[FROM: %s]\033[m\n",
			(signature) ? "" : "--\n", color,
			(anonymous == YEA) ? currboard : currentuser.userid, ctime(&now) + 4,
			(anonymous == YEA) ? "������ʹ�ļ�" : fromhost);
	}
	fclose(fp);
	fclose(out);
	f_mv(outname, fname);
}

/* monster: if the file is delivered as mail, set board to NULL */
void
add_sysheader(FILE *fp, char *board, char *title)
{
	time_t now;
	char buf[STRLEN];

	now = time(NULL);
	if (board != NULL) {
		snprintf(buf, sizeof(buf), "\033[1;41;33m������: %s (�Զ�����ϵͳ), ����: %s", BoardName, board);
	} else {
		snprintf(buf, sizeof(buf), "\033[1;41;33m������: %s (�Զ�����ϵͳ)", BoardName);
	}

	fprintf(fp, "%s%*s\033[m\n", buf, (int)(89 - strlen(buf)), " ");
	fprintf(fp, "��  ��: %s\n", title);
	fprintf(fp, "����վ: %s (%24.24s)\n", BoardName, ctime(&now));

	if (board == NULL) {
		fprintf(fp, "��  Դ: %s\n\n", BBSHOST);
	} else {
		fputc('\n', fp);
	}
}

void
add_syssign(FILE * fp)
{
	fprintf(fp, "\n--\n\033[m\033[1;31m�� ��Դ:��%s %s��[FROM: %s]\033[m\n",
		BoardName, BBSHOST, BBSHOST);
}
