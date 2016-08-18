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

/* monster: ������ڵ���Ч�� */
int
valid_day(int year, int month, int day)
{
	if (day < 1)
		return NA;

	if (month == 2) {
		if ((year % 4 != 0) || ((year % 400 != 0) && (year % 100 == 0))) {
			if (day >= 29)
				return NA;
		} else {
			if (day > 29)
				return NA;
		}
	}

	if ((month < 8 && month % 2 == 1) || (month >= 8 && month % 2 == 0)) {
		if (day > 31)
			return NA;
	} else {
		if (day >= 31)
			return NA;
	}

	return YEA;
}


//Added by cancel at 02.03.02
int
cmpregrec(void *username_ptr, void *rec_ptr)
{
	char *username = (char *)username_ptr;
	struct new_reg_rec *rec = (struct new_reg_rec *)rec_ptr;

	return (!strcmp(username, rec->userid)) ? YEA : NA;
}

void
display_userinfo(struct userec *u)
{
	int num;
	time_t now;
#if 0
#ifdef REG_EXPIRED
#ifndef AUTOGETPERM
	time_t nextreg;
#endif
#endif
#endif
	move(2, 0);
	clrtobot();
	now = time(NULL);
	set_safe_record();
	prints("���Ĵ���     : %-14s", u->userid);
	prints("�ǳ� : %-20s", u->username);
	prints("     �Ա� : %s", (u->gender == 'M' ? "��" : "Ů"));
	prints("\n��ʵ����     : %-40s", u->realname);
	prints("  �������� : %d/%d/%d", u->birthmonth, u->birthday, u->birthyear + 1900);
	prints("\n��ססַ     : %-38s\n", u->address);

/* 	{
 *     		int tyear, tmonth, tday;
 *
 *		tyear = u->birthyear + 1900;
 *      	tmonth = u->birthmonth;
 *     		tday = u->birthday;
 *     		countdays(&tyear, &tmonth, &tday, now);
 *     		prints("�ۼ��������� : %d\n", abs(tyear));
 *  	}
 */

	prints("�����ʼ����� : %s\n", u->email);
	prints("ע����Ϣ     : %s\n", u->reginfo);
	if(HAS_PERM(PERM_ADMINMENU))
		    prints("�ʺ�ע���ַ : %s\n", u->ident);
	prints("������ٻ��� : %-22s", u->lasthost);
	prints("�ն˻���̬ : %s\n", u->termtype);
	getdatestring(u->firstlogin);
	prints("�ʺŽ������� : %s[��� %d ��]\n",
	       datestring, (now - (u->firstlogin)) / 86400);
	getdatestring(u->lastlogin);
	prints("����������� : %s[��� %d ��]\n",
	       datestring, (now - (u->lastlogin)) / 86400);
#if 0
#ifndef AUTOGETPERM
#ifndef REG_EXPIRED
	getdatestring(u->lastjustify);
	prints("���ȷ������ : %s\n",
	       (u->lastjustify == 0) ? "δ��ע��" : datestring);
#else
	if (u->lastjustify == 0)
		prints("���ȷ��     : δ��ע��\n");
	else {
		prints("���ȷ��     : ����ɣ���Ч����: ");
		nextreg = u->lastjustify + REG_EXPIRED * 86400;
		getdatestring(nextreg);
		prints("%14.14s[%s]������ %d ��\n",
			datestring, datestring + 23, (nextreg - now) / 86400);
		if (NULL != sysconf_str("REG_EXPIRED_LOCK")) {
			outs("\033[1;31m��վ���ڰ���ע�������ڣ��ݲ���Ҫ����ע��\033[0m\n");
		}
	}
#endif
#endif
#endif
/* Added by betterman 06.08.31 */
	prints("���ȷ��     : %s\n",	(u->userlevel & PERM_WELCOME ) ? "�����" : "��δ���");

	prints("������Ŀ     : %d\n", u->numposts);
	prints("˽������     : %d ��\n", u->nummails);
	prints("��վ����     : %d ��      ", u->numlogins);
	prints("��վ��ʱ��   : %d �� %d Сʱ %d ����\n",
	       u->stay / 86400, (u->stay / 3600) % 24, (u->stay / 60) % 60);
	strcpy(genbuf, "bTCPRD#@XWBA#VS-DOM-F0s2345678");
	for (num = 0; num < strlen(genbuf); num++)
		if (!(u->userlevel & (1 << num)))
			genbuf[num] = '-';
	prints("ʹ����Ȩ��   : %s\n", genbuf);
	prints("\n");
	if (u->userlevel & PERM_SYSOP) {
		prints("  ���Ǳ�վ��վ��, ��л���������Ͷ�.\n");
	} else if (u->userlevel & PERM_BOARDS) {
		prints("  ���Ǳ�վ�İ���, ��л���ĸ���.\n");
	} else if (u->userlevel & PERM_LOGINOK) {
		prints("  ����ע������Ѿ����, ��ӭ���뱾վ.\n");
	} else if (u->lastlogin - u->firstlogin < 3 * 86400) {
		prints("  ������·, ���Ķ� Announce ������.\n");
	} else {
		prints("  ע����δ�ɹ�, ��ο���վ��վ����˵��.\n");
	}
}

