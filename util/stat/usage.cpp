extern "C" {
#include "bbs.h"
#include "libBBS.h"
}
#include <map>
#include <iostream>
#include <algorithm>
#include <string>

using namespace std;
#define BUFSIZE		8192
#define BBSBOARDS BBSHOME"/.BOARDS"
char datestring[30];

struct binfo {
	char boardname[18];
	char expname[28];
	int	times;
	int sum;
} st[MAXBOARD];

map<string, int> id;

int numboards = 0;

int
brd_cmp(const struct binfo &b, const struct binfo &a)
{
	if (a.times != b.times)
		return (a.times < b.times);
	return a.sum < b.sum;
}

/*
int
apply_record(char *filename, int (*fptr)(), int size)
{
	char abuf[BUFSIZE];
	int fd;

	if (size > BUFSIZE) {
		return -1;
	}

	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return -1;
	while (read(fd, abuf, size) == size)
		if ((*fptr) (abuf) == QUIT) {
			close(fd);
			return QUIT;
		}
	close(fd);
	return 0;
}
*/

int
getdatestring(time_t now)
{
	struct tm *tm;
	char weeknum[7][3] = { "��", "һ", "��", "��", "��", "��", "��" };

	tm = localtime(&now);
	sprintf(datestring, "%4d��%02d��%02d��%02d:%02d:%02d ����%2s",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, weeknum[tm->tm_wday]);
	return 0;
}

int
record_data(char *board, int sec, int times)
{
	int idx = id[board] - 1;
  if( idx >= 0 ) {
    st[idx].times += times;
    st[idx].sum += sec;
  }
	return 0;
}

int
fillbcache(struct boardheader *fptr)
{

	if (numboards >= MAXBOARD)
		return 0;
	
	if (fptr->filename[0] == '\0')
		return 0;

	if ((fptr->level != 0) && !(fptr->level & PERM_NOZAP || fptr->level & PERM_POSTMASK))
		return 0;

	if (fptr->flag & (BRD_RESTRICT | BRD_NOPOSTVOTE))
		return 0;

	id[fptr->filename] = numboards+1;
	strcpy(st[numboards].boardname, fptr->filename);
	strcpy(st[numboards].expname, fptr->title + 8);
	st[numboards].times = 0;
	st[numboards].sum = 0;
	numboards++;
	return 0;
}

int
fillhistorydata()
{
	FILE *fp;
	int sec, times;
	char bname[STRLEN];

	if ((fp = fopen("reclog/usage.history", "r")) == NULL)
		return 1;

	while (!feof(fp)) {
		if (fscanf(fp, "%d %d %s", &sec, &times, bname) != 3)
			break;
		record_data(bname, sec, times);
	}
	fclose(fp);
	return 0;
}

int
savehistorydata()
{
	FILE *fp;
	int i;

	if ((fp = fopen("reclog/usage.history", "w")) != NULL) {
		/* monster: since array st is always sorted when savehistorydata
			    is called, the last "board" should be "Average" which
			    must be omited */
		for (i = 0; i < numboards - 1; i++)
			fprintf(fp, "%d %d %s\n", st[i].sum, st[i].times,
				st[i].boardname);
		fclose(fp);
	}
  return 0;
}

int
fillboard()
{
//	apply_record(BBSBOARDS, fillbcache, sizeof (struct boardheader));

	char abuf[BUFSIZE];
	int fd;
  int size = sizeof(boardheader);

	if (size > BUFSIZE) {
		return -1;
	}

	if ((fd = open(BBSBOARDS, O_RDONLY, 0)) == -1)
		return -1;
	while (read(fd, abuf, size) == size)
		if (fillbcache((struct boardheader*)abuf) == QUIT) {
			close(fd);
			return QUIT;
		}
	close(fd);
	return 0;


}

char *
timetostr(int i)
{
	static char str[30];
	int minute, sec, hour;

	minute = (i / 60);
	hour = minute / 60;
	minute = minute % 60;
	sec = i % 60;
	sprintf(str, "%2d:%02d:%02d", hour, minute, sec);
	return str;
}


