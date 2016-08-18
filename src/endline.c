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

struct ELSHM *endline_shm = NULL;
extern struct anninfo ainfo;

static int spacelen;

/* monster: �ѵײ�������Ϣ���빲���ڴ� */
void
endline_init(void)
{
	FILE *fp;
	int i;
	char buf[ENDLINE_BUFSIZE];

	if (endline_shm == NULL) {
		endline_shm = attach_shm("EL_SHMKEY", sizeof(*endline_shm));
	}

	endline_shm->count = 0;
	for (i = 0; i < ENDLINE_MAXLINE; i++)
		endline_shm->data[i][0] = 0;

	if ((fp = fopen("etc/endline_msg", "r")) != NULL) {
		while (endline_shm->count < ENDLINE_MAXLINE) {
			int len;

			if (fgets(buf, ENDLINE_BUFSIZE, fp) == NULL)
				break;
			/* Pudding: ԭ���봦���β���з������� */
			len = strlen(buf);	/* chop the newline character  */
			while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
				buf[len - 1] = '\0';
				len--;
			}

			if (len > 0) {
				strcpy(endline_shm->data[endline_shm->count],
				       buf);
				endline_shm->count++;
			}
		}
		fclose(fp);
	}
	spacelen = 14 - strlen(currentuser.userid);
}

/* monster: �ػ�״̬�� ����ʾ�ײ�������Ϣ��*/

void
update_endline(void)
{
	char buf[STRLEN];
	time_t now;
	static time_t last;
	static int current_msg;
	int allstay;

	if (!DEFINE(DEF_ENDLINE))
		return;

	clear_line(t_lines - 1);
	now = time(NULL);
	int 	freq = 10;  /* ÿ��10���Ӹ���һ�� */
	if (now - last >= 60) {
		++current_msg;
		last = now;
		if (current_msg >= endline_shm->count * freq) {
			if (is_birth(currentuser)) {
				current_msg = -1;
				outs("\033[1;37;44m                    �������������տ��֣�  �ǵñ���ȫվӴ :P                    \033[m");
				return;
			} else {
				current_msg = 0;
			}
		}
	}

	/* 2005-12-8 pudding: Ϊ���ƶ����� super ugly �޸�, ��ʱ�Ե� */
	struct tm *t = localtime(&now);
	if (  t->tm_hour < 20 ||  current_msg % freq || endline_shm->count == 0 || DEFINE(DEF_NOENDLINE) || uinfo.mode == LOCKSCREEN) {
		allstay = (now - login_start_time) / 60;
		if (uinfo.mode == LOCKSCREEN) {
			strlcpy(buf, "\033[32m������\033[33m", sizeof(buf));
		} else {
			snprintf(buf, sizeof(buf), "\033[36m%3d\033[33m:\033[36m%2d\033[33m", (allstay / 60) % 1000, allstay % 60);
		}

		num_alcounter();
		prints("\033[1;44;33m[\033[36m%24.24s\033[33m] [\033[36m%4d\033[33m��/\033[1;36m%3d\033[33m��] [\033[36m%1s%1s%1s%1s%1s%1s\033[33m] �ʺ�[\033[36m%s\033[33m]%*s[%s]\033[m",
		       ctime(&now), count_users, count_friends,
		       (uinfo.pager & ALL_PAGER) ? "P" : "p",
		       (uinfo.pager & FRIEND_PAGER) ? "O" : "o",
		       (uinfo.pager & ALLMSG_PAGER) ? "M" : "m",
		       (uinfo.pager & FRIENDMSG_PAGER) ? "F" : "f",
		       (DEFINE(DEF_MSGGETKEY)) ? "X" : "x",
		       (uinfo.invisible == 1) ? "C" : "c", 
			currentuser.userid, spacelen, "", buf);
	} else {
		outs(endline_shm->data[current_msg / freq ]);
	}
}

void
update_annendline(void)
{
	if (!DEFINE(DEF_ENDLINE))
		return;

	clear_line(t_lines - 1);
	prints((ainfo.manager == YEA) ?
		"\033[1;31;44m[�� ��] \033[33m˵�� h �� �뿪 Q,�� �� �������� a �� ����Ŀ¼ g �� �༭���� E %s \033[m" :
		"\033[1;31;44m[���ܼ�] \033[33m˵�� h �� �뿪 Q,�� �� �ƶ��α� k,��,j,�� �� ��ȡ���� Rtn,�� %s \033[m", 
		(uinfo.mode == LOCKSCREEN) ? "\033[33m[\033[32m������\033[33m]" : "        ");
}

void
update_atraceendline(void)
{
	if (!DEFINE(DEF_ENDLINE))
		return;

	clear_line(t_lines - 1);
	prints("\033[1;31;44m[������������¼] \033[33m˵�� h �� �뿪 Q,�� �� �鿴��¼ r �� ��ղ�����¼ E  %s \033[m",
	       (uinfo.mode == LOCKSCREEN) ? "\033[33m[\033[32m������\033[33m]" : "        ");
	
}
