/* From http://www.cdf.toronto.edu/~csc369h/winter/assignments/a3/a3.html:

ext2_ls: This program takes two command line arguments. The first is the name of an ext2 formatted virtual disk. The second is an absolute path on the ext2 formatted disk. The program should work like ls -1, printing each directory entry on a separate line. Unlike ls -1, it should also print . and ... In other words, it will print one line for every directory entry in the directory specified by the absolute path. If the directory does not exist print "No such file or diretory". */

#ifndef _EXT2_COMM_H_
#define _EXT2_COMM_H_

#include "ext2.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include "list.h"
#include "utils.h"
#include <sys/types.h>
#include <sys/time.h>

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
    struct ext2_group_desc* egd; 
    struct ext2_super_block* sb; 
    unsigned int inode_table_address;
} Path_info;

Path_info g_info;
void _init_ext2(char*);
void _exit_ext2();
void ext2_ls(char* path);


void read_metadata(int fd);
/* init all the path info  */
void _init_ext2(char* disk_img)
{
    g_info.fd = open(disk_img, O_RDONLY);
    read_metadata(g_info.fd);
}

void _exit_ext2()
{
    /* free(g_info.egd); */
    /* free(g_info.sb); */
    close(g_info.fd);
}

struct ext2_inode* get_inode(int inode_no);

void read_metadata(int fd){

    int ret = -1;
    char* buf = malloc(sizeof(struct ext2_super_block));
    struct ext2_super_block* sb = (struct ext2_super_block*)buf;
    int offset_advance = -1; 
    int read_size = -1; 
    struct ext2_inode* rino; 
    struct ext2_group_desc* egd;

    /* seek to the superblock offset */
    offset_advance = SUPERBLOCK_OFFSET;
    read_size = sizeof(struct ext2_super_block);
    lseek(fd, offset_advance, SEEK_SET);
    if ((ret = read(fd, buf, read_size)) < 1){
	printf("Failed to read from fd\n");
	return;
    }

    free(buf);
    buf = malloc(sizeof(struct ext2_group_desc));
    egd = (struct ext2_group_desc*)buf;
    /* now look for the block destriptor */
    offset_advance = sizeof(struct ext2_group_desc);

    read_size = sizeof(struct ext2_group_desc);
    if ((ret = read(fd, buf, read_size)) < 1){
	printf("Failed to read from fd\n");
	return;
    }
    /* look for the inode table block */
    g_info.inode_table_address = (egd->bg_inode_table) * EXT2_BLOCK_SIZE;    
    g_info.egd = egd;
    g_info.sb = sb;
}

/* read all the entries in a block and update the *entries* paremter */
int read_all_entries_in_a_block(dir_entry_list_t* entries, unsigned int data_block){

    unsigned int offset = data_block * EXT2_BLOCK_SIZE; 
    lseek(g_info.fd, offset, SEEK_SET);

    char* block = malloc( EXT2_BLOCK_SIZE );
    /* read a whole block */
    if( read(g_info.fd, block, EXT2_BLOCK_SIZE) == EXT2_BLOCK_SIZE ){
	int block_offset = 0;
	unsigned int read_inode;
	unsigned short rec_len;
	unsigned char name_len;
	unsigned char file_type;	  	    

	while ( block_offset < EXT2_BLOCK_SIZE ) {
	    unsigned int old = block_offset; 
	    read_inode = *((unsigned int*) (block + block_offset));
	    block_offset += sizeof(unsigned int);
	    rec_len = *((unsigned short*) (block + block_offset));
	    block_offset += sizeof(unsigned short);
	    name_len = *((unsigned char*) (block + block_offset));
	    block_offset += sizeof(unsigned char);
	    file_type = *((unsigned char*) (block + block_offset));
	    block_offset += sizeof(unsigned char);
		
	    struct ext2_dir_entry * dir_entry = (struct ext2_dir_entry*)malloc(rec_len);	      

	    if(dir_entry){
		dir_entry->inode = read_inode;
		dir_entry->rec_len = rec_len;
		dir_entry->name_len = (unsigned short)name_len;
		strncpy(dir_entry->name, (block + block_offset), name_len); /* copy the name */
		(dir_entry->name)[name_len] = '\0';
	    }       
#if DEBUG	       
	    printf("{inode = %u, rec_len = %u, name_len = %u, name = %s}\n",
		   dir_entry->inode, dir_entry->rec_len, dir_entry->name_len,
		   dir_entry->name);
#endif

	    /* add the entry to result */
	    dir_entry_list_t* new_entry = (dir_entry_list_t*) malloc(sizeof(dir_entry_list_t));
	    new_entry->e = dir_entry;		
	    /* if it is new, then simply initialize it */
	    if( entries == NULL ){
		entries = (dir_entry_list_t*) malloc(sizeof(dir_entry_list_t));
		INIT_LIST_HEAD(&(entries->list));		    
	    }

	    list_add_tail(&(new_entry->list), &(entries->list));
	    /* free(dir_entry->name); */
	    /* free(buf3); */
	    block_offset = old + rec_len;
	}   /* end of while loop */
    }
    return 0; 
}    


