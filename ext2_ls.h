/* From http://www.cdf.toronto.edu/~csc369h/winter/assignments/a3/a3.html:

ext2_ls: This program takes two command line arguments. The first is the name of an ext2 formatted virtual disk. The second is an absolute path on the ext2 formatted disk. The program should work like ls -1, printing each directory entry on a separate line. Unlike ls -1, it should also print . and ... In other words, it will print one line for every directory entry in the directory specified by the absolute path. If the directory does not exist print "No such file or diretory". */

#include "ext2.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "list.h"
#include "utils.h"

#define SUPERBLOCK_OFFSET 1024
#define EXT2_INODE_SIZE sizeof(struct ext2_inode)
#define I_BLOCK_MAX 13
#define IMODE_TYPE_MASK 0xF000
#define MAX_REC_LEN 980

#define DEBUG 1

/* ext2 directory entry list */
typedef struct {
    struct ext2_dir_entry* e;
    struct list_head list;
} dir_entry_list_t;

typedef struct {
    int fd;
    /* char* name; */
    unsigned int inode_table_address;
} Path_info;

void ls(char* path);

Path_info g_info;
void _init_ext2(char*);

unsigned int read_inode_table_address(int fd);
/* init all the path info  */
void _init_ext2(char* disk_img)
{
    g_info.fd = open(disk_img, O_RDONLY);
    g_info.inode_table_address = read_inode_table_address(g_info.fd);
    /* g_info.name = path; */
}

void _exit_ext2()
{
    /* close the fd */
    close(g_info.fd);
}

struct ext2_inode* get_inode(int inode_no);


unsigned int read_inode_table_address(int fd){

    int ret = -1;
    char* buf = malloc(sizeof(struct ext2_super_block));
    struct ext2_super_block* sb = (struct ext2_super_block*)buf;
    int offset_advance = -1; 
    int read_size = -1; 
    struct ext2_inode* rino; 
    struct ext2_group_desc* egd;

    /* seek to the offset where superblock starts */
    offset_advance = SUPERBLOCK_OFFSET;
    read_size = sizeof(struct ext2_super_block);
    lseek(fd, offset_advance, SEEK_SET);
    if ((ret = read(fd, buf, read_size)) < 1){
	printf("Failed to read from fd\n");
	return;
    }
#if DEBUG
    printf("inode size: %lu \n", EXT2_INODE_SIZE);
    printf("block size: %u \n", EXT2_BLOCK_SIZE);
#endif
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
    /* look for the inode table block */
    unsigned int inode_table_block_addr = (egd->bg_inode_table) * EXT2_BLOCK_SIZE;
#if DEBUG
    printf("block bitmap is %u \n",egd->bg_block_bitmap);
    printf("inode_table_block_addr is %u \n", inode_table_block_addr);
#endif    
   
    return inode_table_block_addr;
}

/* based from the inode_table_block_addr, read the content from inode */
dir_entry_list_t* read_inode(int inode_no, int* is_regular_file){
    int fd = g_info.fd;
    unsigned inode_table_address = g_info.inode_table_address;
    int read_size = -1;
    int ret = -1; 
    dir_entry_list_t* entries = NULL;
    struct ext2_inode* inode = get_inode(inode_no);
    unsigned short masked_inode = (inode->i_mode) & IMODE_TYPE_MASK;

    if((masked_inode ^ EXT2_S_IFREG) == 0){ /* if it is a file */
	*is_regular_file = 1;
	return NULL;
    }

    if((masked_inode ^ EXT2_S_IFDIR) == 0){ /* if it is a directory */
	int i = 0 ;
    /* TODO: to support multiple link inodes */
    /* read all the i_bloc's, i.e., data blocks */
    while ( inode->i_block[i] != 0  /* i_block needs to non-zero to be valid */
	    && i < I_BLOCK_MAX ){ 
	unsigned int data_block = inode->i_block[i];
	unsigned int offset = data_block * EXT2_BLOCK_SIZE; 
	lseek(fd, offset, SEEK_SET);
	read_size = sizeof(struct ext2_dir_entry);

	int target_inode_number = inode_no;
	int read_all_entries = 0;
	/* read all the directory entries */
	while(!read_all_entries){
	    unsigned int read_inode;
	    unsigned short rec_len;
	    unsigned char name_len;
	    unsigned char file_type;
	    ret = read(fd, &read_inode, sizeof(unsigned int));
	    ret = read(fd, &rec_len, sizeof(unsigned short));
	    ret = read(fd, &name_len, sizeof(unsigned char));
	    ret = read(fd, &file_type, sizeof(unsigned char));
	    
	    if (rec_len >= MAX_REC_LEN) /* if this is the last entry in the list */
		read_all_entries = 1;

	    int name_field_size = rec_len - sizeof(struct ext2_dir_entry) ;
	    struct ext2_dir_entry * dir_entry = (struct ext2_dir_entry*)malloc(rec_len);
	    if ((ret = read(fd, dir_entry->name, name_field_size)) < 1){
		printf("Failed to read from fd\n");
		return;
	    }

	    /* to see if we exhaust all entries. 
	       if we read something like this: {inode = 11, rec_len = 12, name_len = 513, name = . }, then it means it is not in the root node any more. Should exit the loop
	       TODO: may NOT be the best way to do this*/
	    ret = (strcmp(dir_entry->name, ".") == 0);
#if DEBUG
	    if (ret == 1){
		printf("ret == 1, read_inode is %d, target_inode_number = %d\n", read_inode, target_inode_number);
	    }
#endif
	    if( (ret && read_inode != target_inode_number)){
		free(dir_entry);
		break;		/* exit the loop */		
	    }
	    
	    if(dir_entry){
		dir_entry->inode = read_inode;
		dir_entry->rec_len = rec_len;
		dir_entry->name_len = (unsigned short)name_len;
	    }
#if DEBUG	       
	    printf("{inode = %u, rec_len = %u, name_len = %u, name = %s}\n",
		   dir_entry->inode, dir_entry->rec_len, dir_entry->name_len,
		   dir_entry->name);
#endif
	    /* add the entry to result */
	    dir_entry_list_t* new_entry = (dir_entry_list_t*) malloc(sizeof(dir_entry_list_t));
	    new_entry->e = dir_entry;		
	    /* new_entry->list = NULL; */
	    /* if it is new, then simply initialize it */
	    if ( entries == NULL ){
		entries = new_entry;
		INIT_LIST_HEAD(&(entries->list));
	    }else{		
		list_add_tail(&(new_entry->list), &(entries->list));
	    }
	    /* free(dir_entry->name); */
	    /* free(buf3); */
	}   /* end of while loop */
	/* free(buf); */
	i++;
    }
    }
    return entries;
}

