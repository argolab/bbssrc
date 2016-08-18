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
#ifdef AIX
#include <sys/select.h>
#endif

#define OBUFSIZE  (4096)
#define IBUFSIZE  (256)

#define INPUT_ACTIVE 0
#define INPUT_IDLE 1

extern int dumb_term;

static char outbuf[OBUFSIZE];
static int obufsize = 0;

char inbuf[IBUFSIZE];
int ibufsize = 0;
int icurrchar = 0;
int KEY_ESC_arg;

int enabledbchar = YEA;		/* ����ɾ�� */

static int i_mode = INPUT_ACTIVE;
extern struct screenline *big_picture;

#ifdef ALLOWSWITCHCODE

#define BtoGtablefile "etc/b2g_table"
#define GtoBtablefile "etc/g2b_table"

unsigned char *GtoB, *BtoG;

#define GtoB_count 7614
#define BtoG_count 13973

extern int convcode;
extern void redoscr(void);

int
switch_code(void)
{
	convcode = !convcode;
	redoscr();
	return 0;
}

void
resolve_GbBig5Files(void)
{
	int fd;
	int i;

	BtoG =
	    attach_shm("CONV_SHMKEY", GtoB_count * 2 + BtoG_count * 2);

	if ((fd = open(BtoGtablefile, O_RDONLY)) == -1) {
		for (i = 0; i < BtoG_count; i++) {
			BtoG[i * 2] = 0xA1;
			BtoG[i * 2 + 1] = 0xF5;
		}
	} else {
		read(fd, BtoG, BtoG_count * 2);
		close(fd);
	}
	if ((fd = open(GtoBtablefile, O_RDONLY)) == -1) {
		for (i = 0; i < GtoB_count; i++) {
			BtoG[BtoG_count * 2 + i * 2] = 0xA1;
			BtoG[BtoG_count * 2 + i * 2 + 1] = 0xBC;
		}
	} else {
		read(fd, BtoG + BtoG_count * 2, GtoB_count * 2);
		close(fd);
	}
	GtoB = BtoG + BtoG_count * 2;
}

int
write2(int port, char *str, int len)	// write gb to big
{
	int i, locate;
	unsigned char ch1, ch2, *ptr;

	for (i = 0, ptr = (unsigned char*) str; i < len; i++) {
		ch1 = (ptr + i)[0];
		if (ch1 < 0xA1 || (ch1 > 0xA9 && ch1 < 0xB0) || ch1 > 0xF7)
			continue;
		ch2 = (ptr + i)[1];
		i++;
		if (ch2 < 0xA0 || ch2 == 0xFF)
			continue;
		if ((ch1 > 0xA0) && (ch1 < 0xAA))	//01��09��Ϊ��������
			locate = ((ch1 - 0xA1) * 94 + (ch2 - 0xA1)) * 2;
		else		//if((buf > 0xAF) && (buf < 0xF8)){ //16��87��Ϊ����
			locate =
			    ((ch1 - 0xB0 + 9) * 94 + (ch2 - 0xA1)) * 2;
		(ptr + i - 1)[0] = GtoB[locate++];
		(ptr + i - 1)[1] = GtoB[locate];
	}
	return write(port, str, len);
}

