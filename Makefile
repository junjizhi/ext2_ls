all: ext2_ls ext2_mkdir

ext2_ls: 
	gcc -g -o ext2_ls ext2_ls.c

ext2_mkdir:
	gcc -g -o ext2_mkdir ext2_mkdir.c

clean:
	rm ext2_mkdir ext2_ls
