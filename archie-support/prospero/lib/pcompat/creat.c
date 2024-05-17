/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>
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
#include <pmachine.h>


/* syscall is not MT-safe (at least under Solaris)
#ifndef PFS_THREADS
creat(const char *name, 
#ifdef OPEN_MODE_ARG_IS_INT
      int mode
#else
      mode_t mode
#endif
)
{
    char		npath[MAXPATHLEN];

    int		tmp;

    tmp = pfs_access(name, npath, sizeof npath, PFA_CRMAP);

    if(tmp) return(-1);

    return(syscall(SYS_creat,npath,mode));
}
#endif /*PFS_THREADS*/