int
read2(int port, char *str, int len)	// read big from gb
{
	/*
	 * Big-5 ��һ��˫�ֽڱ��뷽�������һ�ֽڵ�ֵ�� 16 ��
	 * �Ƶ� A0��FE ֮�䣬�ڶ��ֽ��� 40��7E �� A1��FE ֮�䡣
	 * ��ˣ����һ�ֽڵ����λ�� 1���ڶ��ֽڵ����λ���
	 * ���� 1��Ҳ������ 0��
	 */
	unsigned char ch1, ch2, *ptr;
	int locate, i = 0;

	if (len == 0)
		return 0;
	len = read(port, str, len);
	if (len < 1)
		return len;

	for (i = 0, ptr = (unsigned char*) str; i < len; i++) {
		ch1 = (ptr + i)[0];
		if (ch1 < 0xA1 || ch1 == 0xFF)
			continue;
		ch2 = (ptr + i)[1];
		i++;
		if (ch2 < 0x40 || (ch2 > 0x7E && ch2 < 0xA1) || ch2 == 255)
			continue;
		if ((ch2 >= 0x40) && (ch2 <= 0x7E))
			locate = ((ch1 - 0xA1) * 157 + (ch2 - 0x40)) * 2;
		else
			locate =
			    ((ch1 - 0xA1) * 157 + (ch2 - 0xA1) + 63) * 2;
		(ptr + i - 1)[0] = BtoG[locate++];
		(ptr + i - 1)[1] = BtoG[locate];
	}
	return len;
}

#define bbs_read(fd, buf, count) \
		(convcode) ? read2(fd, buf, count) : read(fd, buf, count);

#define bbs_write(fd, buf, count) \
		(convcode) ? write2(fd, buf, count) : write(fd, buf, count);

#else

#define bbs_read(fd, buf, count) \
		read(fd, buf, count);

#define bbs_write(fd, buf, count) \
		write(fd, buf, count);

#endif

static void
hit_alarm_clock(int signo)
{
	if (HAS_PERM(PERM_NOTIMEOUT))
		return;
	if (i_mode == INPUT_IDLE || uinfo.mode == LOGIN) {
//              clear();
//              prints("Idle timeout exceeded! Booting...\n");
		safe_kill(getpid());
	}
	i_mode = INPUT_IDLE;
	alarm((uinfo.mode == LOGIN) ? LOGIN_TIMEOUT : IDLE_TIMEOUT);
}

void
init_alarm()
{
	signal(SIGALRM, hit_alarm_clock);
	alarm(IDLE_TIMEOUT);
}

int
next_ch(char *s, int *i)
{
	for ((*i) = 0; (*i) < 20; (*i)++) {
		if (strchr("[0123456789;", s[(*i)]) == 0)
			return s[(*i)];
	}
	return -1;
}

void
oflush(void)
{
	int i, tmp, skip;

	for (i = 0; i < obufsize; i++) {
		if ((unsigned char) outbuf[i] == 255 || outbuf[i] == 14)
			outbuf[i] = ' ';
		if (outbuf[i] == 27) {
			tmp = next_ch(outbuf + i + 1, &skip);
			if (tmp == -1 || tmp == '~' || tmp == '-' || tmp == 'c' || tmp == 'i' || tmp == 'r' /* || tmp == 'M' */ )	//mem by cancel
				outbuf[i] = '*';
			i += skip;
		}
	}
	bbs_write(1, outbuf, obufsize);
	obufsize = 0;
}

void
output(char *s, int len)
{
	/* Invalid if len >= OBUFSIZE */

	int size;
	char *data;

	/* gcc: prevent string overflow */
	size = strlen(s);
	if (size < len) len = size;

	size = obufsize;
	data = outbuf;
	if (size + len > OBUFSIZE) {
		bbs_write(0, data, size);
		size = len;
	} else {
		data += size;
		size += len;
	}
	memcpy(data, s, len);
	obufsize = size;
}

int
ochar(int c)
{
	char *data;
	int size;

	data = outbuf;
	size = obufsize;

	if (size > OBUFSIZE - 2) {	/* doin a oflush */
		bbs_write(0, data, size);
		size = 0;
	}
	data[size++] = c;
	if (c == IAC)
		data[size++] = c;

	obufsize = size;
	return 0;
}

int i_newfd = 0;
struct timeval i_to, *i_top = NULL;
void (*flushf) () = NULL;

void
add_io(int fd, int timeout)
{
	i_newfd = fd;
	if (timeout) {
		i_to.tv_sec = timeout;
		i_to.tv_usec = 0;
		i_top = &i_to;
	} else
		i_top = NULL;
}

