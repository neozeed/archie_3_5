/*
 * Copyright (c) 1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <sys/types.h>
#include <sys/stat.h>

/* Is this a file?  uses stat() */
#include <pfs.h>                /* for prototypes */

/* Is it a regular file? 1 = yes, 0 = no, -1 = failure? */
extern int
is_file(const char native_filename[])
{
    struct stat st_buf;

    if(stat(native_filename, &st_buf) == 0)
        return S_ISREG(st_buf.st_mode) ? 1 : 0;
    else
        return -1;
}
