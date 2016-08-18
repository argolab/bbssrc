#################################
#				#
# Makefile for System Utilities #
#				#
#################################

CC = gcc
ALL = delaccount deljunk logrotate mergeb undeny reset_inboarduser postfile genrss dump_header birthday

all: $(ALL)

# �����ļ��ؽ�����
buildir: buildir.c ../../include/bbs.h
	$(CC) -m32 -o buildir buildir.c -I../../include -L../../lib -lSystem -D__NO_PROTOTYPE__

# ���������˺�
delaccount: delaccount.c ../../include/bbs.h
	$(CC) -m32 -o delaccount delaccount.c -I../../include -L../../lib -lBBS -lSystem -D__NO_PROTOTYPE__

# ��������վ�ͷ�ֽ¨
deljunk: deljunk.c ../../include/bbs.h
	$(CC) -m32 -o deljunk deljunk.c -I../../include

# ����ϵͳ��¼
logrotate: logrotate.c ../../include/bbs.h
	$(CC) -m32 -o logrotate logrotate.c -I../../include

# �ϲ�����
mergeb: mergeb.c 
	$(CC) -m32 -o mergeb mergeb.c

# �Զ����
undeny: undeny.c ../../include/bbs.h
	$(CC) -m32 -o undeny undeny.c -I../../include -L../../lib -lBBS -lSystem

reset_inboarduser: reset_inboard_user.c ../../include/bbs.h
	$(CC) -m32 -o reset_inboarduser reset_inboard_user.c -I../../include -L../../lib -lBBS -lSystem

# ����rss����
genrss: genrss.c ../../include/bbs.h
	$(CC) -m32 -o genrss genrss.c -I../../include -L../../lib -lBBS -lSystem #-liconv

postfile: postfile.c ../../include/bbs.h
	$(CC) -m32 -o postfile postfile.c -I../../include -L../../lib -lBBS -lSystem 

dump_header: dump_header.c ../../include/bbs.h
	$(CC) -m32 -o dump_header dump_header.c -I../../include

#����
birthday: birthday.c
	$(CC) -m32 -g -o birthday birthday.c -I../../include -L../../lib -lBBS -lSystem

clean: 
	-rm -fr *~ *.o *.bak
	-rm -fr $(ALL)

install: all
	cp $(ALL) ../../bin