void
add_flush(void (*flushfunc) ())
{
	flushf = flushfunc;
}

static int
iac_count(char *current)
{
	switch ((unsigned char) (*(current + 1))) {
	case DO:
	case DONT:
	case WILL:
	case WONT:
		return 3;
	case SB:		/* loop forever looking for the SE */
		{
			char *look = current + 2;

			/* monster: ���ڴ�С�����仯 */
			if ((unsigned char) (*look) == TELOPT_NAWS &&
			    (unsigned char) (*(current + 8)) == SE) {
				int lines =
				    (unsigned char) (*(current + 6));

				if (lines != t_lines && lines >= 24 &&
				    lines <= 100) {
					t_lines = lines;
					initscr();
				}

				return 9;
			}

			for (;;) {
				if ((unsigned char) (*look++) == IAC) {
					if ((unsigned char) (*look++) ==
					    SE) {
						return look - current;
					}
				}
			}
		}
	default:
		return 1;
	}
}

int
igetch(void)
{
	static int trailing = 0;
	int ch;
	char *data;

	data = inbuf;

	for (;;) {
		if (ibufsize == icurrchar) {
			fd_set readfds;
			struct timeval to;
			fd_set *rx;
			int fd, nfds;

			rx = &readfds;
			fd = i_newfd;

		      igetnext:
			FD_ZERO(rx);
			FD_SET(0, rx);
			if (fd) {
				FD_SET(fd, rx);
				nfds = fd + 1;
			} else
				nfds = 1;
			to.tv_sec = to.tv_usec = 0;
			if ((ch = select(nfds, rx, NULL, NULL, &to)) <= 0) {
				if ((ch == -1) && (errno != EINTR))
					abort_bbs();	/* monster: old code - return -1; */

				if (flushf)
					(*flushf) ();

				if (big_picture)
					refresh();
				else
					oflush();

				FD_ZERO(rx);
				FD_SET(0, rx);
				if (fd)
					FD_SET(fd, rx);

				while ((ch =
					select(nfds, rx, NULL, NULL,
					       i_top)) < 0) {
					if (errno != EINTR)
						abort_bbs();	/* monster: old code -return -1; */
				}
				if (ch == 0)
					return I_TIMEOUT;
			}
			if (fd && FD_ISSET(fd, rx))
				return I_OTHERDATA;

			do {
				ch = bbs_read(0, data, IBUFSIZE);

				/* ���������ߵĴ��� */
				if ((ch == 0) || (ch < 0 && errno != EINTR))
					abort_bbs();
			} while (ch < 0);

			icurrchar = 0;
			while ((unsigned char) data[icurrchar] == IAC && icurrchar < ch)
				icurrchar += iac_count(data + icurrchar);

			if (icurrchar >= ch)
				goto igetnext;
			ibufsize = ch;
			i_mode = INPUT_ACTIVE;
		}
		ch = data[icurrchar++];
		if (trailing) {
			trailing = 0;
			if (ch == 0 || ch == 0x0a)
				continue;
		}
		if (ch == Ctrl('L')) {
			redoscr();
#ifndef ANTI_KEEPONLINE
			uinfo.idle_time = time(NULL);
			update_utmp();
#endif
			continue;
		}
		if (ch == 0x0d) {
			trailing = 1;
			ch = '\n';
		}
#ifdef ANTI_KEEPONLINE
		if ((unsigned int) ch >= 32) {
			uinfo.idle_time = time(NULL);
			update_utmp();	/* Ӧ������Ҫ update һ�� :X */
		}
#else
		uinfo.idle_time = time(NULL);
		update_utmp();
#endif
		return (ch);
	}
}

