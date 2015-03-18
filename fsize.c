#include <stdio.h>
 #include <string.h>
 /* #include "syscalls.h" */
#include <unistd.h>
#include <fcntl.h> /* flags for read and write */
#include <sys/types.h> /* typedefs */
#include <sys/stat.h> /* structure returned by stat */
#include <stdlib.h>
 
#define NAME_MAX 14 /* longest filename component; */
/* system-dependent */
typedef struct { /* portable directory entry */
    long ino; /* inode number */
    char name[NAME_MAX+1]; /* name + '\0' terminator */
} Dirent;
typedef struct { /* minimal DIR: no buffering, etc. */
    int fd; /* file descriptor for the directory */
    Dirent d; /* the directory entry */
} DIR2;
DIR2 *opendir2(char *dirname);
Dirent *readdir2(DIR2 *dfd);
void closedir2(DIR2 *dfd);

/* stat(name, &stbuf); */


char *name;
struct stat stbuf;
int stat2(char *, struct stat *);

/* #include "dirent.h" */
void fsize(char *);

/* int stat(char *, struct stat *); */
void dirwalk(char *, void (*fcn)(char *));
/* fsize: print the name of file "name" */
void fsize(char *name)
{
    struct stat stbuf;
    if (stat(name, &stbuf) == -1) {
	fprintf(stderr, "fsize: can't access %s\n", name);
	return;
    }
    if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
	dirwalk(name, fsize);
    printf("%8ld %s\n", stbuf.st_size, name);
}

#define MAX_PATH 1024
/* dirwalk: apply fcn to all files in dir */
void dirwalk(char *dir, void (*fcn)(char *))
{
    char name[MAX_PATH];
    Dirent *dp;
    DIR2 *dfd;

    if ((dfd = opendir2(dir)) == NULL) {
	fprintf(stderr, "dirwalk: can't open %s\n", dir);
	return;
    }
 while ((dp = readdir2(dfd)) != NULL) {
     if (strcmp(dp->name, ".") == 0
	 || strcmp(dp->name, ".."))
	 continue; /* skip self and parent */
     if (strlen(dir)+strlen(dp->name)+2 > sizeof(name))
	 fprintf(stderr, "dirwalk: name %s %s too long\n",
		 dir, dp->name);
     else {
	 sprintf(name, "%s/%s", dir, dp->name);
	 (*fcn)(name);
     }
 }
 closedir2(dfd);
}

#ifndef DIR2SIZ
 #define DIR2SIZ 14
 #endif
struct direct { /* directory entry */
    ino_t d_ino; /* inode number */
    char d_name[DIR2SIZ]; /* long name does not have '\0' */
};

int fstat(int fd, struct stat *);
/* opendir2: open a directory for readdir2 calls */
DIR2 *opendir2(char *dirname)
{
    int fd;
    struct stat stbuf;
    DIR2 *dp;
    if ((fd = open(dirname, O_RDONLY, 0)) == -1
	|| fstat(fd, &stbuf) == -1
	|| (stbuf.st_mode & S_IFMT) != S_IFDIR
	|| (dp = (DIR2 *) malloc(sizeof(DIR2))) == NULL)
	return NULL;
    dp->fd = fd;
    return dp;
}

/* closedir2: close directory opened by opendir2 */
void closedir2(DIR2 *dp)
{
    if (dp) {
	close(dp->fd);
	free(dp);
    }
}

#include <sys/dir.h> /* local directory structure */
/* readdir2: read directory entries in sequence */
Dirent *readdir2(DIR2 *dp)
{
    struct direct dirbuf; /* local directory structure */
    static Dirent d; /* return: portable structure */
    while (read(dp->fd, (char *) &dirbuf, sizeof(dirbuf))
	   == sizeof(dirbuf)) {
	if (dirbuf.d_ino == 0) /* slot not in use */
	    continue;
	d.ino = dirbuf.d_ino;
	strncpy(d.name, dirbuf.d_name, DIR2SIZ);
	d.name[DIR2SIZ] = '\0'; /* ensure termination */
	return &d;
    }
    return NULL;
}

/* print file name */
int main(int argc, char **argv)
{
    if (argc == 1) /* default: current directory */
	fsize(".");
    else
	while (--argc > 0)
	    fsize(*++argv);
    return 0;
}
