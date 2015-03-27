CC=gcc
DEPS = ext2_comm.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< 

all: ext2_ls ext2_mkdir

ext2_ls: ext2_ls.o
	gcc -g -o ext2_ls ext2_ls.o

ext2_mkdir: ext2_mkdir.o
	gcc -g -o ext2_mkdir ext2_mkdir.o

.PHONY: clean
clean:
	rm -f ext2_mkdir ext2_ls *.o
