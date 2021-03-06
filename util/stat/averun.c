/*  averun.c  -- calculate the average logon users per hour  */
/* $Id: averun.c,v 1.1.1.1 2003-02-20 19:54:45 bbs Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "config.h"

#define AVEFLE  BBSHOME"/reclog/ave.src"
#define AVEPIC  BBSHOME"/0Announce/bbslist/today"
#define MAX_LINE (15)

/* Added by deardragon 1999.12.2 */
char datestring[30];
void
getdatestring(time_t now)
{
	struct tm *tm;
	char weeknum[7][3] = { "天", "一", "二", "三", "四", "五", "六" };

	tm = localtime(&now);
	sprintf(datestring, "%4d年%02d月%02d日%02d:%02d:%02d 星期%2s",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, weeknum[tm->tm_wday]);
}

/* Added End. */

int
draw_pic()
{
	char *blk[10] = {
		"＿", "＿", "▁", "▂", "▃",
		"▄", "▅", "▆", "▇", "█"
	};
	FILE *fp;
	int max = 0, cr = 0, tm, i, item, j, aver = 0;
	int pic[24];
	char buf[80];
	time_t now;

	now = time(0);
	getdatestring(now);
	if ((fp = fopen(AVEFLE, "r")) == NULL)
		return -1;
	else {
		memset(&pic, 0, sizeof (pic));
		i = 0;
		while (fgets(buf, 50, fp) != NULL) {
			cr = atoi(index(buf, ':') + 1);
			tm = atoi(buf);
			pic[tm] = cr;
			aver += cr;
			i++;
			max = (max > cr) ? max : cr;
		}
		aver = aver / i + 1;
		fclose(fp);
	}

	if ((fp = fopen(AVEPIC, "w")) == NULL)
		return -1;

	item = (max / MAX_LINE) + 1;

	fprintf(fp,
		"\n [1;36m   ┌————————————————————————————————————┐[m\n");

	for (i = MAX_LINE; i >= 0; i--) {
		fprintf(fp, "[1;37m%3d[36m │[32m", (i + 1) * item);
		for (j = 0; j < 24; j++) {
			if ((item * i > pic[j]) && (item * (i - 1) <= pic[j]) &&
			    pic[j]) {
				fprintf(fp, "[35m%-3d[32m", (pic[j]));
				continue;
			}
			if (pic[j] - item * i < item && item * i < pic[j])
				fprintf(fp, "%s ",
					blk[((pic[j] - item * i) * 10) / item]);
			else if (pic[j] - item * i >= item)
				fprintf(fp, "%s ", blk[9]);
			else
				fprintf(fp, "   ");
		}
		fprintf(fp, "[1;36m│[m");
		fprintf(fp, "\n");
	}
	time(&now);
	fprintf(fp,
		"[1;37m  0 [36m└——[37m%-12.12s平均负载人数统计[36m —— [37m%s[36m ——┘[m\n",
		BBSNAME, datestring);
	fprintf(fp,
		"[1;36m      00 01 02 03 04 05 06 07 08 09 10 11 [31m12 13 14");
	fprintf(fp, " 15 16 17 18 19 20 21 22 23[m\n\n");
	fprintf(fp,
		"                         [1;36m    1 [32m■[36m = [37m%3d     [36m 平均上站人数：[37m%3d[m\n",
		item, aver);
	fclose(fp);
	return 0;
}

int
parse_ave(time, ave)
int time, ave;
{
	FILE *fp;

	if ((fp = fopen(AVEFLE, "a+")) == NULL)
		return -1;
	fprintf(fp, "%d:%d\n", time, ave);
	fclose(fp);
	return 0;
}

int
gain_hour(buf)
char *buf;
{
	int retm;

	retm = atoi(buf);
	if (strstr(buf, "pm") && retm != 12)
		retm += 12;
	if (strstr(buf, "am") && retm == 12)
		retm = 0;
	return retm;
}

int
init_base(file, time)
char *file;
int *time;
{
	FILE *fp;
	char buf[80], *p;
	int ave = 0, tmp = 0, i;

	if ((fp = fopen(file, "r")) == NULL) {
		printf("File: %s cannot be opened\n", file);
		exit(-2);
	}

	for (i = 0; i < 12; i++) {
		int once = 0;

		if (fgets(buf, 99, fp) == NULL)
			break;
		if (strstr(buf, "day"))
			once = 1;
		if (i == 0)
			*time = gain_hour(buf);
		strtok(buf, ",");
		if (once)
			strtok(NULL, ",");
		p = strtok(NULL, ",");
		ave = ave + atoi(p);
	}
	tmp = ave / i;
	if (tmp * i != ave)
		tmp++;
	fclose(fp);
	return tmp;
}

int
main(argc, argv)
int argc;
char **argv;
{
	int ave, time;

	if (argc < 2) {
		printf("Usage: %s crontab_output_filename\n", argv[0]);
		exit(-1);
	}
	ave = init_base(argv[1], &time);
	parse_ave(time, ave);
	draw_pic(time);
	return 0;
}
