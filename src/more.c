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

/* mmap_more: oringinally written by ecnegrevid@ytht, adopted by monster */

static int effectiveline;		// ��Ч����, ֻ����ǰ��Ĳ���, ͷ������, ���в���, ǩ��������, ���Բ���
static int moremode = MORE_NONE;
static char *linkinfo = NULL;		// ȫ������

static char *attachinfo = NULL;   	// ������Ϣ
static char *attachlink = NULL;		// ��������

#define BUFFERCOUNT	100		// ���������

enum LINE_CODE {
	LINE_NORMAL,			// ��ͨ, �лس�
	LINE_NORMAL_NOCF,		// ��ͨ, �޻س�
	LINE_QUOTA,			// ����, �лس�
	LINE_QUOTA_NOCF,		// ����, �޻س�
	LINE_ATTACHMENT,		// ��ʾ�û����ĺ��и���(������Ϣ)
	LINE_ATTACHLINK,		// �������� 
	LINE_ATTACHALINK		// ��������
};

struct MemMoreLines {
	char *ptr;			/* ���±�ӳ�䵽�ڴ�����ʼptr */
	int size;			/* �ڴ�size */
	char *line[BUFFERCOUNT];	/* ���� BUFFERCOUNT �� */
	char ty[BUFFERCOUNT];		// 0: ��ͨ, �лس�; 1: ��ͨ, �޻س�; 2: ����, �лس�; 3: ����, �޻س�
	int len[BUFFERCOUNT];
	int s[BUFFERCOUNT];
	int start;			// this->line[start % BUFFERCOUNT]�Ǽ������к���С���У��к�Ϊ start
	int num;			// ��������row��row + num - 1��ô����
	int curr_line;			// ��ǰ�α�λ��
	char *curr;			// ��ǰ�α����
	char currty;
	int currlen;
	int total;
};

/* from KBSdev2.0
	����:����һ�е�����
	p0�����»�����
	size�ǻ�������С
	*l���ڷ����е���ʾ����
	*s������ռ���ֽڳ���	(�����س��ַ�)
	oldty����һ�е�type
	*ty�����е�type
*/
int
measure_line(char *p0, int size, int *l, int *s, char oldty, char *ty)
{
	int i, w, in_esc = 0, db = 0, lastspace = 0, asciiart = 0, autoline = 1;
	char *p = p0;

	if (size <= 0) {
		if ((size != 0) || !(moremode & MORE_ATTACHMENT) || linkinfo == NULL)
			return -1;

		// ��ʾ�û����ĺ��и���
		if (oldty < LINE_ATTACHMENT) {
			*ty = LINE_ATTACHMENT;
			*s = *l = 0;
			return 0;
		}

		/* �������� */
		if (oldty == LINE_ATTACHMENT) {
			*ty = LINE_ATTACHLINK;
			*s = *l = 0;
			return 0;
		}

		/* ȫ�����ӡ�*/
		if ( oldty == LINE_ATTACHLINK ) {
			*ty = LINE_ATTACHALINK; 
			*s = *l = 0;
			return 0;
		}

		return -1;
	}

	for (i = 0, w = 0; i < size; i++, p++) {
		if (*p == '\n') {
			*l = i;
			*s = i + 1;
			break;
		}
		if (*p == '\t') {
			db = 0;
			w = (w + 8) / 8 * 8;
			lastspace = i;
		} else if (*p == '\033') {
			db = 0;
			in_esc = 1;
			lastspace = i - 1;
		} else if (in_esc) {
			if (strchr("suHMfL@PABCDJK", *p) != NULL) {
				asciiart = 1;
				continue;
			}
		} else if (isprint2((int)*p)) {
			if (!db) {
				    if (autoline) {
					if ((w >= t_realcols - 1 && (i >= size - 1 || ((*(p + 1)) & 0x80))) || w >= t_realcols) { /* 0x80�����ַ� */
						    *l = i;
						    *s = i;
						    break;
					}
				}

				    if (((unsigned char)*p) & 0x80)
					db = 1;
				    else if (isblank((int)*p))
					lastspace = i;
			} else {
				    db = 0;
				    lastspace = i;
			}
			w++;
		    }
	}
	if (i >= size) {
		*l = size;
		*s = size;
	}

	if (*s > 0 && p0[*s - 1] == '\n') {
		switch (oldty) {
		case LINE_NORMAL_NOCF:
			*ty = LINE_NORMAL;
			break;
		case LINE_QUOTA_NOCF:
			*ty = LINE_QUOTA;
			break;
		default:
			if (*l < 2 || strncmp(p0, ": ", 2)) {
				*ty = LINE_NORMAL;
			} else {
				*ty = LINE_QUOTA;
			}
		}
	} else {
		switch (oldty) {
		case LINE_NORMAL_NOCF:
			*ty = LINE_NORMAL_NOCF;
			break;
		case LINE_QUOTA_NOCF:
			*ty = LINE_QUOTA_NOCF;
			break;
		default:
			if (*l < 2 || strncmp(p0, ": ", 2)) {
				*ty = LINE_NORMAL_NOCF;
			} else {
				*ty = LINE_QUOTA_NOCF;
			}
		}
	}

	return 0;
}

