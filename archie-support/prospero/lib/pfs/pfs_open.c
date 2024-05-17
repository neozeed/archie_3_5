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
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>

/* needed for SCO Unix*/
#include <fcntl.h>

#include <pfs.h>
#include <pcompat.h>
#include <perrno.h>
#include <pmachine.h>

/*SEE: pfs_fopen if you want a FILE rather than fd returned */

int
pfs_open(VLINK vl,int flags)
{
    char		npath[MAXPATHLEN];
    int		tmp;
    int		mapflags;
    int		open_return;

    /* Test flags for validity. */
    if (flags & (O_EXCL | O_SYNC)) {
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "Bad flags specified to pfs_open().");
        return -1;
    }
    if((flags & (O_ACCMODE)) == O_RDONLY) mapflags = MAP_READONLY;
    else mapflags = MAP_READWRITE;

    tmp = mapname(vl, npath, sizeof npath, mapflags);

    if(tmp && (tmp != PMC_DELETE_ON_CLOSE)) {
        errno = ENOENT;
        return(-1);
    }

    DISABLE_PFS(open_return = open(npath, flags));
    if(tmp == PMC_DELETE_ON_CLOSE) unlink(npath);

    return(open_return);
}

