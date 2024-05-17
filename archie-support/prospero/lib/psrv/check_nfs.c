/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>
#include <sys/param.h>
#include <netdb.h>
#include <string.h>

#include <pfs.h>
#include <pmachine.h>

extern char	*hostname;

/*
 * check_nfs - Check whether file is available by NFS
 *
 * 	  CHECK_NFS takes the name of a file and a network address.
 *        It returns the name of the filesystem (prefix of the file)
 *        which may be exported by NFS to the client.  The prefix is
 *        followed by a space and the suffix.  This is the form of the 
 *        paramters to be returned for access by NFS.
 *
 *    ARGS: path    - Name of file to be retrieved
 *          client  - IP address of the client
 *
 * RETURNS: A newly allocated sequence containing NFS access method for that
 *          file, or NULL if the file is not available to the client by NFS.
 *
 *   NOTES: The returned memory must be freed by the caller.
 *
 *    BUGS: The procedure should check the NFS exports file.  Right now
 *          it only guesses at the prefix and check to make sure the
 *          request is from the local subnet.  It also incorrectly assumes that
 *          the file must be exported by the local host.  Ugh.
 */
#ifdef PFS_THREADS
extern p_th_mutex p_th_mutexPSRV_CHECK_NFS_MYADDR;
#endif

TOKEN
check_nfs(char *path, long client)
{
    TOKEN               nfs_am = NULL;
    static long	myaddr = 0;     /* MUTEXED below */
    char		prefix[MAXPATHLEN];
    char		*suffix;
    char		*slash;

    /* First time called, find out hostname and remember it */
    if(!myaddr) {
#ifdef PFS_THREADS
        p_th_mutex_lock(p_th_mutexPSRV_CHECK_NFS_MYADDR);
        if (!myaddr) {           /* check again */
#endif
            myaddr = myaddress();
#ifdef PFS_THREADS
        }
        p_th_mutex_unlock(p_th_mutexPSRV_CHECK_NFS_MYADDR);
#endif
    }

#if BYTE_ORDER == BIG_ENDIAN
    if((myaddr ^ client) & 0xffffff00) return(NULL);
#else
    if((myaddr ^ client) & 0x00ffffff) return(NULL);
#endif

    strcpy(prefix,path);
    slash = strchr(prefix+1,'/');
    if(slash) {*slash = '\0'; suffix = slash + 1;}
    else return(NULL);
    nfs_am = tkappend("NFS", nfs_am);
    nfs_am = tkappend("INTERNET-D", nfs_am);
    nfs_am = tkappend(hostname, nfs_am);
    nfs_am = tkappend("ASCII", nfs_am);
    nfs_am = tkappend(suffix, nfs_am);
    nfs_am = tkappend(prefix, nfs_am);
    return nfs_am;
}
