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

#include "modes.h"

char *
modetype(int mode)
{
	switch (mode & ~WWW) {
	case IDLE:
		return "";
	case NEW:
		return "��վ��ע��";
	case LOGIN:
		return "���뱾վ";
	case DIGEST:
	case DIGESTRACE:
		return "���������";
	case MMENU:
		return "ת�������";
	case ADMIN:
		return "������ѡ��";
	case SELECT:
		return "ѡ��������";
	case READBRD:
		return "һ����ɽС";
	case READNEW:
		return "����������";
	case READING:
		return "Ʒζ����";
	case POSTING:
		return "�ĺ��ӱ�";
	case SMAIL:
		return "�����Ÿ�";
	case RMAIL:
		return "�����ż�";
	case TMENU:
		return "����ѡ��";
	case LUSERS:
		return "��������:)";
	case FRIEND:
		return "Ѱ�Һ���";
	case MONITOR:
		return "̽������";
	case QUERY:
		return "��ѯ����";
	case TALK:
		return "����";
	case PAGE:
		return "����";
	case CHAT1:
		return "���ʻ�����";
	case CHAT2:
		return "���Ⱥ���";
	case LAUSERS:
		return "̽������";
	case XMENU:
		return "ϵͳ��Ѷ";
	case VOTING:
		return "ͶƱ��...";
	case BBSNET:
		return "BBSNET";
	case EDITUFILE:
		return "�༭���˵�";
	case EDITSFILE:
		return "����ϵͳ��";
	case ZAP:
		return "����������";
	case SYSINFO:
		return "���ϵͳ";
	case DICT:
		return "�����ֵ�";
	case LOCKSCREEN:
		return "��Ļ����";
	case NOTEPAD:
		return "���԰�";
	case GMENU:
		return "������";
	case MSG:
		return "��ѶϢ";
	case USERDEF:
		return "�Զ�����";
	case EDIT:
		return "�޸�����";
	case OFFLINE:
		return "��ɱ��..";
	case EDITANN:
		return "���޾���";
	case LOOKMSGS:
		return "�쿴ѶϢ";
	case WFRIEND:
		return "Ѱ������";
	case WNOTEPAD:
		return "���߻���";
	case WINMINE:
		return "����ɨ��";
	case FIVE:
		return "��ս������";
	case WORKER:
		return "������";
	case PAGE_FIVE:
		return "��������";
	case MOBILE:
		return "�ֻ�ģʽ";
	default:
		return "ȥ������!?";
	}
}
