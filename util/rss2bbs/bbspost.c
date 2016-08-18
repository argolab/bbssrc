#include "rss2bbs.h"

int
write_post(FILE *fp, feeditem_t *post)
{
	static char buf[256];
	char timebuf[256];
	struct tm *tv;
	time_t now = time(NULL);

	tv = localtime(&now);
	strftime(timebuf, sizeof(timebuf), "%a %b %e %T %Y", tv);
	
	
	fprintf(fp, "������: %s (%s), ����: %s\n", currfeed.author, currfeed.author, currfeed.board);
	fprintf(fp, "��  ��: %.56s\n", post->title);
	fprintf(fp, "����վ: %s (%s), �Զ�����\n", currfeed.sitename, timebuf);
	fprintf(fp, "\n");

	fprintf(fp, "\033[1;32m����: \033[m%s\n", ((post->author[0] != '\0') ? post->author : "δ֪"));
	fprintf(fp, "\033[1;32mʱ��: \033[m%s\n", ((post->pubDate[0] != '\0') ? post->pubDate : "δ֪"));	
	fprintf(fp, "\033[1;32m����: \033[m%s\n\n", post->link);

	fprintf(fp, "\033[1;32mժҪ:\033[m\n");

	/* Wrap Print the body */
	char *curr = post->desc;
	char *line = post->desc;
	int len = 0;				/* ǰ�滹���ĸ��ո�, ��ʼ��һ�� */

	for (; *curr != '\0'; curr++) {
		if (len >= 71) {
			strncpy(buf, line, len);
			buf[len] = '\0';
			fprintf(fp, "%s\n", buf);

			len = 0;
			line = curr;
		}

		if (*curr & 0x80) {
			curr++;
			len += 2;
		} else len++;		
	}
	if (*line != '\0') fprintf(fp, "%s\n", line);
	

	/* Print Footter */
	fprintf(fp, "\n--\n\033[m\033[1;31m�� ��Դ:��%s %s��[FROM: RSS2BBS]\033[m\n",
		BBSNAME, BBSHOST);
	return 0;
}

void
post_items(void)
{
	int i;
	FILE *fp;
	for (i = num_items - 1; i >= 0; i--) {
		fp = fopen(TMPFILE, "w");
		if (!fp) return;
		write_post(fp, item + i);
		fclose(fp);
		
		postfile(TMPFILE, currfeed.board, item[i].title, currfeed.author, 0);
	}
}

