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
#include "vote.h"

extern int page, range;
static char *vote_type[] = { "�Ƿ�", "��ѡ", "��ѡ", "����", "�ʴ�" };
struct votebal currvote;
char controlfile[STRLEN];
unsigned int result[33];
int vnum;
int voted_flag;
FILE *sug;

int choose(int update, int defaultn, void (*title_show)(void), int (*key_deal)(int, int, int), int (*list_show)(void), int (*read)(int, int));
int makevote(struct votebal *ball, char *bname);
int setvoteperm(struct votebal *ball);

int
cmpvuid(void *userid, void *uv_ptr)
{
	struct ballot *uv = (struct ballot *)uv_ptr;

	return !strcmp((char *)userid, uv->uid);
}

static void
setvoteflag(char *bname, int flag)
{
	int pos;
	struct boardheader fh;

	if ((pos = search_record(BOARDS, &fh, sizeof(fh), cmpbnames, bname)) > 0) {
		fh.flag = (flag == 0) ? (fh.flag & ~VOTE_FLAG) : (fh.flag | VOTE_FLAG);
		substitute_record(BOARDS, &fh, sizeof(fh), pos);
		refresh_bcache();
	}
}

void
makevdir(char *bname)
{
	char buf[PATH_MAX + 1];

	snprintf(buf, sizeof(buf), "vote/%s", bname);
	f_mkdir(buf, 0755);
}

void
setcontrolfile(void)
{
	setvotefile(controlfile, currboard, "control");
}

int
b_notes_edit(void)
{
	char buf[STRLEN], buf2[STRLEN];
	char ans[4];
	int aborted, notetype;

	if (!HAS_PERM(PERM_ACBOARD))    /* monster: suggested by MidautumnDay */
		if (!current_bm)
			return 0;

	clear();
	move(1, 0);
	outs("�༭/ɾ�����汸��¼");
	getdata(3, 0, "�༭��ɾ�� (0)�뿪 (1)һ�㱸��¼ (2)���ܱ���¼? [1]: ",
		ans, 2, DOECHO, YEA);
	if (ans[0] == '0')
		return FULLUPDATE;
	if (ans[0] != '2') {
		setvotefile(buf, currboard, "notes");
		notetype = 1;
	} else {
		setvotefile(buf, currboard, "secnotes");
		notetype = 2;
	}
	snprintf(buf2, sizeof(buf2), "[%s] (E)�༭ (D)ɾ�� (A)ȡ�� [E]: ",
		(notetype == 1) ? "һ�㱸��¼" : "���ܱ���¼");
	getdata(5, 0, buf2, ans, 2, DOECHO, YEA);
	if (ans[0] == 'A' || ans[0] == 'a') {
		aborted = -1;
	} else if (ans[0] == 'D' || ans[0] == 'd') {
		move(6, 0);
		snprintf(buf2, sizeof(buf2), "���Ҫɾ��[%s]", (notetype == 1) ? "һ�㱸��¼" : "���ܱ���¼");
		if (askyn(buf2, NA, NA) == YEA) {
			unlink(buf);
			move(7, 0);
			prints("[%s]�Ѿ�ɾ��...\n", (notetype == 1) ? "һ�㱸��¼" : "���ܱ���¼");
			pressanykey();
			aborted = 1;
		} else {
			aborted = -1;
		}
	} else {
		aborted = vedit(buf, EDIT_MODIFYHEADER);
	}

	if (aborted == -1) {
		pressreturn();
	} else {
		if (notetype == 1) {
			setvotefile(buf, currboard, "noterec");
		} else if (notetype == 2) {
			setvotefile(buf, currboard, "notespasswd");
		}
		unlink(buf);
	}
	return FULLUPDATE;
}

int
b_notes_passwd(void)
{
	FILE *pass;
	char passbuf[PASSLEN + 1], prepass[PASSLEN + 1];
	char buf[STRLEN];
	unsigned char genpass[MD5_PASSLEN];

	if (!current_bm) {
		return 0;
	}
	clear();
	move(1, 0);
	outs("�趨/����/ȡ�������ܱ���¼������...");
	setvotefile(buf, currboard, "secnotes");
	if (!dashf(buf)) {
		move(3, 0);
		outs("�����������ޡ����ܱ���¼����\n\n");
		outs("������ W ��á����ܱ���¼�������趨����...");
		pressanykey();
		return FULLUPDATE;
	}
	if (!check_notespasswd())
		return FULLUPDATE;
	getdata(3, 0, "�������µ����ܱ���¼����(Enter ȡ������): ", passbuf,
		PASSLEN, NOECHO, YEA);
	if (passbuf[0] == '\0') {
		setvotefile(buf, currboard, "notespasswd");
		unlink(buf);
		outs("�Ѿ�ȡ������¼���롣");
		pressanykey();
		return FULLUPDATE;
	}
	getdata(4, 0, "ȷ���µ����ܱ���¼����: ", prepass, PASSLEN, NOECHO,
		YEA);
	if (strcmp(passbuf, prepass)) {
		outs("\n���벻���, �޷��趨�����....");
		pressanykey();
		return FULLUPDATE;
	}
	setvotefile(buf, currboard, "notespasswd");
	if ((pass = fopen(buf, "w")) == NULL) {
		move(5, 0);
		outs("����¼�����޷��趨....");
		pressanykey();
		return FULLUPDATE;
	}
	genpasswd(passbuf, genpass);
	fwrite(genpass, MD5_PASSLEN, 1, pass);
	fclose(pass);
	move(5, 0);
	outs("���ܱ���¼�����趨���....");
	pressanykey();
	return FULLUPDATE;
}

int
b_suckinfile(FILE *fp, char *fname)
{
	char inbuf[256];
	FILE *sfp;

	if ((sfp = fopen(fname, "r")) == NULL)
		return -1;
	while (fgets(inbuf, sizeof(inbuf), sfp) != NULL)
		fputs(inbuf, fp);
	fclose(sfp);
	return 0;
}

/*Add by SmallPig*/