/* based from the inode_table_block_addr, read the content from inode */
dir_entry_list_t* read_inode(int inode_no, int* is_regular_file){
    /* int fd = g_info.fd; */
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
	int i ;
	/* TODO: to support multiple link inodes */
	/* read all the i_bloc's, i.e., data blocks */
	for (i=0;  inode->i_block[i] != 0 && i < I_BLOCK_MAX; i++ ){   /* i_block needs to non-zero to be valid */ 
	    unsigned int data_block = inode->i_block[i];
	    read_all_entries_in_a_block(entries, data_block);
	    /* free(buf); */
	}
    }
    return entries;
}

dir_entry_list_t* read_root_entry_list(){
    int root_inode = 2;		/* root inode is 2 */
    int is_regular_file = 0;
    return read_inode(root_inode, &is_regular_file);
    assert( is_regular_file == 0 );
}

char get_file_type_char(struct ext2_inode*);

/* print a single inode with a file name */
void print_inode(unsigned int inode_no, char* filename){
	struct ext2_inode* inode = get_inode(inode_no);
	
	/* construct permission string */
	char permissions[10] = "----------";
	permissions[0] = get_file_type_char(inode);	
	if( inode->i_mode & EXT2_S_IRUSR )
	    permissions[1] = 'r';
	if( inode->i_mode & EXT2_S_IWUSR )
	    permissions[2] = 'w';
	if( inode->i_mode & EXT2_S_IXUSR )
	    permissions[3] = 'x';
	if( inode->i_mode & EXT2_S_IRGRP )
	    permissions[4] = 'r';
	if( inode->i_mode & EXT2_S_IWGRP )
	    permissions[5] = 'w';
	if( inode->i_mode & EXT2_S_IXGRP )
	    permissions[6] = 'x';	
	if( inode->i_mode & EXT2_S_IROTH )
	    permissions[7] = 'r';
	if( inode->i_mode & EXT2_S_IWOTH )
	    permissions[8] = 'w';
	if( inode->i_mode & EXT2_S_IXOTH )
	    permissions[9] = 'x';
	
	/* get user name and group name */
	struct passwd* pwd = getpwuid(inode->i_uid);
	struct group* grp = getgrgid(inode->i_gid);
	
	printf("%s %d %s %s %d %d %s\n", 
	    permissions, inode->i_links_count, pwd->pw_name, grp->gr_name, inode->i_size, inode->i_atime, filename);
	free(inode);
	/* free(pwd); */
	/* free(grp); */
}      
	
