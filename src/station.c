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
					/* PERM_ACHATROOM -- 聊天室管理员 */
#define CLOAK(u)        (users[(u)].flags & PERM_CHATCLOAK)
#define ROOMOP(u)       (users[(u)].flags & PERM_CHATROOM)
#define OUTSIDER(u)     (users[(u)].flags & PERM_DENYPOST)
#define PERM_TALK	PERM_NOZAP	/* 借用一下用来记录发言权 */
#define CANTALK(u)	(users[(u)].flags & PERM_TALK)
#define MESG(u)		(users[(u)].flags & PERM_MESSAGE) /* 借用来记录麦克风状态 */

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
char *maintopic = "今天，我们要讨论的是.....";

char *msg_not_op = "\033[1;37m★\033[32m您不是这厢房的老大\033[37m ★\033[m";
char *msg_no_such_id =
    "\033[1;37m★\033[32m [\033[36m%s\033[32m] 不在这间厢房里\033[37m ★\033[m";
char *msg_not_here =
    "\033[1;37m★ \033[32m[\033[36m%s\033[32m] 并没有前来本会议厅\033[37m ★\033[m";

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
					"\033[1;37m★ \033[32m[\033[36m%s\033[32m] 慢慢离开了 \033[37m★\033[m",
					users[unum].chatid);
				if (msg && *msg) {
					strcat(chatbuf, ": ");
					strncat(chatbuf, msg, 80);
				}
				break;

			case EXIT_LOSTCONN:
				snprintf(chatbuf, sizeof(chatbuf),
					"\033[1;37m★ \033[32m[\033[36m%s\033[32m] 像断了线的风筝 ... \033[37m★\033[m",
					users[unum].chatid);
				break;

			case EXIT_KICK:
				snprintf(chatbuf, sizeof(chatbuf),
					"\033[1;37m★ \033[32m[\033[36m%s\033[32m] 被老大赶出去了 \033[37m★\033[m",
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
			     "\033[1;31m◎ \033[37m请指定新的聊天室名称 \033[31m◎\033[m");
		return;
	}
	strncpy(rooms[rnum].name, msg, IDLE);
	snprintf(chatbuf, sizeof(chatbuf), "/r%.11s", msg);
	send_to_room(rnum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;37m★ \033[32m[\033[36m%s\033[32m] 将聊天室名称改为 \033[1;33m%.11s \033[37m★\033[m",
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
		send_to_unum(unum, "\033[1;31m◎ \033[37m请指定话题 \033[31m◎\033[m");
		return;
	}
	strncpy(rooms[rnum].topic, msg, 43);
	rooms[rnum].topic[42] = '\0';
	snprintf(chatbuf, sizeof(chatbuf), "/t%.42s", msg);
	send_to_room(rnum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;37m★ \033[32m[\033[36m%s\033[32m] 将话题改为 \033[1;33m%42.42s \033[37m★\033[m",
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
				     "\033[1;31m◎ \033[37m抱歉，您不能开新包厢 \033[31m◎\033[m");
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
				     "\033[1;31m◎ \033[37m我们的房间满了喔 \033[31m◎\033[m");
			return 0;
		}
	}
	if (!SYSOP(unum))
		if (LOCKED(rnum) && rooms[rnum].invites[unum] == 0) {
			send_to_unum(unum,
				     "\033[1;31m◎ \033[37m本房商讨机密中，非请勿入 \033[31m◎\033[m");
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
			"\033[1;31m□ \033[37m[\033[36;1m%s\033[37m] 进入 \033[35m%s \033[37m包厢 \033[31m□\033[m",
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
		"\033[1;31m□\033[37m 欢迎光临『\033[32m%s\033[37m』的【\033[36m%s\033[37m】\033[31m□\033[m", BBSNAME, chatname);
	send_to_unum(unum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;31m□\033[37m 目前已经有 \033[1;33m%d \033[37m间会议室有客人 \033[31m□\033[m",
		roomc + 1);
	send_to_unum(unum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf), "\033[1;31m□ \033[37m本会议厅内共有 \033[36m%d\033[37m 人 ", userc + 1);
	if (suserc) {
		char buf[40];

		snprintf(buf, sizeof(buf), "[\033[36m%d\033[37m 人在高机密讨论室]", suserc);
		strlcat(chatbuf, buf, sizeof(chatbuf));
	}
	strlcat(chatbuf, "\033[31m□\033[m", sizeof(chatbuf));
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
		send_to_unum(unum, "\033[1;31m◎ \033[37m抱歉！老大不让你看有哪些房间有客人 \033[31m◎\033[m");
		return;
	}
	send_to_unum(unum, "\033[1;33;44m 谈天室名称  │人数│话题            \033[m");
	for (i = 0; i < MAXROOM; i++) {
		occupants = rooms[i].occupants;
		if (occupants > 0) {
			if (!SYSOP(unum))
				if ((rooms[i].flags & ROOM_SECRET) &&
				    (users[unum].room != i))
					continue;
			snprintf(chatbuf, sizeof(chatbuf),
				" \033[1;32m%-12s\033[37m│\033[36m%4d\033[37m│\033[33m%s\033[m",
				rooms[i].name, occupants, rooms[i].topic);
			if (rooms[i].flags & ROOM_LOCKED)
				strlcat(chatbuf, " [锁住]", sizeof(chatbuf));
			if (rooms[i].flags & ROOM_SECRET)
				strlcat(chatbuf, " [机密]", sizeof(chatbuf));
			if (rooms[i].flags & ROOM_NOEMOTE)
				strlcat(chatbuf, " [禁止动作]", sizeof(chatbuf));
			if (rooms[i].flags & ROOM_SCHOOL)
				strlcat(chatbuf, " [一人提问]", sizeof(chatbuf));
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
		     "\033[1;33;44m 聊天代号│使用者代号  │聊    天    室□Op□来自                          \033[m");
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
				"\033[1;5m%c\033[0;1;37m%-8s│\033[31m%s%-12s\033[37m│\033[32m%-14s\033[37m□\033[34m%-2s\033[37m□\033[35m%-30s\033[m",
				(users[i].cloak == 1) ? 'C' : ' ',
				users[i].chatid,
				OUTSIDER(i) ? "\033[1;35m" : "\033[1;36m",
				users[i].userid, rooms[rnum].name,
				ROOMOP(i) ? "是" : "否", users[i].host);
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
				"\033[1;31m◎ \033[37m没 %s 这个房间喔 \033[31m◎\033[m",
				roomstr);
			send_to_unum(unum, chatbuf);
			return;
		}
		if ((rooms[whichroom].flags & ROOM_SECRET) && !SYSOP(unum)) {
			send_to_unum(unum,
				     "\033[1;31m◎ \033[37m本会议厅的房间皆公开的，没有秘密 \033[31m◎\033[m");
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
		     "\033[1;33;44m 聊天代号 使用者代号  │ 聊天代号 使用者代号  │ 聊天代号 使用者代号 \033[m");
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
				users[i].userid, (c < 2 ? "│" : "  "));
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
			     "\033[1;31m⊙ \033[37m请告诉柜台您要的房间是: {[\033[32m+\033[37m(设定)][\033[32m-\033[37m(取消)]}{[\033[32ml\033[37m(锁住)][\033[32ms\033[37m(秘密)]}\033[m");
		return;
	}
	while (*modestr) {
		flag = 0;
		switch (*modestr) {
		case 'l':
		case 'L':
			flag = ROOM_LOCKED;
			fstr = "锁住";
			break;
		case 's':
		case 'S':
			flag = ROOM_SECRET;
			fstr = "机密";
			break;
		case 'e':
		case 'E':
			flag = ROOM_NOEMOTE;
			fstr = "禁止动作";
			break;
		case 'a':
		case 'A':
			flag = ROOM_SCHOOL;
			fstr = "一人提问";
			break;
		default:
			snprintf(chatbuf, sizeof(chatbuf),
				"\033[1;31m◎ \033[37m抱歉，看不懂你的意思：[\033[36m%c\033[37m] \033[31m◎\033[m", *modestr);
			send_to_unum(unum, chatbuf);
			return;
		}
		if (flag && ((rooms[rnum].flags & flag) != sign * flag)) {
			rooms[rnum].flags ^= flag;
			snprintf(chatbuf, sizeof(chatbuf),
				"\033[1;37m★\033[32m 这房间被 %s %s%s的形式 \033[37m★\033[m",
				users[unum].chatid, sign ? "设定为" : "取消", fstr);
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
			     "\033[1;31m◎ \033[37m您的名字是不是写错了\033[31m ◎\033[m");
		return;
	}
	othernum = chatid_to_indx(0, chatid);
	if (othernum != -1 && othernum != unum) {
		send_to_unum(unum,
			     "\033[1;31m◎ \033[37m抱歉！有人跟你同名，所以你不能进来 \033[31m◎\033[m");
		return;
	}
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;31m◎ \033[36m%s \033[0;37m已经改名为 \033[1;33m%s \033[31m◎\033[m",
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
			strlcpy(chatbuf, "\033[1;31m ◎\033[37m 那位参与者叫什么名字 \033[31m◎\033[m", sizeof(chatbuf));
		}
		send_to_unum(unum, chatbuf);
		return;
	}
	if (!MESG(recunum)) {
		strlcpy(chatbuf, "\033[1;31m ◎\033[37m 对方关闭了便笺箱 \033[31m◎\033[m", sizeof(chatbuf));
		send_to_unum(unum, chatbuf);
		return;
	}
	if (*msg) {
		snprintf(chatbuf, sizeof(chatbuf),
			"\033[1;32m ※ \033[36m%s \033[37m传纸条小秘书来到\033[m: ",
			users[unum].chatid);
		strlcat(chatbuf, msg, sizeof(chatbuf));
		send_to_unum(recunum, chatbuf);
		snprintf(chatbuf, sizeof(chatbuf), "\033[1;32m ※ \033[37m纸条已经交给了 \033[36m%s\033[m: ", users[recunum].chatid);
		strlcat(chatbuf, msg, sizeof(chatbuf));
		send_to_unum(unum, chatbuf);
	} else {
		strlcpy(chatbuf, "\033[1;31m ◎\033[37m 你要跟对方说些什么呀？\033[31m◎\033[m", sizeof(chatbuf));
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
		send_to_unum(unum, "本聊天室禁止动作");
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
		snprintf(chatbuf, sizeof(chatbuf), "\033[1;36m%s \033[37m%s 隐身状态...\033[m",
			users[unum].chatid,
			(users[unum].cloak == 1) ? "进入" : "停止");
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
			     "\033[1;31m◎ \033[37m你只能在这里聊天 \033[31m◎\033[m");
		return;
	}
	if (*roomid == '\0') {
		send_to_unum(unum, "\033[1;31m◎ \033[37m请问哪个房间 \033[31m◎\033[m");
		return;
	}
	if (!is_valid_chatid(roomid)) {
		send_to_unum(unum,
			     "\033[1;31m◎\033[37m房间名中不能有【*:/%】等不合法字符\033[31m◎\033[37m");
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
		strlcpy(chatbuf, "\033[1;37m★ \033[32m你忘了你本来就是老大喔 \033[37m★\033[m", sizeof(chatbuf));
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
		"\033[1;37m★ \033[36m %s\033[32m决定让 \033[35m%s \033[32m当老大 \033[37m★\033[m",
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
		snprintf(chatbuf, sizeof(chatbuf), "\033[1;37m★ \033[36m%s \033[32m等一下就来 \033[37m★\033[m", users[recunum].chatid);
		send_to_unum(unum, chatbuf);
		return;
	}
	rooms[rnum].invites[recunum] = 1;
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;37m★ \033[36m%s \033[32m邀请您到 [\033[33m%s\033[32m] 房间聊天\033[37m ★\033[m",
		users[unum].chatid, rooms[rnum].name);
	send_to_unum(recunum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf), "\033[1;37m★ \033[36m%s \033[32m等一下就来 \033[37m★\033[m", users[recunum].chatid);
	send_to_unum(unum, chatbuf);
}

void
chat_broadcast(int unum, char *msg)
{
	if (!SYSOP(unum)) {
		send_to_unum(unum,
			     "\033[1;31m◎ \033[37m你不可以在会议厅内大声喧哗 \033[31m◎\033[m");
		return;
	}
	if (*msg == '\0') {
		send_to_unum(unum, "\033[1;37m★ \033[32m广播内容是什么 \033[37m★\033[m");
		return;
	}
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1m Ding Dong!! 传达室报告： \033[36m%s\033[37m 有话对大家宣布：\033[m",
		users[unum].chatid);
	send_to_room(ROOM_ALL, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf), "\033[1;34m【\033[33m%s\033[34m】\033[m", msg);
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
	char *verb;		/* 动词 */
	char *part1_msg;	/* 介词 */
	char *part2_msg;	/* 动作 */
};

struct action party_data[] = {
	{"admire", "对", "的景仰之情犹如滔滔江水连绵不绝"},
	{"agree", "完全同意", "的看法"},
	{"bearhug", "热情的拥抱", ""},
	{"ben", "笑呵呵地对", "说：“笨, 连这都不知道... :P”"},
	{"bless", "祝福", "心想事成"},
	{"bow", "毕躬毕敬的向", "鞠躬"},
	{"bye", "向", "一拱手道：“青山依旧，喷水常来，咱们下回再会!”"},
	{"bye1", "哭哭啼啼地拉着", "的衣角:“舍不得你走啊...”"},
	{"bye2", "凄婉地对",
	 "说道:“世上没有不散的宴席，我先走一步了，大家多保重.”"},
	{"byebye", "望着", "离去的背影渐渐消失，两滴晶莹的泪花从腮边滑落"},
	{"bite", "狠狠的咬了", "一口，把他痛得哇哇大叫...真爽,哈哈！"},
	{"blink", "假装什么也不知道，对", "天真地眨了眨眼睛"},
	{"breath", ":赶快给", "做人工呼吸!"},
	{"brother", "对",
	 "一抱拳,道:“难得你我肝胆相照,不若你我义结金兰,意下如何?”"},
	{"bigfoot", "伸出大脚，对准", "的三寸金莲狠狠地踩下去"},
	{"consider", "开始认真考虑杀死", "的可能性和技术难度"},
	{"caress", "抚摸", ""},
	{"cringe", "向", "卑躬屈膝，摇尾乞怜"},
	{"cry", "向", "嚎啕大哭"},
	{"cry1", "越想越伤心，不禁趴在", "的肩膀上嚎啕大哭起来"},
	{"cry2", "扑在",
	 "身上，哭得死去活来:“别走呀!刚泡的方便面你还没吃完呢!”"},
	{"cold", "听了", "的话，冷得直起鸡皮疙瘩"},
	{"comfort", "温言安慰", ""},
	{"clap", "向", "热烈鼓掌"},
	{"dance", "拉了", "的手翩翩起舞"},
	{"die", "很酷地掏出一把防锈水枪，「碰!」一枪把", "给毙了."},
	{"dog", "关门、放狗！ 把", "咬的落花流水"},
	{"dogleg", "是正宗狗腿王～汪汪～, 使劲拍拍", "的马屁"},
	{"dontno", "对", "摇摇头说道: “这个...我不知道...”"},
	{"dontdo", "对", "说:“小朋友， 不可以这样哦，这样做是不好的哟”"},
	{"drivel", "对著", "流口水"},
	{"dunno", "瞪大眼睛，天真地问：", "，你说什么我不懂耶... :("},
	{"dlook", "呆呆地望着", "，口水哗啦啦地流了一地"},
	{"dove", "给了", "一块DOVE，说:“呐，给你一块DOVE，要好好听话哦”"},
	{"emlook", "上下端详了",
	 "两眼“就你小子呀,我当是谁呢,也敢在这里撒野!”"},
	{"face", "顽皮的对", "做了个鬼脸."},
	{"faceless", "对着", "大叫道：“嘿嘿, 你的面子卖多少钱一斤?”"},
	{"farewell", "含泪,依依不舍地向", "道别"},
	{"fear", "对", "露出怕怕的表情"},
	{"flook", "痴痴的望着", ", 那深情的眼神说明了一切."},
	{"forgive", "大度的对", "说：算了，原谅你了"},
	{"giggle", "对著", "傻傻的呆笑"},
	{"grin", "对", "露出邪恶的笑容"},
	{"growl", "对", "咆哮不已"},
	{"hammer", "举起惠香的50000000T铁锤往$", "上用力一敲!,*** 　� 锵 〗!"},
	{"hand", "跟", "握手"},
	{"heng", "看都不看",
	 "一眼， 哼了一声，高高的把头扬起来了,不屑一顾的样子..."},
	{"hi", "对", "很有礼貌地说了一声：“Hi! 你好!”"},
	{"hug", "轻轻地拥抱", ""},
	{"kick", "把", "踢的死去活来"},
	{"kiss", "轻吻", "的脸颊"},
	{"laugh", "大声嘲笑", ""},
	{"look", "贼贼地看着", "，不知道在打什么馊主意。"},
	{"love", "深情地望着", "，发现自己爱上了ta"},
	{"love2", "对", "情深款款的说：在天愿为孖公仔，在地愿为<油炸鬼>"},
	{"lovelook", "一双水汪汪的大眼睛含情脉脉地看着", "!"},
	{"missyou", "甜甜一笑，眼中却流下眼泪:", "，真的是你吗，这不是作梦？"},
	{"meet", "对", "一抱拳，说道：“久闻大名，今日一见，真是三生有幸！”"},
	{"moon", "拉着",
	 "的小手，指着初升的月亮说：“天上明月，是我们的证人”"},
	{"nod", "向", "点头称是"},
	{"nothing", "赶忙扶起", "：小事一桩，何足挂齿？"},
	{"nudge", "用手肘顶", "的肥肚子"},
	{"oh", "对", "说：“哦，酱子啊！”"},
	{"pad", "轻拍", "的肩膀"},
	{"papa", "紧张地对", "说：“我好怕怕哦...”"},
	{"papaya", "敲了敲", "的木瓜脑袋"},
	{"praise", "对", "说道: 果然高明! 佩服佩服!"},
	{"pinch", "用力的把", "拧的黑青"},
	{"poor", "拉着", "的手说：“可怜的孩子！”眼泪唰唰地掉了下来....."},
	{"punch", "狠狠揍了", "一顿"},
	{"puke", "对着", "吐啊吐啊，据说吐多几次就习惯了"},
	{"pure", "对", "露出纯真的笑容"},
	{"qmarry", "向", "勇敢地跪了下来：你愿意嫁给我吗---真是勇气可嘉啊!"},
	{"report", "偷偷地对", "说：“报告我好吗？”"},
	{"rose", "突然从身后拿出一朵-`-,-<@ 深情地献给", "！"},
	{"rose999", "对", "唱道：“我已经为你种下，九百九十九朵玫瑰……”"},
	{"run", "气喘吁吁地对",
	 "说:我换了八匹快马日夜兼程赶来.能再见到你死了也心甘"},
	{"shrug", "无奈地向", "耸了耸肩膀"},
	{"sigh", "对", "叹了一口气,道: 曾经沧海难为水,除却巫山不是云..."},
	{"slap", "啪啪的巴了", "一顿耳光"},
	{"smooch", "拥吻著", ""},
	{"snicker", "嘿嘿嘿..的对", "窃笑"},
	{"sniff", "对", "嗤之以鼻"},
	{"spank", "用巴掌打", "的臀部"},
	{"squeeze", "紧紧地拥抱著", ""},
	{"sorry", "感到对", "十二万分的歉意, 于是低声说道:我感到非常的抱歉!"},
	{"thank", "向", "道谢"},
	{"thanks", "跪在地上对", "恭敬的磕了几个头:你的大恩大德，没齿难亡!"},
	{"tickle", "咕叽!咕叽!搔", "的痒"},
	{"wake", "努力的摇摇", "，在其耳边大叫：“快醒醒，会着凉的！”"},
	{"wakeup", "摇著",
	 "，试著把他叫醒。大声在他耳边大叫：「 猪! 起来了! 」"},
	{"wave", "对著", "拼命的摇手"},
	{"welcome", "热烈欢迎", "的到来"},
	{"wing", "拿着一盒维它奶傻傻的说：你。。就是", "吗？"},
	{"wink", "对", "神秘的眨眨眼睛"},
	{"zap", "对", "疯狂的攻击"},
	{"xinku", "感动得热泪盈眶，向着", "振臂高呼：“首长辛苦了！”"},
	{"bupa", "爱怜地摸着",
	 "的头,说:“小妹妹,不怕,受了什么委屈大哥哥替你报仇!”"}, {"gril",
								  "赶紧给",
								  "捶捶背,心想:孩子小,别岔过气去."},
	{":)..", "对", "垂涎三尺，不知道下一步会有何举动"},
	{"?", "很疑惑的看着", ""},
	{"@@", "睁大了眼睛惊奇地盯着", ":“这...这也太....”"},
	{"@@!", "狠狠地瞪了", "一眼, 他立刻被看得缩小了一半"},
	{"mail", "给", "从美国发了一封有白色粉末的邮件"},
	{NULL, NULL, NULL}
};

int
party_action(int unum, char *cmd, char *party, int self)
{
	int i;

	for (i = 0; party_data[i].verb; i++) {
		if (!strcmp(cmd, party_data[i].verb)) {
			if (*party == '\0') {
				party = "大家";
			} else {
				int recunum = fuzzy_chatid_to_indx(unum, party);

				if (recunum < 0) {
					/* no such user, or ambiguous */
					if (recunum == -1) {
						snprintf(chatbuf, sizeof(chatbuf), msg_no_such_id, party);
					} else {
						strlcpy(chatbuf, "\033[1;31m◎ \033[37m请问哪间房间 \033[31m◎\033[m", sizeof(chatbuf));
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
				send_to_unum(unum, "\033[1;37m动作演示：\033[m");
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
	{"ask", "询问", NULL},
	{"chant", "歌颂", NULL},
	{"cheer", "喝采", NULL},
	{"chuckle", "轻笑", NULL},
	{"curse", "咒骂", NULL},
	{"demand", "要求", NULL},
	{"frown", "蹙眉", NULL},
	{"groan", "呻吟", NULL},
	{"grumble", "发牢骚", NULL},
	{"hum", "喃喃自语", NULL},
	{"moan", "悲叹", NULL},
	{"notice", "注意", NULL},
	{"order", "命令", NULL},
	{"ponder", "沈思", NULL},
	{"pout", "噘著嘴说", NULL},
	{"pray", "祈祷", NULL},
	{"request", "恳求", NULL},
	{"shout", "大叫", NULL},
	{"sing", "唱歌", NULL},
	{"smile", "微笑", NULL},
	{"smirk", "假笑", NULL},
	{"swear", "发誓", NULL},
	{"tease", "嘲笑", NULL},
	{"whimper", "呜咽的说", NULL},
	{"yawn", "哈欠连天", NULL},
	{"yell", "大喊", NULL},
	{NULL, NULL, NULL}
};

int
speak_action(int unum, char *cmd, char *msg, int self)
{
	int i;

	for (i = 0; speak_data[i].verb; i++) {
		if (!strcmp(cmd, speak_data[i].verb)) {
			snprintf(chatbuf, sizeof(chatbuf), "\033[1;36m%s \033[32m%s：\033[33m %s\033[37;0m",
				users[unum].chatid, speak_data[i].part1_msg, msg);
			if (YEA == self) {
				send_to_unum(unum, "\033[1;37m动作演示：\033[m");
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
	{"acid", "说道：“好肉嘛唷~~~”", NULL},
	{"addoil", "望空高喊: 加油!!", NULL},
	{"applaud", "啪啪啪啪啪啪啪....", NULL},
	{"blush", "脸都红了", NULL},
	{"boss", "放声大叫：哇塞，不得了，老板来了，我要逃了，再见了各位.",
	 NULL},
	{"bug", "大声说“报告站长，我抓到一只虫子”。", NULL},
	{"cool", "大叫起来：哇塞～～～好cool哦～～", NULL},
	{"cough", "咳了几声", NULL},
	{"die", "口吐白沫，双眼一翻，身体搐了几下，不动了，你一看死了。", NULL},
	{"goeat", "的肚子又咕咕的叫了,唉,不得不去吃食堂那#$%&的饭菜了", NULL},
	{"faint", "咣当一声，晕倒在地...", NULL},
	{"faint1", "口吐白沫，昏倒在地上。", NULL},
	{"faint2", "口中狂喷鲜血，翻身倒在地上。", NULL},
	{"haha", "哈哈哈哈......", NULL},
	{"handup", "拼命地伸长自己的手臂，高声叫道：“我，我，我！”", NULL},
	{"happy", "r-o-O-m....听了真爽！", NULL},
	{"heihei", "冷笑一声", NULL},
	{"jump", "高兴地像小孩子似的，在聊天室里蹦蹦跳跳。", NULL},
	{"pavid", "脸色苍白!好像惧怕什么!", NULL},
	{"puke", "真恶心，我听了都想吐", NULL},
	{"shake", "摇了摇头", NULL},
	{"sleep", "Zzzzzzzzzz，真无聊，都快睡著了", NULL},
	{"so", "就酱子!!", NULL},
	{"strut", "大摇大摆地走", NULL},
	{"suicide", "真想买块豆腐来一头撞死, 摸了摸身边又没零钱, 只好忍一手",
	 NULL},
	{"toe", "觉得真是无聊, 于是专心地玩起自己的脚趾头来", NULL},
	{"tongue", "吐了吐舌头", NULL},
	{"think", "歪著头想了一下", NULL},
	{"wa", "哇！:-O", NULL},
	{"wawl", "惊天动地的哭", NULL},
	{"xixi", "偷笑：嘻嘻～～", NULL},
	{"ya", "得意的作出胜利的手势! 「 V 」:“ 哈哈哈...”", NULL},
	{":(", "愁眉苦脸的,一副倒霉样", NULL},
	{":)", "的脸上露出愉快的表情.", NULL},
	{":D", "乐的合不拢嘴", NULL},
	{":P", "吐了吐舌头,差点咬到自己", NULL},
	{"@@", "要死菜了 %^#@%$#&^*&(&^$#%$#@(*&()*)_*&(#@%$^%&^.", NULL},
	{"sing1", "凉风有信，秋月无边。。。。。。", NULL},
	{"mail", "给", "从美国发了一封有白色粉末的邮件。"},
	{"wonder", "怀疑的说：是不是真的？", NULL},
	{"apache", "坐着阿帕奇直升机离开。真是酷呆了！", NULL},
	{"parapara", "的手在空中乱舞，那样子似乎在跳parapara。", NULL},
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
				send_to_unum(unum, "\033[1;37m动作演示：\033[m");
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
	"\033[1m【 Verb + Nick：   动词 + 对方名字 】\033[m",
	"\033[1m【 Verb + Message：动词 + 要说的话 】\033[m",
	"\033[1m【 Verb：动词 】\033[m", NULL
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
	if (speak_action(unum, id, "你爱我来我爱你", YEA) == 0)
		return;
	if (condition_action(unum, id, YEA) == 1)
		return;

	send_to_unum(unum, "\033[1;37m用 //help [编号] 获得动作列表，用 //help [动作] 观看动作示范\033[m");
	send_to_unum(unum, "  ");
	for (i = 0; dscrb[i]; i++) {
		snprintf(chatbuf, sizeof(chatbuf), "%d. %s", i + 1, dscrb[i]);
		send_to_unum(unum, chatbuf);
	}
	send_to_unum(unum, " ");
}

/* Henry: 增加一人提问的实现，提问前需要获得麦克风^_^ */

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
		strlcpy(chatbuf, "\033[1;37m★ \033[32m你是老大啊，有话直接说就行了 \033[37m★\033[m", sizeof(chatbuf));
		send_to_unum(unum, chatbuf);
		return;
	}

	users[recunum].flags ^= PERM_TALK;
	if (users[recunum].flags & PERM_TALK) {
		snprintf(chatbuf, sizeof(chatbuf),
			"\033[1;37m★ \033[36m%s \033[32m决定把麦克风交给 \033[35m%s \033[32m \033[37m★\033[m",
			users[unum].chatid, users[recunum].chatid);
	} else {
		snprintf(chatbuf, sizeof(chatbuf),
			"\033[1;37m★ \033[36m%s \033[32m决定没收 \033[35m%s \033[32m的麦克风  \033[37m★\033[m",
			users[unum].chatid, users[recunum].chatid);
	}
	send_to_room(users[recunum].room, chatbuf);
}

void
chat_hand(int unum, char *msg)
{
	int i;

	if (!NOTALK(users[unum].room)) {
		send_to_unum(unum, "\033[1;31m ◎\033[37m 请你畅所欲言，无需举手 \033[31m◎\033[m");
		return;
	}

	for (i = 0; i < MAXACTIVE; i++) {
		if (SYSOP(i) || ROOMOP(i)) {
			snprintf(chatbuf, sizeof(chatbuf), "\033[1;37m★ \033[36m%s \033[32m举起了手，好像想说什么的样子 \033[37m★\033[m", users[unum].chatid);
			send_to_unum(i, chatbuf);
		}
	}

	snprintf(chatbuf, sizeof(chatbuf), "\033[1;37m★ \033[36m%s \033[32m向老大们举手示意 \033[37m★\033[m", users[unum].chatid);
	send_to_unum(unum, chatbuf);
}

/* Henry: 增加功能便笺箱，可以切换状态是否接收纸条 */

void chat_paper(int unum, char *msg)
{
	users[unum].flags ^= PERM_MESSAGE;
	snprintf(chatbuf, sizeof(chatbuf), "\033[1;31m◎ \033[37m您的便笺箱的状态切换为 [%s] \033[31m◎\033[m",
		MESG(unum) ? "开启" : "关闭" );
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
			strlcpy(chatbuf, "\033[1;31m ◎\033[37m 你现在没有麦克风，无法说话 \033[31m◎\033[m", sizeof(chatbuf));
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
			send_to_unum(unum, "本聊天室禁止动作");
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
			"\033[1;32m ◎ \033[37m抱歉，看不懂你的意思：\033[36m/%s \033[31m◎\033[m", cmd);
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