/* ��fname��2�п�ʼ��������ӵ� fp ���� */
int
catnotepad(FILE *fp, char *fname)
{
	char inbuf[256];
	FILE *sfp;
	int count;

	count = 0;
	if ((sfp = fopen(fname, "r")) == NULL) {
		fprintf(fp,
			"\033[1;34m  ��\033[44m__________________________________________________________________________\033[m \n\n");
		return -1;
	}
	while (fgets(inbuf, sizeof(inbuf), sfp) != NULL) {
		if (count != 0)
			fputs(inbuf, fp);
		else
			count++;
	}
	fclose(sfp);
	return 0;
}

int
b_closepolls(void)
{
	char buf[80];
	time_t now, nextpoll;
	int i, end;

	now = time(NULL);
	resolve_boards();

	if (now < brdshm->pollvote) {
		return 0;
	}
	move(t_lines - 1, 0);
	outs("�Բ���ϵͳ�ر�ͶƱ�У����Ժ�...");
	refresh();
	nextpoll = now + 7 * 3600;
	brdshm->pollvote = nextpoll;
	strcpy(buf, currboard);
	for (i = 0; i < brdshm->number; i++) {
		strcpy(currboard, (&bcache[i])->filename);
		setcontrolfile();
		end = get_num_records(controlfile, sizeof(currvote));
		for (vnum = end; vnum >= 1; vnum--) {
			time_t closetime;

			get_record(controlfile, &currvote, sizeof(currvote), vnum);
			closetime = currvote.opendate + currvote.maxdays * 86400;
			if (now > closetime) {
				mk_result(vnum);
			} else if (nextpoll > closetime) {
				nextpoll = closetime + 300;
				brdshm->pollvote = nextpoll;
			}
		}
	}
	strcpy(currboard, buf);

	return 0;
}

int
count_result(void *bal_ptr, int unused)
{
	int i, j = 0;
	char choices[33];
	struct ballot *bal = (struct ballot *)bal_ptr;

	if (bal->msg[0][0] != '\0') {
		if (currvote.type == VOTE_ASKING) {
			fprintf(sug, "\033[1m%s \033[m���������£�\n", bal->uid);
		} else {
			if (currvote.type != VOTE_VALUE) {
				memset(choices, 0, sizeof(choices));
		                for (i = 0; i < 32; i++) {
                		        if ((bal->voted >> i) & 1) {
						choices[j] = 'A' + i;
						++j;
					}
				}
			}

			if (j == 0 || !currvote.report) {
				fprintf(sug, "\033[1m%s\033[m �Ľ������£�\n", bal->uid);
			} else {
				fprintf(sug, "\033[1m%s\033[m ��ѡ��Ϊ \033[1;36m%s\033[m ���������£�\n", bal->uid, choices);
			}
		}
		for (i = 0; (bal->msg[i] && i < 3); i++) {
			fprintf(sug, "%s\n", bal->msg[i]);
		}
		fputc('\n', sug);
	}

	++result[32];
	if (currvote.type != VOTE_ASKING) {
		if (currvote.type != VOTE_VALUE) {
			for (i = 0; i < 32; i++) {
				if ((bal->voted >> i) & 1)
					(result[i])++;
			}
		} else {
			result[31] += bal->voted;
			result[(bal->voted * 10) / (currvote.maxtkt + 1)]++;
		}
	}
	return 0;
}

int
count_voteip(void *bal_ptr, int unused)
{
	time_t t;
	struct ballot *bal = (struct ballot *)bal_ptr;

	t = bal->votetime;
	fprintf(sug, "%-12s    %-16s   %24.24s\n", bal->uid, bal->votehost, ctime(&t));
	return 0;
}

void
get_result_title(void)
{
	char buf[PATH_MAX + 1];

	getdatestring(currvote.opendate);
	fprintf(sug, "�� ͶƱ�����ڣ�\033[1m%s\033[m  ���\033[1m%s\033[m\n",
		datestring, vote_type[currvote.type - 1]);
	fprintf(sug, "�� ���⣺\033[1m%s\033[m\n", currvote.title);
	if (currvote.type == VOTE_VALUE)
		fprintf(sug, "�� �˴�ͶƱ��ֵ���ɳ�����\033[1m%d\033[m\n\n",
			currvote.maxtkt);
	fprintf(sug, "�� Ʊѡ��Ŀ������\n\n");
	snprintf(buf, sizeof(buf), "vote/%s/desc.%d", currboard, (int)currvote.opendate);
	b_suckinfile(sug, buf);
}

/* monster: ���ɲ�ɫ����ͼ */
void
make_colorbar(int ratio, char *bar)
{
	bar[0] = 0;

	while (ratio >= 10) {
		ratio -= 10;
		strcat(bar, "��");
	}

	switch (ratio) {
	case 9:
	case 8:
		strcat(bar, "��");
		break;
	case 7:
		strcat(bar, "��");
		break;
	case 6:
		strcat(bar, "��");
		break;
	case 5:
		strcat(bar, "��");
		break;
	case 4:
		strcat(bar, "��");
		break;
	case 3:
		strcat(bar, "��");
		break;
	case 2:
	case 1:
		strcat(bar, "��");
		break;
	}
}

