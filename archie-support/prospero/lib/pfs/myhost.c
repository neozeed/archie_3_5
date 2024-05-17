/*
 * Copyright (c) 1993,1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 *
 * Original author: BCN (washington)  (1989, 1991).
 * Hacked by SWA.
 */

#include <usc-license.h>
#include <pcompat.h>            /* for DISABLE_PFS() */

#include <sys/param.h>
#include <netdb.h>
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 255      /* limit in 4.2BSD manual pages. */
#endif
#include <pfs.h>         /* for definitions of myhostname and myaddress.*/ 

/* These are mutexed below. */
static char	myhname[MAXHOSTNAMELEN + 1];
static long	myhaddr = 0L;

char 		*myhostname();

/* The auto-initialization mutexing here works, but is not used. */
/* If you turn it on, you'll have to change lib/pfs/pfs_mutexes.c and
   include/pfs.h to add p_th_mutexPFS_MYHOSTNAME. */
#undef P__MYHOST_MUTEX_AUTOINITIALIZATION

long 
myaddress(void)
{
    /* First time called, make sure myhostname has been run */
    if(!myhaddr) {
#ifndef P__MYHOST_MUTEX_AUTOINITIALIZATION
        /* myaddress() should be called as part of any general
           library initialization when running multi-threaded. */
        assert(P_IS_THIS_THREAD_MASTER());
#endif
        myhostname();
    }
    return(myhaddr);
}

/* myhostname() always returns the official name of the host (see comment below
   for an exception to this claim; do not rely on it. 
   This might be different from the version returned by gethostname(), and
   might well be different from the version stored in the 'hstname' environment
   variable defined in dirsrv.c */
   
/* This is normally called by dirsrv in p_init_mutexes() so that it
   will not fail in a multi-threaded environment.  */


/*
 * Implementation and Specification Comments:
 * From the SunOS 4.1.3 manual page for "gethostbyname(3)":
 * "The members of this structure are: 
    h_name              Official name of the host. [ ... ]"
 * 
 * Under Solaris 2.3, Mitra reports that a short name that is not the fully
 * qualified domain name is being returned in the h_name member.  This makes
 * myhostname() return a name that is not the official name of the host.
 *
 *
 * Problems if we don't get the fully qualified domain name (swa went through
 * all the existing code calling this function to check and make sure no 
 * problems would arise):
 * dirsrv.c: no problem; envar overrides #define overrides myhostname()
 * ftp.c (in vcache): used to contstruct password for anonymous FTP.  
 * So: no problem.
 * Not used elsewhere in the existing code; future code will be aware of the
 * potential problem.
 */
 

char *
myhostname()
{
    static int	initialized = 0;

    /* First time called, find out hostname and remember it */
    if(!initialized) {
        struct hostent	*current_host;
        
#ifdef P__MYHOST_MUTEX_AUTOINITIALIZATION
#ifdef PFS_THREADS
        p_th_mutex_lock(p_th_mutexPFS_MYHOSTNAME);
        if (!initialized) {           /* check again */
#endif
#else
            assert(P_IS_THIS_THREAD_MASTER()); /* autoinitializing */
#endif
            gethostname(myhname,sizeof(myhname));
            /* gethostbyname reads files, so we must disable pfs */
            p_th_mutex_lock(p_th_mutexGETHOSTBYNAME);
            DISABLE_PFS(current_host = gethostbyname(myhname));
            strcpy(myhname,current_host->h_name);
            /* Save the address too */
            /* Length of the address is h_length; in practice, is always 4. */
            bcopy(current_host->h_addr,&myhaddr, current_host->h_length);
            /* We're now done copying data out of the current_host structure,
               so we can safely unlock the mutex around GETHOSTBYNAME */
            p_th_mutex_unlock(p_th_mutexGETHOSTBYNAME);
            ucase(myhname);
            ++initialized;
#ifdef P__MYHOST_MUTEX_AUTOINITIALIZATION
#ifdef PFS_THREADS
            p_th_mutex_unlock(p_th_mutexPFS_MYHOSTNAME);
        }
#endif
#endif
    }
    return(myhname);
}
