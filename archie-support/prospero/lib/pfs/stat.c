/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
/* Stuff that handles stat, scrounged from other places */

#include <sys/types.h>
#include <sys/stat.h>

#include <time.h>

#include <pfs.h>                /* for prototypes for these functions */

/* Stat and get a modification time.  Returns 0 upon failure. */
extern time_t
mtime(const char native_dirname[])
{
    struct stat st_buf;

    if(stat(native_dirname, &st_buf) == 0)
        return st_buf.st_mtime;
    else
        return 0;
}

/* Is it a directory? 1 = yes, 0 = no, -1 = failure? */
extern int
is_dir(const char native_filename[])
{
    struct stat st_buf;

    if(stat(native_filename, &st_buf) == 0)
        return S_ISDIR(st_buf.st_mode) ? 1 : 0;
    else
        return -1;
}

/* Return age of file in seconds -1 on failure*/
int 
stat_age(const char *path) {
	time_t	then;

	return ((then = mtime(path)) ? time((time_t *) 0)-then : -1);
	
}