int
mk_result(int num)
{
	char fname[PATH_MAX + 1], nname[PATH_MAX + 1], sugname[PATH_MAX + 1];
	char title[TITLELEN], color_bar[21];
	int i, ratio;
	unsigned int total = 0;

	setcontrolfile();
	snprintf(fname, sizeof(fname), "vote/%s/flag.%d", currboard, (int)currvote.opendate);
/*      count_result(NULL); */
	sug = NULL;
	snprintf(sugname, sizeof(sugname), "vote/%s/tmp.%d", currboard, uinfo.pid);
	if ((sug = fopen(sugname, "w")) == NULL)
		goto error_process;
	memset(result, 0, sizeof(result));

	fprintf(sug,
		"\033[1;44;36m������������������������������ʹ����%s������������������������������\033[m\n\n\n",
		(currvote.type != VOTE_ASKING) ? "��������" : "�˴ε�����");

	if (apply_record(fname, count_result, sizeof(struct ballot)) == -1)
		report("Vote apply flag error");
	fclose(sug);

	snprintf(nname, sizeof(nname), "vote/%s/results", currboard);
	if ((sug = fopen(nname, "w")) == NULL)
		goto error_process;

	get_result_title();
	fprintf(sug, "** ͶƱ���:\n\n");
	if (currvote.type == VOTE_VALUE) {
		total = result[32];
		for (i = 0; i < 10; i++) {
			/* monster: ʹ�ò�ɫ����ͼ����ʾͶƱ���, based on henry's idea */
			ratio = (total <= 1) ? (result[i] * 100) : (result[i] * 100) / total;
			make_colorbar(ratio, color_bar);
			fprintf(sug,
				"\033[1m  %4d\033[m �� \033[1m%4d\033[m ֮���� \033[1m%4d\033[m Ʊ \033[1;%dm%s\033[m\033[1m%d%%\033[m\n",
				(i * currvote.maxtkt) / 10 + ((i == 0) ? 0 : 1),
				((i + 1) * currvote.maxtkt) / 10, result[i],
				31 + i % 7, color_bar, ratio);

		}
		fprintf(sug, "�˴�ͶƱ���ƽ��ֵ��: \033[1m%d\033[m\n", (total <= 1) ? (result[31]) : (result[31] / total));
	} else if (currvote.type == VOTE_ASKING) {
		total = result[32];
	} else {
		for (i = 0; i < currvote.totalitems; i++) {
			total += result[i];
		}
		for (i = 0; i < currvote.totalitems; i++) {
			ratio = (result[i] * 100) / ((total <= 0) ? 1 : total);
			make_colorbar(ratio, color_bar);
			fprintf(sug,
				"(%c) %-40s  %4d Ʊ \033[1;%dm%s\033[m\033[1m\033[1m%d%%\033[m\n",
				'A' + i, currvote.items[i], result[i],
				31 + i % 7, color_bar, ratio);
		}
	}

	fprintf(sug, "\nͶƱ������ = \033[1m%d\033[m ��\n", result[32]);
	fprintf(sug, "ͶƱ��Ʊ�� =\033[1m %d\033[m Ʊ\n\n", total);
	b_suckinfile(sug, sugname);
	unlink(sugname);

	if (currvote.report) {
		fprintf(sug, "\n\033[1;44;36m��������������������������������ͶƱ�����ϩ���������������������������������\033[m\n\n");
		fprintf(sug, "ͶƱ��          ͶƱIP             ͶƱʱ��\n");
		fprintf(sug, "============================================================\n");
		apply_record(fname, count_voteip, sizeof(struct ballot));
	}

	add_syssign(sug);
	fclose(sug);

	if (!strcmp(currboard, SYSTEM_VOTE)) {
		strlcpy(title, "[����] ȫվͶƱ���", sizeof(title));
	} else {
		snprintf(title, sizeof(title), "[����] %s ���ͶƱ���", currboard);
	}

	if (can_post_vote(currboard))
		postfile(nname, VOTE_BOARD, title, 1);
	if (strcmp(currboard, VOTE_BOARD) != 0)
		postfile(nname, currboard, title, 1);
	dele_vote(num);
	return 0;

error_process:
	if (askyn("\033[1;31m����ͶƱʱ��������, �Ƿ�ɾ����ͶƱ", NA, NA) == YEA)
		dele_vote(num);
	pressanykey();
	return -1;
}

int
get_vitems(struct votebal *bal)
{
	int num;
	char buf[STRLEN];

	move(3, 0);
	outs("�����������ѡ����, �� ENTER ����趨.\n");
	for (num = 0; num < 32; num++) {
		snprintf(buf, sizeof(buf), "%c) ", num + 'A');
		getdata((num & 15) + 4, (num >> 4) * 40, buf, bal->items[num], 36, DOECHO, YEA);
		if (bal->items[num][0] == '\0') {
			if (num == 0) {
				num = -1;
			} else {
				break;
			}
		}
	}
	bal->totalitems = num;
	return num;
}

/* monster: ��鵱ǰ���Ƿ񹫿�ͶƱ��� */
int
can_post_vote(char *board)
{
	struct boardheader *bp;

	/* monster: ȫվͶƱ���빫��ͶƱ��� */
	if (!strcmp(board, SYSTEM_VOTE)) {
		return YEA;
	}

	bp = getbcache(board);
	return (bp->flag & BRD_NOPOSTVOTE) ? NA : YEA;
}

int
vote_maintain(char *bname)
{
	char buf[STRLEN], ans[2];
	struct votebal *ball = &currvote;

	setcontrolfile();
	if (!HAS_PERM(PERM_SYSOP | PERM_OVOTE))
		if (!current_bm)
			return 0;

	stand_title("����ͶƱ��");
	makevdir(bname);
	for (;;) {
		getdata(2, 0,
			"(1)�Ƿ� (2)��ѡ (3)��ѡ (4)��ֵ (5)�ʴ� (6)ȡ�� [6]: ",
			ans, 2, DOECHO, YEA);
		ans[0] -= '0';
		if (ans[0] < 1 || ans[0] > 5) {
			outs("ȡ���˴�ͶƱ\n");
			return FULLUPDATE;
		}
		ball->type = (int) ans[0];
		break;
	}
	ball->opendate = time(NULL);
	outc('\n');
	ball->report = askyn("�Ƿ��¼ͶƱ������ ��ͶƱIP��ʱ�䣩", NA, NA);
	if (makevote(ball, bname))
		return FULLUPDATE;
	setvoteflag(currboard, 1);
	clear();
	strcpy(ball->userid, currentuser.userid);
	if (append_record(controlfile, ball, sizeof(*ball)) == -1) {
		outs("�޷�����ͶƱ������ϵͳά����ϵ");
		b_report("Append Control file Error!!");
	} else {
		char votename[PATH_MAX + 1];
		int i;

		b_report("OPEN");
		outs("ͶƱ�俪���ˣ�\n");
		range++;
		if (!can_post_vote(currboard)) {
			pressreturn();
			return FULLUPDATE;
		}
		snprintf(votename, sizeof(votename), "tmp/votetmp.%s.%05d", currentuser.userid, uinfo.pid);
		if ((sug = fopen(votename, "w")) != NULL) {
			if (!strcmp(currboard, SYSTEM_VOTE)) {
				snprintf(buf, sizeof(buf), "[֪ͨ] ȫվͶƱ��%s", ball->title);
			} else {
				snprintf(buf, sizeof(buf), "[֪ͨ] %s �ٰ�ͶƱ��%s", currboard, ball->title);
			}
			get_result_title();
			if (ball->type != VOTE_ASKING &&
			    ball->type != VOTE_VALUE) {
				fprintf(sug, "\n��\033[1mѡ������\033[m��\n");
				for (i = 0; i < ball->totalitems; i++) {
					fprintf(sug, "(\033[1m%c\033[m) %-40s\n",
						'A' + i, ball->items[i]);
				}
			}
			add_syssign(sug);
			fclose(sug);

			if (can_post_vote(currboard))
				postfile(votename, VOTE_BOARD, buf, 1);
			if (strcmp(currboard, VOTE_BOARD) != 0)
				postfile(votename, currboard, buf, 1);

			unlink(votename);
		}
	}
	pressreturn();
	return FULLUPDATE;
}