int
igetkey(void)
{
	int mode;
	int ch, last;
	extern int RMSG;

	mode = last = 0;
	while (1) {
		if ((uinfo.in_chat == YEA || uinfo.mode == TALK ||
		     uinfo.mode == PAGE || INBBSGAME(uinfo.mode)) &&
		    RMSG == YEA) {
			char a;
			int a_len;

			a_len = bbs_read(0, &a, 1);
			if (a_len == 0 || (a_len < 0 && errno != EINTR))
				abort_bbs();
			if ((a_len == -1) && (errno == EINTR))
				continue;
			ch = (int)a;
		} else {
			ch = igetch();
			if (ch == -1)	// ������� -1 ����ֵ���д���
				abort_bbs();
		}
		if ((ch == Ctrl('Z')) && (RMSG == NA) &&
		    uinfo.mode != LOCKSCREEN && !guestuser) {
			r_msg2();
			continue;	/* Pudding: Ӧ����ʲô�¶�û������һ�� */
		}
		if (mode == 0) {
			if (ch == KEY_ESC)
				mode = 1;
			else
				return ch;	/* Normal Key */
		} else if (mode == 1) {	/* Escape sequence */
			if (ch == '[' || ch == 'O')
				mode = 2;
			else if (ch == '1' || ch == '4')
				mode = 3;
			else {
				KEY_ESC_arg = ch;
				return KEY_ESC;
			}
		} else if (mode == 2) {	/* Cursor key */
			if (ch >= 'A' && ch <= 'D')
				return KEY_UP + (ch - 'A');
			else if (ch >= '1' && ch <= '6')
				mode = 3;
			else
				return ch;
		} else if (mode == 3) {	/* Ins Del Home End PgUp PgDn */
			if (ch == '~')
				return KEY_HOME + (last - '1');
			else
				return ch;
		}
		last = ch;
	}
}

int
ask(char *prompt)
{
	int ch;

	if (editansi) {
		outs(ANSI_RESET);
		refresh();
	}
	move(0, 0);
	clrtoeol();
	standout();
	outs(prompt);
	standend();

	ch = igetkey();
	move(0, 0);
	clrtoeol();
	return (ch);
}

/* gcc: fix length bug of getdata */
int
ansistrlen(char *str)
{
	int c = 0;
	while (*str) {
		if (*str == KEY_ESC) {
			while (!isalpha(*str) && *str) str++;
			if (*str) str++;
			continue;
		}
		str++;
		c++;
	}
	return c;
}