void
check_uinfo(struct userec *u, int MUST)
{
	int changeIT = 0, changed = 0, pos = 2;
	char ans[5];

	while (1) {		// ����ǳ�
		changeIT = MUST || (strlen(u->username) < 2);
		/* || (strstr(u->username, "  "))||(strstr(u->username, "��")) */
		if (!changeIT) {
			if (changed) {
				pos++;
				changed = 0;
			}
			break;
		} else {
			MUST = 0;
			changed = 1;
		}
		getdata(pos, 0, "�����������ǳ� (Enter nickname): ", u->username, sizeof(u->username), DOECHO, YEA);
		strlcpy(uinfo.username, u->username, sizeof(uinfo.username));
		update_utmp();
	}

	while (1) {		// �����ʵ����
		changeIT = MUST || (strlen(u->realname) < 4) || (strstr(u->realname, "  ")) || (strstr(u->realname, "��"));
		if (!changeIT) {
			if (changed) {
				pos += 2;
				changed = 0;
			}
			break;
		} else {
			MUST = 0;
			changed = 1;
		}
		move(pos, 0);
		prints("������������ʵ���� (Enter realname):\n");
		getdata(pos + 1, 0, "> ", u->realname, sizeof(u->realname), DOECHO, YEA);
		trim(u->realname);
	}

	while (1) {		// ���ͨѶ��ַ
		changeIT = MUST || (strlen(u->address) < 10) || (strstr(u->address, "  ")) || (strstr(u->address, "��"));
		if (!changeIT) {
			if (changed) {
				pos += 2;
				changed = 0;
			}
			break;
		} else {
			MUST = 0;
			changed = 1;
		}
		move(pos, 0);
		prints("����������ͨѶ��ַ (Enter home address)��\n");
		getdata(pos + 1, 0, "> ", u->address, sizeof(u->address), DOECHO, YEA);
		trim(u->address);
	}

	while (1) {		// ����ʼ���ַ
		changeIT = MUST || invalid_email(u->email);
		if (!changeIT) {
#ifdef MAILCHECK
			if (changed) {
				pos += 4;
				changed = 0;
			}
#else
			if (changed) {
				pos += 3;
				changed = 0;
			}
#endif
			break;
		} else {
			MUST = 0;
			changed = 1;
		}
		move(pos, 0);
		prints("���������ʽΪ: \033[1;37muserid@your.domain.name\033[m\n");
#ifdef MAILCHECK
#ifndef CHECK_ZSUMAIL
		prints("\033[32m��վ�Ѿ��ṩ\033[33m�ʼ�ע��\033[32m����, ������ͨ���ʼ����ٵ�ͨ��ע����֤.\033[m\n");
#endif
#endif
		prints("������������䣺\n");
#ifdef MAILCHECK
#ifdef CHECK_ZSUMAIL
		getdata(pos + 2, 0, "> ", u->email, sizeof(u->email), DOECHO, YEA);
#else
		getdata(pos + 3, 0, "> ", u->email, sizeof(u->email), DOECHO, YEA);
#endif
#else
		getdata(pos + 2, 0, "> ", u->email, sizeof(u->email), DOECHO, YEA);
#endif
		trim(u->email);
	}

	while (1) {		// ����Ա�
		changeIT = MUST || (strchr("MF", u->gender) == NULL);
		if (changeIT) {
			getdata(pos, 0, "�����������Ա�: M.�� F.Ů [M]: ", ans, 2, DOECHO, YEA);
			u->gender = toupper((unsigned int)ans[0]);
			if (ans[0] == '\0')
				u->gender = 'M';
			pos++;
		} else {
			break;
		}
	}

	while (1) {		// ��������
		changeIT = MUST || (u->birthyear < 20) || (u->birthyear > 98);
		if (!changeIT) {
			if (changed) {
				pos++;
				changed = 0;
			}
			break;
		} else {
			MUST = 0;
			changed = 1;
		}
		getdata(pos, 0, "�����������������(��λ��): ", ans, 5, DOECHO, YEA);
		if (atoi(ans) < 1920 || atoi(ans) > 1998) {
			MUST = 1;
			continue;
		}
		u->birthyear = atoi(ans) - 1900;
	}

	while (1) {		// ��������
		changeIT = MUST || (u->birthmonth < 1) || (u->birthmonth > 12);
		if (!changeIT) {
			if (changed) {
				pos++;
				changed = 0;
			}
			break;
		} else {
			MUST = 0;
			changed = 1;
		}
		getdata(pos, 0, "���������������·�: ", ans, 3, DOECHO, YEA);
		u->birthmonth = atoi(ans);
	}

	while (1) {		// ��������
		changeIT = MUST ||
		    (valid_day(u->birthyear, u->birthmonth, u->birthday) == 0);

		if (!changeIT) {
			if (changed) {
				pos++;
				changed = 0;
			}
			break;
		} else {
			MUST = 0;
			changed = 1;
		}

		getdata(pos, 0, "���������ĳ�����: ", ans, 3, DOECHO, YEA);
		u->birthday = atoi(ans);
	}
}

int
onekeyactivation(struct userec *lookupuser, int usernum)
{
	int fore_user; /* �����¾��û� */
	struct denyheader dh;

	fore_user = (lookupuser->userlevel != PERM_BASIC);

	lookupuser->lastjustify = time(NULL);
	lookupuser->userlevel |= PERM_DEFAULT;
#ifdef AUTHHOST
	lookupuser->userlevel |= PERM_WELCOME;
#endif
   if (search_record("boards/.DENYLIST", &dh, sizeof(struct denyheader), 
							denynames, lookupuser->userid)) 
		lookupuser->userlevel &= ~PERM_POST;
	if (substitute_record(PASSFILE, lookupuser, sizeof(*lookupuser), usernum) == -1) {
     		presskeyfor("ϵͳ��������ϵϵͳά��Ա\n");
		return DIRCHANGED;
	}

	if(fore_user){ /* ���û� */
		mail_sysfile("etc/Activa_fore_users", lookupuser->userid, "��ϲ����������ڸ��س���Argo");
	}else{ /* ���û� */
		mail_sysfile("etc/smail", lookupuser->userid, "��ӭ���뱾վ����");
	}

	snprintf(genbuf, sizeof(genbuf), "�� %s ͨ�����ȷ��.", lookupuser->userid);
	do_securityreport(genbuf, lookupuser, YEA, NULL);  //Henry: �����ββ�һ��
	return DIRCHANGED;
}