int
makevote(struct votebal *ball, char *bname)
{
	char buf[PATH_MAX + 1];

	outs("\n�밴�κμ���ʼ�༭�˴� [ͶƱ������]: \n");
	igetkey();
	setvotefile(genbuf, bname, "desc");
	snprintf(buf, sizeof(buf), "%s.%d", genbuf, (int)ball->opendate);
	if (vedit(buf, EDIT_MODIFYHEADER) == -1) {
		clear();
		outs("ȡ���˴�ͶƱ�趨\n");
		pressreturn();
		return 1;
	}

	clear();
	do {
		getdata(0, 0, "�˴�ͶƱ��������[1]: ", buf, 3, DOECHO, YEA);
		if (buf[0] == '\0') {
			ball->maxdays = 1;
			break;
		}
	} while ((ball->maxdays = atoi(buf)) <= 0);

	while (1) {
		getdata(1, 0, "ͶƱ��ı���: ", ball->title, TITLELEN, DOECHO, YEA);
		if (killwordsp(ball->title) != 0)
			break;
		bell();
	}

	switch (ball->type) {
	case VOTE_YN:
		ball->maxtkt = 0;
		strcpy(ball->items[0], "�޳�  ���ǵģ�");
		strcpy(ball->items[1], "���޳ɣ����ǣ�");
		strcpy(ball->items[2], "û������������");
		ball->maxtkt = 1;
		ball->totalitems = 3;
		break;
	case VOTE_SINGLE:
		get_vitems(ball);
		ball->maxtkt = 1;
		break;
	case VOTE_MULTI:
		get_vitems(ball);
		for (;;) {
			snprintf(buf, sizeof(buf), "һ������༸Ʊ? [%d]: ", ball->totalitems);
			getdata(21, 0, buf, buf, 5, DOECHO, YEA);
			ball->maxtkt = atoi(buf);
			if (ball->maxtkt <= 0)
				ball->maxtkt = ball->totalitems;
			if (ball->maxtkt > ball->totalitems)
				continue;
			break;
		}
		if (ball->maxtkt == 1)
			ball->type = VOTE_SINGLE;
		break;
	case VOTE_VALUE:
		for (;;) {
			getdata(3, 0, "������ֵ��󲻵ó��� [100]: ", buf, 4,
				DOECHO, YEA);
			ball->maxtkt = atoi(buf);
			if (ball->maxtkt <= 0)
				ball->maxtkt = 100;
			break;
		}
		break;
	case VOTE_ASKING:
		ball->maxtkt = 0;
		currvote.totalitems = 0;
		break;
	default:
		ball->maxtkt = 1;
		break;
	}
	return 0;
}