int
getdata(int line, int col, char *prompt, char *buf, int len, int echo,
	int clearlabel)
{
	int ch, clen = 0, curr = 0, x, y;
	char tmp[STRLEN];
	int dbchar, i;
	extern unsigned char scr_cols;
	extern int RMSG;
	extern int msg_num;
	int init = YEA;

	move(line, col);
	if (prompt) {
		outs(prompt);
		col += ansistrlen(prompt);
	}
	y = line;
	x = col;

	if (clearlabel) {
		memset(buf, 0, len);
		curr = 0;
	} else {
		clen = strlen(buf);
		curr = (clen >= len) ? len - 1 : clen;
		buf[curr] = '\0';
		outs(buf);
	}

	refresh();
	if (dumb_term || echo == NA) {
		while ((ch = igetkey()) != '\r') {
			if (RMSG == YEA && msg_num == 0) {
				if (ch == Ctrl('Z') || ch == KEY_UP) {
					buf[0] = Ctrl('Z');
					clen = 1;
					break;
				}
				if (ch == Ctrl('A') || ch == KEY_DOWN) {
					buf[0] = Ctrl('A');
					clen = 1;
					break;
				}
			}
			if (ch == '\n')
				break;
			if (ch == '\177' || ch == Ctrl('H')) {
				if (clen == 0) {
					continue;
				}
				clen--;
				ochar(Ctrl('H'));
				ochar(' ');
				ochar(Ctrl('H'));
				continue;
			}
			if (!isprint2(ch)) {
				continue;
			}
			if (clen >= len - 1) {
				continue;
			}
			buf[clen++] = ch;
			if (echo)
				ochar(ch);
			else
				ochar('*');
		}
		buf[clen] = '\0';
		outc('\n');
		oflush();
		return clen;
	}
	clrtoeol();
	while (1) {
		move(y, x);
		if (init && !clearlabel) outs("\x1b[4m");
		outs(buf);
		clrtoeol();
		if (init && !clearlabel) outs("\x1b[m");
		move(y, x + curr);
		if (RMSG == YEA &&
		    (uinfo.in_chat == YEA || uinfo.mode == TALK ||
		     INBBSGAME(uinfo.mode))) {
			refresh();
		}
		ch = igetkey();
		if ((RMSG == YEA) && msg_num == 0) {
			if (ch == Ctrl('Z') || ch == KEY_UP) {
				buf[0] = Ctrl('Z');
				clen = 1;
				break;
			}
			if (ch == Ctrl('A') || ch == KEY_DOWN) {
				buf[0] = Ctrl('A');
				clen = 1;
				break;
			}
		}
		if (ch == '\n' || ch == '\r')
			break;
		if (ch == Ctrl('R')) {
			init = NA;
			enabledbchar = ~enabledbchar & 1;
			continue;
		}
		if (ch == '\177' || ch == Ctrl('H')) {
			if (init == YEA) {
				init = NA;
				curr = 0;
				clen = 0;
				buf[0] = 0;
			}
			if (curr == 0) {
				continue;
			}
			strcpy(tmp, &buf[curr]);
			if (enabledbchar) {
				dbchar = 0;
				for (i = 0; i < curr - 1; i++)
					if (dbchar)
						dbchar = 0;
					else if (buf[i] & 0x80)
						dbchar = 1;
				if (dbchar) {
					curr--;
					clen--;
				}
			}
			buf[--curr] = '\0';
			strcat(buf, tmp);
			clen--;
			continue;
		}
		if (ch == Ctrl('D') || ch == KEY_DEL) {
			if (init) {
				init = NA;
				buf[0] = 0;
				curr = 0;
				clen = 0;
			}
			if (curr >= clen) {
				curr = clen;
				continue;
			}
			strcpy(tmp, &buf[curr + 1]);
			buf[curr] = '\0';
			strcat(buf, tmp);
			clen--;
			continue;
		}
		if (ch == Ctrl('K')) {
			init = NA;
			buf[curr] = '\0';
			clen = curr;
			continue;
		}
		if (ch == Ctrl('B') || ch == KEY_LEFT) {
			init = NA;
			if (curr == 0) {
				continue;
			}
			curr--;
			continue;
		}
		if (ch == Ctrl('E') || ch == KEY_END) {
			init = NA;
			curr = clen;
			continue;
		}
		if (ch == Ctrl('A') || ch == KEY_HOME) {
			init = NA;
			curr = 0;
			continue;
		}
		if (ch == Ctrl('F') || ch == KEY_RIGHT) {
			init = NA;
			if (curr >= clen) {
				curr = clen;
				continue;
			}
			curr++;
			continue;
		}
		if (!isprint2(ch)) {
			init = NA;
			continue;
		}

		if (x + clen >= scr_cols || clen >= len - 1) {
			continue;
		}

		if (init) {
			init = NA;
			buf[0] = 0;
			clen = 0;
			curr = 0;
		}
		if (!buf[curr]) {
			buf[curr + 1] = '\0';
			buf[curr] = ch;
		} else {
			strncpy(tmp, &buf[curr], len);
			buf[curr] = ch;
			buf[curr + 1] = '\0';
			strncat(buf, tmp, len - curr);
		}
		curr++;
		clen++;
	}
	buf[clen] = '\0';
	if (echo) {
		move(y, x);
		outs(buf);
	}
	outc('\n');
	refresh();
	return clen;
}