int
uinfo_query(struct userec *u, int real, int unum)
{
	struct userec newinfo;
	char ans[3], buf[STRLEN], genbuf[STRLEN];
	char src[PATH_MAX + 1], dst[PATH_MAX + 1];
	int i, mailchanged = NA, fail = 0;
	time_t now;
	struct tm *tmnow;

	/* monster: ӵ��SYSOPȨ��ID���ϲ��ܱ��޸� */
	if (strcmp(currentuser.userid, "SYSOP")) {
		if ((strcmp(u->userid, currentuser.userid)) && (u->userlevel & PERM_SYSOP) && real) {
			getdata(t_lines - 1, 0, "��ѡ�� (0)���� [0]: ", ans, 2, DOECHO, YEA);
			return 0;
		}
	}

	memcpy(&newinfo, u, sizeof (currentuser));
	getdata(t_lines - 1, 0, real ?
		"��ѡ�� (0)���� (1)�޸����� (2)�趨���� (3)�� ID (4)����ϵͳ�ƺ� (5)����ID [0]: "
		: "��ѡ�� (0)���� (1)�޸����� (2)�趨���� (3)ѡǩ���� [0]: ",
		ans, 2, DOECHO, YEA);
	clear();
	refresh();
	now = time(NULL);
	tmnow = localtime(&now);

	i = 3;
	move(i++, 0);
	if (ans[0] != '3' || real)
		prints("ʹ���ߴ���: %s\n", u->userid);
	switch (ans[0]) {
	case '1':
		move(1, 0);
		outs("�������޸�,ֱ�Ӱ� <ENTER> ����ʹ�� [] �ڵ����ϡ�\n");
		snprintf(genbuf, sizeof(genbuf), "�ǳ� [%s]: ", u->username);
		do {
			getdata(5, 0, genbuf, buf, NICKNAMELEN, DOECHO, YEA);
			if (!buf[0])
				break;
		} while (strlen(buf) < 2);
		if (buf[0]) {
			strlcpy(newinfo.username, buf, sizeof(newinfo.username));
		}

		snprintf(genbuf, sizeof(genbuf), "��ʵ���� [%s]: ", u->realname);
		getdata(6, 0, genbuf, buf, NAMELEN, DOECHO, YEA);
		if (buf[0]) {
			strlcpy(newinfo.realname, buf, sizeof(newinfo.realname));
		}

		snprintf(genbuf, sizeof(genbuf), "��ס��ַ [%s]: ", u->address);
		do {
			getdata(7, 0, genbuf, buf, STRLEN - 10, DOECHO, YEA);
			if (!buf[0])
				break;
		} while (strlen(buf) < 10);
		if (buf[0]) {
			strlcpy(newinfo.address, buf, sizeof(newinfo.address));
		}

		snprintf(genbuf, sizeof(genbuf), "�������� [%s]: ", u->email);
		do {
			getdata(8, 0, genbuf, buf, 48, DOECHO, YEA);
		} while (buf[0] & invalid_email(buf));

		if (buf[0]) {
#ifdef MAILCHECK
#ifdef MAILCHANGED
			if (!strcmp(u->userid, currentuser.userid))
				mailchanged = YEA;
#endif
#endif
			strlcpy(newinfo.email, buf, sizeof(newinfo.email));
		}

		snprintf(genbuf, sizeof(genbuf), "�ն˻���̬ [%s]: ", u->termtype);
		getdata(9, 0, genbuf, buf, 16, DOECHO, YEA);
		if (buf[0]) {
			strlcpy(newinfo.termtype, buf, sizeof(newinfo.termtype));
		}

		snprintf(genbuf, sizeof(genbuf), "������ [%d]: ", u->birthyear + 1900);
		do {
			getdata(10, 0, genbuf, buf, 5, DOECHO, YEA);
		} while (buf[0] && (atoi(buf) <= 1920 || atoi(buf) >= 1998));
		if (buf[0]) {
			newinfo.birthyear = atoi(buf) - 1900;
		}

		snprintf(genbuf, sizeof(genbuf), "������ [%d]: ", u->birthmonth);
		do {
			getdata(11, 0, genbuf, buf, 3, DOECHO, YEA);
		} while (buf[0] && (atoi(buf) < 1 || atoi(buf) > 12));
		if (buf[0]) {
			newinfo.birthmonth = atoi(buf);
		}

		snprintf(genbuf, sizeof(genbuf), "������ [%d]: ", u->birthday);
		do {
			getdata(12, 0, genbuf, buf, 3, DOECHO, YEA);
		} while (valid_day(newinfo.birthyear, newinfo.birthmonth, newinfo.birthday) == NA);
		if (buf[0]) {
			newinfo.birthday = atoi(buf);
		}

		snprintf(genbuf, sizeof(genbuf), "�Ա� M.�� F.Ů [%c]: ", u->gender);
		for (;;) {
			getdata(13, 0, genbuf, buf, 2, DOECHO, YEA);

			if (buf[0] == 'M' || buf[0] == 'm') {
				newinfo.gender = 'M';
			} else if (buf[0] == 'F' || buf[0] == 'f') {
				newinfo.gender = 'F';
			} else if (buf[0] != '\0') {
				continue;
			}
				
			break;
		}

		i = 14;
		if (real) {
			snprintf(genbuf, sizeof(genbuf), "���ߴ��� [%d]: ", u->numlogins);
			getdata(i++, 0, genbuf, buf, 10, DOECHO, YEA);
			if (buf[0] != '\0' && atoi(buf) >= 0)
				newinfo.numlogins = atoi(buf);

			snprintf(genbuf, sizeof(genbuf), "���������� [%d]: ", u->numposts);
			getdata(i++, 0, genbuf, buf, 10, DOECHO, YEA);
			if (buf[0] != '\0' && atoi(buf) >= 0)
				newinfo.numposts = atoi(buf);
		}
		break;
	case '2':
		if (!real) {
			getdata(i++, 0, "������ԭ����: ", buf, PASSLEN, NOECHO, YEA);
			if (*buf == '\0' || !checkpasswd2(buf, &currentuser)) {
				prints("\n\n�ܱ�Ǹ, ����������벻��ȷ��\n");
				fail++;
				break;
			}
		}
		getdata(i++, 0, "���趨������: ", buf, PASSLEN, NOECHO, YEA);
		if (buf[0] == '\0') {
			prints("\n\n�����趨ȡ��, ����ʹ�þ�����\n");
			fail++;
			break;
		}
		strlcpy(genbuf, buf, PASSLEN);
		getdata(i++, 0, "����������������: ", buf, PASSLEN, NOECHO,
			YEA);
		if (strncmp(buf, genbuf, PASSLEN)) {
			prints("\n\n������ȷ��ʧ��, �޷��趨�����롣\n");
			fail++;
			break;
		}
		setpasswd(buf, &newinfo);
		break;
	case '3':
		if (!real) {
			snprintf(genbuf, sizeof(genbuf), "Ŀǰʹ��ǩ���� [%d]: ", u->signature);
			getdata(i++, 0, genbuf, buf, 16, DOECHO, YEA);
			if (atoi(buf) > 0)
				newinfo.signature = atoi(buf);
		} else {
			struct user_info uin;

			if (!strcmp(u->userid, currentuser.userid)) {
				outs("\n�Բ��𣬲��ܸ����Լ��� ID��");
				fail++;
			} else if (t_search_ulist(&uin, t_cmpuids, unum, NA, NA) != 0) {
				outs("\n�Բ��𣬸��û�Ŀǰ�������ϡ�");
				fail++;
			} else if (!strcmp(u->userid, "SYSOP")) {
				outs("\n�Բ������������޸� SYSOP �� ID��");
				fail++;
			} else {
				getdata(i++, 0, "�µ�ʹ���ߴ���: ", genbuf, IDLEN + 1, DOECHO, YEA);
				killwordsp(genbuf);
				if (genbuf[0] != '\0' && strcmp(genbuf, u->userid)) {
					if (strcasecmp(genbuf, u->userid) && getuser(genbuf, NULL)) {
						outs("\n�Բ���! �Ѿ���ͬ�� ID ��ʹ����\n");
						fail++;
					} else {
						int temp;
						char new_passwd[9];

						strlcpy(newinfo.userid, genbuf, sizeof(newinfo.userid));

						/* monster: �������һ��������û� */
						srandom(time(NULL));
						new_passwd[0] = 0;
						for (temp = 0; temp < 8; temp++)
							new_passwd[temp] = (random() % 95) + 33;
						prints("  �µ��û�����: %s\n", new_passwd);
						setpasswd(new_passwd, &newinfo);
					}
				} else {
					outs("\nʹ���ߴ���û�иı�\n");
					fail++;
				}
			}
		}
		break;
	case '4':		/* monster: �޸��û����/�ƺ� */
		clear();
		if (!real)
			return 0;
		prints("����ʹ����ϵͳ�ƺ�\n\n�趨ʹ���� '%s' ��ϵͳ�ƺ�\n\n", u->userid);
		newinfo.usertitle = setperms(newinfo.usertitle, "ϵͳ�ƺ�", NUMTITLES, showtitleinfo);
		break;
/* freestyler: 1������ */
	case '5':
		if( !real)
			return 0;
		clear();
		prints("���� %s", u->userid);

		if (askyn("ȷ��Ҫ�ı���", NA, YEA) == YEA)  
			onekeyactivation(u, unum);
	default:
		clear();
		return 0;
	}
	if (fail != 0) {
		pressreturn();
		clear();
		return 0;
	}
	if (askyn("ȷ��Ҫ�ı���", NA, YEA) == YEA) {
		if (real) {
			char secu[STRLEN];

			snprintf(secu, sizeof(secu), "�޸� %s �Ļ������ϻ����롣", u->userid);
			do_securityreport(secu, u, YEA, NULL);
		}
		if (strcmp(u->userid, newinfo.userid)) {
			snprintf(src, sizeof(src), "mail/%c/%s", mytoupper(u->userid[0]), u->userid);
			snprintf(dst, sizeof(dst), "mail/%c/%s", mytoupper(newinfo.userid[0]), newinfo.userid);
			rename(src, dst);
			sethomepath(src, u->userid);
			sethomepath(dst, newinfo.userid);
			rename(src, dst);
			sethomefile(src, u->userid, "register");
			unlink(src);
			sethomefile(src, u->userid, "register.old");
			unlink(src);
			setuserid(unum, newinfo.userid);
		}
		if (!strcmp(u->userid, currentuser.userid)) {
			strlcpy(uinfo.username, newinfo.username, sizeof(uinfo.username));
			#ifdef NICKCOLOR
			renew_nickcolor(newinfo.usertitle);
			#endif
			update_utmp();
		}
#ifdef MAILCHECK
#ifdef MAILCHANGED
		if ((mailchanged == YEA) && !HAS_PERM(PERM_SYSOP)) {
			if (!invalidaddr(newinfo.email)
			    && !invalid_email(newinfo.email)
			    && strstr(newinfo.email, BBSHOST) == NULL

#ifdef CHECK_ZSUMAIL
			    && (strstr(newinfo.email, "@student.sysu.edu.cn")
				|| strstr(newinfo.email, "@mail.sysu.edu.cn")
				|| strstr(newinfo.email, "@mail2.sysu.edu.cn")
				|| strstr(newinfo.email, "@mail3.sysu.edu.cn"))
			    
#endif
			) {
				strlcpy(u->email, newinfo.email, sizeof(u->email));
				send_regmail(u);
			} else {
				move(t_lines - 4, 0);
				prints("������ĵ����ʼ���ַ ��\033[1;33m%s\033[m��\n"
				       "ˡ���ܱ�վ���ϣ�ϵͳ����Ͷ��ע���ţ������������...",
				       newinfo.email);
				pressanykey();
				return 0;
			}
		}
#endif
#endif
		memcpy(u, &newinfo, sizeof(currentuser));
#ifdef MAILCHECK
#ifdef MAILCHANGED
		if ((mailchanged == YEA) && !HAS_PERM(PERM_SYSOP)) {
			newinfo.userlevel &= ~(PERM_LOGINOK | PERM_PAGE | PERM_MESSAGE | PERM_SENDMAIL);
			sethomefile(src, newinfo.userid, "register");
			sethomefile(dst, newinfo.userid, "register.old");
			rename(src, dst);
		}
#endif
#endif
//Added by cancel at 01.12.30
		if (!real) {
			set_safe_record();
			newinfo.numposts = currentuser.numposts;
			newinfo.userlevel = currentuser.userlevel;
			newinfo.numlogins = currentuser.numlogins;
			newinfo.stay = currentuser.stay;
			newinfo.usertitle = currentuser.usertitle;
		}
		substitute_record(PASSFILE, &newinfo, sizeof (newinfo), unum);
	}
	clear();
	return 0;
}

