
#include "ext2_mkdir.h"

int 
main(int argc, char** argv){

    _init_ext2(argv[1]);
    ext2_mkdir(argv[2]);
    _exit_ext2();

    return 0;
}
