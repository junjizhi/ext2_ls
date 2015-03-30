
#include "ext2_comm.h"

void ext2_mkdir(char* path){
    const int NOT_FOUND = -1; 
    int n = -1;
    int is_regular_file;
    int leaf_node = NOT_FOUND;
    
    int i; 

     /* read root dir */
    dir_entry_list_t* current_entry_list = read_root_entry_list();
    dir_entry_list_t* prev = current_entry_list; 
    dir_entry_list_t* parent_entry_list = current_entry_list; 
    int parent_inode = 2; 	/* start from root inode */

    /* split the path into n splits */
    char** splits = str_split(path, '/', &n);
    int parent_path_exist = -1;

    /* loop through 0 to n-2 of splits and make sure the parent inode exists */
    for ( i=0; i < n; i++ ){
	int found_inode = NOT_FOUND;
	int is_dir = -1; 
	dir_entry_list_t* tmp; 

	list_for_each_entry(tmp, &current_entry_list->list, list){
	    if ( strcmp(tmp->e->name, splits[i]) == 0 ){
		found_inode = tmp->e->inode;
		is_regular_file = 0 ;
		/* found, then update the current entry list*/
		prev = current_entry_list; 
		current_entry_list = read_inode(found_inode, &is_regular_file);	     
		if( current_entry_list == NULL ){		    
		    assert(is_regular_file); /* invalid combination */
		}
		break;		/* exit the entry list loop */
	    }
	}
	
	/* if the path is found */
	if (found_inode != NOT_FOUND && i == (n-1) ) {
	    leaf_node = found_inode; 
	    break;		/* break the for loop */
	}

	/* if path does not exists, and it is not the end of list yet  */
	if (found_inode == NOT_FOUND && i != (n-1) ) {
	    parent_path_exist = 0;
	    break;
	}

	/* if path is found, but one of its dir is a file */
	/* TODO: maybe there are better logic here */
	if (found_inode != NOT_FOUND && is_regular_file ){
	    parent_path_exist = 0;
	    break;
	}

	/* if path does not exists, and it is the end of list, then it means
	 it is okay to create a new inode*/
	if (found_inode == NOT_FOUND && i == (n-1) ) {
	    leaf_node = NOT_FOUND;
	    parent_path_exist = 1; 
	    parent_entry_list = prev; 
	    break;
	}
	
	/* when the code reaches here, then it is safe to update parent inode */
	if (found_inode != NOT_FOUND)
	    parent_inode = found_inode; 
    }

    if ( leaf_node == NOT_FOUND && parent_path_exist ){	/* not found the inode */
	assert (parent_entry_list);
	assert (parent_inode != NOT_FOUND);
	
	/* 2 things to determine: 
	   A. inode number of the new inode, determined by reading the inode bitmap block from group desc
	   B. data block no. (in which to store '.' and '..' entries, by reading the data bitmap block of the group descriptor */
	unsigned int new_inode_no = allocate_inode(); 
	unsigned int new_data_block = allocate_data_block();

	/* create a new inode structure */
	struct ext2_inode* new_inode = create_dir_inode(parent_inode);
	new_inode-> i_block[0] = new_data_block;

#if DEBUG
	printf("new inode no.  %d\n", new_inode_no);
	printf("new data block no.  %d\n", new_data_block);
#endif
	/* write the new inode structure and data blocks into the disk */
	write_inode(new_inode_no, new_inode); 
        write_new_dir_data_block(new_data_block, new_inode_no, parent_inode);

	/* update parent inode */
	update_parent_dir(parent_inode, new_inode_no, splits[n-1]);

	/* TODO: update superblock, free blocks and inodes count
	   update group descriptor: two bitmaps, free blocks and inodes count, diretory count */  
	persist_metadata();
	
    }else{

	if (leaf_node != NOT_FOUND){	/* found the inode */
	    printf("ext2_mkdir: cannot create directory ‘%s’: File exists\n", path);
	    exit(0);
	}

	if ( !parent_path_exist ){	    	   
	    printf("ext2_mkdir: cannot create directory ‘%s’: No such file or directory\n", path);
	    exit(0);
	}
    }
    
}
