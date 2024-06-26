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
static char sccsid[] = "@(#)readdir.c	5.7 (Berkeley) 6/1/90";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <pmachine.h>

#ifdef USE_SYS_DIR_H
#include <sys/dir.h>
#else
#include <dirent.h>
#endif

/*
 * get next entry in a directory.
 */
struct dirent *
readdir(dirp)
	register DIR *dirp;
{
	register struct dirent *dp;

	for (;;) {
		if (dirp->dd_loc == 0) {
		    if(dirp->dd_fd >= 0)
#if defined(SUNOS)
			dirp->dd_size = getdents(dirp->dd_fd,
 			    dirp->dd_buf, dirp->dd_bsize);
#else
			dirp->dd_size = getdirentries(dirp->dd_fd,
			    dirp->dd_buf, dirp->dd_bsize, &dirp->dd_bbase);
#endif
		    else
			dirp->dd_size = p__getvdirentries(dirp->dd_fd,
			    dirp->dd_buf, dirp->dd_bsize, &dirp->dd_bbase);
		    if (dirp->dd_size <= 0)
			return NULL;
		}
		if (dirp->dd_loc >= dirp->dd_size) {
			dirp->dd_loc = 0;
			continue;
		}
		dp = (struct dirent *)(dirp->dd_buf + dirp->dd_loc);
		if ((int)dp & 03)	/* bogus pointer check */
			return NULL;
#if defined(SUNOS)
		dirp->dd_bbase = dp->d_off;
#endif
		if (dp->d_reclen <= 0 ||
		    dp->d_reclen > dirp->dd_bsize + 1 - dirp->dd_loc)
			return NULL;
		dirp->dd_loc += dp->d_reclen;
		if (dp->d_ino == 0)
			continue;
		return (dp);
	}
}
