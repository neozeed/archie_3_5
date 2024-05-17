/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
#include <ardp.h>
#include <perrno.h>

#include <sys/types.h>          /* for gethostbyname() */
#include <sys/socket.h>         /* for gethostbyname */
#include <netdb.h>              /* for gethostbyname. */
#include <netinet/in.h>         /* for struct sockaddr_in */
#include <pmachine.h>		/* for bzero */
#include <string_with_strcasecmp.h>		/* for strcasecmp */

#ifdef PROSPERO
#include <pserver.h>                /* For DNSCACHE_MAX */
#ifdef DNSCACHE_MAX
#include "dnscache_alloc.h"
#endif
#include <pcompat.h>
#else /* not PROSPERO */
#define DISABLE_PFS_START()
#define DISABLE_PFS_END()
#endif /* not PROSPERO */

extern int stcaseequal(const char *s1,const char *s2);

/*
 * This function serves as a thread-safe version of gethostbyname(),
 * which is not a re-entrant function since it uses static data.
 * 
 * It normally accepts a hostname and initializes the socket address
 * appropriately, then returns PSUCCESS.  Also accepts numeric addresses.
 * Does not currently handle the port #, although that would be a useful future
 * extension. 
 *
 * It returns ARDP_BAD_HOSTNAME if the hostname could not be resolved.
 * gethostbyname should not be called anywhere else in multi-threaded versions
 * of the Prospero code; to this end, an error define occurs for it in 
 * pfs_threads.h
 * 
 * Oh no there isn't a definition in pfs_threads.h   - Mitra
 *
 * Note that gethostbyname() is not multi-threaded internally, and
 * does block, so we might block on name resolution as a bottleneck.  
 * Probably want a multithreaded name resolver library.  Release such a thing
 * as freeware? 
 *
 * It also converts numeric addresses appropriately.
 */
/* If change this - uncomment/comment initialization in server/dirsrv.c */

#ifdef DNSCACHE_MAX
#include "dnscache_alloc.h"
#include <mitra_macros.h>		/* FIND_FNCTN_LIST */
#include <string.h>			/* For strcmp */

DNSCACHE	alldnscaches = NULL;
int           alldnscache_count = 0;

void
sockaddr_copy(struct sockaddr_in *src, struct sockaddr_in *destn)
{
	/* Nothing in a sockaddr_in is a pointer */
	memcpy(destn, src, sizeof(struct sockaddr_in));
}
#endif /*DNSCACHE_MAX*/

/* Caching has been added, to this - take care that it does what you
   want, I'm certainly open to changes if this isnt what we need.
   Currently it is called by something at a higher layer, with a 
   hostname of the name to cache, and a hostaddr of NULL.
   Currently this is used to cache all the hostnames hard coded
   into include/* on the assumption that these are unlikely to move around
   while a server is running.   Later, we may want to delete a cached
   entry periodically, or if it fails. 

   The first thing in alldnscaches is always a copy of the last thing
   found, to allow really quick returns on repeat requests.

   Note - great care is taken here to 
   a) avoid deadlock between GETHOSTBYNAME and DNSCACHE mutexes
   b) avoid requirement for GETHOSTBYNAME mutex, if found in cache 
   This allows multiple cached results to be returned while a 
   single thread calls gethostbyname
*/
  
void
ardp_hostname2addr_initcache()
{
}

static void
dnscache_clean()
{
      DNSCACHE    dc, nextdc;
       if (alldnscache_count > DNSCACHE_MAX) {
    if (! p_th_mutex_trylock(p_th_mutexALLDNSCACHE)) {
      /* Since this is only optimisation, skip if its locked already */
	 for (dc = alldnscaches; dc ; dc = nextdc) {
	   nextdc = dc->next;
	   if (!(--dc->usecount)) {
	     EXTRACT_ITEM(dc,alldnscaches);
	     dnscache_free(dc);
	     alldnscache_count--;
	   }
	 }
	 p_th_mutex_unlock(p_th_mutexALLDNSCACHE);
    }
  }
}

