/*
 * Copyright (c) 1991--1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>
#include <sys/param.h>

#if 0                           /* This code should be deleted if this
                                   clean-compiles.  */
/* For SCO which doesn't define MAXPATHLEN */
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
#endif /* 0 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include <pfs.h>
#include <pcompat.h>
#include <psite.h>
#include <perrno.h>
#include <mitra_macros.h>		/* For L2 */
#include <pmachine.h>           /* for INCLUDE_FILES_LACK_TEMPNAM_PROTOTYPE */

static int pmap_getcache(char *npath, TOKEN am_args);

#define SECONDSPERDAY (60*60*24)

/* This defines our caching policy -- a simple cut-off.  A more sophisticated
 * policy could be used but doesn't seem necessary.
 *
 * This policy does not take into account the issues of swiftly-changing data.
 */

#define MAXFILECACHEAGE (1*SECONDSPERDAY)

/* p__map_cache(): Retrieve a file and cache it.
 *
 * The interface should be changed to be char **npath and use stcopy for name.
 */

/* Note returns 0 or PMC_DELETE_ON_CLOSE for success */

#ifndef P_CACHE_ENABLED
/* Retrieve file caching it in /tmp/pfs_cache */
int
p__map_cache(VLINK vl, 	/* link to object - NOT USED at this time. */
	char *npath, 	/* Buffer to write filename into */
	int npathlen, 	/* length of buffer */
	TOKEN am_args)  /* Access mode args 
				access_method,INTERNET-D,host,ASCII,remote*/
{
    qsprintf(npath, npathlen, "/tmp/pfs_cache/%s/%s%s%s", elt(am_args,2),
             elt(am_args, 0), ((*elt(am_args,4)  == '/') ? "" : "/"),
             elt(am_args, 4));
    return pmap_getcache(npath,am_args);

}
#else
/* Retrieve file caching it in P_CACHE_VC */
int
p__map_cache(VLINK vl, 	/* link to object - NOT USED at this time. */
	char *npath, 	/* Buffer to write filename into */
	int npathlen, 	/* length of buffer */
	TOKEN am_args)  /* Access mode args 
				access_method,INTERNET-D,host,ASCII,remote*/
{
    int	retval = 0;

    char *cachename = NULL;
    char *tempfile = NULL;

    /* Name of cached version is -- P_CACHE_VC/host/method/remote */

    cachename = qsprintf_stcopyr(cachename, "%s/%s/%s%s%s", 
		P_CACHE_VC, elt(am_args,2),
             elt(am_args, 0), ((*elt(am_args,4)  == '/') ? "" : "/"),
             elt(am_args, 4));

    if ((strlen(cachename) > 250 ) 
        && (strncmp(elt(am_args, 4), "WAIS-GW",7) == 0) ) {
	/* Horrible Kludge alert, make component length smaller for 
	   WAIS docid's  */
	char	*cp;
	for (cp=cachename; *cp; cp++)
		if (*cp == '%')
			*cp = '/';
    }
    /* Determine whether a cached copy already exists */
    if (!file_incache(cachename) || (stat_age(cachename) > MAXFILECACHEAGE) ) {
	switch (retval = pmap_getcache(cachename,am_args)) {
	case PMC_DELETE_ON_CLOSE:
	case PSUCCESS:
		break;
	/*PMC_RETRIEVE_FAILED:*/
	default:
	    if (file_incache(cachename)) {
		/* Old copy stil available */
		break;
	    } else {
		/* Failed */
		stfree(cachename);
		return(retval);
	}
    }
    }
    /* Either succeeded in fetching, or already had it */
    /* Assuming caller opens this file, then it will touch access time*/
    tempfile = tempnam(P_CACHE_P,NULL);
    if (link(cachename, tempfile)) {
	retval=errno;
    } else {
    	qsprintf(npath,npathlen,tempfile);
    }
    stfree(cachename);
    free(tempfile);
    return(retval);
}
#endif /* P_CACHE_ENABLED */

/*
 * Retrieves a file with access method am_args into the file
 * named NPATH.  This breakout is attributable to mitra (thanks).
 *
 * Return codes:
 * 0 or PMC_DELETE_ON_CLOSE is success.
 * All other return codes indicate a failure.  Currently always returns
 * PMC_DELETE_ON_CLOSE. 
 */
