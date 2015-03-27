CC=gcc
DEPS = ext2_comm.h


all: ext2_ls ext2_mkdir

ext2_ls: 
	gcc -g -o ext2_ls ext2_ls.c

ext2_mkdir: 
	gcc -g -o ext2_mkdir ext2_mkdir.c

.PHONY: clean ext_ls ext2_mkdir
clean:
	rm -f ext2_mkdir ext2_ls *.o