static void
init_MemMoreLines(struct MemMoreLines *l, char *ptr, int size)
{
	int i, s, u;
	char *p0, oldty = LINE_NORMAL;

	l->ptr = ptr;
	l->size = size;
	l->start = 0;
	l->num = 0;
	l->total = 0;
	effectiveline = 0;
	for (i = 0, p0 = ptr, s = size; i < 25 && s >= 0; i++) {
		u = (l->start + l->num) % BUFFERCOUNT;
		l->line[u] = p0;
		if (measure_line(p0, s, &l->len[u], &l->s[u], oldty, &l->ty[u]) == -1)
			break;

		oldty = l->ty[u];
		s -= l->s[u];
		p0 = l->line[u] + l->s[u];
		l->num++;
		if (effectiveline >= 0) {
			if (l->len[u] >= 2
			    && strncmp(l->line[u], "--", 2) == 0)
				effectiveline = -effectiveline;
			else if (l->num > 3 && l->len[u] >= 2
				 && l->ty[u] < 2)
				effectiveline++;
		}
	}
	if (effectiveline < 0)
		effectiveline = -effectiveline;
	if (s == 0)
		l->total = l->num;
	l->curr_line = 0;
	l->curr = l->line[0];
	l->currlen = l->len[0];
	l->currty = l->ty[0];
}

static int
next_MemMoreLines(struct MemMoreLines *l)
{
	int n;
	char *p0;

	if (l->curr_line + 1 >= l->start + l->num) {
		char oldty;

		n = (l->start + l->num - 1) % BUFFERCOUNT;
/*
		if (l->ptr + l->size == (l->line[n] + l->s[n])) {
			return -1;
		}
*/
		if (l->num == BUFFERCOUNT) {
			l->start++;
			l->num--;
		}
		oldty = l->ty[n];
		p0 = l->line[n] + l->s[n];
		n = (l->start + l->num) % BUFFERCOUNT;
		l->line[n] = p0;
		if (measure_line(p0, l->size - (p0 - l->ptr), &l->len[n], &l->s[n], oldty, &l->ty[n]) == -1)
			return -1;

		l->num++;
		if (l->size - (p0 - l->ptr) == l->s[n]) {
			l->total = l->start + l->num;
		}
	}
	l->curr_line++;
	l->curr = l->line[l->curr_line % BUFFERCOUNT];
	l->currlen = l->len[l->curr_line % BUFFERCOUNT];
	l->currty = l->ty[l->curr_line % BUFFERCOUNT];
	return l->curr_line;
}

static int
seek_MemMoreLines(struct MemMoreLines *l, int n)
{
	int i;

	if (n < 0) {
		seek_MemMoreLines(l, 0);
		return -1;
	}
	if (n < l->start) {
		i = l->total;
		init_MemMoreLines(l, l->ptr, l->size);
		l->total = i;
	}
	if (n < l->start + l->num) {
		l->curr_line = n;
		l->curr = l->line[l->curr_line % BUFFERCOUNT];
		l->currlen = l->len[l->curr_line % BUFFERCOUNT];
		l->currty = l->ty[l->curr_line % BUFFERCOUNT];
		return l->curr_line;
	}
	while (l->curr_line != n)
		if (next_MemMoreLines(l) < 0)
			return -1;
	return l->curr_line;
}

void
bell_delay_filter(char *ptr, int len)
{
	char *esc, *eptr = ptr + len;

	while (ptr < eptr) {
		if (*ptr == '\033') {
			if (*(ptr + 1) == '[') {
				esc = ptr;
				ptr += 2;
				while ((*ptr >= '0' && *ptr <= '9') || *ptr == ';' || *ptr == ' ')
					++ptr;
				if (*ptr == 'M' || *ptr == 'G' || *ptr == '=' || *ptr == 'U' || *ptr == '-')
					*esc = '*';
			}
		}
		++ptr;
	}
}

