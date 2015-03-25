
#include "ext2_comm.h"

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