int
setvoteperm(struct votebal *ball)
{
	int flag = NA, changeit = YEA;
	char buf[6], msgbuf[STRLEN];

	if (!HAS_PERM(PERM_SYSOP | PERM_OVOTE))
		return 0;

	clear();
	prints("��ǰͶƱ���⣺[\033[1;33m%s\033[m]", ball->title);
	outs("\n���Ǳ�վ��ͶƱ����Ա�������Զ�ͶƱ����ͶƱ�����趨\n"
	     "\nͶƱ�����趨�����������ƣ����������ƿ������ۺ��޶���\n"
	     "    (1) Ȩ������ [\033[32m�û�����ӵ��ĳЩȨ�޲ſ���ͶƱ\033[m]\n"
	     "    (2) ͶƱ���� [\033[35m������ͶƱ�����е��˲ſ���ͶƱ\033[m]\n"
	     "    (3) ������� [\033[36m�û���������һ���������ſ�ͶƱ\033[m]\n"
	     "\n����ϵͳ��һ��һ��������������ͶƱ�����趨��\n\n");
	if (askyn("��ȷ��Ҫ��ʼ�Ը�ͶƱ����ͶƱ�����趨��", NA, NA) == NA)
		return 0;
	flag = (ball->level & ~(LISTMASK | VOTEMASK)) ? YEA : NA;
	prints("\n(\033[36m1\033[m) Ȩ������  [Ŀǰ״̬: \033[32m%s\033[m]\n\n",
	     flag ? "������" : "������");
	if (flag == YEA) {
		if (askyn("��Ҫȡ��ͶƱ�ߵ�Ȩ��������", NA, NA) == YEA)
			flag = NA;
		else
			changeit =
			    askyn("����Ҫ�޸�ͶƱ�ߵ�Ȩ��������", NA, NA);
	} else
		flag = askyn("��ϣ��ͶƱ�߱���߱�ĳ��Ȩ����", NA, NA);
	if (flag == NA) {
		ball->level = ball->level & (LISTMASK | VOTEMASK);
		outs("\n\n��ǰͶƱ\033[32m��Ȩ������\033[m��");
	} else if (changeit == NA) {
		outs("\n��ǰͶƱ\033[32m��������Ȩ������\033[m��");
	} else {
		clear();
		outs("\n�趨\033[32mͶƱ�߱���\033[m��Ȩ�ޡ�");
		ball->level = setperms(ball->level, "ͶƱȨ��", NUMPERMS, showperminfo);
		move(1, 0);
		if (ball->level & ~(LISTMASK | VOTEMASK)) {
			outs("���Ѿ�\033[32m�趨��\033[mͶƱ�ı���Ȩ�ޣ�ϵͳ���������趨");
		} else {
			outs("������\033[32mȡ����\033[mͶƱ��Ȩ���޶���ϵͳ���������趨");
		}
	}
	outs("\n\n\n\033[33m����һ����\033[m����������ͶƱ���Ƶ��趨\n");
	pressanykey();
	clear();
	flag = (ball->level & LISTMASK) ? YEA : NA;
	prints("\n(\033[36m2\033[m) ͶƱ����  [Ŀǰ״̬: \033[32m%s\033[m]\n\n",
	       flag ? "�������е� ID �ſ�ͶƱ" : "������������");
	if (askyn("����ı䱾���趨��", NA, NA) == YEA) {
		if (flag) {
			ball->level &= ~LISTMASK;
			outs("\n�����趨��\033[32m��ͶƱ����ͶƱ�������޶�\033[m\n");
		} else {
			ball->level |= LISTMASK;
			outs("\n�����趨��\033[32mֻ���������е� ID �ſ���ͶƱ\033[m\n");
			outs("[\033[33m��ʾ\033[m]�趨��Ϻ��� Ctrl+K ���༭ͶƱ����\n");
		}
	} else {
		prints("\n�����趨��[\033[32m�����趨\033[m]%s\n",
		       flag ? "�������е� ID �ſ���ͶƱ" : "������������");
	}
	flag = (ball->level & VOTEMASK) ? YEA : NA;
	prints("\n(\033[36m3\033[m) �������  [Ŀǰ״̬: \033[32m%s\033[m]\n\n",
	       flag ? "����������" : "������������");
	if (flag == YEA) {
		if (askyn("����ȡ��ͶƱ�ߵ���վ��������վʱ��ȵ�������", NA, NA) == YEA) {
			flag = NA;
		} else {
			changeit = askyn("���������趨����������", NA, NA);
		}
	} else {
		changeit = YEA;
		flag = askyn("ͶƱ���Ƿ���Ҫ�ܵ���վ����������������", NA, NA);
	}
	if (flag == NA) {
		ball->level &= ~VOTEMASK;
		outs("\n�����趨��\033[32m���������޶�\033[m\n\nͶƱ�����趨���\n\n");
	} else if (changeit == NA) {
		outs("\n�����趨��\033[32m���������޶�\033[m\n\nͶƱ�����趨���\n\n");
	} else {
		ball->level |= VOTEMASK;

		snprintf(msgbuf, sizeof(msgbuf), "��վ�������ٴﵽ���ٴΣ�[%d]: ", ball->x_logins);
		do {
			getdata(11, 4, msgbuf, buf, 5, DOECHO, YEA);
		} while (buf[0] == '\0' || (ball->x_logins = atoi(buf)) < 0);

		snprintf(msgbuf, sizeof(msgbuf), "��������������ж���ƪ��[%d]: ", ball->x_posts);
		do {
			getdata(12, 4, msgbuf, buf, 5, DOECHO, YEA);
		} while (buf[0] == '\0' || (ball->x_posts = atoi(buf)) < 0);

		snprintf(msgbuf, sizeof(msgbuf), "�ڱ�վ���ۼ���վʱ�������ж���Сʱ��[%d]: ", ball->x_stay);
		do {
			getdata(13, 4, msgbuf, buf, 5, DOECHO, YEA);
		} while (buf[0] == '\0' || (ball->x_stay = atoi(buf)) < 0);
	
		snprintf(msgbuf, sizeof(msgbuf), "���ʺ�ע��ʱ�������ж����죿[%d]: ", ball->x_live);
		do {
			getdata(14, 4, msgbuf, buf, 5, DOECHO, YEA);
		} while (buf[0] == '\0' || (ball->x_live = atoi(buf)) < 0);

		outs("\nͶƱ�����趨���\n\n");
	}
	if (askyn("��ȷ��Ҫ�޸�ͶƱ������", NA, NA) == YEA) {
		snprintf(msgbuf, sizeof(msgbuf), "�޸� %s ��ͶƱ[ͶƱ����]", currboard);
		securityreport(msgbuf);
		return 1;
	} else {
		return 0;
	}
}

int
vote_flag(char *bname, char val, int mode)
{
	char buf[PATH_MAX + 1], flag;
	int fd, num, size;

	num = usernum - 1;
	switch (mode) {
	case 2:
		snprintf(buf, sizeof(buf), "reclog/Welcome.rec");     /* ��վ�� Welcome ���� */
		break;
	case 1:
		setvotefile(buf, bname, "noterec");        /* ����������¼����� */
		break;
	default:
		return -1;
	}
	if (num >= MAXUSERS) {
		report("Vote Flag, Out of User Numbers");
		return -1;
	}
	if ((fd = open(buf, O_RDWR | O_CREAT, 0600)) == -1) {
		return -1;
	}
	f_exlock(fd);
	size = (int) lseek(fd, 0, SEEK_END);
	memset(buf, 0, sizeof(buf));
	while (size <= num) {
		write(fd, buf, sizeof(buf));
		size += sizeof(buf);
	}
	lseek(fd, (off_t) num, SEEK_SET);
	read(fd, &flag, 1);
	if ((flag == 0 && val != 0)) {
		lseek(fd, (off_t) num, SEEK_SET);
		write(fd, &val, 1);
	}
	f_unlock(fd);
	close(fd);
	return flag;
}

int
vote_check(int bits)
{
	int i, count;

	for (i = count = 0; i < 32; i++) {
		if ((bits >> i) & 1)
			count++;
	}
	return count;
}