void
mem_printline(char *ptr, int len, char ty)
{
	char buf[len + 1], stuffbuf[256];

	/* ������Ϣ */
	if (ty == LINE_ATTACHMENT) {
		if( attachinfo != NULL)
			prints("\033[m\n%s ����:\n", attachinfo);
		return;
	}
	
	/* �������� */
	if (ty == LINE_ATTACHLINK) {
		if( attachlink != NULL) 
			prints("\033[4m%s\033[m\n", attachlink);
		return;
	}
	
	/* ȫ�����ӡ�*/
	if (ty == LINE_ATTACHALINK) {
		if( linkinfo != NULL) {
			outs("\033[m����ȫ������:\n");
			prints("%s\n", linkinfo);
		}
		return;
	}

	if (DEFINE(DEF_NOANSI)) {
		memcpy(buf, ptr, len);
		bell_delay_filter(buf, len);
		ptr = buf;
	}

	if (!strncmp(ptr, "�� ����", 7) || !strncmp(ptr, "==>", 3) || !strncmp(ptr, "�� ��", 5) || !strncmp(ptr, "�� ����", 7)) {
		outns("\033[1;33m", 7);
		outns(ptr, len);
		outns("\033[m\n", 4);
		return;
	}

	if ((ty == LINE_QUOTA) || (ty == LINE_QUOTA_NOCF)) { /* ���� */
		outs("\033[36m");
	}

	if (moremode & MORE_STUFF) {
		if (len >= 256) {
			memcpy(stuffbuf, ptr, 255);
			stuffbuf[255] = '\0';
		} else {
			memcpy(stuffbuf, ptr, len);
			stuffbuf[len] = '\0';
		}
		showstuff(stuffbuf);
	} else {
		outns(ptr, len);
	}

	if ((ty == LINE_QUOTA) || (ty == LINE_QUOTA_NOCF)) {
		outs("\033[m\n");
	} else {
		outc('\n');
	}
}

/* �ӵ�row�п�ʼ, ��ʾptr��ָ����numlines��, �����ʾһ�� */
void
mem_show(char *ptr, int size, int row, int numlines)
{
	struct MemMoreLines l;
	int i, curr_line;
	
	char buf[BUFLEN];
	char author[IDLEN];
	char title[TITLELEN];
	
	init_MemMoreLines(&l, ptr, size);
	move(row, 0);
	clrtobot();
	curr_line = l.curr_line;
	
	/* gcc: preview post */
	if (previewmode && uinfo.mode == READING) {
	    if (!strncmp(l.curr, "������: ", 8))
	    {
	        snprintf(author, strchr(l.curr, '(') - l.curr - 8, "%s", l.curr + 8);
		next_MemMoreLines(&l);
		snprintf(title, l.currlen - 7, "%s", l.curr + 8);
		snprintf(buf, 255, "\033[m\033[1;44;32m������: \033[33m%-12.12s\033[32m��  ��: \033[33m%-50.45s ", author, title);
		outs(buf);
		next_MemMoreLines(&l);
		next_MemMoreLines(&l);
	    }
	}
	outs("\033[m");

	for (i = 0; i < t_lines - 1 - row && i < numlines; i++) {
		mem_printline(l.curr, l.currlen, l.currty);
		if (next_MemMoreLines(&l) < 0)
			break;
	}
}

void
mem_printbotline(int read, int size)
{
	clear_line(t_lines - 1);
	if (moremode & MORE_MSGVIEW) {
		prints("\033[1;44;32m%s (%3d%%)  \033[33m�� ���� Q,�� �� ����  R  �� ���  C  �� �Ļ�����  M         \033[m",
			(read >= size) ? "����ĩβ��" : "���滹���",
			(read >= size) ? 100 : (100 * read / size));
	} else {
		prints("\033[1;44;32m%s (%3d%%)  \033[33m�� ���� Q,�� �� �ƶ� ��/��/PgUp/PgDn/Home/End �� ����˵�� H \033[m",
			(read >= size) ? "����ĩβ��" : "���滹���",
			(read >= size) ? 100 : (100 * read / size));
	}
}

