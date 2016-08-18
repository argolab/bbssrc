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

int
offline(void)
{
	int i, oldlevel, silent;
	char buf[STRLEN], lastword[640] = { '\0' };

	modify_user_mode(OFFLINE);
	clear();

	if (HAS_ORGPERM(PERM_SYSOP) || HAS_ORGPERM(PERM_BOARDS) ||
	    HAS_ORGPERM(PERM_ADMINMENU) || HAS_ORGPERM(PERM_SEEULEVELS)) {
		move(1, 0);
		prints("\n\n������������, ���������ɱ��!!\n");
		pressreturn();
		clear();
		return 0;
	}

	if (count_self() > 1) {
		move(1, 0);
		prints("\n\n�����ر�������������ִ�д�����!!\n");
		pressreturn();
		clear();
		return 0;
	}

	/* modified by Gophy at 1999.5.4 */
	if (currentuser.numlogins < 20 || !HAS_ORGPERM(PERM_POST)) {	
		move(1, 0);
		prints("\n\n�Բ���, ����δ���ʸ�ִ�д�����!!\n");
		prints("��д�Ÿ� SYSOP ˵����ɱԭ��, лл��\n");
		pressreturn();
		clear();
		return 0;
	}

	if (confirm_userident("��ɱ") == NA)
		return 0;

	move(1, 0);
	prints("\033[1;5;31m����\033[0;1;31m: ��ɱ��, �����޷����ô��ʺŽ��뱾վ����");
	move(3, 0);
	prints("\033[1;32m���ʺ�Ҫ�� 3 ���Ż�ɾ�������ѹ�� :( .....\033[m");
	move(5, 0);

	i = 0;
	if (askyn("�����᲻���㣬����֮ǰ��ʲô����˵ô", NA, NA) == YEA) {
		strcpy(lastword, ">\n>");
		buf[0] = '\0';
		for (i = 0; i < 8; i++) {
			getdata(i + 6, 0, ": ", buf, 77, DOECHO, YEA);
			if (buf[0] == '\0')
				break;
			strcat(lastword, buf);
			strcat(lastword, "\n>");
		}
		if (i == 0) {
			lastword[0] = '\0';
		} else {
			strcat(lastword, "\n\n");
		}
		move(i + 8, 0);
		if (i == 0) {
			prints("�����㻹��ʲô����Ը��˵���ǲ��ǻ�����˼δ�ˣ�");
		} else if (i <= 4) {
			prints("�������㲵��������Ķ����� ... ");
		} else {
			prints("�һ�ǵ���ģ����ѣ���Ҳ֪������뿪Ҳ��û�а취���£�������");
		}
		silent = (i > 0);
	} else {
		silent = askyn("������뾲�����뿪�ⷳ���ĳ���", NA, NA);
	}

	move(i + 10, 0);
	if (askyn("��ȷ��Ҫ�뿪������ͥ", NA, NA) == 1) {
		clear();
		oldlevel = currentuser.userlevel;
		currentuser.userlevel &= 0x3F;
		currentuser.userlevel ^= PERM_SUICIDE;
		substitute_record(PASSFILE, &currentuser, sizeof(struct userec), usernum);
		mail_info(lastword, oldlevel, silent);
		modify_user_mode(OFFLINE);
		// kick_user(&uinfo);
		// exit(0);
		abort_bbs();
	}
	return 0;
}