unsigned int
showvoteitems(unsigned int pbits, int i, int flag)
{
	int count;

	if (flag == YEA) {
		count = vote_check(pbits);
		if (count > currvote.maxtkt)
			return NA;
		move(2, 0);
		clrtoeol();
		prints("���Ѿ�Ͷ�� \033[1m%d\033[m Ʊ", count);
	}
	move(i + 6 - ((i > 15) ? 16 : 0), 0 + ((i > 15) ? 40 : 0));
	prints("%c.%2.2s%-36.36s", 'A' + i,
		((pbits >> i) & 1 ? "��" : "  "), currvote.items[i]);

	refresh();
	return YEA;
}

void
show_voteing_title(void)
{
	time_t closedate;
	char buf[PATH_MAX + 1];

	if (currvote.type != VOTE_VALUE && currvote.type != VOTE_ASKING) {
		snprintf(buf, sizeof(buf), "��ͶƱ��: \033[1m%d\033[m Ʊ", currvote.maxtkt);
	} else {
		buf[0] = '\0';
	}
	closedate = currvote.opendate + currvote.maxdays * 86400;
	getdatestring(closedate);
	prints("ͶƱ��������: \033[1m%s\033[m  %s  %s\n",
	       datestring, buf, (voted_flag) ? "(\033[5;1m�޸�ǰ��ͶƱ\033[m)" : "");
	prints("ͶƱ������: \033[1m%-50s\033[m����: \033[1m%s\033[m \n", currvote.title,
	       vote_type[currvote.type - 1]);
	if (currvote.report) {
		outs("\033[1;31mע�⣺��ͶƱ����¼ͶƱ������\033[m\n");
	}
}

int
getsug(struct ballot *uv)
{
	int i, line;

	clear();
	if (currvote.type == VOTE_ASKING) {
		show_voteing_title();
		line = 3;
		outs("��������������(����):\n");
	} else {
		line = 1;
		outs("����������������(����):\n");
	}
	move(line, 0);
	for (i = 0; i < 3; i++) {
		prints(": %s\n", uv->msg[i]);
	}
	for (i = 0; i < 3; i++) {
		getdata(line + i, 0, ": ", uv->msg[i], STRLEN - 2, DOECHO, NA);
		if (uv->msg[i][0] == '\0')
			break;
	}
	return i;
}

int
multivote(struct ballot *uv)
{
	unsigned int i;

	i = uv->voted;
	move(0, 0);
	show_voteing_title();
	uv->voted = setperms(uv->voted, "ѡƱ", currvote.totalitems, showvoteitems);
	if (uv->voted == i)
		return -1;
	return 1;
}

int
valuevote(struct ballot *uv)
{
	unsigned int chs;
	char buf[10];

	chs = uv->voted;
	move(0, 0);
	show_voteing_title();
	prints("�˴������ֵ���ܳ��� \033[1m%d\033[m", currvote.maxtkt);
	if (uv->voted != 0) {
		snprintf(buf, sizeof(buf), "%d", uv->voted);
	} else {
		memset(buf, 0, sizeof(buf));
	}
	do {
		getdata(3, 0, "������һ��ֵ? [0]: ", buf, 5, DOECHO, NA);
		uv->voted = abs(atoi(buf));
	} while (uv->voted > currvote.maxtkt && buf[0] != '\n' && buf[0] != '\0');
	if (buf[0] == '\n' || buf[0] == '\0' || uv->voted == chs)
		return -1;
	return 1;
}

int
user_vote(int num, int unused)
{
	char fname[PATH_MAX + 1], bname[PATH_MAX + 1], buf[PATH_MAX + 1];
	struct ballot uservote, tmpbal;
	int votevalue, result = NA;
	int aborted = NA, pos;

	move(t_lines - 2, 0);
	get_record(controlfile, &currvote, sizeof(struct votebal), num);
	if (currentuser.firstlogin > currvote.opendate) {
		outs("�Բ���, ��ͶƱ�����ʺ�����֮ǰ������������ͶƱ\n");
	} else if (!HAS_PERM(currvote.level & ~(LISTMASK | VOTEMASK))) {
		outs("�Բ�����Ŀǰ����Ȩ�ڱ�Ʊ��ͶƱ\n");
	} else if (currvote.level & LISTMASK) {
		char listfilename[PATH_MAX + 1];

		setvotefile(listfilename, currboard, "vote.list");
		if (!dashf(listfilename)) {
			outs("�Բ��𣬱�Ʊ����Ҫ�趨��ͶƱ���᷽�ɽ���ͶƱ\n");
		} else if (!seek_in_file(listfilename, currentuser.userid)) {
			outs("�Բ���, ͶƱ�������Ҳ������Ĵ���\n");
		} else {
			result = YEA;
		}
	} else if (currvote.level & VOTEMASK) {
		if (currentuser.numlogins < currvote.x_logins
		    || currentuser.numposts < currvote.x_posts
		    || currentuser.stay < currvote.x_stay * 3600
		    || currentuser.firstlogin >
		    currvote.opendate - currvote.x_live * 86400) {
			outs("�Բ�����Ŀǰ�в����ʸ��ڱ�Ʊ��ͶƱ\n");
		} else {
			result = YEA;
		}
	} else {
		result = YEA;
	}

	if (result == NA) {
		pressanykey();
		return 0;
	}
	snprintf(fname, sizeof(fname), "vote/%s/flag.%d", currboard, (int)currvote.opendate);
	if ((pos = search_record(fname, &uservote, sizeof(uservote), cmpvuid, currentuser.userid)) <= 0) {
		memset(&uservote, 0, sizeof(uservote));
		voted_flag = NA;
	} else {
		voted_flag = YEA;
	}
	strcpy(uservote.uid, currentuser.userid);
	snprintf(bname, sizeof(bname), "desc.%d", (int)currvote.opendate);
	setvotefile(buf, currboard, bname);
	ansimore(buf, YEA);
	clear();
	switch (currvote.type) {
	case VOTE_SINGLE:
	case VOTE_MULTI:
	case VOTE_YN:
		votevalue = multivote(&uservote);
		if (votevalue == -1)
			aborted = YEA;
		break;
	case VOTE_VALUE:
		votevalue = valuevote(&uservote);
		if (votevalue == -1)
			aborted = YEA;
		break;
	case VOTE_ASKING:
		uservote.voted = 0;
		aborted = !getsug(&uservote);
		break;
	}
	clear();
	if (aborted == YEA) {
		prints("���� ��\033[1m%s\033[m��ԭ���ĵ�ͶƱ��\n", currvote.title);
	} else {
		strlcpy(uservote.votehost, currentuser.lasthost, sizeof(uservote.votehost));
		uservote.votetime = time(NULL);

		if (currvote.type != VOTE_ASKING)
			getsug(&uservote);

		if ((pos = search_record(fname, &tmpbal, sizeof(tmpbal), cmpvuid, currentuser.userid)) <= 0) {
			if (append_record(fname, &uservote, sizeof(uservote)) == -1) {
				clear_line(2);
				outs("ͶƱʧ��! ����ϵͳά����ϵ\n");
				pressreturn();
			}
		} else {
			substitute_record(fname, &uservote, sizeof(uservote), pos);
		}
		outs("\n�Ѿ�����Ͷ��Ʊ����...\n");
	}
	pressanykey();
	return result;
}