int
mem_more(char *ptr, int size, char *keystr)
{
	struct MemMoreLines l;
	static char searchstr[30];
	char buf[256];
	int i, ch = 0, curr_line, last_line, change;

	init_MemMoreLines(&l, ptr, size);
	outs("\033[m");

	while (1) {
		clear();
		i = 0;
		curr_line = l.curr_line;
		while (1) {
			mem_printline(l.curr, l.currlen, l.currty);
			i++;
			if (i >= t_lines - 1)
				break;
			if (next_MemMoreLines(&l) < 0)
				break;
		}
		last_line = l.curr_line;
		if (l.total && l.total <= t_lines - 1)
			return 0;
		if (l.line[last_line % BUFFERCOUNT] - ptr + l.s[last_line % BUFFERCOUNT] ==
		    size && (ch == KEY_RIGHT || ch == KEY_PGDN || ch == ' ' || ch == Ctrl('f'))) {
			clear_line(t_lines - 1);
			return 0;
		}
		change = 0;
		do {
			mem_printbotline(l.line[last_line % BUFFERCOUNT] - ptr + l.s[last_line % BUFFERCOUNT], size);
			ch = egetch();
			clear_line(t_lines - 1);
			switch (ch) {
			case 'k':
			case KEY_UP:
				change = -1;
				break;
			case 'j':
			case KEY_DOWN:
			case '\n':
				change = 1;
				break;
			case Ctrl('B'):
			case KEY_PGUP:
				change = -t_lines + 2;
				break;
			case ' ':
			case Ctrl('F'):
			case KEY_PGDN:
			case KEY_RIGHT:
				if (!l.total)
					seek_MemMoreLines(&l, last_line + t_lines);
				change = t_lines - 2;
				if (l.total && last_line < l.total && curr_line + change + t_lines - 1 > l.total)
					change = l.total - curr_line - t_lines + 1;
				break;
			case KEY_HOME:
				change = -curr_line;
				break;
			case '$':
			case KEY_END:
				if (!l.total) {
					while (next_MemMoreLines(&l) >= 0) ;
					curr_line = l.curr_line;
				} else {
					curr_line = l.total - 1;
				}
				change = -t_lines + 2;
				break;
			case 'G':
			case 'g':
				getdata(t_lines - 1, 0, "��ת�����к�: ", buf, 9, DOECHO, YEA);
				if (isdigit(buf[0])) {
					change = atoi(buf) - curr_line;
				}
				break;
			case '/':
			case '?':
				getdata(t_lines - 1, 0, ch == '/' ? "���²����ַ���: " : "���ϲ����ַ���: ", searchstr, sizeof(searchstr) - 1, DOECHO, NA);
				if (searchstr[0] != '\0') {
					int i = curr_line;

					while (1) {
						if (ch == '/') {
							i++;
						} else {
							i--;
						}
						if (seek_MemMoreLines(&l, i) < 0)
							break;
						memcpy(buf, l.curr, (l.currlen >= 256) ? 255 : l.currlen);
						buf[(l.currlen >= 256) ? 255 : l.currlen] = 0;
						if (strcasestr(buf, searchstr) != NULL) {
							change = i - curr_line;
							break;
						}
					}
					if (change == 0)
						continue;
				}
				break;
			case KEY_LEFT:
			case 'Q':
			case 'q':
				return ch;
			case 'H':
			case 'h':
				show_help("help/morehelp");
				curr_line += t_lines - 1;
				change = 1 - t_lines;
				break;
			default:
				if (ch < 32)		// monster: ���Կ��Ƽ�
					continue;

				if (keystr != NULL && strchr(keystr, ch) != NULL)
					return ch;
			}

			if (change < 0 && curr_line == 0)
				return KEY_UP;			// change = 0;

			if (change == 1) {
				if (seek_MemMoreLines(&l, curr_line + t_lines - 1) >= 0) {
					curr_line++;
					last_line++;
					scroll();
					move(t_lines - 2, 0);
					clrtoeol();
					mem_printline(l.curr, l.currlen, l.currty);
					if ((ch == KEY_PGDN || ch == ' '
					     || ch == Ctrl('F')
					     || ch == KEY_RIGHT
					     || ch == KEY_DOWN || ch == 'j'
					     || ch == '\n')
					    && l.line[last_line % BUFFERCOUNT] - ptr + l.s[last_line % BUFFERCOUNT] == size) {
						clear_line(t_lines - 1);
						return 0;
					}
				} else {
					return 0;
				}
				change = 0;
			}

			if (change == -1) {
				if (seek_MemMoreLines(&l, curr_line - 1) >= 0) {
					curr_line--;
					last_line--;
					rscroll();
					move(0, 0);
					mem_printline(l.curr, l.currlen, l.currty);
				}
				change = 0;
			}
		} while (change == 0);
		seek_MemMoreLines(&l, curr_line + change);
	}
	return 0;
}

