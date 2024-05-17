/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <string.h>
#include <netdb.h>

#include <pfs.h>
#include <psite.h>
#include <pcompat.h>
#include <perrno.h>
#include <pmachine.h>

/*
 * see pcompat.h for the meanings of the flags
 */
/* This function looks a heck of a lot like retrieve_link() in user/vget.c.  If
   there's a bug here, there's one there too. */
/* This can be easily changed to use the stcopyr() interface, and should be at
   some point in the future. */
int
mapname(vl,npath, npathlen, flags)
    VLINK	vl;
    char	*npath;         /* local pathname for file you can work with.
                                   This is filled in by mapname(). */ 
    int         npathlen;
    int		flags;
{

    TOKEN           am_args; /* filled in by pget_am */
    int		am;
    int		tmp;
    int		methods = P_AM_LOCAL; /* local filenames always are supported.
                                         */ 

#ifdef P_NFS
    methods |= P_AM_NFS;
#endif P_NFS

#ifdef P_AFS
    methods |= P_AM_AFS;
#endif P_AFS


    /* P_AM_FTP requires prompting for a password, which is not something
       transparent to the user.  That's why the programmer must explicitly
       specify the MAP_PROMPT_OK flag. */
    if ((flags | MAP_READONLY) && (flags | MAP_PROMPT_OK)) methods |= P_AM_FTP;
    if(flags | MAP_READONLY) 
        methods |= P_AM_AFTP | P_AM_GOPHER | P_AM_RCP \
		| P_AM_PROSPERO_CONTENTS | P_AM_WAIS;

    am = pget_am(vl,&am_args,methods);

    if(!am && perrno) {
        return(perrno);
    }

    switch(am) {

#ifdef P_NFS
    case P_AM_NFS: 
        /* XXX You must change pmap_nfs() to meet the needs of your site. */
        tmp = pmap_nfs(vl->host,vl->hsoname,npath, npathlen, am_args);
        return(tmp);
#endif P_NFS

#ifdef P_AFS
    case P_AM_AFS: 
        strcpy(npath,P_AFS);
        strcat(npath,elt(am_args,4)); /* 4th element is the hsoname. */
        return(PSUCCESS);
#endif P_AFS

    case P_AM_AFTP: 
    case P_AM_FTP: 
    case P_AM_GOPHER: 
    case P_AM_WAIS:
    case P_AM_PROSPERO_CONTENTS:

        tmp = p__map_cache(vl, npath, npathlen, am_args);
        return(tmp);

    case P_AM_LOCAL:
        strcpy(npath, elt(am_args, 4));
        return PSUCCESS;

    default:
        return(PFSA_AM_NOT_SUPPORTED);
    }
}