void print_list(dir_entry_list_t* current_entry_list){

    dir_entry_list_t* tmp;
    if (current_entry_list == NULL){
	/* printf("ERROR: list is empty.\n"); */
	return;
    }

    list_for_each_entry(tmp, &current_entry_list->list, list){
#if DEBUG	       
	printf("PRINT_LIST: {inode = %u, rec_len = %u, name_len = %u, name = %s}\n",
		   tmp->e->inode, tmp->e->rec_len, tmp->e->name_len,
		   tmp->e->name);
#endif
	print_inode(tmp->e->inode, tmp->e->name);
    }
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
 
char get_file_type_char(struct ext2_inode* inode){
    char type = 'u';
    switch (get_file_type(inode)){
    case EXT2_S_IFREG:
	type = '-';
	break;
    case EXT2_S_IFDIR:
	type = 'd';
	break;
    case EXT2_S_UNKNOWN:
    default:
	type = 'u';
	break;
    }
    return type;
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


/* read a block in the disk */
char* read_block(unsigned int block_no){
    char* block = malloc(EXT2_BLOCK_SIZE);
    if ( block ){
	int offset = block_no * EXT2_BLOCK_SIZE;      
	lseek(g_info.fd, offset, SEEK_SET);
	if (read(g_info.fd, block, EXT2_BLOCK_SIZE) != EXT2_BLOCK_SIZE){ /* if not read the whole block */
	    free(block);
	    return NULL;
	}
    }
    return block;       
}

int is_zero( unsigned int bits, unsigned int pos) {
    assert ( pos >=0 && pos < 32 ); 
    
    unsigned int mask = 1 << (pos);
    if (! (mask & bits) ) 
	return 1;
    else	
	return 0; 
}    


/* TODO: assuming the unsigned int is 32-bit */
unsigned int find_free_inode(char* bitmap){
    if(bitmap){
	unsigned int found = ERR_RET; 
	int i;
	for (i = EXT2_GOOD_OLD_FIRST_INO; i < EXT2_MAX_INO; i++){
	    unsigned offset = (i / 32) * 4; /* 32 bits is 4 bytes, so the offset is multiple of 4 bytes */
	    unsigned bits = *((unsigned int*) (bitmap + offset));
	    if ( is_zero(bits, i % 32) ){
		found = i + 1; 
		break;
	    }
	}
	return found; 
    }
    return ERR_RET;
}

unsigned int find_free_data_block(char* bitmap){
    if(bitmap){
	unsigned int found = ERR_RET; 
	int i;
	for (i = 0; i < EXT2_MAX_BLOCKS; i++){
	    unsigned offset = (i / 32) * 4; /* 32 bits is 4 bytes, so the offset is multiple of 4 bytes */
	    unsigned bits = *((unsigned int*) (bitmap + offset));
	    if ( is_zero(bits, i % 32) ){
		found = i + 1; 
		break;
	    }
	}
	return found; 
    }
    return ERR_RET;
}

/* read group desc struct and allocate a new inode number */
unsigned int allocate_inode(){
    unsigned int ret = -1;
    unsigned int inode_bitmap_block = g_info.egd->bg_inode_bitmap; 
    char* bitmap = read_block(inode_bitmap_block); 
    if (bitmap){
	ret = find_free_inode(bitmap);
    }
    return ret; 
}

unsigned int allocate_data_block(){

    unsigned int ret = -1;
    unsigned int block_bitmap_block = g_info.egd->bg_block_bitmap; 
    char* bitmap = read_block(block_bitmap_block); 
    if (bitmap){
	ret = find_free_data_block(bitmap);
    }
    return ret; 
}

/* create a regular file inode. The returned inode struct is only partially initialized. 
   Some members, e.g., i_blocks, are not initialized */
struct ext2_inode* create_inode(unsigned int parent_inode){
    int i; 
    struct ext2_inode* new = (struct ext2_inode*) malloc(sizeof(struct ext2_inode));
    /* create a permission like this: -rwxr-xr-x */
    new->i_mode = 0 |  EXT2_S_IRUSR | EXT2_S_IWUSR | EXT2_S_IXUSR |  EXT2_S_IRGRP |
	 EXT2_S_IXGRP | EXT2_S_IROTH |  EXT2_S_IXOTH; 
    new->i_uid = getuid();
    new->i_gid = getgid();
    /* TODO: a new dir size == 4096? */
    new->i_size = 4096;
    /* get creation time stamp */
    struct timeval tv;
    gettimeofday(&tv,NULL);
    new->i_atime = tv.tv_sec;
    new->i_ctime = tv.tv_sec;
    new->i_mtime = tv.tv_sec;
    new->i_dtime = 0;
    new->i_links_count = 1; 
    /* TODO: how many i_blocks? */
    new->i_blocks = 2;

    /* initiate the i_blocks */
    for ( i=0; i < EXT2_IBLOCK_COUNT; i++)
	new->i_block[i] = 0 ;

    /* TODO: leave the file flags alone */
    new->i_flags = 0;
    new->i_generation = 0;
    new->i_file_acl = 0;
    new->i_dir_acl = 0;
    new->i_faddr = 0;    
    
    return new; 
}

/* create a new directory inode. The only diff. from a regular file is that 
 the i_mode is not fill */
struct ext2_inode* create_dir_inode(unsigned int parent_inode){
    struct ext2_inode* new = create_inode(parent_inode);
    new->i_mode |= EXT2_S_IFDIR; /* update imode */
    return new; 
}

/* write the inode to the disk */
int write_inode(unsigned int inode_no, struct ext2_inode* inode){
    unsigned int inode_table_offset = g_info.inode_table_address + (inode_no-1) * EXT2_INODE_SIZE;
    lseek(g_info.fd, inode_table_offset, SEEK_SET);
    int write_size = sizeof(struct ext2_inode);
    if( write(g_info.fd, inode, write_size) != write_size ){
	printf("Write inode: %d failed. \n", inode_no);
	return 1;
    }
    return 0;
}

struct ext2_dir_entry_2* create_entry(unsigned int inode, char* name){
    unsigned int name_len = strlen(name);
    unsigned short rec_len =  sizeof(struct ext2_dir_entry_2) + name_len;
    struct ext2_dir_entry_2* dirent1 = (struct ext2_dir_entry_2*) malloc(rec_len);
    dirent1->inode = inode;
    dirent1->rec_len = rec_len;
    dirent1->name_len = (char) name_len;
    strncpy (dirent1->name, name, name_len);
    (dirent1->name)[name_len] = '\0';
    return dirent1; 
}

/* write two directory entries, i.e., "." and ".." to the new data block */
int write_new_dir_data_block(unsigned int new_data_block, unsigned int new_inode_no, unsigned int parent_inode){
    assert(new_data_block > 0);
    assert(new_inode_no > 0);
    assert(parent_inode > 0);

    struct ext2_dir_entry_2* de1 = create_entry(new_inode_no, ".");
    struct ext2_dir_entry_2* de2 = create_entry(parent_inode, "..");

    /* since there are only 2 entries, we need to merge the rest of the 
       free space in the block wth the second entry */
    int real_rec_len = de2->rec_len;
    de2->rec_len = EXT2_BLOCK_SIZE - de1->rec_len; 

    /* write to the data block */
    int offset = new_data_block * EXT2_BLOCK_SIZE; 
    lseek(g_info.fd, offset, SEEK_SET);    

    int write_size = de1->rec_len; 
    if (write(g_info.fd, (char*)de1, write_size) != write_size){
	printf("Failed to write the directory entry 1\n");
	return 1; 
    }

    write_size = real_rec_len; 
    if (write(g_info.fd, (char*)de2, write_size) != write_size){
	printf("Failed to write the directory entry 2\n");
	return 1;
    }
    return 0;
}

int write_entries_to_data_block(unsigned int data_block, dir_entry_list_t* entries){
    int offset = data_block * EXT2_BLOCK_SIZE;
    seek(g_info.fd, offset, SEEK_SET);
    dir_entry_list_t* tmp;
    list_for_each_entry(tmp, &entries->list, list){
	int write_size = tmp->e->name_len + sizeof(struct ext2_dir_entry_2);
	if ( write(g_info.fd, tmp->e, write_size) != write_size ){
	    printf( "Entry write failure.\n" );
	    return 1; 
	}
    }

    return  0;
}


int update_parent_dir(unsigned int parent_inode, unsigned int leaf_node, char* name){
    int name_len = strlen(name);
    struct ext2_dir_entry_2* de = create_entry(leaf_node, name);
    struct ext2_inode* parent = get_inode(parent_inode);
    int i; 
    
    for( i=0; i < EXT2_IBLOCK_COUNT; i++ ){
	if ( parent->i_block[i] == 0 ) /* the first zero block, meaning the prev block is the last data block */
	    break;
    }
    int data_block = parent->i_block[i-1];    
    dir_entry_list_t* tmp;
    dir_entry_list_t* old_entries = NULL;

    assert ( data_block > 0 ); 
    read_all_entries_in_a_block(old_entries, data_block);
    int rec_len_total = 0;
    
    list_for_each_entry(tmp, &old_entries->list, list){
	rec_len_total += (tmp->e->name_len) + sizeof(struct ext2_dir_entry_2);
    }

    dir_entry_list_t* one_entry = (dir_entry_list_t*) malloc(sizeof(dir_entry_list_t));
    one_entry->e = de; 
    
    if ( rec_len_total + de->rec_len > EXT2_BLOCK_SIZE ){
	/* We need to allocate a new i_block */
	if ( i >= EXT2_IBLOCK_COUNT - 1 ){
	    printf("i_blocks limit reached. Can't create a new i_block\n");
	    exit(1);
	}
	/*TODO: Previously allocated inode info has not been updated yet, so this will not return the correct data block. */
	data_block = allocate_data_block();
	old_entries = (dir_entry_list_t*) malloc(sizeof(dir_entry_list_t));
	INIT_LIST_HEAD(&(entries->list));

	de->rec_len = EXT2_BLOCK_SIZE; /* this is the only entry in the block */	
    }else{			/* it reuses the old block */
	dir_entry_list_t* tail = entries->list->prev; 
	/* update the tail */
	tail->e->rec_len = tail->e->name_len + sizeof(struct ext2_dir_entry_2);
	/* the rest of the block is yours now */
	de->rec_len = EXT2_BLOCK_SIZE - rec_len_total; 
    }
    /* add the new entry to list */
    list_add_tail(&(entries->list), &(one_entry->list));
    write_entries_to_data_block(data_block, old_entries); 		

    return 0;
}

#endif
