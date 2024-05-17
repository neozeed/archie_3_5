/*
 * Copyright (c) 1991-1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <errno.h>

#include <pfs.h>
#include <pcompat.h>
#include <perrno.h>
#include <pmachine.h>

/*SEE: pfs_open if you want a fd instead of a file returned */

FILE *pfs_fopen(VLINK vl, const char *type)
{
    char		npath[MAXPATHLEN];
    int		tmp;
    int		mapflags;
    FILE		*fopen_return;

    if(strcmp(type,"r") == 0)  mapflags = MAP_READONLY;
    else mapflags = MAP_READWRITE;

    tmp = mapname(vl, npath, sizeof npath, mapflags);

    if(tmp && (tmp != PMC_DELETE_ON_CLOSE)) {
        errno = ENOENT;
        return(NULL);
    }

    DISABLE_PFS(fopen_return = fopen(npath,type));
    if(tmp == PMC_DELETE_ON_CLOSE) unlink(npath);

    return(fopen_return);
}

