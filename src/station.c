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

#define CHATD

#include "bbs.h"
#include "chat.h"

#ifdef AIX
#include <sys/select.h>
#endif

#if !RELIABLE_SELECT_FOR_WRITE
#include <fcntl.h>
#endif

#if USES_SYS_SELECT_H
#include <sys/select.h>
#endif

#if NO_SETPGID
#define setpgid setpgrp
#endif

#ifndef L_XTND
#define L_XTND          2	/* relative to end of file */
#endif

#define RESTRICTED(u)   (users[(u)].flags == 0)	/* guest */
#define SYSOP(u)        (users[(u)].flags & (PERM_ACHATROOM|PERM_SYSOP))
					/* PERM_ACHATROOM -- �����ҹ���Ա */
#define CLOAK(u)        (users[(u)].flags & PERM_CHATCLOAK)
#define ROOMOP(u)       (users[(u)].flags & PERM_CHATROOM)
#define OUTSIDER(u)     (users[(u)].flags & PERM_DENYPOST)
#define PERM_TALK	PERM_NOZAP	/* ����һ��������¼����Ȩ */
#define CANTALK(u)	(users[(u)].flags & PERM_TALK)
#define MESG(u)		(users[(u)].flags & PERM_MESSAGE) /* ��������¼��˷�״̬ */

#define ROOM_LOCKED     0x1
#define ROOM_SECRET     0x2
#define ROOM_NOEMOTE    0x4
#define ROOM_SCHOOL     0x8

#define LOCKED(r)       (rooms[(r)].flags & ROOM_LOCKED)
#define SECRET(r)       (rooms[(r)].flags & ROOM_SECRET)
#define NOEMOTE(r)      (rooms[(r)].flags & ROOM_NOEMOTE)
#define NOTALK(r)       (rooms[(r)].flags & ROOM_SCHOOL)

#define ROOM_ALL        (-2)
#define PERM_CHATROOM PERM_CHAT

struct chatuser {
	int sockfd;		/* socket to bbs server */
	int utent;		/* utable entry for this user */
	int room;		/* room: -1 means none, 0 means main */
	int flags;
	char cloak;
	char userid[IDLEN + 2];	/* real userid */
	char chatid[CHAT_IDLEN];	/* chat id */
	char ibuf[128];		/* buffer for sending/receiving */
	int ibufsize;		/* current size of ibuf */
	char host[30];
} users[MAXACTIVE];

struct chatroom {
	char name[IDLEN];	/* name of room; room 0 is "main" */
	short occupants;	/* number of users in room */
	short flags;		/* ROOM_LOCKED, ROOM_SECRET */
	char invites[MAXACTIVE];	/* Keep track of invites to rooms */
	char topic[48];		/* Let the room op to define room topic */
} rooms[MAXROOM];

struct chatcmd {
	char *cmdstr;
	void (*cmdfunc) ();
	int exact;
};

int chatroom, chatport;
int sock = -1;			/* the socket for listening */
int nfds;			/* number of sockets to select on */
int num_conns;			/* current number of connections */
fd_set allfds;			/* fd set for selecting */
struct timeval zerotv;		/* timeval for selecting */
char chatbuf[256];		/* general purpose buffer */
char chatname[19];

/* name of the main room (always exists) */

char mainroom[] = "main";
char *maintopic = "���죬����Ҫ���۵���.....";

char *msg_not_op = "\033[1;37m��\033[32m���������᷿���ϴ�\033[37m ��\033[m";
char *msg_no_such_id =
    "\033[1;37m��\033[32m [\033[36m%s\033[32m] ��������᷿��\033[37m ��\033[m";
char *msg_not_here =
    "\033[1;37m�� \033[32m[\033[36m%s\033[32m] ��û��ǰ����������\033[37m ��\033[m";

static int local;	/* whether the chatroom accepts local connection only */

int
is_valid_chatid(char *id)
{
	int i;

	if (*id == '\0')
		return 0;

	for (i = 0; i < CHAT_IDLEN && *id; i++, id++) {
		if (strchr(BADCIDCHARS, *id))
			return 0;
	}
	return 1;
}

char *
nextword(char **str)
{
	char *p;

	while (isspace(**str))
		(*str)++;
	p = *str;
	while (**str && !isspace(**str))
		(*str)++;
	if (**str) {
		**str = '\0';
		(*str)++;
	}

	return p;
}

int
chatid_to_indx(int unum, char *chatid)
{
	int i;

	for (i = 0; i < MAXACTIVE; i++) {
		if (users[i].sockfd == -1)
			continue;
		if (!strcasecmp(chatid, users[i].chatid)) {
			if (users[i].cloak == 0 || !CLOAK(unum))
				return i;
		}
	}
	return -1;
}

int
fuzzy_chatid_to_indx(int unum, char *chatid)
{
	int i, indx = -1;
	unsigned int len = strlen(chatid);

	for (i = 0; i < MAXACTIVE; i++) {
		if (users[i].sockfd == -1)
			continue;
		if (!strncasecmp(chatid, users[i].chatid, len) ||
		    !strncasecmp(chatid, users[i].userid, len)) {
			if (len == strlen(users[i].chatid) ||
			    len == strlen(users[i].userid)) {
				indx = i;
				break;
			}
			if (indx == -1)
				indx = i;
			else
				return -2;
		}
	}
	if (users[indx].cloak == 0 || CLOAK(unum))
		return indx;
	else
		return -1;
}

int
roomid_to_indx(char *roomid)
{
	int i;

	for (i = 0; i < MAXROOM; i++) {
		if (i && rooms[i].occupants == 0)
			continue;
		if (!strcasecmp(roomid, rooms[i].name))
			return i;
	}
	return -1;
}

void
do_send(fd_set *writefds, char *str)
{
	int i;
	int len = strlen(str);

	if (select(nfds, NULL, writefds, NULL, &zerotv) > 0) {
		for (i = 0; i < nfds; i++) {
			if (FD_ISSET(i, writefds)) {
				send(i, str, len + 1, 0);
			}
		}
	}
}

void
send_to_room(int room, char *str)
{
	int i;
	fd_set writefds;

	FD_ZERO(&writefds);
	for (i = 0; i < MAXACTIVE; i++) {
		if (users[i].sockfd == -1)
			continue;
		if (room == ROOM_ALL || room == users[i].room)
			/* write(users[i].sockfd, str, strlen(str) + 1); */
			FD_SET(users[i].sockfd, &writefds);
	}
	do_send(&writefds, str);
}

void
send_to_unum(int unum, char *str)
{
	fd_set writefds;

	FD_ZERO(&writefds);
	FD_SET(users[unum].sockfd, &writefds);
	do_send(&writefds, str);
}

void
exit_room(int unum, int disp, char *msg)
{
	int oldrnum = users[unum].room;

	if (oldrnum != -1) {
		if (--rooms[oldrnum].occupants) {
			switch (disp) {
			case EXIT_LOGOUT:
				snprintf(chatbuf, sizeof(chatbuf),
					"\033[1;37m�� \033[32m[\033[36m%s\033[32m] �����뿪�� \033[37m��\033[m",
					users[unum].chatid);
				if (msg && *msg) {
					strcat(chatbuf, ": ");
					strncat(chatbuf, msg, 80);
				}
				break;

			case EXIT_LOSTCONN:
				snprintf(chatbuf, sizeof(chatbuf),
					"\033[1;37m�� \033[32m[\033[36m%s\033[32m] ������ߵķ��� ... \033[37m��\033[m",
					users[unum].chatid);
				break;

			case EXIT_KICK:
				snprintf(chatbuf, sizeof(chatbuf),
					"\033[1;37m�� \033[32m[\033[36m%s\033[32m] ���ϴ�ϳ�ȥ�� \033[37m��\033[m",
					users[unum].chatid);
				break;
			}
			if (users[unum].cloak == 0)
				send_to_room(oldrnum, chatbuf);
		}
	}
	users[unum].flags &= ~PERM_CHATROOM;
	users[unum].room = -1;
}