char *
make_multiline(char *dest, int size, char *buf, int maxcol,
	       char *background)
{
	int i, curcol, writep;
	char tmp[BUFLEN];
	char dbchar = NA, in_esc = NA;

	dest[0] = '\0';
	tmp[0] = '\0';
	writep = 0;
	curcol = 0;
	for (i = 0; buf[i] != '\0'; i++) {
		if (dbchar) {
			dbchar = NA;
		} else if (buf[i] & 0x80) {
			dbchar = YEA;
		}

		if (!in_esc && buf[i] == '\033')
			in_esc = YEA;

		if (buf[i] == '\r')
			continue;
		if (buf[i] == '\n') {
			for (; curcol < maxcol; curcol++) {
				tmp[writep] = ' ';
				tmp[++writep] = '\0';
			}

			/* д��һ�� */
			if (background)
				strncat(dest, background, size);
			strlcat(dest, tmp, size);
			strlcat(dest, "\n", size);

			tmp[0] = '\0';
			writep = 0;
			curcol = 0;
			continue;
		}
		if (!in_esc &&
		    (curcol >= maxcol ||
		     (curcol + 1 >= maxcol && dbchar))) {
			for (; curcol < maxcol; curcol++) {
				tmp[writep] = ' ';
				tmp[++writep] = '\0';
			}

			/* д��һ�� */
			if (background)
				strncat(dest, background, size);
			strlcat(dest, tmp, size);
			strlcat(dest, "\n", size);

			tmp[0] = '\0';
			writep = 0;
			curcol = 0;
		}
		/* ����ַ� */
		tmp[writep] = buf[i];
		tmp[++writep] = '\0';
		if (!in_esc)
			curcol++;
		if (in_esc && buf[i] == 'm')
			in_esc = NA;
	}

	/* ����ĩһ��д�� */
	if (writep > 0) {
		if (background) {
			strlcat(tmp, background, sizeof (tmp));
			writep += strlen(background);
		}
		for (; curcol < maxcol; curcol++) {
			tmp[writep] = ' ';
			tmp[++writep] = '\0';
		}
		if (background)
			strncat(dest, background, size);
		strlcat(dest, tmp, size);
		strlcat(dest, "\n", size);
	}
	strlcat(dest, "\033[m", size);
	return dest;
}