int
mmap_show(char *filename, int row, int numlines)
{
	int fd;
	char *ptr;
	struct stat st;

	if ((fd = open(filename, O_RDONLY)) == -1)
		return -1;

	if (fstat(fd, &st) == -1 || !S_ISREG(st.st_mode) || st.st_size < 0) {
		close(fd);
		return -1;
	}

	if (st.st_size == 0) {
		close(fd);
		return 0;
	}

	ptr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);
	close(fd);

	if (ptr == MAP_FAILED) {
		return -1;
	}

	TRY
		mem_show(ptr, st.st_size, row, numlines);
	END

	munmap(ptr, st.st_size);
	return 0;
}

int
mmap_more(char *filename, char *keystr, int fixsize)
{
	int fd, retv = 0;
	char *ptr, *eptr = NULL;
	struct stat st;

	if ((fd = open(filename, O_RDONLY)) == -1)
		return -1;

	if (fstat(fd, &st) == -1 || !S_ISREG(st.st_mode) || st.st_size < 0) {
		close(fd);
		return -1;
	}

	if (st.st_size == 0) {
		close(fd);
		return 0;
	}

	ptr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);
	close(fd);

	if (ptr == MAP_FAILED) {
		return -1;
	}

	TRY
		if (fixsize)
			eptr = (char *)memchr(ptr, 0, st.st_size); /* scan for '\0' */

		retv = mem_more(ptr, (eptr == NULL) ? st.st_size : (eptr - ptr), keystr);
	END

	munmap(ptr, st.st_size);
	return retv;
}

int rawmore(char *filename, int promptend, int stuff, int fixsize)
{
	int ch;

	clear();
	moremode |= stuff;
	ch = mmap_more(filename, "RrEexp", fixsize); /* R,r,E,e,x,p are keys to quit more */
	if (promptend)
		pressanykey();
	move(t_lines - 1, 0);
	outs("\033[m\033[m");
	return ch;
}

/* promptend = YEA  -->  pressanykey */
int
ansimore(char *filename, int promptend)
{
	moremode = MORE_NONE;
	return rawmore(filename, promptend, check_stuffmode(), NA);
}

int
ansimore2(char *filename, int promptend, int row, int numlines)
{
	int ch;

	moremode = MORE_NONE;
	if (numlines) {
		ch = mmap_show(filename, row, numlines);
	} else {
		ch = mmap_more(filename, NULL, NA);
	}

	if (promptend)
		pressanykey();

	return ch;
}

int
ansimore3(char *filename, int promptend)
{
	moremode = MORE_NONE;
	return rawmore(filename, promptend, YEA, NA);
}

/* freestyler: ���������� */
int
ansimore4(char* filename, char *attinfo, char *attlink, char *alink, int prometend)
{
	attachinfo = attinfo; /* ������Ϣ */
	attachlink = attlink; /* �������� */
	linkinfo   = alink;   /* ȫ������ */
	moremode   = MORE_ATTACHMENT;
	return rawmore(filename, prometend, check_stuffmode(), NA);
}

int
mesgmore(char *filename)
{
	int ch;
	char title[TITLELEN];

	clear();
	moremode = MORE_MSGVIEW;
	ch = mmap_more(filename, "RrCcMm", NA);

	if ((ch == 0) || (strchr("QqRrCcMm", ch) == NULL && ch != KEY_LEFT)) {
		move(t_lines - 1, 0);
		outs("\033[1;31;44m[ѶϢ���]  \033[33m����  R  �� ���  C  �� �Ļ�����  M  ��                            \033[m");
		ch = egetch();
	}
	int res  = 0;

	switch (ch) {
		case 'C':               // ���
		case 'c':
			unlink(filename);
			break;
		case 'M':		// �Ļ�����
		case 'm':
			getdatestring(time(NULL));
			snprintf(title, sizeof(title), "[%s] ����ѶϢ����", datestring);
			if (askyn("�Ƿ�Ļص����ʼ����� (�ش�N��Ļ�վ������)", NA, YEA) == NA) {
				res = mail_sysfile(filename, currentuser.userid, title);
			} else {
#ifdef INTERNET_EMAIL
				res = bbs_sendmail(filename, title, currentuser.email, YEA);
#endif
			}
			if( res != -1 ) /* �Ļسɹ���ɾ�� */
				unlink(filename);
			break;
	}

	return 0;
}
