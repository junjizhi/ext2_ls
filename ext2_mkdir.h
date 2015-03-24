
/* ext2_mkdir: This program takes two command line arguments. The first is the name of an ext2 formatted virtual disk. The second is an absolute path on your ext2 formatted disk. The program should work like mkdir, creating the final directory on the specified path on the disk. If any component on the path to the location where the final directory is to be created does not exist or if the specified directory already exists, then your program should return the appropriate error (ENOENT or EEXIST). */

#include "ext2_ls.h"

void ext2_mkdir(char* path){
    int n = -1;
    int found_inode = -1;
    int parent_inode = 0; 
    dir_entry_list_t* parent_entry_list = NULL;

     /* read root dir */
    dir_entry_list_t*  current_entry_list = read_root_entry_list();

    /* split the path into n splits */
    char** splits = str_split(path, '/', &n);
    int parent_path_exist = -1;

    /* loop through 0 to n-2 of splits and make sure the parent inode exists */
    for ( i=0; i < n-1; i++ ){

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

	if ( found_inode == -1 ){
	    if(i != (n-1)){	/* not reaching the end. Some parent directories  */
		parent_path_exist = 0;
	    /* TODO: free resources, too */
		current_entry_list = NULL;
	    }
	    else
		parent_path_exist = 1;
	    break;
	}else{ 			/* if found */
	    if ( i == (n-2) ){	/* the intermediat parent directory */
		parent_inode = found_inode;
		parent_entry_list = current_entry_list;
	    }
	}
    }

    if ( found_inode == -1 && parent_path_exist ){	/* not found the inode */
	/* create a new inode structure */
	struct ext2_inode* new_inode = (struct ext2_inode*) malloc(sizeof(struct ext2_inode));
	assert (parent_entry_list);
	assert (parent_inode);
	
	/* 2 things to determine: 
	   A. inode number of the new inode, determined by reading the inode bitmap block from group desc
	   B. data block no. (in which to store '.' and '..' entries, by reading the data bitmap block of the group descriptor */
	unsigned int new_inode_no = allocate_inode(); 
	unsigned int new_data_block = allocate_data_block();
	/* write the new inode structure and data blocks into the disk */
	/* update superblock, free blocks and inodes count
	   update group descriptor: two bitmaps, free blocks and inodes count, diretory count */    	        
    }else{

	if (found_inode != -1){	/* found the inode */
	    printf("mkdir: cannot create directory ‘%s’: File exists\n", path);
	    exit(0);
	}

	if ( !parent_path_exist ){	    	   
	    printf("mkdir: cannot create directory ‘%s’: No such file or directory\n", path);
	    exit(0);
	}
    }
    
}