int
x_info(void)
{
	if (!guestuser) {
		modify_user_mode(GMENU);
		display_userinfo(&currentuser);
		uinfo_query(&currentuser, 0, usernum);
	}
	return 0;
}

#ifndef AUTHHOST
void
getfield(int line, char *info, char *desc, char *buf, int len)
{
	char prompt[STRLEN];

	move(line, 0);
	prints("  ԭ���趨: %-20.20s \033[1;32m(%s)\033[m", (buf[0] == '\0') ? "(δ�趨)" : buf, info);
	snprintf(prompt, sizeof(prompt), "  %s: ", desc);
	getdata(line + 1, 0, prompt, genbuf, len, DOECHO, YEA);
	if (genbuf[0] != '\0')
		strlcpy(buf, genbuf, len);
	clear_line(line);
	prints("  %s: %s\n", desc, buf);
}

int
x_fillform(void)
{
	char rname[NAMELEN], addr[STRLEN];
	char phone[STRLEN], dept[STRLEN], assoc[STRLEN];
	char ans[5], *mesg;
	FILE *fn;
	struct new_reg_rec regrec;

	if (guestuser)
		return -1;

	modify_user_mode(NEW);
	clear();
	move(2, 0);
	clrtobot();

	if (currentuser.userlevel & PERM_LOGINOK) {
		prints("���Ѿ���ɱ�վ��ʹ����ע������, ��ӭ���뱾վ������.");
		pressreturn();
		return 0;
	}

#ifdef PASSAFTERTHREEDAYS
	if (currentuser.lastlogin - currentuser.firstlogin < 3 * 86400) {
		prints("���״ε��뱾վδ������(72��Сʱ)...\n");
		prints("�����Ĵ���Ϥһ�£����������Ժ�����дע�ᵥ��");
		pressreturn();
		return 0;
	}
#endif

/*
	if ((fn = fopen("new_register", "r")) != NULL) {
		while (fgets(genbuf, STRLEN, fn) != NULL) {
			if ((ptr = strchr(genbuf, '\n')) != NULL)
				*ptr = '\0';
			if (strncmp(genbuf, "userid: ", 8) == 0
			    && strcmp(genbuf + 8, currentuser.userid) == 0) {
				fclose(fn);
				prints("վ����δ��������ע�����뵥, ���ȵ���������.");
				pressreturn();
				return 0;
			}
		}
		fclose(fn);
	}
*/

	if (search_record("new_register.rec", &regrec, sizeof (regrec),
			  cmpregrec, currentuser.userid)) {
		prints("վ����δ��������ע�����뵥, ���ȵ���������.");
		pressreturn();
		return 0;
	}

	strlcpy(rname, currentuser.realname, sizeof(rname));
	strlcpy(addr, currentuser.address, sizeof(addr));
	dept[0] = phone[0] = assoc[0] = '\0';
	while (1) {
		move(3, 0);
		clrtoeol();
		prints("%s ����, ���ʵ��д���µ�����:\n", currentuser.userid);
		getfield(6, "��������,��������ĺ�������ƴ��", "��ʵ����",
			 rname, NAMELEN);
		getfield(8, "ѧУϵ����λȫ��", "ѧУϵ��", dept, STRLEN);
		getfield(10, "����嵽���һ����ƺ���", "Ŀǰסַ", addr,
			 STRLEN);
		getfield(12, "����������ʱ��", "����绰", phone, STRLEN);
		getfield(14, "У�ѻ���ҵѧУ", "У �� ��", assoc, STRLEN);
		mesg = "���������Ƿ���ȷ, �� Q ����ע�� (Y/N/Quit)? [Y]: ";
		getdata(t_lines - 1, 0, mesg, ans, 3, DOECHO, YEA);
		if (ans[0] == 'Q' || ans[0] == 'q')
			return 0;
		if (ans[0] != 'N' && ans[0] != 'n')
			break;
	}
	strlcpy(currentuser.realname, rname, sizeof(currentuser.realname));
	strlcpy(currentuser.address, addr, sizeof(currentuser.address));
/*	if ((fn = fopen("new_register", "a")) != NULL) {
		now = time(NULL);
		getdatestring(now);
		fprintf(fn, "usernum: %d, %s\n", usernum, datestring);
		fprintf(fn, "userid: %s\n", currentuser.userid);
		fprintf(fn, "realname: %s\n", rname);
		fprintf(fn, "dept: %s\n", dept);
		fprintf(fn, "addr: %s\n", addr);
		fprintf(fn, "phone: %s\n", phone);
		fprintf(fn, "assoc: %s\n", assoc);
		fprintf(fn, "----\n");
		fclose(fn);
	}
*/
//Rewrite by cancel at 02.03.10 use new struct in register file
	regrec.regtime = time(NULL);
	regrec.usernum = usernum;
	strcpy(regrec.userid, currentuser.userid);
	strcpy(regrec.rname, rname);
	strcpy(regrec.dept, dept);
	strcpy(regrec.addr, addr);
	strcpy(regrec.phone, phone);
	strcpy(regrec.assoc, assoc);
	regrec.Sname = count_same_reg(currentuser.realname, '1', NA);
	regrec.Slog = count_same_reg(currentuser.lasthost, '3', NA);
	regrec.Sip = count_same_reg(currentuser.ident, '2', NA);
	regrec.mark = ' ';
	append_record("new_register.rec", &regrec, sizeof (regrec));
//Rewrite end

	setuserfile(genbuf, "mailcheck");
	if ((fn = fopen(genbuf, "w")) != NULL) {
		fprintf(fn, "usernum: %d\n", usernum);
		fclose(fn);
	}
	return 0;
}
#endif