void
voteexp(void)
{
	clrtoeol();
	prints("\033[1;44m��� ����ͶƱ���� ������ %-40s��� ���� ����\033[m\n", "ͶƱ����");
}

int
printvote(void *ent_ptr, int unused)
{
	static int i;
	struct ballot uservote;
	char buf[PATH_MAX + 1], flagname[PATH_MAX + 1];
	int num_voted;
	struct votebal *ent = (struct votebal *)ent_ptr;

	if (ent == NULL) {
		move(2, 0);
		voteexp();
		i = 0;
		return 0;
	}
	i++;
	if (i > page + (t_lines - 5) || i > range)
		return QUIT;
	else if (i <= page)
		return 0;
	snprintf(buf, sizeof(buf), "flag.%d", (int)ent->opendate);
	setvotefile(flagname, currboard, buf);
	if (search_record(flagname, &uservote, sizeof(uservote), cmpvuid,  currentuser.userid) <= 0) {
		voted_flag = NA;
	} else {
		voted_flag = YEA;
	}
	num_voted = get_num_records(flagname, sizeof(struct ballot));
	getdatestring(ent->opendate);
	prints(" %s%3d %-12.12s %6.6s%s%-40.40s%-4.4s %3d  %4d\033[m\n",
		(voted_flag == NA) ? "\033[1m" : "", i, ent->userid,
		datestring + 6, ent->level ? "\033[33ms\033[37m" : " ",
		ent->title, vote_type[ent->type - 1], ent->maxdays, num_voted);
	return 0;
}

int
dele_vote(int num)
{
	char buf[PATH_MAX + 1];

	snprintf(buf, sizeof(buf), "vote/%s/flag.%d", currboard, (int)currvote.opendate);
	unlink(buf);
	snprintf(buf, sizeof(buf), "vote/%s/desc.%d", currboard, (int)currvote.opendate);
	unlink(buf);
	if (delete_record(controlfile, sizeof(currvote), num) == -1) {
		outs("������������ϵͳά����ϵ...");
		pressanykey();
	}
	range--;
	if (get_num_records(controlfile, sizeof(currvote)) == 0)
		setvoteflag(currboard, NA);
	return PARTUPDATE;
}

int
vote_results(char *bname)
{
	char buf[PATH_MAX + 1];

	setvotefile(buf, bname, "results");
	if (ansimore(buf, YEA) == -1) {
		move(3, 0);
		outs("Ŀǰû���κ�ͶƱ�Ľ��");
		clrtobot();
		pressreturn();
	} else
		clear();
	return FULLUPDATE;
}

int
b_vote_maintain(void)
{
	return vote_maintain(currboard);
}

void
vote_title(void)
{

	docmdtitle("[ͶƱ���б�]",
		   "[\033[1;32m��\033[m,\033[1;32me\033[m] �뿪 [\033[1;32mh\033[m] ���� [\033[1;32m��\033[m,\033[1;32mr <cr>\033[m] ����ͶƱ [\033[1;32m��\033[m,\033[1;32m��\033[m] ��,��ѡ�� \033[1m������\033[m��ʾ��δͶƱ");
	update_endline();
}

