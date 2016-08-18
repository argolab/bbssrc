/*
  written by jieer(����֮��~С�к�)   bbs.dqpi.edu.cn, modified by monster

  ������� bbssrc/util/local_utl/��
  make perms
  ����ʹ�ã����谴ʱͳ�ƣ������cron.bbs��

*/

#include "bbs.h"

#define PERM_NUM	17

const char perms[PERM_NUM][30] = {
	"�ٲ�ίԱ���Ա", "����վ������"
	"�˺Ź���Ա", "�������ܹ�", "�������ܹ�", "������ܹ�",
	"ͶƱ����Ա", "��վ������", "�����ҹ���Ա",
	"����IP", "��͸����", "����", "�˺����ñ���", "ǿ�Ʒ���Ϣ",
	"�ӳ�����ʱ��", "����"
};

const int levels[PERM_NUM] = {
	PERM_JUDGE, PERM_INTERNAL,
	PERM_ACCOUNTS, PERM_OBOARDS, PERM_ANNOUNCE, PERM_ACBOARD,
	PERM_OVOTE, PERM_CHATCLOAK, PERM_ACHATROOM,
	PERM_SEEIP, PERM_SEECLOAK, PERM_CLOAK, PERM_XEMPT, PERM_FORCEPAGE,
	PERM_EXT_IDLE, PERM_BOARDS
};

int
main()
{
	FILE *fp;
	char sysop[1000][14];
	char record[PERM_NUM][18000][14];
	struct userec users;
	char buf[100];
	int x, y, a = 0, z, s[16], i;

	for (x = 0; x < 17; x++)
		s[x] = 0;
	sprintf(buf, "%s/.PASSWDS", BBSHOME);
	if ((fp = fopen(buf, "rb")) == NULL) {
		printf("Can't open password file.\n");
		return 1;
	}
	while (fread(&users, sizeof (struct userec), 1, fp) > 0) {
		if (users.userlevel & PERM_SYSOP) {
			strcpy(sysop[a++], users.userid);
		} else {
			for (i = 0; i < PERM_NUM; i++) {
				if (users.userlevel & levels[i])
					strcpy(record[i][s[i]++], users.userid);
			}
		}
	}
	fclose(fp);

	sprintf(buf, "%s/0Announce/sysops/perms", BBSHOME);
	if ((fp = fopen(buf, "w")) == NULL) {
		fclose(fp);
		printf("Can't write to perms file.\n");
		return 1;
	}

	fprintf(fp, "\n\t\t\t\t\t\t\t%s ����Ȩ��ͳ��\n\n", BBSNAME);
	fprintf(fp, "\n ����վ��Ȩ�޵�ID:\n");
	for (x = 0; x < a; x++) {
		if (x % 4 == 0)
			fprintf(fp, "\n");
		fprintf(fp, " [31m��[0;1;32m  %s  [0;1;31m��[0;1m   ",
			sysop[x]);
	}
	for (z = 0; z < 16; z++) {
		a = s[z];
		if (a < 1) ;
		else {
			fprintf(fp, "\n\n ����%sȨ�޵�ID:\n", perms[z]);
			for (y = 0; y < a; y++) {
				if (y % 4 == 0)
					fprintf(fp, "\n");
				fprintf(fp, " %-16s", record[z][y]);
			}
		}
	}
	fprintf(fp, "\n\n");
	fclose(fp);
	return 0;
}
