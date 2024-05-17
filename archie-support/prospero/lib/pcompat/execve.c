/*
 * Copyright (c) 1992, 1994 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <usc-license.h>
 */

#include <usc-license.h>

#include <sys/param.h>
#ifdef SOLARIS
#include <sys/syscall.h>
#else
#include <syscall.h>
#endif
#include <errno.h>


#include <pfs.h>
#include <pcompat.h>


/* syscall is not MT-safe (at least under Solaris) */
#ifndef PFS_THREADS
int
execve(const char *name, char * const*argv, char * const *envp)
{
    char		npath[MAXPATHLEN];

    int		tmp;

    tmp = pfs_access(name, npath, sizeof npath, PFA_MAP);

    /* Should figure out what correct error return should be */
    if(tmp) return(-1);

    return(syscall(SYS_execve,npath,argv,envp));
}
#endif /*PFS_THREADS*/
