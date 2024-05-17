/*
 * Derived from Berkeley source code.  Those parts are
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)opendir.c	5.10 (Berkeley) 6/1/90";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <fcntl.h>
#include <errno.h>

#include <pmachine.h>

#ifdef USE_SYS_DIR_H
#include <sys/dir.h>
#else
#include <dirent.h>
#endif
#if defined(AIX) && defined(__GNUC__) /* lucb */
#define dd_bsize dd_size
#endif

#ifndef DIRBLKSIZ
#define DIRBLKSIZ 512
#endif

#include <stdlib.h>              /* For malloc */
long _rewinddir;

/*
 * open a directory.
 */
DIR *
opendir(const char *name)
{
    register DIR *dirp;
    register int fd;

    fd = p__readvdirentries(name);

    if(fd > 0) {
        errno = ENOENT;
        return(NULL);
    }

    if(fd == 0) {
        if ((fd = open(name, 0, 0)) == -1)
            return NULL;
        if (fcntl(fd, F_SETFD, 1) == -1) {
            close (fd);
            return(NULL);
        }
    }

    if((dirp = (DIR *)malloc(sizeof(DIR))) == NULL) return(NULL);

    /*
     * If CLSIZE is an exact multiple of DIRBLKSIZ, use a CLSIZE
     * buffer that it cluster boundary aligned.
     * Hopefully this can be a big win someday by allowing page trades
     * to user space to be done by getdirentries()
     */
#ifdef CLSIZE
    if ((CLSIZE % DIRBLKSIZ) == 0) {
        dirp->dd_buf = malloc(CLSIZE);
        dirp->dd_bsize = CLSIZE;
    } else {
#endif
        dirp->dd_buf = malloc(DIRBLKSIZ);
        dirp->dd_bsize = DIRBLKSIZ;
#ifdef CLSIZE
    }
#endif
    if (dirp->dd_buf == NULL) {
        close (fd);
        return NULL;
    }
    dirp->dd_fd = fd;
    dirp->dd_loc = 0;
#ifdef SUNOS     /* lucb */    
    dirp->dd_bbase = 0;
#endif    
    /*
     * Set up seek point for rewinddir.
     */
    _rewinddir = telldir(dirp);
    return dirp;
}