#ifdef AUTHHOST
/* <------- Added by betterman 06/07/27 -------> */
static int col[3] = { 2, 24, 48 };
static int ccol[3] = { 0, 22, 46 };
static int pagetotal;

void
dept_choose_redraw(char *title, slist *list, int current, int pagestart, int pageend)
{
	int i, j, len1, len2;

	len1 = 39 - strlen(BoardName) / 2;
	len2 = len1 - strlen(title);

	clear();
	move(0, 0);
	prints("\033[1;33;44m%s\033[1;37m%*s%s%*s\033[m\n", title, len2, " ", BoardName, len1, " ");
	prints("ѡ��[\033[1;32mRtn\033[m] ȡ��[\033[1;32mq\033[m] \n");
	prints("\033[1;37;44m  ��  ��                ��  ��                  ��  ��                        \033[m\n");

	for (i = pagestart, j = 3; i < pageend && i < list->length; i++) {
		move(j, col[i % 3]);
		if(strlen(list->strs[i]) > 22)//����רҵ��̫��
			prints("%22s",list->strs[i]);
		else
			outs(list->strs[i]);
		if (i % 3 == 2) ++j;
	}

	update_endline();
}


int
dept_choose(char *buf, int buf_len, char *listfile, char *title/*, int (*callback)(int action, slist *list, char *uident)*/)
{
	slist *list;
	int current = 0, len, pagestart, pageend, oldx, oldy;

	if ((list = slist_init()) == NULL)
		return 0;
	slist_loadfromfile(list, listfile);

	oldy = 3;
	oldx = 0;
	pagetotal = (t_lines - 4) * 3;
	pagestart = 0;
	pageend = pagetotal - 1;
	dept_choose_redraw(title, list, current, pagestart, pageend);
	if (list->length > 0) {
		move(3, 0);
		outc('>');
	}

	while (1) {
		switch (egetch()) {
		case 'h':
			show_help("help/listedithelp");
			dept_choose_redraw(title, list, current, pagestart, pageend);
			break;
		case '\n':
			strlcpy(buf, list->strs[current], buf_len);
			goto out;
		case 'q':
			buf[0] = '\0';
			goto out;
		case KEY_HOME:
			if (list->length == 0 || current == 0)
				goto unchanged;
			current = 0;
			break;
		case KEY_END:
			if (list->length == 0 || current == list->length - 1)
				goto unchanged;
			current = list->length - 1;
			break;
		case KEY_LEFT:
			if (current == 0)
				goto unchanged;
			current--;
			break;
		case KEY_RIGHT:
			if (current == list->length - 1)
				goto unchanged;
			current++;
			break;
		case KEY_UP:
			if (current < 3)
				goto unchanged;
			current -= 3;
			break;
		case KEY_DOWN:
			if (current >= list->length - 3)
				goto unchanged;
			current += 3;
			break;
		case KEY_PGUP:
			if (current < pagetotal)
				goto unchanged;
			current -= pagetotal;
		case KEY_PGDN:
			if (current >= list->length - pagetotal)
				goto unchanged;
			current += pagetotal;
			break;
		default:
			goto unchanged;
		}

		if (current - current % pagetotal != pagestart || current - current % pagetotal + pagetotal != pageend) {
			pagestart = current - current % pagetotal;
			pageend = current - current % pagetotal + pagetotal;
			dept_choose_redraw(title, list, current, pagestart, pageend);
		} else {
			move(oldy, oldx);
			outc(' ');
		}

		if (list->length > 0) {
			oldy = 3 + (current - pagestart) / 3;
			oldx = ccol[current % 3];
			move(oldy, oldx);
			outc('>');
		}
unchanged: ;
	}

out:
	clear();
	len = list->length;
	slist_savetofile(list, listfile);
	slist_free(list);
	return len;
}
#endif