void print_list(dir_entry_list_t* current_entry_list){
    dir_entry_list_t* tmp;

    if (current_entry_list == NULL){
	/* printf("ERROR: list is empty.\n"); */
	return;
    }

    list_for_each_entry(tmp, &current_entry_list->list, list){
	printf("PRINT_LIST: {inode = %u, rec_len = %u, name_len = %u, name = %s}\n",
		   tmp->e->inode, tmp->e->rec_len, tmp->e->name_len,
		   tmp->e->name);
    }
}

dir_entry_list_t* read_root_entry_list(){
    int root_inode = 2;		/* root inode is 2 */
    int is_regular_file = 0;
    return read_inode(root_inode, &is_regular_file);
    assert( is_regular_file == 0 );
}

void print_dir(struct ext2_inode* inode){
    int n = -1;
    printf("Total: %d\n", n);
}

void print_file(struct ext2_inode* inode){
    int n = 1;
    printf("Total: %d\n", n);    
}

/* return the file type of an inode */
unsigned short get_file_type(struct ext2_inode* inode){

    assert(inode != NULL);
    unsigned short masked_inode = (inode->i_mode) & IMODE_TYPE_MASK;
    if((masked_inode ^ EXT2_S_IFREG) == 0){ /* if it is a file */
	return EXT2_S_IFREG;
    }

    if((masked_inode ^ EXT2_S_IFDIR) == 0){ /* if it is a directory */
	return EXT2_S_IFDIR;
    }
    return EXT2_S_UNKNOWN;
}


void ls(char* path){

    int inode_no = get_inode_no(path);

    if (inode_no == -1){
	printf("No such file or diretory\n");
	return;
    }

    struct ext2_inode* inode = get_inode(inode_no);

    switch( get_file_type(inode) ){
    case EXT2_S_IFDIR:
	print_dir(inode); 
	break;
    case EXT2_S_IFREG:
	print_file(inode); 
	break;
    case EXT2_S_UNKNOWN:
    default:
	printf("ERROR: unknown file type\n");
	break;
    }
}
 
int get_inode_no(char* path){
    int i, n = -1;
    int ret = -1; 
    /* split the path into dir splits */
    char** splits = str_split(path, '/', &n);

    /* read root dir */
    dir_entry_list_t*  current_entry_list = read_root_entry_list();

    /* loop through each dir */
    for ( i=0; i < n; i++ ){
	int found_inode = -1;
	int is_dir = -1; 
	dir_entry_list_t* tmp; 

	list_for_each_entry(tmp, &current_entry_list->list, list){
	    if ( strncmp(tmp->e->name, splits[i], tmp->e->name_len) == 0 ){
		found_inode = tmp->e->inode;
		int is_regular_file = 0 ;
		/* found, then update the current entry list*/
		dir_entry_list_t* tmp2 = read_inode(found_inode, &is_regular_file);	     
	    /* if it is a regular file, then */
		if( tmp == NULL ){		    
		    assert(is_regular_file); /* invalid combination */
		}else{
		    current_entry_list = tmp2;
		}		  
		break;
	    }
	}
	ret = found_inode;
	if (found_inode == -1){
	    ret = -1;
	    /* TODO: free resources, too */
	    current_entry_list = NULL;
	    break;
	}	   
    }    
    return ret; 
}

struct ext2_inode* get_inode(int inode_no){
    int ret = -1;
    struct ext2_inode* inode = NULL; 
    unsigned int inode_table_offset = g_info.inode_table_address + (inode_no-1) * EXT2_INODE_SIZE;
    /* set to inode table offset */
    lseek(g_info.fd, inode_table_offset, SEEK_SET);
    unsigned int read_size = sizeof(struct ext2_inode);
    char* buf2 = malloc(sizeof(struct ext2_inode));
    inode = (struct ext2_inode*) buf2;
    if ((ret = read(g_info.fd, buf2, read_size)) < 1){
	printf("Failed to read from fd\n");
	exit(0);
    }   
    return inode;
}