int
ardp_hostname2addr(const char *hostname, struct sockaddr_in *hostaddr)
{
    struct hostent *hp;		/* Remote host we're connecting to. */
    int      retval;		/* Value to return */
#define RETURN(rv) { retval = (rv); goto cleanup; }
#ifdef PROSPERO
    int DpfStmp;                /* for DISABLE_PFS_START() */
#endif
#ifdef DNSCACHE_MAX
	DNSCACHE	acache = NULL;

	acache = alldnscaches;
	
	p_th_mutex_lock(p_th_mutexALLDNSCACHE);
	/* Cant use TH_FIND_FNCTN_LIST because must retain lock */
	FIND_FNCTN_LIST(acache, name, hostname, stcaseequal);
	if(acache)  {
	  acache->usecount++;
	    if (hostaddr) {
			/* Note is is pointless, but not harmfull to call again
		   	for same hostaddr - may just want to rerun*/
			sockaddr_copy(&(acache->sockad),hostaddr);
	    }
	    p_th_mutex_unlock(p_th_mutexALLDNSCACHE); /* Also released below*/
	    return(ARDP_SUCCESS); /* Dont free acache */
	}
	p_th_mutex_unlock(p_th_mutexALLDNSCACHE); /* Note also released above*/
		acache = dnscache_alloc(); /*locks DNSCACHE temporarily*/
		acache->name = stcopy(hostname);
    if  (!hostaddr) { hostaddr = &(acache->sockad); }
#endif /*DNSCACHE_MAX*/

    DISABLE_PFS_START();        /* Turn off compatibility library if on */
    p_th_mutex_lock(p_th_mutexGETHOSTBYNAME);
    hp = gethostbyname((char *) hostname); /* cast to char * in case bad prototype.
                                          */ 
    DISABLE_PFS_END();          /* Restore compat. lib. */
    if (hp == NULL) {
        p_th_mutex_unlock(p_th_mutexGETHOSTBYNAME);
        /* Try to see if it might be a numeric address.  */
        /* Check if a numeric address */
        hostaddr->sin_family = AF_INET;
        hostaddr->sin_addr.s_addr = inet_addr(hostname);
        if(hostaddr->sin_addr.s_addr == -1) {
            p_clear_errors();       /* clear p_err_string if set. */
            RETURN(perrno = ARDP_BAD_HOSTNAME);
        }
	RETURN(ARDP_SUCCESS);
    }
    bzero((char *) hostaddr, sizeof *hostaddr);
    memcpy((char *)&hostaddr->sin_addr, hp->h_addr, hp->h_length);
    hostaddr->sin_family = hp->h_addrtype;
    /* Don't unlock the mutex until we're no longer reading from hp. */
    p_th_mutex_unlock(p_th_mutexGETHOSTBYNAME); /* Note can be unlocked above*/
    /* Copy last result into cache */
#ifdef DNSCACHE_MAX
    if (alldnscaches == NULL)   
	ardp_hostname2addr_initcache();
     sockaddr_copy(hostaddr,&(acache->sockad));
    /* Boost initially, to bias towards keeping recent*/
    acache->usecount = 5; 
    p_th_mutex_lock(p_th_mutexALLDNSCACHE);
    APPEND_ITEM(acache,alldnscaches);
    alldnscache_count++;
    p_th_mutex_unlock(p_th_mutexALLDNSCACHE);
    dnscache_clean();   /* Only does anything if cache too big */
#endif /*DNSCACHE_MAX*/
    return(ARDP_SUCCESS);	/* Dont free acache */

  cleanup:
#ifdef DNSCACHE_MAX
    if (acache) dnscache_free(acache);
#endif
    return(retval);
}

/* Prototype needs to go into here, not just in pfs.h. */
int
stcaseequal(const char *s1,const char *s2)
{
    if (s1 == s2)               /* test for case when both NULL*/
        return TRUE;
    if (!s1 || !s2)             /* test for one NULL */
        return FALSE;
    return (strcasecmp(s1, s2) == 0);
}