int
countauths(void *uentp_ptr, int unused)
{
	static int totalusers;
	struct userec *uentp = (struct userec *)uentp_ptr;

	if (uentp == NULL) {
		int c = totalusers;

		totalusers = 0;
		return c;
	}
	if (uentp->userlevel & (PERM_WELCOME | PERM_BASIC) && 
                    memcmp(uentp->reginfo,genbuf, MD5_PASSLEN) == 0) /* alarm: strncmp() , but not strcmp() */
		totalusers++;
	return 0;
}

int multi_auth_check(unsigned char auth[MD5_PASSLEN])
{
	strncpy(genbuf, (const char*) auth, MD5_PASSLEN);
	countmails(NULL, 0);
	if (apply_record(PASSFILE, countauths, sizeof (struct userec)) == -1) {
		return 0;
	}
	return countmails(NULL, 0);	
}

int check_auth_info(struct new_reg_rec *regrec/*, int graduate, char *realname, char *birthday, char *dept, char *account*/)
{
	FILE *fp;
	char name[STRLEN], dept[STRLEN], birth[STRLEN], account[STRLEN], birthday[11];
	char buf[256];
	char *ptr, *ptr2;
	int passover;

	clear();

	setauthfile(genbuf, regrec->graduate);

/* 06/10/10 betterman: 99��������й.���99������ */
	if(regrec->graduate == 2003)
		return -1;

/* 09/05/21 rovingcloud: ӦmonsonҪ�����Զ�ע��033521*, ������й */
	if(strstr(regrec->account, "033521") == regrec->account) 
		return -1;

	fp = fopen(genbuf,"r");
	if(fp == NULL){
		presskeyfor("û�иñ�ҵ��ݵ�����, ����ʧ��");
		return -1;
	}

	sprintf(birthday, "%.4d-%.2d-%.2d", regrec->birthyear + 1900, regrec->birthmonth, regrec->birthday);
	while(fgets(buf,256,fp) != NULL)
	{
		passover = 0;
		ptr = ptr2 = buf;
		if(*ptr == '\0' || *ptr == '\n' || *ptr == '\r' || *ptr == '#')
			continue;

		if((ptr = strchr(ptr,';')) == NULL)	
			continue;
		if (ptr - ptr2 - 1 > STRLEN)
			continue;
		strlcpy(name, ptr2, ptr - ptr2 +1);		
		if(strcmp(name, regrec->rname) != 0)
			continue;

		if((ptr2 = strchr(ptr+1,';')) == NULL)	
			continue;		
		if (ptr2 - ptr - 1 > STRLEN)
			continue;
		strlcpy(dept, ptr+1, ptr2 - ptr );
		if(strlen(dept) == 0 ){
			passover++;
		}else if(strcmp(dept, regrec->dept) != 0)
			continue;

		if((ptr = strchr(ptr2+1,';')) == NULL)	
			continue;		
		if (ptr - ptr2 - 1 > STRLEN)
			continue;
		strlcpy(account, ptr2+1, ptr - ptr2 );
		if(strlen(account) == 0 ){
			passover++;
		}else if(strcmp(account,regrec->account) != 0 &&
		            (strncmp(account,"0",1) !=0 || strcmp(account+1,regrec->account) != 0 ) ) /* ģ����λ��0 */
			continue;

		if((ptr2 = strchr(ptr+1,';')) == NULL)	
			continue;		
		if (ptr2 - ptr - 1 > STRLEN)
			continue;
		strlcpy(birth, ptr+1, ptr2 - ptr );
		if (strlen(birth) == 7){ //�����ֶ�����: 1985-11
			if(strncmp(birth,birthday,7) != 0 ) 
				continue;
		}else if(strlen(birth) == 10){  //�����ֶ�����: 1985-11-16
			if(strcmp(birth,birthday) != 0 && 
			   (strncmp(birth, birthday, 7)!=0 || strncmp(birth+8,"01",2)!=0 ) &&  /* ����¼������������01�Ŷ��û������������ͨ�� */
			   (strncmp(birth, birthday, 4)!=0 || strncmp(birth+5,"01-01",5)!=0  ) ) /* ����¼������������01-01���û����������ͨ�� */
				continue;
		}
		else /* ȱ�����ֶλ����ֶγ��Ȳ����Ϲ淶 */
			passover++;

		if(passover >= 2) /* ȱ���������������ֶ���ͨ�� */
			continue;
		
		igenpass(buf, regrec->rname, regrec->auth);
		return YEA;

	}
	fclose(fp);
	return NA;
}

