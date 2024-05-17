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
static char sccsid[] = "@(#)closedir.c	5.8 (Berkeley) 6/1/90";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <errno.h>

#include <pmachine.h>

#ifdef USE_SYS_DIR_H
#include <sys/dir.h>
#else
#include <dirent.h>
#endif

/*
 * close a directory.
 */
#ifdef CLOSEDIR_RET_TYPE_VOID
void 
#else
int
#endif
closedir(dirp)
	register DIR *dirp;
{
	int fd;

	fd = dirp->dd_fd;
	dirp->dd_fd = -1;
	dirp->dd_loc = 0;
	(void)free((void *)dirp->dd_buf);
	(void)free((void *)dirp);
	if(fd < 0) {
	    if(p__delvdirentries(fd)) {
		errno = EBADF;
#ifdef CLOSEDIR_RET_TYPE_VOID
		return;
#else
		return(-1);
#endif
	    }
#ifdef CLOSEDIR_RET_TYPE_VOID
	    return;
#else
	    return(0);
#endif
	}
#ifdef CLOSEDIR_RET_TYPE_VOID
	else return;
#else
	else return(close(fd));
#endif
}
