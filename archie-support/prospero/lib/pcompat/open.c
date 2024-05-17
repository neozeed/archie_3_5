/*
 * Copyright (c) 1992,1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

/* Originally written by Cliff Neuman, 1989
   Modified by Steven Augart, 1992, 1994
*/

#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/file.h>
#include <sys/param.h>
#ifdef SOLARIS
#include <sys/syscall.h>
#else
#include <syscall.h>
#endif
#include <errno.h>

#include <pfs.h>
#include <pcompat.h>
#include <perrno.h>
#include <pmachine.h>

extern int	pfs_quiet;

/* HPUX prototype:   extern int open(const char *, int, ...); 
   SunOS prototype: int open()  */

int
#ifndef PROTOTYPE_FOR_OPEN_HAS_EMPTY_ARGLIST
open(const char *path, int flags, ...)
{
    va_list             ap;
#ifdef OPEN_MODE_ARG_IS_INT
    int                 mode;
#else
    mode_t              mode;
#endif
#else
open(path, flags, mode)
    char *path;
    int flags;
#ifdef OPEN_MODE_ARG_IS_INT
    int                 mode;
#else
    mode_t              mode;
#endif
{
#endif
    char		npath[MAXPATHLEN];

    int		pfaflags;
    int		tmp;
    int		open_return;

#ifndef PROTOTYPE_FOR_OPEN_HAS_EMPTY_ARGLIST
    va_start(ap, flags);
#ifdef OPEN_MODE_ARG_IS_INT
    mode = va_arg(ap, int);
#else
    mode = va_arg(ap, mode_t);
#endif
#endif
    if(flags & O_CREAT) pfaflags = PFA_CRMAP;
    else pfaflags = PFA_MAP;
    if((flags & (O_ACCMODE)) == O_RDONLY) pfaflags |= PFA_RO;

    tmp = pfs_access(path,npath, sizeof npath, pfaflags);

    if(tmp && (tmp != PMC_DELETE_ON_CLOSE)) {
        if(!pfs_quiet) printf("open failed: %s\n",p_err_text[tmp]);
        errno = ENOENT;
        return(-1);
    }

    open_return = syscall(SYS_open,npath,flags,mode);
    if(tmp == PMC_DELETE_ON_CLOSE) unlink(npath);

    return(open_return);
}