#ifdef AUTHHOST
int
auth_fillform(struct userec *u, int unum)
{
	char buf[STRLEN];
	char secu[STRLEN];
	struct new_reg_rec regrec;
	struct userec newinfo;
	FILE *authfile;
	static int fail_count = 0;
	struct tm time_s;
	time_t time_t_i = time(NULL);
	int fore_user = (u->userlevel != PERM_BASIC); /* �����¾��û� */

	if (guestuser)
		return -1;
	
	if(strcmp(u->userid,"SYSOP") == 0)
		return YEA;

	modify_user_mode(NEW);
	clear();
	move(0, 0);
	//clrtobot();

	if(fail_count >= 3){
		presskeyfor("��֤������࣬��������֤\n");
		return NA;
	}

#ifdef PASSAFTERTHREEDAYS
	if (u->lastlogin - u->firstlogin < 3 * 86400) {
		prints("���״ε��뱾վδ������(72��Сʱ)...\n");
		prints("�����Ĵ���Ϥһ�£����������Ժ��ټ����ʺš�");
		pressreturn();
		return 0;
	}
#endif
	regrec.regtime = time(NULL);

	outs("����ɼ������ǰ, ϵͳ��Ҫ��������ݽ���ȷ��: ");
	getdata(1, 0, "��������������: ", buf, PASSLEN, NOECHO, YEA);
	if (*buf == '\0' || !checkpasswd2(buf, u))
		goto auth_fail;

	do {
		getdata(2, 0, "��ҵ���(4λ) : ", buf, 5, DOECHO, YEA);
	} while (buf[0] && (atoi(buf) <= 1920 || atoi(buf) >= 2020));
	if (!buf[0]) 
		goto auth_fail;
	gmtime_r(&time_t_i, &time_s);
	if(time_s.tm_mon >= 6){ /* second half year */
	  if(time_s.tm_year + 1900 < atoi(buf)){
	    presskeyfor("�Ǳ�ҵ����ʹ��У�����伤��\n");
	    goto auth_fail;
	  }	    
	}else{ /* first half year */
	  if(time_s.tm_year + 1900 <= atoi(buf)){
	    presskeyfor("�Ǳ�ҵ����ʹ��У�����伤��\n");
	    goto auth_fail;
	  }
	}
	regrec.graduate = atoi(buf);

	snprintf(genbuf, sizeof(genbuf), "��ʵ���� [%s]: ", u->realname);
	getdata(3, 0, genbuf, regrec.rname, NAMELEN, DOECHO, YEA);
	if (!regrec.rname[0]) 
		strlcpy(regrec.rname, u->realname, sizeof(u->realname));

	snprintf(genbuf, sizeof(genbuf), "������ [%d]: ", u->birthyear + 1900);
	do {
		getdata(4, 0, genbuf, buf, 5, DOECHO, YEA);
	} while (buf[0] && (atoi(buf) <= 1920 || atoi(buf) >= 1998));
	if (buf[0]) {
		regrec.birthyear = atoi(buf) - 1900;
	}else{
		regrec.birthyear = u->birthyear;
	}

	snprintf(genbuf, sizeof(genbuf), "������ [%d]: ", u->birthmonth);
	do {
		getdata(5, 0, genbuf, buf, 3, DOECHO, YEA);
	} while (buf[0] && (atoi(buf) < 1 || atoi(buf) > 12));
	if (buf[0]) {
		regrec.birthmonth = atoi(buf);
	}else{
		regrec.birthmonth = u->birthmonth;
	}

	snprintf(genbuf, sizeof(genbuf), "������ [%d]: ", u->birthday);
	do {
		getdata(6, 0, genbuf, buf, 3, DOECHO, YEA);
	} while (buf[0] && (atoi(buf) < 1 || atoi(buf) > 31));
	if (buf[0]) {
		regrec.birthday = atoi(buf);
	}else{
		regrec.birthday = u->birthday;
	}
	if(valid_day(regrec.birthyear, regrec.birthmonth, regrec.birthday) == NA) goto auth_fail;

	getdata(7, 0, "��������ѧ�� : ",
		regrec.account, STRLEN, DOECHO, YEA);
	if (!regrec.account[0])
		goto auth_fail;

	do {
		getdata(8, 0, "Ŀǰסַ,����嵽���һ����ƺ���: ",
			regrec.addr, STRLEN, DOECHO, YEA);
	} while (!regrec.addr[0] || strlen(regrec.addr) < 4 );
	
	do {
		getdata(9, 0, "����绰, ����������ʱ��: ",  
			regrec.phone, STRLEN, DOECHO, YEA);
	} while (!regrec.phone[0] || strlen(regrec.phone) < 8 );

	setdeptfile(genbuf, regrec.graduate);
	if (!dashf(genbuf)) {
		clear();
		prints("����ı�ҵ��ݻ���ϵͳ��û�ṩ�ñ�ҵ��ݵ�����,��ʹ�����伤��\n");
	}
	dept_choose(regrec.dept, sizeof(regrec.dept), genbuf, "רҵѡ��");
	if (!regrec.dept[0])
		goto auth_fail;	

	if(check_auth_info(&regrec) == YEA){

		if(multi_auth_check(regrec.auth) >= MULTIAUTH){
			presskeyfor("\n\033[1;32m ����: ��ǰ�����Ѿ��������! ��������!\033[m\n");
			return NA;
		}				

		memcpy(&newinfo, u, sizeof (newinfo));	
		set_safe_record();
		strcpy(newinfo.realname, regrec.rname);
		newinfo.birthyear = regrec.birthyear;		
		newinfo.birthmonth = regrec.birthmonth;
		newinfo.birthday = regrec.birthday;
      newinfo.lastjustify = time(NULL);
		strcpy(newinfo.address, regrec.addr);
		memcpy(newinfo.reginfo, regrec.auth, MD5_PASSLEN);
		newinfo.reginfo[MD5_PASSLEN] = 0;
		memcpy(u, &newinfo, sizeof(newinfo));
      newinfo.userlevel |= ( PERM_WELCOME | PERM_DEFAULT );
      if (deny_me_fullsite()) newinfo.userlevel &= ~PERM_POST;

      if (substitute_record(PASSFILE, &newinfo, sizeof (newinfo), unum) == -1) {
      	presskeyfor("ϵͳ��������ϵϵͳά��Ա\n");
			return NA;
      }else{
      	outs("������!! �����ʺ���˳������.\n");
			pressanykey();
			if(fore_user){ /* ���û� */
				mail_sysfile("etc/Activa_fore_users", u->userid, "��ϲ����������ڸ��س���Argo");
			}else{ /* ���û� */
				mail_sysfile("etc/smail", u->userid, "��ӭ���뱾վ����");
			}
			snprintf(secu, sizeof(secu), "���� %s ���ʺ�", newinfo.userid);
			//todo : �ı�����ʽ
			securityreport2(secu, YEA, NULL);
        	}
		
		setuserfile(buf, ".regpass"); 
		unlink(buf);
		setuserfile(buf, "auth");
		if (dashf(buf)) {
			setuserfile(genbuf, "auth.old");
			rename(buf, genbuf);
		}
		if ((authfile = fopen(buf, "w")) != NULL) {
			fprintf(authfile, "unum: %d, %s", unum,
				ctime(&(regrec.regtime)));
			fprintf(authfile, "userid: %s\n", u->userid);
			fprintf(authfile, "realname: %s\n", regrec.rname);
			fprintf(authfile, "dept: %s\n", regrec.dept);
			fprintf(authfile, "addr: %s\n", regrec.addr);
			fprintf(authfile, "phone: %s\n", regrec.phone);
			fprintf(authfile, "account: %s\n", regrec.account);
			//fprintf(authfile, "assoc: %s\n", regrec.assoc);			
			fprintf(authfile, "birthday: %d\n",regrec.birthyear + 1900);			
			fprintf(authfile, "birthday: %d\n",regrec.birthmonth);
			fprintf(authfile, "birthday: %d\n",regrec.birthday);
			fprintf(authfile, "graduate: %d\n",regrec.graduate);
			fprintf(authfile, "auth: %s\n",regrec.auth + 1);
			time_t now = time(NULL);
			fprintf(authfile, "Date: %s", ctime(&now));
			fprintf(authfile, "Approved: %s", u->userid);
			fclose(authfile);
		}
		return YEA;
	}else{	
		fail_count++;		
		prints("�� %d ����֤���� ! \n", fail_count);
		if(fail_count >= 3){ 
			regrec.regtime = time(NULL);
			regrec.usernum = usernum;
			strcpy(regrec.userid, u->userid);
			regrec.Sname = count_same_reg(regrec.rname, '1', NA);
			regrec.Slog = count_same_reg(u->lasthost, '3', NA);
			regrec.Sip = count_same_reg(u->ident, '2', NA);
			regrec.mark = ' ';
			if(append_record("new_register.rec", &regrec, sizeof (regrec)) == -1)
				outs("ϵͳ��������ϵϵͳά��Ա\n");
			else
				outs("���������Ѿ��ύ���ʺŹ���Ա���������ĵȺ����Ա��������\n�ڴ��ڼ䣬�����������м����ʺ�\n");
		}else{
			outs("���ύ�����Ϻ����ݿ��еĲ�������ص����˹�����������д\n");
		}
		pressreturn();
		return NA;
	}

auth_fail:
	clear();
	presskeyfor("\n\n�ܱ�Ǹ, ���ṩ�����ϲ���ȷ, ������ɸò���.");
	return NA;


}
#endif


