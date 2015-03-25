/* ext2_mkdir: This program takes two command line arguments. The first is the name of an ext2 formatted virtual disk. The second is an absolute path on your ext2 formatted disk. The program should work like mkdir, creating the final directory on the specified path on the disk. If any component on the path to the location where the final directory is to be created does not exist or if the specified directory already exists, then your program should return the appropriate error (ENOENT or EEXIST). */

#include "ext2_mkdir.h"

int 
main(int argc, char** argv){

    _init_ext2(argv[1]);
    ext2_mkdir(argv[2]);
    _exit_ext2();

    return 0;
}