void
chat_rename(int unum, char *msg)
{
	int rnum;

	rnum = users[unum].room;

	if (!ROOMOP(unum) && !SYSOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}
	if (*msg == '\0') {
		send_to_unum(unum,
			     "\033[1;31m�� \033[37m��ָ���µ����������� \033[31m��\033[m");
		return;
	}
	strncpy(rooms[rnum].name, msg, IDLE);
	snprintf(chatbuf, sizeof(chatbuf), "/r%.11s", msg);
	send_to_room(rnum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;37m�� \033[32m[\033[36m%s\033[32m] �����������Ƹ�Ϊ \033[1;33m%.11s \033[37m��\033[m",
		users[unum].chatid, msg);
	send_to_room(rnum, chatbuf);
}

void
chat_topic(int unum, char *msg)
{
	int rnum;

	rnum = users[unum].room;

	if (!ROOMOP(unum) && !SYSOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}
	if (*msg == '\0') {
		send_to_unum(unum, "\033[1;31m�� \033[37m��ָ������ \033[31m��\033[m");
		return;
	}
	strncpy(rooms[rnum].topic, msg, 43);
	rooms[rnum].topic[42] = '\0';
	snprintf(chatbuf, sizeof(chatbuf), "/t%.42s", msg);
	send_to_room(rnum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;37m�� \033[32m[\033[36m%s\033[32m] �������Ϊ \033[1;33m%42.42s \033[37m��\033[m",
		users[unum].chatid, msg);
	send_to_room(rnum, chatbuf);
}

int
enter_room(int unum, char *room, char *msg)
{
	int rnum;
	int op = 0;
	int i;

	rnum = roomid_to_indx(room);
	if (rnum == -1) {
		/* new room */
		if (OUTSIDER(unum)) {
			send_to_unum(unum,
				     "\033[1;31m�� \033[37m��Ǹ�������ܿ��°��� \033[31m��\033[m");
			return 0;
		}
		for (i = 1; i < MAXROOM; i++) {
			if (rooms[i].occupants == 0) {
				rnum = i;
				memset(rooms[rnum].invites, 0, MAXACTIVE);
				strcpy(rooms[rnum].topic, maintopic);
				strncpy(rooms[rnum].name, room, IDLEN - 1);
				rooms[rnum].name[IDLEN - 1] = '\0';
				rooms[rnum].flags = 0;
				op++;
				break;
			}
		}
		if (rnum == -1) {
			send_to_unum(unum,
				     "\033[1;31m�� \033[37m���ǵķ�������� \033[31m��\033[m");
			return 0;
		}
	}
	if (!SYSOP(unum))
		if (LOCKED(rnum) && rooms[rnum].invites[unum] == 0) {
			send_to_unum(unum,
				     "\033[1;31m�� \033[37m�������ֻ����У��������� \033[31m��\033[m");
			return 0;
		}
	exit_room(unum, EXIT_LOGOUT, msg);
	users[unum].room = rnum;
	if (op)
		users[unum].flags |= PERM_CHATROOM;
	rooms[rnum].occupants++;
	rooms[rnum].invites[unum] = 0;
	if (users[unum].cloak == 0) {
		snprintf(chatbuf, sizeof(chatbuf),
			"\033[1;31m�� \033[37m[\033[36;1m%s\033[37m] ���� \033[35m%s \033[37m���� \033[31m��\033[m",
			users[unum].chatid, rooms[rnum].name);
		send_to_room(rnum, chatbuf);
	}
	snprintf(chatbuf, sizeof(chatbuf), "/r%s", room);
	send_to_unum(unum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf), "/t%s", rooms[rnum].topic);
	send_to_unum(unum, chatbuf);
	return 0;
}

void
logout_user(int unum)
{
	int i, sockfd = users[unum].sockfd;

	close(sockfd);
	FD_CLR(sockfd, &allfds);
	memset(&users[unum], 0, sizeof (users[unum]));
	users[unum].sockfd = users[unum].utent = users[unum].room = -1;
	for (i = 0; i < MAXROOM; i++) {
		if (rooms[i].invites != NULL)
			rooms[i].invites[unum] = 0;
	}
	num_conns--;
}

int
print_user_counts(int unum)
{
	int i, c, userc = 0, suserc = 0, roomc = 0;

	for (i = 0; i < MAXROOM; i++) {
		c = rooms[i].occupants;
		if (i > 0 && c > 0) {
			if (!SECRET(i) || SYSOP(unum))
				roomc++;
		}
		c = users[i].room;
		if (users[i].sockfd != -1 && c != -1 && users[i].cloak == 0) {
			if (SECRET(c) && !SYSOP(unum))
				suserc++;
			else
				userc++;
		}
	}
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;31m��\033[37m ��ӭ���١�\033[32m%s\033[37m���ġ�\033[36m%s\033[37m��\033[31m��\033[m", BBSNAME, chatname);
	send_to_unum(unum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;31m��\033[37m Ŀǰ�Ѿ��� \033[1;33m%d \033[37m��������п��� \033[31m��\033[m",
		roomc + 1);
	send_to_unum(unum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf), "\033[1;31m�� \033[37m���������ڹ��� \033[36m%d\033[37m �� ", userc + 1);
	if (suserc) {
		char buf[40];

		snprintf(buf, sizeof(buf), "[\033[36m%d\033[37m ���ڸ߻���������]", suserc);
		strlcat(chatbuf, buf, sizeof(chatbuf));
	}
	strlcat(chatbuf, "\033[31m��\033[m", sizeof(chatbuf));
	send_to_unum(unum, chatbuf);
	return 0;
}

int
login_user(int unum, char *msg)
{
	int i, utent;		/* , fd = users[unum].sockfd; */
	char *utentstr;
	char *level;
	char *userid;
	char *chatid;
	char *cloak;

	utentstr = nextword(&msg);
	level = nextword(&msg);
	userid = nextword(&msg);
	chatid = nextword(&msg);
	cloak = nextword(&msg);

	utent = atoi(utentstr);
	for (i = 0; i < MAXACTIVE; i++) {
		if (users[i].sockfd != -1 && users[i].utent == utent) {
			send_to_unum(unum, CHAT_LOGIN_BOGUS);
			return -1;
		}
	}
	if (!is_valid_chatid(chatid)) {
		send_to_unum(unum, CHAT_LOGIN_INVALID);
		return 0;
	}
	if (chatid_to_indx(0, chatid) != -1) {
		/* userid in use */
		send_to_unum(unum, CHAT_LOGIN_EXISTS);
		return 0;
	}
	if (!strncasecmp("localhost" /* MY_BBS_DOMAIN */ , users[unum].host, 30)) {
		users[unum].flags = atoi(level) & ~(PERM_DENYPOST);
		users[unum].cloak = atoi(cloak);
	} else {
		if (!(atoi(level) & PERM_LOGINOK) && !strncasecmp(chatid, "guest", 8)) {
			send_to_unum(unum, CHAT_LOGIN_INVALID);
			return 0;
		}
		users[unum].flags = PERM_DENYPOST;
		users[unum].cloak = 0;
	}

	users[unum].utent = utent;
	strcpy(users[unum].userid, userid);
	strncpy(users[unum].chatid, chatid, CHAT_IDLEN - 1);
	users[unum].chatid[CHAT_IDLEN - 1] = '\0';
	send_to_unum(unum, CHAT_LOGIN_OK);
	print_user_counts(unum);
	enter_room(unum, mainroom, (char *)NULL);
	return 0;
}

void
chat_list_rooms(int unum, char *msg)
{
	int i, occupants;

	if (RESTRICTED(unum)) {
		send_to_unum(unum, "\033[1;31m�� \033[37m��Ǹ���ϴ����㿴����Щ�����п��� \033[31m��\033[m");
		return;
	}
	send_to_unum(unum, "\033[1;33;44m ̸��������  ������������            \033[m");
	for (i = 0; i < MAXROOM; i++) {
		occupants = rooms[i].occupants;
		if (occupants > 0) {
			if (!SYSOP(unum))
				if ((rooms[i].flags & ROOM_SECRET) &&
				    (users[unum].room != i))
					continue;
			snprintf(chatbuf, sizeof(chatbuf),
				" \033[1;32m%-12s\033[37m��\033[36m%4d\033[37m��\033[33m%s\033[m",
				rooms[i].name, occupants, rooms[i].topic);
			if (rooms[i].flags & ROOM_LOCKED)
				strlcat(chatbuf, " [��ס]", sizeof(chatbuf));
			if (rooms[i].flags & ROOM_SECRET)
				strlcat(chatbuf, " [����]", sizeof(chatbuf));
			if (rooms[i].flags & ROOM_NOEMOTE)
				strlcat(chatbuf, " [��ֹ����]", sizeof(chatbuf));
			if (rooms[i].flags & ROOM_SCHOOL)
				strlcat(chatbuf, " [һ������]", sizeof(chatbuf));
			send_to_unum(unum, chatbuf);
		}
	}
}

void
chat_do_user_list(int unum, char *msg, int whichroom)
{
	int start, stop, curr = 0;
	int i, rnum, myroom = users[unum].room;

	while (*msg && isspace(*msg))
		msg++;
	start = atoi(msg);
	while (*msg && isdigit(*msg))
		msg++;
	while (*msg && !isdigit(*msg))
		msg++;
	stop = atoi(msg);
	send_to_unum(unum,
		     "\033[1;33;44m ������ũ�ʹ���ߴ���  ����    ��    �ҡ�Op������                          \033[m");
	for (i = 0; i < MAXACTIVE; i++) {
		rnum = users[i].room;
		if (users[i].sockfd != -1 && rnum != -1 &&
		    !(users[i].cloak == 1 && !CLOAK(unum))) {
			if (whichroom != -1 && whichroom != rnum)
				continue;
			if (myroom != rnum) {
				if (RESTRICTED(unum))
					continue;
				if ((rooms[rnum].flags & ROOM_SECRET) &&
				    !SYSOP(unum))
					continue;
			}
			curr++;
			if (curr < start)
				continue;
			else if (stop && (curr > stop))
				break;
			snprintf(chatbuf, sizeof(chatbuf),
				"\033[1;5m%c\033[0;1;37m%-8s��\033[31m%s%-12s\033[37m��\033[32m%-14s\033[37m��\033[34m%-2s\033[37m��\033[35m%-30s\033[m",
				(users[i].cloak == 1) ? 'C' : ' ',
				users[i].chatid,
				OUTSIDER(i) ? "\033[1;35m" : "\033[1;36m",
				users[i].userid, rooms[rnum].name,
				ROOMOP(i) ? "��" : "��", users[i].host);
			send_to_unum(unum, chatbuf);
		}
	}
}

void
chat_list_by_room(int unum, char *msg)
{
	int whichroom;
	char *roomstr;

	roomstr = nextword(&msg);
	if (*roomstr == '\0')
		whichroom = users[unum].room;
	else {
		if ((whichroom = roomid_to_indx(roomstr)) == -1) {
			snprintf(chatbuf, sizeof(chatbuf),
				"\033[1;31m�� \033[37mû %s �������� \033[31m��\033[m",
				roomstr);
			send_to_unum(unum, chatbuf);
			return;
		}
		if ((rooms[whichroom].flags & ROOM_SECRET) && !SYSOP(unum)) {
			send_to_unum(unum,
				     "\033[1;31m�� \033[37m���������ķ���Թ����ģ�û������ \033[31m��\033[m");
			return;
		}
	}
	chat_do_user_list(unum, msg, whichroom);
}

void
chat_list_users(int unum, char *msg)
{
	chat_do_user_list(unum, msg, -1);
}

void
chat_map_chatids(int unum, int whichroom)
{
	int i, c, myroom, rnum;

	myroom = users[unum].room;
	send_to_unum(unum,
		     "\033[1;33;44m ������� ʹ���ߴ���  �� ������� ʹ���ߴ���  �� ������� ʹ���ߴ��� \033[m");
	for (i = 0, c = 0; i < MAXACTIVE; i++) {
		rnum = users[i].room;
		if (users[i].sockfd != -1 && rnum != -1 &&
		    !(users[i].cloak == 1 && !CLOAK(unum))) {
			if (whichroom != -1 && whichroom != rnum)
				continue;
			if (myroom != rnum) {
				if (RESTRICTED(unum))
					continue;
				if ((rooms[rnum].flags & ROOM_SECRET) &&
				    !SYSOP(unum))
					continue;
			}
			snprintf(chatbuf + (c * 50), sizeof(chatbuf) - c * 50,
				"\033[1;34;5m%c\033[m\033[1m%-8s%c%s%-12s%s\033[m",
				(users[i].cloak == 1) ? 'C' : ' ',
				users[i].chatid, (ROOMOP(i)) ? '*' : ' ',
				OUTSIDER(i) ? "\033[1;35m" : "\033[1;36m",
				users[i].userid, (c < 2 ? "��" : "  "));
			if (++c == 3) {
				send_to_unum(unum, chatbuf);
				c = 0;
			}
		}
	}
	if (c > 0)
		send_to_unum(unum, chatbuf);
}

void
chat_map_chatids_thisroom(int unum, char *msg)
{
	chat_map_chatids(unum, users[unum].room);
}

void
chat_setroom(int unum, char *msg)
{
	char *modestr;
	int rnum = users[unum].room;
	int sign = 1;
	int flag;
	char *fstr;

	modestr = nextword(&msg);
	if (!ROOMOP(unum) && !SYSOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}
	if (*modestr == '+')
		modestr++;
	else if (*modestr == '-') {
		modestr++;
		sign = 0;
	}
	if (*modestr == '\0') {
		send_to_unum(unum,
			     "\033[1;31m�� \033[37m����߹�̨��Ҫ�ķ�����: {[\033[32m+\033[37m(�趨)][\033[32m-\033[37m(ȡ��)]}{[\033[32ml\033[37m(��ס)][\033[32ms\033[37m(����)]}\033[m");
		return;
	}
	while (*modestr) {
		flag = 0;
		switch (*modestr) {
		case 'l':
		case 'L':
			flag = ROOM_LOCKED;
			fstr = "��ס";
			break;
		case 's':
		case 'S':
			flag = ROOM_SECRET;
			fstr = "����";
			break;
		case 'e':
		case 'E':
			flag = ROOM_NOEMOTE;
			fstr = "��ֹ����";
			break;
		case 'a':
		case 'A':
			flag = ROOM_SCHOOL;
			fstr = "һ������";
			break;
		default:
			snprintf(chatbuf, sizeof(chatbuf),
				"\033[1;31m�� \033[37m��Ǹ�������������˼��[\033[36m%c\033[37m] \033[31m��\033[m", *modestr);
			send_to_unum(unum, chatbuf);
			return;
		}
		if (flag && ((rooms[rnum].flags & flag) != sign * flag)) {
			rooms[rnum].flags ^= flag;
			snprintf(chatbuf, sizeof(chatbuf),
				"\033[1;37m��\033[32m �ⷿ�䱻 %s %s%s����ʽ \033[37m��\033[m",
				users[unum].chatid, sign ? "�趨Ϊ" : "ȡ��", fstr);
			send_to_room(rnum, chatbuf);
		}
		modestr++;
	}
}

void
chat_nick(int unum, char *msg)
{
	char *chatid;
	int othernum;

	chatid = nextword(&msg);
	chatid[CHAT_IDLEN - 1] = '\0';
	if (!is_valid_chatid(chatid)) {
		send_to_unum(unum,
			     "\033[1;31m�� \033[37m���������ǲ���д����\033[31m ��\033[m");
		return;
	}
	othernum = chatid_to_indx(0, chatid);
	if (othernum != -1 && othernum != unum) {
		send_to_unum(unum,
			     "\033[1;31m�� \033[37m��Ǹ�����˸���ͬ���������㲻�ܽ��� \033[31m��\033[m");
		return;
	}
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;31m�� \033[36m%s \033[0;37m�Ѿ�����Ϊ \033[1;33m%s \033[31m��\033[m",
		users[unum].chatid, chatid);
	send_to_room(users[unum].room, chatbuf);
	strcpy(users[unum].chatid, chatid);
	snprintf(chatbuf, sizeof(chatbuf), "/n%s", users[unum].chatid);
	send_to_unum(unum, chatbuf);
}

void
chat_private(int unum, char *msg)
{
	char *recipient;
	int recunum;

	recipient = nextword(&msg);
	recunum = fuzzy_chatid_to_indx(unum, recipient);
	if (recunum < 0) {
		/* no such user, or ambiguous */
		if (recunum == -1) {
			snprintf(chatbuf, sizeof(chatbuf), msg_no_such_id, recipient);
		} else {
			strlcpy(chatbuf, "\033[1;31m ��\033[37m ��λ�����߽�ʲô���� \033[31m��\033[m", sizeof(chatbuf));
		}
		send_to_unum(unum, chatbuf);
		return;
	}
	if (!MESG(recunum)) {
		strlcpy(chatbuf, "\033[1;31m ��\033[37m �Է��ر��˱���� \033[31m��\033[m", sizeof(chatbuf));
		send_to_unum(unum, chatbuf);
		return;
	}
	if (*msg) {
		snprintf(chatbuf, sizeof(chatbuf),
			"\033[1;32m �� \033[36m%s \033[37m��ֽ��С��������\033[m: ",
			users[unum].chatid);
		strlcat(chatbuf, msg, sizeof(chatbuf));
		send_to_unum(recunum, chatbuf);
		snprintf(chatbuf, sizeof(chatbuf), "\033[1;32m �� \033[37mֽ���Ѿ������� \033[36m%s\033[m: ", users[recunum].chatid);
		strlcat(chatbuf, msg, sizeof(chatbuf));
		send_to_unum(unum, chatbuf);
	} else {
		strlcpy(chatbuf, "\033[1;31m ��\033[37m ��Ҫ���Է�˵Щʲôѽ��\033[31m��\033[m", sizeof(chatbuf));
		send_to_unum(unum, chatbuf);
	}
}

void
put_chatid(int unum, char *str)
{
	int i;
	char *chatid = users[unum].chatid;

	memset(str, ' ', 10);
	for (i = 0; chatid[i]; i++)
		str[i] = chatid[i];
	str[i] = ':';
	str[10] = '\0';
}

void
chat_allmsg(int unum, char *msg)
{
	if (*msg) {
		put_chatid(unum, chatbuf);
		strcat(chatbuf, msg);
		send_to_room(users[unum].room, chatbuf);
	}
}

void
chat_act(int unum, char *msg)
{
	if (NOEMOTE(users[unum].room)) {
		send_to_unum(unum, "�������ҽ�ֹ����");
		return;
	}

	if (*msg) {
		snprintf(chatbuf, sizeof(chatbuf), "\033[1;36m%s \033[37m%s\033[m", users[unum].chatid, msg);
		send_to_room(users[unum].room, chatbuf);
	}
}

void
chat_cloak(int unum, char *msg)
{
	if (CLOAK(unum)) {
		if (users[unum].cloak == 1)
			users[unum].cloak = 0;
		else
			users[unum].cloak = 1;
		snprintf(chatbuf, sizeof(chatbuf), "\033[1;36m%s \033[37m%s ����״̬...\033[m",
			users[unum].chatid,
			(users[unum].cloak == 1) ? "����" : "ֹͣ");
		send_to_unum(unum, chatbuf);
	}
}

void
chat_join(int unum, char *msg)
{
	char *roomid;

	roomid = nextword(&msg);
	if (RESTRICTED(unum)) {
		send_to_unum(unum,
			     "\033[1;31m�� \033[37m��ֻ������������ \033[31m��\033[m");
		return;
	}
	if (*roomid == '\0') {
		send_to_unum(unum, "\033[1;31m�� \033[37m�����ĸ����� \033[31m��\033[m");
		return;
	}
	if (!is_valid_chatid(roomid)) {
		send_to_unum(unum,
			     "\033[1;31m��\033[37m�������в����С�*:/%���Ȳ��Ϸ��ַ�\033[31m��\033[37m");
		return;
	}
	enter_room(unum, roomid, msg);
	return;
}

void
chat_kick(int unum, char *msg)
{
	char *twit;
	int rnum = users[unum].room;
	int recunum;

	twit = nextword(&msg);
	if (!ROOMOP(unum) && !SYSOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}
	if ((recunum = chatid_to_indx(unum, twit)) == -1) {
		snprintf(chatbuf, sizeof(chatbuf), msg_no_such_id, twit);
		send_to_unum(unum, chatbuf);
		return;
	}
	if (rnum != users[recunum].room) {
		snprintf(chatbuf, sizeof(chatbuf), msg_not_here, users[recunum].chatid);
		send_to_unum(unum, chatbuf);
		return;
	}
	exit_room(recunum, EXIT_KICK, (char *) NULL);

	if (rnum == 0) {
		logout_user(recunum);
	} else {
		enter_room(recunum, mainroom, (char *) NULL);
	}
}

void
chat_makeop(int unum, char *msg)
{
	char *newop = nextword(&msg);
	int rnum = users[unum].room;
	int recunum;

	if (!ROOMOP(unum) && !SYSOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}
	if ((recunum = chatid_to_indx(unum, newop)) == -1) {
		/* no such user */
		snprintf(chatbuf, sizeof(chatbuf), msg_no_such_id, newop);
		send_to_unum(unum, chatbuf);
		return;
	}
	if (unum == recunum) {
		strlcpy(chatbuf, "\033[1;37m�� \033[32m�������㱾�������ϴ�� \033[37m��\033[m", sizeof(chatbuf));
		send_to_unum(unum, chatbuf);
		return;
	}
	if (rnum != users[recunum].room) {
		snprintf(chatbuf, sizeof(chatbuf), msg_not_here, users[recunum].chatid);
		send_to_unum(unum, chatbuf);
		return;
	}
	users[unum].flags &= ~PERM_CHATROOM;
	users[recunum].flags |= PERM_CHATROOM;
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;37m�� \033[36m %s\033[32m������ \033[35m%s \033[32m���ϴ� \033[37m��\033[m",
		users[unum].chatid, users[recunum].chatid);
	send_to_room(rnum, chatbuf);
}

void
chat_invite(int unum, char *msg)
{
	char *invitee = nextword(&msg);
	int rnum = users[unum].room;
	int recunum;

	if (!ROOMOP(unum) && !SYSOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}
	if ((recunum = chatid_to_indx(unum, invitee)) == -1) {
		snprintf(chatbuf, sizeof(chatbuf), msg_not_here, invitee);
		send_to_unum(unum, chatbuf);
		return;
	}
	if (rooms[rnum].invites[recunum] == 1) {
		snprintf(chatbuf, sizeof(chatbuf), "\033[1;37m�� \033[36m%s \033[32m��һ�¾��� \033[37m��\033[m", users[recunum].chatid);
		send_to_unum(unum, chatbuf);
		return;
	}
	rooms[rnum].invites[recunum] = 1;
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;37m�� \033[36m%s \033[32m�������� [\033[33m%s\033[32m] ��������\033[37m ��\033[m",
		users[unum].chatid, rooms[rnum].name);
	send_to_unum(recunum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf), "\033[1;37m�� \033[36m%s \033[32m��һ�¾��� \033[37m��\033[m", users[recunum].chatid);
	send_to_unum(unum, chatbuf);
}

void
chat_broadcast(int unum, char *msg)
{
	if (!SYSOP(unum)) {
		send_to_unum(unum,
			     "\033[1;31m�� \033[37m�㲻�����ڻ������ڴ������� \033[31m��\033[m");
		return;
	}
	if (*msg == '\0') {
		send_to_unum(unum, "\033[1;37m�� \033[32m�㲥������ʲô \033[37m��\033[m");
		return;
	}
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1m Ding Dong!! �����ұ��棺 \033[36m%s\033[37m �л��Դ��������\033[m",
		users[unum].chatid);
	send_to_room(ROOM_ALL, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf), "\033[1;34m��\033[33m%s\033[34m��\033[m", msg);
	send_to_room(ROOM_ALL, chatbuf);
}

void
chat_goodbye(int unum, char *msg)
{
	exit_room(unum, EXIT_LOGOUT, msg);
}

/* -------------------------------------------- */
/* MUD-like social commands : action             */
/* -------------------------------------------- */

struct action {
	char *verb;		/* ���� */
	char *part1_msg;	/* ��� */
	char *part2_msg;	/* ���� */
};

struct action party_data[] = {
	{"admire", "��", "�ľ���֮���������Ͻ�ˮ���಻��"},
	{"agree", "��ȫͬ��", "�Ŀ���"},
	{"bearhug", "�����ӵ��", ""},
	{"ben", "Ц�Ǻǵض�", "˵������, ���ⶼ��֪��... :P��"},
	{"bless", "ף��", "�����³�"},
	{"bow", "�Ϲ��Ͼ�����", "�Ϲ�"},
	{"bye", "��", "һ���ֵ�������ɽ���ɣ���ˮ�����������»��ٻ�!��"},
	{"bye1", "�޿����������", "���½�:���᲻�����߰�...��"},
	{"bye2", "����ض�",
	 "˵��:������û�в�ɢ����ϯ��������һ���ˣ���Ҷౣ��.��"},
	{"byebye", "����", "��ȥ�ı�Ӱ������ʧ�����ξ�Ө���Ứ�����߻���"},
	{"bite", "�ݺݵ�ҧ��", "һ�ڣ�����ʹ�����۴��...��ˬ,������"},
	{"blink", "��װʲôҲ��֪������", "�����գ��գ�۾�"},
	{"breath", ":�Ͽ��", "���˹�����!"},
	{"brother", "��",
	 "һ��ȭ,��:���ѵ����Ҹε�����,��������������,�������?��"},
	{"bigfoot", "�����ţ���׼", "����������ݺݵز���ȥ"},
	{"consider", "��ʼ���濼��ɱ��", "�Ŀ����Ժͼ����Ѷ�"},
	{"caress", "����", ""},
	{"cringe", "��", "������ϥ��ҡβ����"},
	{"cry", "��", "�������"},
	{"cry1", "Խ��Խ���ģ�����ſ��", "�ļ���Ϻ����������"},
	{"cry2", "����",
	 "���ϣ��޵���ȥ����:������ѽ!���ݵķ������㻹û������!��"},
	{"cold", "����", "�Ļ������ֱ��Ƥ���"},
	{"comfort", "���԰�ο", ""},
	{"clap", "��", "���ҹ���"},
	{"dance", "����", "������������"},
	{"die", "�ܿ���ͳ�һ�ѷ���ˮǹ������!��һǹ��", "������."},
	{"dog", "���š��Ź��� ��", "ҧ���仨��ˮ"},
	{"dogleg", "�����ڹ�������������, ʹ������", "����ƨ"},
	{"dontno", "��", "ҡҡͷ˵��: �����...�Ҳ�֪��...��"},
	{"dontdo", "��", "˵:��С���ѣ� ����������Ŷ���������ǲ��õ�Ӵ��"},
	{"drivel", "����", "����ˮ"},
	{"dunno", "�ɴ��۾���������ʣ�", "����˵ʲô�Ҳ���Ү... :("},
	{"dlook", "����������", "����ˮ������������һ��"},
	{"dove", "����", "һ��DOVE��˵:���ţ�����һ��DOVE��Ҫ�ú�����Ŷ��"},
	{"emlook", "���¶�����",
	 "���ۡ�����С��ѽ,�ҵ���˭��,Ҳ����������Ұ!��"},
	{"face", "��Ƥ�Ķ�", "���˸�����."},
	{"faceless", "����", "��е������ٺ�, �������������Ǯһ��?��"},
	{"farewell", "����,�����������", "����"},
	{"fear", "��", "¶�����µı���"},
	{"flook", "�ճյ�����", ", �����������˵����һ��."},
	{"forgive", "��ȵĶ�", "˵�����ˣ�ԭ������"},
	{"giggle", "����", "ɵɵ�Ĵ�Ц"},
	{"grin", "��", "¶��а���Ц��"},
	{"growl", "��", "��������"},
	{"hammer", "��������50000000T������$", "������һ��!,*** ��� �� ��!"},
	{"hand", "��", "����"},
	{"heng", "��������",
	 "һ�ۣ� ����һ�����߸ߵİ�ͷ��������,��мһ�˵�����..."},
	{"hi", "��", "������ò��˵��һ������Hi! ���!��"},
	{"hug", "�����ӵ��", ""},
	{"kick", "��", "�ߵ���ȥ����"},
	{"kiss", "����", "������"},
	{"laugh", "������Ц", ""},
	{"look", "�����ؿ���", "����֪���ڴ�ʲô�����⡣"},
	{"love", "���������", "�������Լ�������ta"},
	{"love2", "��", "�������˵������ԸΪ�I���У��ڵ�ԸΪ<��ը��>"},
	{"lovelook", "һ˫ˮ�����Ĵ��۾����������ؿ���", "!"},
	{"missyou", "����һЦ������ȴ��������:", "������������ⲻ�����Σ�"},
	{"meet", "��", "һ��ȭ��˵���������Ŵ���������һ���������������ң���"},
	{"moon", "����",
	 "��С�֣�ָ�ų���������˵�����������£������ǵ�֤�ˡ�"},
	{"nod", "��", "��ͷ����"},
	{"nothing", "��æ����", "��С��һ׮������ҳݣ�"},
	{"nudge", "�����ⶥ", "�ķʶ���"},
	{"oh", "��", "˵����Ŷ�����Ӱ�����"},
	{"pad", "����", "�ļ��"},
	{"papa", "���ŵض�", "˵�����Һ�����Ŷ...��"},
	{"papaya", "������", "��ľ���Դ�"},
	{"praise", "��", "˵��: ��Ȼ����! ������!"},
	{"pinch", "�����İ�", "š�ĺ���"},
	{"poor", "����", "����˵���������ĺ��ӣ��������৵ص�������....."},
	{"punch", "�ݺ�����", "һ��"},
	{"puke", "����", "�°��°�����˵�¶༸�ξ�ϰ����"},
	{"pure", "��", "¶�������Ц��"},
	{"qmarry", "��", "�¸ҵع�����������Ը��޸�����---���������ɼΰ�!"},
	{"report", "͵͵�ض�", "˵���������Һ��𣿡�"},
	{"rose", "ͻȻ������ó�һ��-`-,-<@ ������׸�", "��"},
	{"rose999", "��", "�����������Ѿ�Ϊ�����£��Űپ�ʮ�Ŷ�õ�塭����"},
	{"run", "���������ض�",
	 "˵:�һ��˰�ƥ������ҹ��̸���.���ټ���������Ҳ�ĸ�"},
	{"shrug", "���ε���", "�����ʼ��"},
	{"sigh", "��", "̾��һ����,��: �����׺���Ϊˮ,��ȴ��ɽ������..."},
	{"slap", "žž�İ���", "һ�ٶ���"},
	{"smooch", "ӵ����", ""},
	{"snicker", "�ٺٺ�..�Ķ�", "��Ц"},
	{"sniff", "��", "��֮�Ա�"},
	{"spank", "�ð��ƴ�", "���β�"},
	{"squeeze", "������ӵ����", ""},
	{"sorry", "�е���", "ʮ����ֵ�Ǹ��, ���ǵ���˵��:�Ҹе��ǳ��ı�Ǹ!"},
	{"thank", "��", "��л"},
	{"thanks", "���ڵ��϶�", "�����Ŀ��˼���ͷ:��Ĵ����£�û������!"},
	{"tickle", "��ߴ!��ߴ!ɦ", "����"},
	{"wake", "Ŭ����ҡҡ", "��������ߴ�У��������ѣ��������ģ���"},
	{"wakeup", "ҡ��",
	 "�������������ѡ������������ߴ�У��� ��! ������! ��"},
	{"wave", "����", "ƴ����ҡ��"},
	{"welcome", "���һ�ӭ", "�ĵ���"},
	{"wing", "����һ��ά����ɵɵ��˵���㡣������", "��"},
	{"wink", "��", "���ص�գգ�۾�"},
	{"zap", "��", "���Ĺ���"},
	{"xinku", "�ж�������ӯ��������", "��۸ߺ������׳������ˣ���"},
	{"bupa", "����������",
	 "��ͷ,˵:��С����,����,����ʲôί���������㱨��!��"}, {"gril",
								  "�Ͻ���",
								  "������,����:����С,������ȥ."},
	{":)..", "��", "�������ߣ���֪����һ�����кξٶ�"},
	{"?", "���ɻ�Ŀ���", ""},
	{"@@", "�������۾�����ض���", ":����...��Ҳ̫....��"},
	{"@@!", "�ݺݵص���", "һ��, �����̱�������С��һ��"},
	{"mail", "��", "����������һ���а�ɫ��ĩ���ʼ�"},
	{NULL, NULL, NULL}
};

int
party_action(int unum, char *cmd, char *party, int self)
{
	int i;

	for (i = 0; party_data[i].verb; i++) {
		if (!strcmp(cmd, party_data[i].verb)) {
			if (*party == '\0') {
				party = "���";
			} else {
				int recunum = fuzzy_chatid_to_indx(unum, party);

				if (recunum < 0) {
					/* no such user, or ambiguous */
					if (recunum == -1) {
						snprintf(chatbuf, sizeof(chatbuf), msg_no_such_id, party);
					} else {
						strlcpy(chatbuf, "\033[1;31m�� \033[37m�����ļ䷿�� \033[31m��\033[m", sizeof(chatbuf));
					}
					send_to_unum(unum, chatbuf);
					return 0;
				}
				party = users[recunum].chatid;
			}
			snprintf(chatbuf, sizeof(chatbuf),
				"\033[1;36m%s \033[32m%s\033[33m %s \033[32m%s\033[37;0m",
				users[unum].chatid, party_data[i].part1_msg,
				party, party_data[i].part2_msg);
			if (YEA == self) {
				send_to_unum(unum, "\033[1;37m������ʾ��\033[m");
				send_to_unum(unum, chatbuf);
				send_to_unum(unum, "");
			} else {
				send_to_room(users[unum].room, chatbuf);
			}
			return 0;
		}
	}
	return 1;
}

/* -------------------------------------------- */
/* MUD-like social commands : speak              */
/* -------------------------------------------- */

struct action speak_data[] = {
	{"ask", "ѯ��", NULL},
	{"chant", "����", NULL},
	{"cheer", "�Ȳ�", NULL},
	{"chuckle", "��Ц", NULL},
	{"curse", "����", NULL},
	{"demand", "Ҫ��", NULL},
	{"frown", "��ü", NULL},
	{"groan", "����", NULL},
	{"grumble", "����ɧ", NULL},
	{"hum", "������", NULL},
	{"moan", "��̾", NULL},
	{"notice", "ע��", NULL},
	{"order", "����", NULL},
	{"ponder", "��˼", NULL},
	{"pout", "������˵", NULL},
	{"pray", "��", NULL},
	{"request", "����", NULL},
	{"shout", "���", NULL},
	{"sing", "����", NULL},
	{"smile", "΢Ц", NULL},
	{"smirk", "��Ц", NULL},
	{"swear", "����", NULL},
	{"tease", "��Ц", NULL},
	{"whimper", "���ʵ�˵", NULL},
	{"yawn", "��Ƿ����", NULL},
	{"yell", "��", NULL},
	{NULL, NULL, NULL}
};

int
speak_action(int unum, char *cmd, char *msg, int self)
{
	int i;

	for (i = 0; speak_data[i].verb; i++) {
		if (!strcmp(cmd, speak_data[i].verb)) {
			snprintf(chatbuf, sizeof(chatbuf), "\033[1;36m%s \033[32m%s��\033[33m %s\033[37;0m",
				users[unum].chatid, speak_data[i].part1_msg, msg);
			if (YEA == self) {
				send_to_unum(unum, "\033[1;37m������ʾ��\033[m");
				send_to_unum(unum, chatbuf);
				send_to_unum(unum, "");
			} else {
				send_to_room(users[unum].room, chatbuf);
			}
			return 0;
		}
	}
	return 1;
}

/* -------------------------------------------- */
/* MUD-like social commands : condition          */
/* -------------------------------------------- */

struct action condition_data[] = {
	{"acid", "˵�������������~~~��", NULL},
	{"addoil", "���ոߺ�: ����!!", NULL},
	{"applaud", "žžžžžžž....", NULL},
	{"blush", "��������", NULL},
	{"boss", "������У������������ˣ��ϰ����ˣ���Ҫ���ˣ��ټ��˸�λ.",
	 NULL},
	{"bug", "����˵������վ������ץ��һֻ���ӡ���", NULL},
	{"cool", "���������������������coolŶ����", NULL},
	{"cough", "���˼���", NULL},
	{"die", "���°�ĭ��˫��һ�������崤�˼��£������ˣ���һ�����ˡ�", NULL},
	{"goeat", "�Ķ����ֹ����Ľ���,��,���ò�ȥ��ʳ����#$%&�ķ�����", NULL},
	{"faint", "�۵�һ�����ε��ڵ�...", NULL},
	{"faint1", "���°�ĭ���赹�ڵ��ϡ�", NULL},
	{"faint2", "���п�����Ѫ�������ڵ��ϡ�", NULL},
	{"haha", "��������......", NULL},
	{"handup", "ƴ�����쳤�Լ����ֱۣ������е������ң��ң��ң���", NULL},
	{"happy", "r-o-O-m....������ˬ��", NULL},
	{"heihei", "��Цһ��", NULL},
	{"jump", "���˵���С�����Ƶģ�����������ı�������", NULL},
	{"pavid", "��ɫ�԰�!�������ʲô!", NULL},
	{"puke", "����ģ������˶�����", NULL},
	{"shake", "ҡ��ҡͷ", NULL},
	{"sleep", "Zzzzzzzzzz�������ģ�����˯����", NULL},
	{"so", "�ͽ���!!", NULL},
	{"strut", "��ҡ��ڵ���", NULL},
	{"suicide", "������鶹����һͷײ��, �����������û��Ǯ, ֻ����һ��",
	 NULL},
	{"toe", "������������, ����ר�ĵ������Լ��Ľ�ֺͷ��", NULL},
	{"tongue", "��������ͷ", NULL},
	{"think", "����ͷ����һ��", NULL},
	{"wa", "�ۣ�:-O", NULL},
	{"wawl", "���춯�صĿ�", NULL},
	{"xixi", "͵Ц����������", NULL},
	{"ya", "���������ʤ��������! �� V ��:�� ������...��", NULL},
	{":(", "��ü������,һ����ù��", NULL},
	{":)", "������¶�����ı���.", NULL},
	{":D", "�ֵĺϲ�£��", NULL},
	{":P", "��������ͷ,���ҧ���Լ�", NULL},
	{"@@", "Ҫ������ %^#@%$#&^*&(&^$#%$#@(*&()*)_*&(#@%$^%&^.", NULL},
	{"sing1", "�������ţ������ޱߡ�����������", NULL},
	{"mail", "��", "����������һ���а�ɫ��ĩ���ʼ���"},
	{"wonder", "���ɵ�˵���ǲ�����ģ�", NULL},
	{"apache", "���Ű�����ֱ�����뿪�����ǿ���ˣ�", NULL},
	{"parapara", "�����ڿ������裬�������ƺ�����parapara��", NULL},
	{NULL, NULL, NULL}
};

int
condition_action(int unum, char *cmd, int self)
{
	int i;

	for (i = 0; condition_data[i].verb; i++) {
		if (!strcmp(cmd, condition_data[i].verb)) {
			snprintf(chatbuf, sizeof(chatbuf), "\033[1;36m%s \033[33m%s\033[37;0m",
				users[unum].chatid, condition_data[i].part1_msg);
			if (YEA == self) {
				send_to_unum(unum, "\033[1;37m������ʾ��\033[m");
				send_to_unum(unum, chatbuf);
				send_to_unum(unum, "");
			} else {
				send_to_room(users[unum].room, chatbuf);
			}
			return 1;
		}
	}
	return 0;
}

/* -------------------------------------------- */
/* MUD-like social commands : help               */
/* -------------------------------------------- */

char *dscrb[] = {
	"\033[1m�� Verb + Nick��   ���� + �Է����� ��\033[m",
	"\033[1m�� Verb + Message������ + Ҫ˵�Ļ� ��\033[m",
	"\033[1m�� Verb������ ��\033[m", NULL
};
struct action *verbs[] = { party_data, speak_data, condition_data, NULL };

#define SCREEN_WIDTH    80
#define MAX_VERB_LEN    10
#define VERB_NO         8

/* monster: following function was written and enhanced by me */

void
view_action_verb(int unum, char *id)
{
	int i, j;
	char *p;

	i = atoi(id) - 1;
	if (i >= 0 && i <= 2) {
		j = 0;
		chatbuf[0] = '\0';
		send_to_unum(unum, dscrb[i]);
		while ((p = verbs[i][j++].verb)) {
			strcat(chatbuf, p);
			if ((j % VERB_NO) == 0) {
				send_to_unum(unum, chatbuf);
				chatbuf[0] = '\0';
			} else {
				strncat(chatbuf, "        ",
					MAX_VERB_LEN - strlen(p));
			}
		}
		if (j % VERB_NO)
			send_to_unum(unum, chatbuf);

		send_to_unum(unum, " ");
		return;
	}

	if (party_action(unum, id, "", YEA) == 0)
		return;
	if (speak_action(unum, id, "�㰮�����Ұ���", YEA) == 0)
		return;
	if (condition_action(unum, id, YEA) == 1)
		return;

	send_to_unum(unum, "\033[1;37m�� //help [���] ��ö����б��� //help [����] �ۿ�����ʾ��\033[m");
	send_to_unum(unum, "  ");
	for (i = 0; dscrb[i]; i++) {
		snprintf(chatbuf, sizeof(chatbuf), "%d. %s", i + 1, dscrb[i]);
		send_to_unum(unum, chatbuf);
	}
	send_to_unum(unum, " ");
}

/* Henry: ����һ�����ʵ�ʵ�֣�����ǰ��Ҫ�����˷�^_^ */

void
chat_maketalk(int unum, char *msg)
{
	char *newop = nextword(&msg);
	int recunum;

	if (!SYSOP(unum) && !ROOMOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}

	if ((recunum = chatid_to_indx(unum, newop)) == -1) {
		/* no such user */
		snprintf(chatbuf, sizeof(chatbuf), msg_no_such_id, newop);
		send_to_unum(unum, chatbuf);
		return;
	}

	if (unum == recunum) {
		strlcpy(chatbuf, "\033[1;37m�� \033[32m�����ϴ󰡣��л�ֱ��˵������ \033[37m��\033[m", sizeof(chatbuf));
		send_to_unum(unum, chatbuf);
		return;
	}

	users[recunum].flags ^= PERM_TALK;
	if (users[recunum].flags & PERM_TALK) {
		snprintf(chatbuf, sizeof(chatbuf),
			"\033[1;37m�� \033[36m%s \033[32m��������˷罻�� \033[35m%s \033[32m \033[37m��\033[m",
			users[unum].chatid, users[recunum].chatid);
	} else {
		snprintf(chatbuf, sizeof(chatbuf),
			"\033[1;37m�� \033[36m%s \033[32m����û�� \033[35m%s \033[32m����˷�  \033[37m��\033[m",
			users[unum].chatid, users[recunum].chatid);
	}
	send_to_room(users[recunum].room, chatbuf);
}

void
chat_hand(int unum, char *msg)
{
	int i;

	if (!NOTALK(users[unum].room)) {
		send_to_unum(unum, "\033[1;31m ��\033[37m ���㳩�����ԣ�������� \033[31m��\033[m");
		return;
	}

	for (i = 0; i < MAXACTIVE; i++) {
		if (SYSOP(i) || ROOMOP(i)) {
			snprintf(chatbuf, sizeof(chatbuf), "\033[1;37m�� \033[36m%s \033[32m�������֣�������˵ʲô������ \033[37m��\033[m", users[unum].chatid);
			send_to_unum(i, chatbuf);
		}
	}

	snprintf(chatbuf, sizeof(chatbuf), "\033[1;37m�� \033[36m%s \033[32m���ϴ��Ǿ���ʾ�� \033[37m��\033[m", users[unum].chatid);
	send_to_unum(unum, chatbuf);
}

/* Henry: ���ӹ��ܱ���䣬�����л�״̬�Ƿ����ֽ�� */

void chat_paper(int unum, char *msg)
{
	users[unum].flags ^= PERM_MESSAGE;
	snprintf(chatbuf, sizeof(chatbuf), "\033[1;31m�� \033[37m���ı�����״̬�л�Ϊ [%s] \033[31m��\033[m",
		MESG(unum) ? "����" : "�ر�" );
	send_to_unum(unum, chatbuf);
}

struct chatcmd chatcmdlist[] = {
	{ "act", 	chat_act, 			0 },
	{ "bye", 	chat_goodbye, 			0 },
	{ "flags", 	chat_setroom, 			0 },
	{ "invite", 	chat_invite, 			0 },
	{ "join", 	chat_join, 			0 },
	{ "kick", 	chat_kick, 			0 },
	{ "msg", 	chat_private, 			0 },
	{ "nick", 	chat_nick, 			0 },
	{ "operator", 	chat_makeop, 			0 },
	{ "talk", 	chat_maketalk,			0 },
	{ "rooms", 	chat_list_rooms, 		0 },
	{ "whoin", 	chat_list_by_room, 		1 },
	{ "wall", 	chat_broadcast, 		1 },
	{ "cloak", 	chat_cloak, 			1 },
	{ "who", 	chat_map_chatids_thisroom, 	0 },
	{ "list", 	chat_list_users, 		0 },
	{ "topic", 	chat_topic, 			0 },
	{ "hand", 	chat_hand,			0 },
	{ "rname", 	chat_rename, 			0 },
	{ "paper",	chat_paper,			0 },
	{ NULL, 	NULL, 				0 }
};

int
command_execute(int unum)
{
	char *msg = users[unum].ibuf;
	char *cmd;
	struct chatcmd *cmdrec;
	int match = 0;

	/* Validation routine */
	if (users[unum].room == -1) {
		/* MUST give special /! command if not in the room yet */
		if (msg[0] != '/' || msg[1] != '!')
			return -1;
		else
			return (login_user(unum, msg + 2));
	}
	/* If not a /-command, it goes to the room. */
	if (msg[0] != '/') {
		if (NOTALK(users[unum].room) && !SYSOP(unum) && !ROOMOP(unum) && !CANTALK(unum)) {
			strlcpy(chatbuf, "\033[1;31m ��\033[37m ������û����˷磬�޷�˵�� \033[31m��\033[m", sizeof(chatbuf));
			send_to_unum(unum, chatbuf);
			return 0;
		}
		chat_allmsg(unum, msg);
		return 0;
	}
	msg++;
	cmd = nextword(&msg);

	if (cmd[0] == '/') {
		if (!strcmp(cmd + 1, "help") || cmd[1] == '\0') {
			cmd = nextword(&msg);
			view_action_verb(unum, cmd);
			match = 1;
		} else if (NOEMOTE(users[unum].room)) {
			send_to_unum(unum, "�������ҽ�ֹ����");
			match = 1;
		} else if (party_action(unum, cmd + 1, msg, NA) == 0)
			match = 1;
		else if (speak_action(unum, cmd + 1, msg, NA) == 0)
			match = 1;
		else
			match = condition_action(unum, cmd + 1, NA);
	} else {
		for (cmdrec = chatcmdlist; !match && cmdrec->cmdstr; cmdrec++) {
			if (cmdrec->exact)
				match = !strcasecmp(cmd, cmdrec->cmdstr);
			else
				match =
				    !strncasecmp(cmd, cmdrec->cmdstr,
						 strlen(cmd));
			if (match)
				cmdrec->cmdfunc(unum, msg);
		}
	}

	if (!match) {
		snprintf(chatbuf, sizeof(chatbuf),
			"\033[1;32m �� \033[37m��Ǹ�������������˼��\033[36m/%s \033[31m��\033[m", cmd);
		send_to_unum(unum, chatbuf);
	}
	memset(users[unum].ibuf, 0, sizeof (users[unum].ibuf));
	return 0;
}

int
process_chat_command(int unum)
{
	int i;
	int rc, ibufsize;

	if ((rc = recv(users[unum].sockfd, chatbuf, sizeof (chatbuf), 0)) <= 0) {
		/* disconnected */
		exit_room(unum, EXIT_LOSTCONN, (char *) NULL);
		return -1;
	}
	ibufsize = users[unum].ibufsize;
	for (i = 0; i < rc; i++) {
		/* if newline is two characters, throw out the first */
		if (chatbuf[i] == '\r')
			continue;

		/* carriage return signals end of line */
		else if (chatbuf[i] == '\n') {
			users[unum].ibuf[ibufsize] = '\0';
			if (command_execute(unum) == -1)
				return -1;
			ibufsize = 0;
		}
		/* add other chars to input buffer unless size limit exceeded */
		else {
			if (ibufsize < 127)
				users[unum].ibuf[ibufsize++] = chatbuf[i];
		}
	}
	users[unum].ibufsize = ibufsize;

	return 0;
}

int
bind_local(char *localname, struct sockaddr *addr, socklen_t *addrlen)
{
	int sock;
	static struct sockaddr_un addrun;

	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;

	file_append("reclog/chatd.trace", localname);
	file_append("reclog/chatd.trace", "\n");

	strlcpy(addrun.sun_path, localname, sizeof(addrun.sun_path));
	addrun.sun_family = AF_UNIX;
	*addrlen = sizeof(addrun) - sizeof(addrun.sun_path) + strlen(addrun.sun_path);
	addr = (struct sockaddr *)&addrun;

	if (bind(sock, addr, *addrlen) == -1)
		return -1;

	return sock;
}

#ifdef DUALSTACK

int
bind_port(char *portname, struct sockaddr *addr, socklen_t *addrlen)
{
	int sock, err;
	const int on = 1;
	struct addrinfo hints;
	struct addrinfo *res, *ressave;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	if ((err = getaddrinfo(NULL, portname, &hints, &res)) != 0) {
		freeaddrinfo(res);
		return -1;
	}
	ressave = res;

	do {
		if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
			continue;

		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		if (bind(sock, res->ai_addr, res->ai_addrlen) == 0)
			break;
		close(sock);
	} while ((res = res->ai_next) != NULL);

	if (res == NULL) {
		freeaddrinfo(ressave);
		return -1;
	}

	*addrlen = res->ai_addrlen;
	freeaddrinfo(ressave);
	return sock;
}

void
getremotename(struct sockaddr *addr, int addrlen, char *rhost, int len)
{
	getnameinfo(addr, addrlen, rhost, len, NULL, 0, 0);
	if (strncmp(rhost, "::ffff:", 7) == 0)
		memmove(rhost, rhost + 7, len - 7);
}

#else

int
bind_port(char *portname, struct sockaddr *addr, socklen_t *addrlen)
{
	int port, sock;
	const int on = 1;
	static struct sockaddr_in addrin;

	/* we only bind to port larger than 1024 to avoid unnecessary security issues */
	if ((port = atoi(portname)) < 1024)
		return -1;

	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		return -1;

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset(&addrin, 0, sizeof(addrin));
	addrin.sin_family = AF_INET;
	addrin.sin_port = htons(port);
	*addrlen = sizeof(addrin);
	addr = (struct sockaddr *)&addrin;

	if (bind(sock, addr, sizeof(addrin)) == -1)
		return -1;

	return sock;
}

void
getremotename(struct sockaddr *addr, int addrlen, char *rhost, int len)
{
	struct sockaddr_in *from = (struct sockaddr_in *)addr;

	strlcpy(rhost, inet_ntoa(from->sin_addr), len);
}

#endif

void 
show_usage(const char * program)
{
	printf( "Usage:\n" );
	printf( "%s <chatroom number> -port <port num>\n", program );
	printf( "%s <chatroom number> -localfile <local sockfile name>\n", program );
	printf( "Remember to check include/chat.h first.\n" );
	printf( "\n" );
}

int
main(int argc, char **argv)
{
	int i, sr, clisock;
	fd_set readfds;
	struct timeval tv;
	struct sockaddr *addr = NULL, *cliaddr = NULL;
	socklen_t addrlen;

	if (argc < 4) {
		show_usage(argv[0]);
		return -1;
	}

	if (chdir(BBSHOME) == -1)
		return -1;

	close(0);
	close(1);
	close(2);

	chatroom = atoi(argv[1]) - 1;
	if (chatroom < 0 || chatroom >= PREDEFINED_ROOMS)
		return -1;

	if (!strcmp(argv[2], "-port")) {
		local = NA;
		if ((sock = bind_port(argv[3], addr, &addrlen)) == -1)
			return -1;
	} else if (!strcmp(argv[2], "-localfile")) {
		local = YEA;
		if ((sock = bind_local(argv[3], addr, &addrlen)) == -1)
			return -1;
	} else {
		return -1;
	}

	if ((cliaddr = malloc(addrlen)) == NULL)
		return -1;

	if (listen(sock, 5) == -1)
		return -1;

	/* init chatroom */
	maintopic = chatrooms[chatroom].topic;
	strlcpy(chatname, chatrooms[chatroom].name, sizeof(chatname));
	strlcpy(rooms[0].name, mainroom, sizeof(rooms[0].name));
	strlcpy(rooms[0].topic, maintopic, sizeof(rooms[0].topic));

	for (i = 0; i < MAXACTIVE; i++) {
		users[i].chatid[0] = '\0';
		users[i].sockfd = users[i].utent = -1;
	}

	if (fork()) return 0;
	setpgid(0, 0);

	/* ------------------------------ */
	/* trap signals                   */
	/* ------------------------------ */

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGURG, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	FD_ZERO(&allfds);
	FD_SET(sock, &allfds);
	nfds = sock + 1;

	while (1) {
		memcpy(&readfds, &allfds, sizeof (readfds));

		tv.tv_sec = 60 * 30;
		tv.tv_usec = 0;
		if ((sr = select(nfds, &readfds, NULL, NULL, &tv)) < 0) {
			if (errno == EINTR)
				sleep(50);
			continue;
		} else if (!sr)
			continue;

#if 0
		if (sr == 0) {
			exit(0);	/* normal chat server shutdown */
		}
#endif

		if (FD_ISSET(sock, &readfds)) {
			if ((clisock = accept(sock, cliaddr, &addrlen)) == -1)
				continue;

			for (i = 0; i < MAXACTIVE; i++) {
				if (users[i].sockfd == -1) {
					if (local == NA) {
						getremotename(cliaddr, addrlen, users[i].host, sizeof(users[i].host));
						if (!strcmp(users[i].host, "127.0.0.1") || !strcmp(users[i].host, "::"))
							strlcpy(users[i].host, "localhost", sizeof(users[i].host));
					} else {
						strlcpy(users[i].host, "localhost", sizeof(users[i].host));
					}
					users[i].sockfd = clisock;
					users[i].room = -1;
					break;
				}
			}

			if (i >= MAXACTIVE) {
				/* full -- no more chat users */
				close(clisock);
			} else {

#if !RELIABLE_SELECT_FOR_WRITE
				int flags = fcntl(clisock, F_GETFL, 0);

				flags |= O_NDELAY;
				fcntl(clisock, F_SETFL, flags);
#endif

				FD_SET(clisock, &allfds);
				if (clisock >= nfds)
					nfds = clisock + 1;
				num_conns++;
			}
		}

		for (i = 0; i < MAXACTIVE; i++) {
			/* we are done with clisock, so re-use the variable */
			clisock = users[i].sockfd;
			if (clisock != -1 && FD_ISSET(clisock, &readfds)) {
				if (process_chat_command(i) == -1) {
					logout_user(i);
				}
			}
		}
#if 0
		if (num_conns <= 0) {
			/* one more pass at select, then we go bye-bye */
			tv = zerotv;
		}
#endif
	}
	/* NOTREACHED */
}