// deardragon 2000.09.26 over

char *
getuserlevelstr(unsigned int level)
{
	if (level & PERM_ACCOUNTS)
		return "[\033[1;33m�ʺŹ���Ա\033[m]";
	if (level & PERM_OBOARDS)
		return "[\033[1;33m����������\033[m]";
	if (level & PERM_ACBOARD)
		return "[\033[1;33m����\033[m]";
	if (level & PERM_ACHATROOM)
		return "[\033[1;33m�����ҹ���Ա\033[m]";
	if (level & PERM_JUDGE)
		return "[\033[1;33m�ٲ�ίԱ��\033[m]";
/*
	if (level & PERM_INTERNSHIP)
		return "[\033[1;33mʵϰվ��\033[m]";
*/

	return (level & PERM_SUICIDE) ? "[\033[1;31m��ɱ��\033[m]" : "\0";
}

void
getusertitlestr(unsigned char title, char *name)
{
	name[0] = '\0';

	if (title & 0x08)
		strcat(name, "�������Ա ");
	if (title & 0x10)
		strcat(name, "��������Ա ");
	if (title & 0x20)
		strcat(name, "ϵͳά��Ա ");
	if (title & 0x40)
		strcat(name, "�ʺŹ���Ա ");
	if (title & 0x80)
		strcat(name, "�������Ա ");
	if (title & 0x01)
		strcat(name, "ϵͳ���� ");
	if (title & 0x02)
		strcat(name, "��Ѱ��� ");
	if (title & 0x04)
		strcat(name, "���������� ");

	if (name[0] != '\0')
		name[strlen(name) - 1] = '\0';
}

/* monster: ȷ���û���� (����ɱ, �޸����뱣������ǰ��Ҫ��ɵĲ���) */
int
confirm_userident(char *operation)
{
	char buf[STRLEN];
	int day, month, year;

	clear();
	set_safe_record();
	prints("����ɡ�%s������ǰ, ϵͳ��Ҫ��������ݽ���ȷ��: ", operation);

	getdata(3, 0, "��������������: ", buf, PASSLEN, NOECHO, YEA);
	if (*buf == '\0' || !checkpasswd2(buf, &currentuser))
		goto auth_fail;

	getdata(5, 0, "��������ʲô����? ", buf, NAMELEN, DOECHO, YEA);
	if (*buf == '\0' || strcmp(buf, currentuser.realname))
		goto auth_fail;

	getdata(7, 0, "������������ (��\033[1;32m��/��/��\033[m�ĸ�ʽ����, ��\033[1;32m11/7/1981\033[m): ",
		buf, 12, DOECHO, YEA);
	if (*buf == '\0' || sscanf(buf, "%d/%d/%d", &month, &day, &year) != 3)
		goto auth_fail;
	if (currentuser.birthyear + 1900 != year || currentuser.birthmonth != month || currentuser.birthday != day)
		goto auth_fail;

	clear();
	return YEA;

auth_fail:
	presskeyfor("\n\n�ܱ�Ǹ, ���ṩ�����ϲ���ȷ, ������ɸò���.");
	clear();
	return NA;
}