static
int
pmap_getcache(char *npath, TOKEN am_args)
{
    static char		*vcachebin;
    int 		pid;
#ifdef BSD_UNION_WAIT
    union wait 	status;
#else
    int     status;
#endif
    int		tmp;
    char    *vcargv[12];        /* vcache's ARGV.  Enough to hold any known
                                   access method arguments. */
    char    **vcargvp = vcargv; /* pointer to vcache's argv. */
#ifndef INCREASE_CLIENT_EXECUTABLE_SIZE_DO_NOT_EXEC_VCACHE
    char *p_binaries;
#else /*INCREASE_CLIENT_EXECUTABLE_SIZE_DO_NOT_EXEC_VCACHE*/
    char	*host, *method, *remote;
#endif /*INCREASE_CLIENT_EXECUTABLE_SIZE_DO_NOT_EXEC_VCACHE*/

  assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
#ifndef INCREASE_CLIENT_EXECUTABLE_SIZE_DO_NOT_EXEC_VCACHE
    /* should really do this on our own without */
    /* calling system, but...                   */
    /* Set args first.  This makes it easier to debug on debuggers that don't
       take kindly to subprocesses forking. */
    p_binaries = getenv("P_BINARIES");

    assert(P_IS_THIS_THREAD_MASTER());
#ifdef P_BINARIES
    if (!p_binaries)
        p_binaries = P_BINARIES;
#endif

    *vcargvp++ = "vcache";
#ifdef P_CACHE_ENABLED
    *vcargvp++ = "-m";           /* manage the cache argument */
#endif
    *vcargvp++ = npath;
#else /*INCREASE_CLIENT_EXECUTABLE_SIZE_DO_NOT_EXEC_VCACHE*/
    if (!am_args) return PMC_RETRIEVE_FAILED;
    method = am_args->token ; am_args = am_args->next;
    if (!am_args) return PMC_RETRIEVE_FAILED;
    if (strcmp(am_args->token,"INTERNET-D")) return PMC_RETRIEVE_FAILED;
 	am_args = am_args->next;
    if (!am_args) return PMC_RETRIEVE_FAILED;
    host = am_args->token ; am_args = am_args->next;
    if (!am_args) return PMC_RETRIEVE_FAILED;
    if (strcmp(am_args->token,"ASCII")) return PMC_RETRIEVE_FAILED;
 	am_args = am_args->next;
    if (!am_args) return PMC_RETRIEVE_FAILED;
    remote = am_args->token ; am_args = am_args->next;
#endif /*INCREASE_CLIENT_EXECUTABLE_SIZE_DO_NOT_EXEC_VCACHE*/
    for (;am_args; am_args = am_args->next) {
        *vcargvp++ = am_args->token;
    }
    *vcargvp = NULL;

#ifndef INCREASE_CLIENT_EXECUTABLE_SIZE_DO_NOT_EXEC_VCACHE
    pid = fork();
    if (pid < 0) {
        if (pfs_debug) {
            perror("p__map_cache(): failed to fork().");
        }
        return PMC_RETRIEVE_FAILED;
    }
#if 0
#define atline() if(pfs_debug > 10) fprintf(stderr, "%s:%d\n", __FILE__, __LINE__)
#else
#define atline() do ; while (0)
#endif
    if (pid == 0) {
        if (pfs_debug > 10)
            fprintf(stderr, "p__map_cache(): just forked.\n");
        atline();
        if (!p_binaries || !*p_binaries) {
            atline();
            /* In execl, add the "-m" option if cache to be managed */
            /* Add the "-r" option if existing item is out of date  */
            DISABLE_PFS(execvp("vcache",vcargv));
            atline();
            if (!vcachebin) vcachebin = stcopy("vcache");
            strcpy(vcachebin, "vcache"); /* for error reporting */
            atline();
        } else {
            atline();
            if (!vcachebin) 
                vcachebin = qsprintf_stcopyr(vcachebin, 
                                             "%s/vcache",p_binaries);
            atline();
            /* In execl, add the "-m" option if cache to be managed */
            /* Add the "-r" option if existing item is out of date  */
            DISABLE_PFS(execv(vcachebin,vcargv));
            atline();
        }
        atline();
        if (pfs_debug) {
            fprintf(stderr,
                    "p__map_cache(): exec failed for %s (errno=%d): ",
                    vcachebin,errno);
            perror(NULL);
        }
        atline();
        exit(1);
    }
    else {
        wait(&status); 
    }
#ifdef BSD_UNION_WAIT
    tmp = status.w_T.w_Retcode;
#else
    tmp = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif

#else  /*INCREASE_CLIENT_EXECUTABLE_SIZE_DO_NOT_EXEC_VCACHE*/
	/* This is an experiment in calling vcache directly */
	DISABLE_PFS(tmp=vcache2a(host, remote, npath, method, vcargv,
#ifdef P_CACHE_ENABLED
                                TRUE /* argument to manage the cache */
#else
                                FALSE
#endif
                                ));
#endif /*INCREASE_CLIENT_EXECUTABLE_SIZE_DO_NOT_EXEC_VCACHE*/
    if(tmp) return(PMC_RETRIEVE_FAILED);

    /* Return PMC_DELETE_ON_CLOSE if cache is not being managed */
    /* I left this, because we are making a copy, that the caller
	needs to delete -- Mitra  */
    return(PMC_DELETE_ON_CLOSE);

    /* return(PSUCCESS);*/

}

int
file_incache(char *local)
{
    struct stat buf;

    /* This is probably lazy bad code, stat fails if and only if the 
	file doesn't exist in the cache*/
    return (stat(local,&buf) == 0);
}


#if 0                           /* not used; different test up above. */
int
file_incache_and_uptodate(char *local)
{
	/* For now, everything is up to date, need to be much cleverer !!*/
	return (file_incache(local));
}
#endif