int
show_multiline(int line, int col, char *buf, int maxcol, char *background,
	       int *offset)
{
	int i, curcol, curline;
	char dbchar = NA, in_esc = NA;

	curline = line;
	curcol = col;
	move(curline, curcol);
	clrtoeol();

	if (background)
		outs(background);
	for (i = 0; buf[i] != '\0'; i++) {
		if (dbchar) {
			dbchar = NA;
		} else if (buf[i] & 0x80) {
			dbchar = YEA;
		}

		if (!in_esc && buf[i] == '\033') {
			in_esc = YEA;
		}

		/* ���ƻ��� here */
		if (buf[i] == '\r')
			continue;
		if (!in_esc && buf[i] == '\n') {
			for (; curcol < maxcol; curcol++)
				outc(' ');
			curline++;
			curcol = 0;
			move(curline, curcol);
			clrtoeol();
			if (background)
				outs(background);
			continue;
		}
		if (!in_esc && (curcol >= maxcol ||
				(curcol + 1 >= maxcol && dbchar))) {
			if (curcol < maxcol)
				outc(' ');
			curline++;
			curcol = 0;
			move(curline, curcol);
			clrtoeol();
			if (background)
				outs(background);
		}
		/* print the char */
		if (buf[i] != '\033') {
			outc(buf[i]);
		} else {
			if (showansi) {
				outc('\033');
			} else {
				outc('*');
			}
		}

		if (!in_esc || !showansi)
			curcol++;
		if (in_esc && buf[i] == 'm')
			in_esc = 0;
	}
	if (offset)
		*offset = curcol;
	return (curline - line + 1);
}
/* Writtern by Pudding 2004/2/27 */
int
multi_getdata(int line, int col, char *prompt, char *buf, int len,
	      int maxcol, int maxline, int clearlabel)
{
	/* Text status */
	int clen;
	int can_insert = NA, total_line = 0;

	/* Current state of entry */
	int curr = 0;
	int curr_dbchar = NA, prev_dbchar = NA;
	int curline, curcol;

	int ch;
	extern int RMSG;
	extern int msg_num;

	int y, x, i, j, jj, dbchar;
	char tmp[BUFLEN];

	int tmpansi;
	int prompt_offset;
	int line_offset;

	tmpansi = showansi;
	showansi = NA;

	/* Fix col */
	if (prompt) {
		move(line, col);
		clrtoeol();
		outs(prompt);
		oflush();
		col += strlen(prompt);
	}
	prompt_offset = col;
	if (len > BUFLEN)
		len = BUFLEN;
	curline = line;
	curcol = col;

	/* Init string */
	if (clearlabel) {
		clen = 0;
		curr = 0;
		buf[0] = '\0';
	} else {
		clen = strlen(buf);
		curr = (clen >= len) ? len - 1 : clen;
		buf[curr] = '\0';
		show_multiline(line, col, buf, maxcol, NULL, NULL);
		clrtoeol();
	}

	while (1) {
		/* ����currˢ��״̬ */
		dbchar = NA;
		j = 0;
		jj = 0;
		y = line;
		x = col;
		for (i = 0; i < clen; i++) {
			if (dbchar) {
				dbchar = NA;
			} else if (buf[i] & 0x80) {
				dbchar = YEA;
			}

			/* ���� */
			if (x >= maxcol || (x + 1 >= maxcol && dbchar)) {
				y++;
				x = 0;
			}

			if (i == curr) {
				curline = y;
				curcol = x;
				prev_dbchar = jj;
				curr_dbchar = dbchar;
			}

			jj = j;
			j = dbchar;
			if (buf[i] == '\n' || buf[i] == '\r') {
				y++;
				x = 0;
				continue;
			}
			x++;
		}
		/* curr�ڴ�β����� */
		if (curr >= clen) {
			curline = y;
			curcol = x;
			prev_dbchar = jj;
			curr_dbchar = NA;
		}
		can_insert = (clen >= len - 1 ||
			      (x + 2 >= maxcol &&
			       total_line >= maxline)) ? NA : YEA;

		/* ���Ӳ��� */
		j = total_line;
		total_line =
		    show_multiline(line, col, buf, maxcol, NULL, NULL);
		clrtoeol();

		/* ɨ���� */
		for (i = total_line + line; i < j + line; i++)
			clear_line(i);
		move(curline, curcol);

		if (RMSG == YEA &&
		    (uinfo.in_chat == YEA || uinfo.mode == TALK ||
		     INBBSGAME(uinfo.mode))) {
			refresh();
		}
		/* ����ָ�� */
		ch = igetkey();
		if (RMSG == YEA && msg_num == 0) {
			if (ch == Ctrl('Z') || ch == KEY_UP) {
				buf[0] = Ctrl('Z');
				clen = 1;
				/* total_line = 1; */
				break;
			}
			if (ch == Ctrl('A') || ch == KEY_DOWN) {
				buf[0] = Ctrl('A');
				clen = 1;
				/* total_line = 1; */
				break;
			}
		}
		if (ch == '\n' || ch == '\r')
			break;
		if (ch == Ctrl('R')) {
			enabledbchar = ~enabledbchar & 1;
			continue;
		}
		if (ch == '\177' || ch == Ctrl('H')) {
			if (curr == 0)
				continue;
			strlcpy(tmp, buf + curr, BUFLEN - 1);
			if (enabledbchar && prev_dbchar) {
				curr -= 2;
				clen -= 2;
			} else {
				curr--;
				clen--;
			}
			buf[curr] = '\0';
			strlcat(buf, tmp, len);
			continue;
		}
		if (ch == KEY_DEL) {
			if (curr >= clen)
				continue;
			if (enabledbchar && curr_dbchar &&
			    clen - curr >= 2) {
				strlcpy(tmp, buf + curr + 2, BUFLEN - 1);
				clen -= 2;
			} else {
				strlcpy(tmp, buf + curr + 1, BUFLEN - 1);
				clen -= 1;
			}
			buf[curr] = '\0';
			strlcat(buf, tmp, len);
			continue;
		}
		if (ch == Ctrl('A') || ch == KEY_HOME) {
			if (curr > 0 && buf[curr] == '\n')
				curr--;
			for (; curr > 0 && buf[curr] != '\n'; curr--) ;
			if (buf[curr] == '\n')
				curr++;
			continue;
		}
		if (ch == Ctrl('E') || ch == KEY_END) {
			for (; curr < clen && buf[curr] != '\n'; curr++) ;
			continue;
		}
		if (ch == KEY_UP) {
			/* To the head for line */
			if (curr > 0 && buf[curr] == '\n')
				curr--;
			i = maxcol;
			for (; curr > 0 && i > 0 && buf[curr] != '\n'; curr--) i--;
			if (i == 0) continue;
			if (curr > 0) {
				curr--;
				i = 0;
				for (; curr > 0 && buf[curr] != '\n';
				     curr--) i++;
				if (curr > 0) curr++;
				if (curr == 0) {
					line_offset =
					    curcol - prompt_offset;
					if (line_offset < 0)
						line_offset = 0;
				} else
					line_offset = curcol;
				/* ���Զ�����������������ƫ�Ƽ��� */
				line_offset += (i / maxcol) * maxcol;
				/* Move to the right position */
				for (i = 0;
				     i < line_offset && buf[curr] != '\n'
				     && curr < clen; i++)
					curr++;
				if (buf[curr] == '\n')
					curr--;
			}
			continue;
		}
		if (ch == KEY_DOWN) {
			/* To the end of line */
			i = maxcol;
     			for (; curr < clen && i > 0 && buf[curr] != '\n'; curr++) i--;
			if (i == 0) continue;
			if (curr < clen) {
				curr++;
				for (i = 0;
				     i < curcol && buf[curr] != '\n' &&
				     curr < clen; i++)
					curr++;
				if (curr > 0 && buf[curr] == '\n')
					curr--;
			}
			continue;
		}
		if (ch == KEY_LEFT) {
			if (curr <= 0)
				continue;
			curr--;
			if (enabledbchar && prev_dbchar)
				curr--;
			continue;
		}
		if (ch == KEY_RIGHT) {
			if (curr >= clen)
				continue;
			curr++;
			if (enabledbchar && curr_dbchar)
				curr++;
			continue;
		}
		if (ch == Ctrl('Q')) {
			if (can_insert && total_line < maxline)
				ch = '\n';
			else
				continue;
		}
		/* Pudding: �ܵ���˵��ɫ�༭���Ǻ���������ȫ,
		   �ر����ַ����϶̵�ʱ�������Ļ�ĺ�������� */
/* 		 if (ch == KEY_ESC) ch = '\033'; */

		/* �����ַ����� */
		if (!isprint2(ch) && ch != '\n')
			continue;
		if (!can_insert)
			continue;

		if (curr == clen) {
			buf[curr] = ch;
			buf[curr + 1] = '\0';
		} else {
			strlcpy(tmp, &buf[curr], len);
			buf[curr] = ch;
			buf[curr + 1] = '\0';
			strcat(buf, tmp);
		}
		curr++;
		clen++;
	}
	buf[clen] = '\0';
	if (buf[0] != Ctrl('Z') && buf[0] != Ctrl('A'))	/* Pudding: �޲���ʾ��"*"������ */
		show_multiline(line, col, buf, maxcol, NULL, NULL);

	showansi = tmpansi;
	clrtoeol();
	refresh();

	return total_line;	/* ���������� */
}