int
vote_key(int ch, int allnum, int pagenum)
{
	int deal = 0;
	char buf[STRLEN];

	switch (ch) {
	case 'v':
	case 'V':
	case '\n':
	case '\r':
	case 'r':
	case KEY_RIGHT:
		user_vote(allnum + 1, 0);
		deal = 1;
		break;
	case 'R':
		vote_results(currboard);
		deal = 1;
		break;
	case 'H':
	case 'h':
		show_help("help/votehelp");
		deal = 1;
		break;
	case 'A':
	case 'a':
		if (!current_bm)
			return YEA;
		vote_maintain(currboard);
		deal = 1;
		break;
	case 'O':
	case 'o':
		if (!current_bm)
			return YEA;
		clear();
		deal = 1;
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		prints("\033[5;1;31m����!!\033[m\nͶƱ����⣺\033[1m%s\033[m\n", currvote.title);
		if (askyn("��ȷ��Ҫ����������ͶƱ��", NA, NA) != YEA) {
			move(2, 0);
			outs("ȡ���������ͶƱ�ж�\n");
			pressreturn();
			clear();
			break;
		}
		mk_result(allnum + 1);
		snprintf(buf, sizeof(buf), "�������ͶƱ %s", currvote.title);
		securityreport(buf);
		break;
	case 'T':
		if (!current_bm)
			return YEA;
		deal = 1;
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		getdata(t_lines - 1, 0, "��ͶƱ���⣺", currvote.title, 51, DOECHO, YEA);
		if (currvote.title[0] != '\0') {
			substitute_record(controlfile, &currvote,
					  sizeof(struct votebal), allnum + 1);
		}
		break;
	case 's':
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		if (currvote.level == 0)
			break;
		deal = 1;
		clear_line(t_lines - 8);
		prints("\033[47;30m��\033[31m%s\033[30m��ͶƱ����˵����\033[m\n", currvote.title);
		if (currvote.level & ~(LISTMASK | VOTEMASK)) {
			int num, len;

			strcpy(genbuf, "bTCPRD#@XWBA#VS-DOM-F0s2345678");
			len = strlen(genbuf);
			for (num = 0; num < len; num++)
				if (!(currvote.level & (1 << num)))
					genbuf[num] = '-';
			prints("Ȩ�����ޣ�[\033[32m%s\033[m]\n", genbuf);
		} else {
			outs("Ȩ�����ޣ�[\033[32m��Ʊ��ͶƱ����Ȩ������\033[m]\n");
		}
		if (currvote.level & LISTMASK) {
			outs("�������ޣ�[\033[35mֻ��ͶƱ�����е� ID �ſ�ͶƱ  \033[m]\n");
		} else {
			outs("�������ޣ�[\033[35m��Ʊ��ͶƱ����ͶƱ��������    \033[m]\n");
		}
		if (currvote.level & VOTEMASK) {
			prints("          �� �١��ڱ�վ����վ�������� [%-4d] ��\n", currvote.x_logins);
			prints("�������ޣ��� �ڡ��ڱ�վ������������   [%-4d] ƪ\n", currvote.x_posts);
			prints("          �� �ۡ�ʵ���ۼ���վʱ������ [%-4d] Сʱ\n", currvote.x_stay);
			prints("          �� �ܡ��� ID ��ע��ʱ������ [%-4d] ��\n", currvote.x_live);
		} else {
			outs("�������ޣ�[\033[36m��Ʊ��ͶƱ���ܸ�����������\033[m    ]\n");
		}
		pressanykey();
		break;
	case 'S':
		if (!HAS_PERM(PERM_SYSOP | PERM_OVOTE))
			return YEA;
		deal = 1;
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		if (setvoteperm(&currvote) != 0) {
			substitute_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		}
		break;
	case Ctrl('K'):
		if (!HAS_PERM(PERM_SYSOP | PERM_OVOTE))
			return YEA;
		deal = 1;
		setvotefile(genbuf, currboard, "vote.list");
		listedit(genbuf, "�༭��ͶƱ������", NULL);
		break;
	case 'E':
		if (!current_bm)
			return YEA;
		deal = 1;
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		setvotefile(genbuf, currboard, "desc");
		snprintf(buf, sizeof(buf), "%s.%d", genbuf, (int)currvote.opendate);
		vedit(buf, EDIT_MODIFYHEADER);
		break;
	case 'M':
	case 'm':
		if (!current_bm)
			return YEA;
		clear();
		deal = 1;
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		prints("\033[5;1;31m����!!\033[m\nͶƱ����⣺\033[1m%s\033[m\n", currvote.title);
		if (askyn("��ȷ��Ҫ�޸����ͶƱ���趨��", NA, NA) != YEA) {
			move(2, 0);
			outs("ȡ���޸�ͶƱ�ж�\n");
			pressreturn();
			clear();
			break;
		}
		outc('\n');
		currvote.report = askyn("�Ƿ��¼ͶƱ������ ��ͶƱIP��ѡ�ʱ�䣩", currvote.report, NA);
		makevote(&currvote, currboard);
		substitute_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		snprintf(buf, sizeof(buf), "�޸�ͶƱ�趨 %s", currvote.title);
		securityreport(buf);
		break;
	case 'D':
	case 'd':
		if (!HAS_PERM(PERM_SYSOP | PERM_OVOTE)) {
			if (!current_bm)
				return YEA;
		}
		deal = 1;
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		clear();
		prints("\033[5;1;31m����!!\033[m\nͶƱ����⣺\033[1m%s\033[m\n", currvote.title);
		if (askyn("��ȷ��Ҫǿ�ƹر����ͶƱ��", NA, NA) != YEA) {
			move(2, 0);
			outs("ȡ��ǿ�ƹر��ж�\n");
			pressreturn();
			clear();
			break;
		}
		snprintf(buf, sizeof(buf), "ǿ�ƹر�ͶƱ %s", currvote.title);
		securityreport(buf);
		dele_vote(allnum + 1);
		break;
	default:
		return 0;
	}
	if (deal) {
		show_votes();
		vote_title();
	}
	return 1;
}

int
show_votes(void)
{
	move(3, 0);
	clrtobot();
	printvote(NULL, 0);
	setcontrolfile();
	if (apply_record(controlfile, printvote, sizeof(struct votebal)) == -1) {
		outs("����û��ͶƱ�俪��....");
		pressreturn();
	} else {
		clrtobot();
	}
	return 0;
}

int
b_vote(void)
{
	int num_of_vote;
	int voting;

	if (!HAS_PERM(PERM_VOTE) || (currentuser.stay < 1800))
		return DONOTHING;

	setcontrolfile();
	if ((num_of_vote = get_num_records(controlfile, sizeof(struct votebal))) == 0) {
		move(2, 0);
		clrtobot();
		outs("\n��Ǹ, Ŀǰ��û���κ�ͶƱ���С�\n");
		pressreturn();
		return FULLUPDATE;
	}
	setvoteflag(currboard, 1);
	modify_user_mode(VOTING);
	range = num_of_vote;            /* setlistrange(num_of_vote); */
	clear();
	voting = choose(NA, 0, vote_title, vote_key, show_votes, user_vote);
	clear();
	return FULLUPDATE;
}

int
b_results(void)
{
	return vote_results(currboard);
}

int
m_vote(void)
{
	char buf[STRLEN];

	strcpy(buf, currboard);
	strcpy(currboard, SYSTEM_VOTE);
	modify_user_mode(ADMIN);
	vote_maintain(SYSTEM_VOTE);
	strcpy(currboard, buf);
	return 0;
}

int
x_vote(void)
{
	char buf[STRLEN];

	modify_user_mode(XMENU);
	strcpy(buf, currboard);
	strcpy(currboard, SYSTEM_VOTE);
	b_vote();
	strcpy(currboard, buf);
	return 0;
}

int
x_results(void)
{
	modify_user_mode(XMENU);
	return vote_results(SYSTEM_VOTE);
}