int
main(int argc, char **argv)
{
	FILE *fp;
	FILE *op;
	char buf[256], buf2[256], *p, bname[20];
	int mode;
	int c[3];
	int max[3];
	int ave[3];
	int now, sec;
	int i, j, k;
	struct stat fst;
	char blk[10][3] = {
		"��", "��", "�x", "�y", "�z",
		"�{", "�|", "�}", "�~", "��",
	};

	mode = (argc > 1) ? atoi(argv[1]) : 1;

	chdir(BBSHOME);

	sprintf(buf, "%s/0Announce/bbslist/board%d.tmp", BBSHOME,
		(mode == 1) ? 2 : 1);

	if ((op = fopen(buf, "w")) == NULL) {
		printf("Can't Write file\n");
		return 1;
	}

	fillboard();
	fillhistorydata();

	now = time(0);
	getdatestring(now);

  if ( mode != 1) {
    system("cat reclog/use_board > reclog/use_board.tmp");
    truncate("reclog/use_board", 0);
    if ((fp = fopen("reclog/use_board.tmp", "r")) == NULL || fstat(fileno(fp), &fst) == -1) {
      printf("can't open use_board\n");
      return 1;
    }
  

    while (fgets(buf, 256, fp)) {
      if (strlen(buf) < 57)
        continue;
      if (!strncmp(buf + 25, "USE", 3)) {
        p = strstr(buf, "USE");
        p += 4;
        p = strtok(p, " ");
        strcpy(bname, p);
        if ((p = (char *) strstr(buf + 50, "Stay: "))!=NULL) {
          sec = atoi(p + 6);
        } else
          sec = 0;
        record_data(bname, sec, 1);
      }
    }
  
    fclose(fp);

    system("cat reclog/use_board.tmp >> reclog/use_board.week");
    f_rm((char*)"reclog/use_board.tmp");
  }

	sort(st, st+numboards, brd_cmp);

	ave[0] = 0;
	ave[1] = 0;
	ave[2] = 0;
	max[1] = 0;
	max[0] = 0;
	max[2] = 0;
	for (i = 0; i < numboards; i++) {
		ave[0] += st[i].times;
		ave[1] += st[i].sum;
		ave[2] += st[i].times == 0 ? 0 : st[i].sum / st[i].times;
		if (max[0] < st[i].times) {
			max[0] = st[i].times;
		}
		if (max[1] < st[i].sum) {
			max[1] = st[i].sum;
		}
		if (max[2] < (st[i].times == 0 ? 0 : st[i].sum / st[i].times)) {
			max[2] =
			    (st[i].times == 0 ? 0 : st[i].sum / st[i].times);
		}
	}
	c[0] = max[0] / 30 + 1;
	c[1] = max[1] / 30 + 1;
	c[2] = max[2] / 30 + 1;
	numboards++;
	st[numboards - 1].times = ave[0] / numboards;
	st[numboards - 1].sum = ave[1] / numboards;
	strcpy(st[numboards - 1].boardname, "Average");
	strcpy(st[numboards - 1].expname, "��ƽ��");

	if (mode == 1) {
		fprintf(op, "[1;37m���� %-15.15s%-28.28s %5s     %8s  %8s[m\n",
			"����������", "��������", "�˴�", "�ۻ�ʱ��",
			"ƽ��ʱ��");
	} else {
		fprintf(op,
			"      [1;37m1 [m[34m%2s[1;37m= %d (���˴�) [1;37m1 [m[32m%2s[1;37m= %s (�ۻ���ʱ��) [1;37m1 [m[31m%2s[1;37m= %d ��(ƽ��ʱ��)\n\n",
			blk[9], c[0], blk[9], timetostr(c[1]), blk[9], c[2]);
	}

	for (i = 0; i < numboards; i++) {
		if (mode == 1) {
			fprintf(op,
				"[1m%4d[m %-15.15s%-28.28s %5d %12s %9d\n",
				i + 1, st[i].boardname, st[i].expname,
				st[i].times, timetostr(st[i].sum),
				st[i].times == 0 ? 0 : st[i].sum / st[i].times);
    } else {
			fprintf(op,
				"      [1;37m��[31m%3d [37m�� ���������ƣ�[31m%s [35m%s[m\n",
				i + 1, st[i].boardname, st[i].expname);
			fprintf(op,
				"[1;37m    ��������������������������������������������������������������������������\n");
			fprintf(op, "[1;37m�˴Ω�[m[34m");
			for (j = 0; j < st[i].times / c[0]; j++) {
				fprintf(op, "%2s", blk[9]);
			}
			fprintf(op, "%2s [1;37m%d[m\n",
				blk[(st[i].times % c[0]) * 10 / c[0]],
				st[i].times);
			fprintf(op, "[1;37mʱ�䩦[m[32m");
			for (j = 0; j < st[i].sum / c[1]; j++) {
				fprintf(op, "%2s", blk[9]);
			}
			fprintf(op, "%2s [1;37m%s[m\n",
				blk[(st[i].sum % c[1]) * 10 / c[1]],
				timetostr(st[i].sum));
			j = st[i].times == 0 ? 0 : st[i].sum / st[i].times;
			fprintf(op, "[1;37mƽ����[m[31m");
			for (k = 0; k < j / c[2]; k++) {
				fprintf(op, "%2s", blk[9]);
			}
			fprintf(op, "%2s [1;37m%s[m\n",
				blk[(j % c[2]) * 10 / c[2]], timetostr(j));
			fprintf(op,
				"[1;37m    ��������������������������������������������������������������������������[m\n\n");
		}
	}
	fclose(op);

  sprintf(buf, "%s/0Announce/bbslist/board%d.tmp", BBSHOME,
    (mode == 1) ? 2 : 1);
  sprintf(buf2, "%s/0Announce/bbslist/board%d", BBSHOME,
    (mode == 1) ? 2 : 1);

  if(f_mv(buf, buf2) == -1)
    f_rm(buf);
  
	savehistorydata();


	return 0;
}
