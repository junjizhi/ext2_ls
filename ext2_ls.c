/* From http://www.cdf.toronto.edu/~csc369h/winter/assignments/a3/a3.html:

ext2_ls: This program takes two command line arguments. The first is the name of an ext2 formatted virtual disk. The second is an absolute path on the ext2 formatted disk. The program should work like ls -1, printing each directory entry on a separate line. Unlike ls -1, it should also print . and ... In other words, it will print one line for every directory entry in the directory specified by the absolute path. If the directory does not exist print "No such file or diretory". */

#include "ext2.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SUPERBLOCK_OFFSET 1024
#define EXT2_INODE_SIZE sizeof(struct ext2_inode)

void print_superblock(char*);

int main(int argc, char* argv[]){    
    char* disk_img = argv[1];
    /* printf("disk_img is %s\n", disk_img); */
    char* path = argv[2];

    /* disk image disk descriptor */
    print_superblock(disk_img);
}

void print_superblock(char* disk_img){
    int fd = open(disk_img, O_RDONLY);
    int ret = -1;
    char* buf = malloc(sizeof(struct ext2_super_block));
    struct ext2_super_block* sb = (struct ext2_super_block*)buf;
    int offset_advance = -1; 
    int read_size = -1; 
    struct ext2_inode* rino; 
    struct ext2_group_desc* egd;

    printf("superblock struct size is %lu\n" , sizeof(struct ext2_super_block));
    /* seek to the offset where superblock starts */
    offset_advance = SUPERBLOCK_OFFSET;
    read_size = sizeof(struct ext2_super_block);
    lseek(fd, offset_advance, SEEK_CUR);
    if ((ret = read(fd, buf, read_size)) < 1){
	printf("Failed to read from fd\n");
	return;
    }

    printf("inode size: %lu \n", EXT2_INODE_SIZE);
    printf("block size: %u \n", EXT2_BLOCK_SIZE);
    /* printf("seekcur is %d\n", SEEK_CUR); */

    free(buf);
    buf = malloc(sizeof(struct ext2_group_desc));
    egd = (struct ext2_group_desc*)buf;
    /* now look for the block destriptor */
    offset_advance = sizeof(struct ext2_group_desc);
    /* lseek(fd, offset_advance, SEEK_CUR);     */
    read_size = sizeof(struct ext2_group_desc);
    if ((ret = read(fd, buf, read_size)) < 1){
	printf("Failed to read from fd\n");
	return;
    }
    printf("block bitmap is %u \n",egd->bg_block_bitmap);
    
    /* look for the inode table block */
    unsigned int inode_table_block_addr = (egd->bg_inode_table) * EXT2_BLOCK_SIZE;
    
    int root_inode_index = 1;
    /* int containing_block = (root_inode_index * inode_size) / block_size; */

    unsigned int inode_table_offset = inode_table_block_addr + (root_inode_index) * EXT2_INODE_SIZE;
    /* set to inode table offset */
    lseek(fd, inode_table_offset, SEEK_SET);
    struct ext2_inode* root_inode; 
    read_size = sizeof(struct ext2_inode);
    char* buf2 = malloc(sizeof(struct ext2_inode));
    root_inode = (struct ext2_inode*) buf2;
    if ((ret = read(fd, buf2, read_size)) < 1){
	printf("Failed to read from fd\n");
	return;
    }

    /* free(buf); */
}


int get_total(){
    return 44;
}

void output(){
    
}

/*struct stat{
unsigned int st_mode;

};
#define S_IFMT 0
#define S_IFDIR 1
*/
int read_stat(int fd, char* path, struct stat* st){
return 0;
}

/* read the single file or path. */
void ls(int fd, char* path){
    struct stat stbuf;
    
    if(read_stat(fd, path, &stbuf) == -1){
	fprintf(stderr, "fsize: can't access %s\n", path);
	return;
    }
    /* if the path is a dir */
    if ((stbuf.st_mode & S_IFMT) == S_IFDIR){
	/* output total */
	int total = get_total();
	printf("total %d\n", total);
	/* dirwalk(fd, path, output);  */
    }else{			/* else it is a file */
	output();
    }
}