/* Rewrite by cancel at 01/09/16 */
void
getuinfo(FILE *fn, struct userec *userinfo)
{
	int num;
	char buf[40];

	fprintf(fn, "\n���Ĵ���     : %s\n", userinfo->userid);
	fprintf(fn, "�����ǳ�     : %s\n", userinfo->username);
	fprintf(fn, "��ʵ����     : %s\n", userinfo->realname);
	fprintf(fn, "��ססַ     : %s\n", userinfo->address);
	fprintf(fn, "�����ʼ����� : %s\n", userinfo->email);
	fprintf(fn, "ע����Ϣ     : %s\n", userinfo->reginfo);
	fprintf(fn, "�ʺ�ע���ַ : %s\n", userinfo->ident);
	getdatestring(userinfo->firstlogin);
	fprintf(fn, "�ʺŽ������� : %s\n", datestring);
	getdatestring(userinfo->lastlogin);
	fprintf(fn, "����������� : %s\n", datestring);
	fprintf(fn, "������ٻ��� : %s\n", userinfo->lasthost);
	fprintf(fn, "��վ����     : %d ��\n", userinfo->numlogins);
	fprintf(fn, "������Ŀ     : %d\n", userinfo->numposts);
	fprintf(fn, "��վ��ʱ��   : %d Сʱ %d ����\n",
		userinfo->stay / 3600, (userinfo->stay / 60) % 60);
	strcpy(buf, "bTCPRp#@XWBA#VS-DOM-F012345678");
	for (num = 0; num < 30; num++)
		if (!(userinfo->userlevel & (1 << num)))
			buf[num] = '-';
	buf[num] = '\0';
	fprintf(fn, "ʹ����Ȩ��   : %s\n\n", buf);
}

/* Rewrite End. */

void
mail_info(char *lastword, int userlevel, int silent)
{
	FILE *fn;
	time_t now;
	char filename[PATH_MAX + 1];

	now = time(NULL);
	getdatestring(now);
	snprintf(filename, sizeof(filename), "tmp/suicide.%s", currentuser.userid);
	if ((fn = fopen(filename, "w")) != NULL) {
		fprintf(fn,
			"\033[1m%s\033[m �Ѿ��� \033[1m%s\033[m �Ǽ���ɱ��\n�������������ϣ��뱣��: \n",
			currentuser.userid, datestring);
		getuinfo(fn, &currentuser);
		fclose(fn);
		postfile(filename, "syssecurity", "�Ǽ���ɱ֪ͨ(3�����Ч)...", 2);
		unlink(filename);
	}
	snprintf(filename, sizeof(filename), "suicide/suicide.%s", currentuser.userid);
	if (NA == silent) {
		unlink(filename);
		return;
	}
	if ((fn = fopen(filename, "w")) != NULL) {
		fprintf(fn, "������: %s (%s), ����: ID\n", currentuser.userid,
			currentuser.username);
		fprintf(fn, "��  ��: %s ���ٱ�����\n", currentuser.userid);
		fprintf(fn, "����վ: %s (%24.24s), վ���ż�\n\n", BoardName,
			ctime(&now));

		fprintf(fn, "��Һ�,\n\n");
		fprintf(fn, "���� %s (%s)���Ҽ����뿪�����ˡ�\n\n",
			currentuser.userid, currentuser.username);
		getdatestring(currentuser.firstlogin);
		fprintf(fn,
			"�� %14.14s �������Ѿ����� %d ���ˣ������ܼ� %d ���ӵ����������У�\n",
			datestring, currentuser.numlogins,
			currentuser.stay / 60);
		fprintf(fn, "������λ����������أ������ҵ�����...  ���εΣ������������У�\n\n");
		if (lastword != NULL && lastword[0] != '\0') 
			fprintf(fn, "%s", lastword);
		fprintf(fn,
			"�����ǣ���� %s �����ǵĺ����������õ��ɡ���Ϊ�Ҽ��������뿪������!\n\n",
			currentuser.userid);
		fprintf(fn, "�����г�һ���һ�����ġ� ����!! �ټ�!!\n\n\n");
		getdatestring(now);
		fprintf(fn, "%s �� %s ��.\n\n", currentuser.userid, datestring);
		fprintf(fn, "--\n\033[1;%dm�� ��Դ:. %s %s. [FROM: %s]\033[m\n",
			(currentuser.numlogins % 7) + 31, BoardName, BBSHOST,
			currentuser.lasthost);
		fclose(fn);

		// postfile(filename, "notepad", "�Ǽ���ɱ����...", 2);
		// unlink(filename);
	}
}
