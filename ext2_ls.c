/* From http://www.cdf.toronto.edu/~csc369h/winter/assignments/a3/a3.html:

ext2_ls: This program takes two command line arguments. The first is the name of an ext2 formatted virtual disk. The second is an absolute path on the ext2 formatted disk. The program should work like ls -1, printing each directory entry on a separate line. Unlike ls -1, it should also print . and ... In other words, it will print one line for every directory entry in the directory specified by the absolute path. If the directory does not exist print "No such file or diretory". */

#include "ext2.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "ext2_ls.h"

int main(int argc, char* argv[]){    
    _init_ext2(argv[1]);
    ls(argv[2]);
    _exit_ext2();
}
