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
#include <pwd.h>
#include <grp.h>
#include "list.h"
#include "utils.h"

#define SUPERBLOCK_OFFSET 1024
#define EXT2_INODE_SIZE sizeof(struct ext2_inode)
#define I_BLOCK_MAX 13
#define IMODE_TYPE_MASK 0xF000
#define MAX_REC_LEN 980

#define DEBUG 0

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
    free(g_info.egd);
    free(g_info.sb);
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
	int i ;
	/* TODO: to support multiple link inodes */
	/* read all the i_bloc's, i.e., data blocks */
	for (i=0;  inode->i_block[i] != 0 && i < I_BLOCK_MAX; i++ ){   /* i_block needs to non-zero to be valid */ 
	    unsigned int data_block = inode->i_block[i];
	    unsigned int offset = data_block * EXT2_BLOCK_SIZE; 
	    lseek(fd, offset, SEEK_SET);

	    char* block = malloc( EXT2_BLOCK_SIZE );
	    /* read a whole block */
	    if( read(fd, block, EXT2_BLOCK_SIZE) == EXT2_BLOCK_SIZE ){
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

void ext2_ls(char* path){
    int i, n = -1;
    int ret = -1;
    int is_regular_file = 0 ;
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
	    if ( strcmp(tmp->e->name, splits[i]) == 0 ){
		found_inode = tmp->e->inode;
		is_regular_file = 0 ;
		/* found, then update the current entry list*/
		dir_entry_list_t* tmp2 = read_inode(found_inode, &is_regular_file);	     
	    /* if it is a regular file, then */
		if( tmp == NULL ){		    
		    assert(is_regular_file); /* invalid combination */
		    /* found_inode = -1; */
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

    if (ret != -1){	/* found */
	if (!is_regular_file)
	    print_list(current_entry_list);	
	else
	    print_inode(ret, splits[n-1]);
    }else
	printf("No such file or diretory\n"); 
    
    return; 
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
    
    unsigned int mask = 1 << (32 - pos - 1);
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
