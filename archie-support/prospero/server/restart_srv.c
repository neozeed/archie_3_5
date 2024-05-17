/*
 * Copyright (c) 1991 by the University of Washington
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 *
 * Written  by bcn 1991     to restart server on command or failure
 * Modified by bcn 1/20/93  to use new arguments to dirsrv
 * Modified by swa 12/93    to not use local buffers or sprintf().
 */

#include <usc-license.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>              /* for def of external errno variable */
#include <sys/param.h>

#include <ardp.h>
#include <pfs.h>
#include <pserver.h>
#include <plog.h>
#include <pmachine.h>
#include <psrv.h>         /* For close_plog */

#include <pparse.h>
#include "dirsrv.h"
#include <sockettime.h>

/* NOTE: For SOLARIS, this must NOT be <signal.h> which we override
   in ../include/signal.h so that sigset_t gets defined.
   This is unfortunately neccessary since we are POSIX_SIGNAL
   compliant, but not in general (yet) POSIX compliant */

#include <posix_signal.h>	/* for SIG_SETMASK & sigset_t*/

/*
 * restart server restarts the server by calling exec.  Before
 * restarting, it logs the current statisitcs which would otherwise
 * be lost.
 */
#ifndef NDEBUG
int attemptRestart = 0;
#endif

static char * in_port_str = NULL;
static char * fcount_str = NULL;

void
set_restart_params(int fcount,  /* count of # of failures */
                   int inport /* UDP port # we use.  (NOT a descriptor #). */ )
{
  assert(P_IS_THIS_THREAD_MASTER());
  in_port_str = qsprintf_stcopyr(in_port_str, "-p#%d", inport);
  fcount_str = qsprintf_stcopyr(fcount_str, "-F%d",
                                 (fcount < 99999 ? fcount : 99999));

}

void
restart_server(int fcount, char *estring)
{
    static int	still_going = 0; /* set if restart attempt in progress */
    int		f;
    char	*dsargv[50]; /* Args to dirsrv          */
    int	        dsargc = 0;  /* Count of args to dirsrv */


#ifndef NDEBUG
    attemptRestart = 1;		    /* Set a flag so can debug things */	
#endif
    if(still_going > 10) exit(1);  /* It's the energizer rabbit */

#ifdef TIMEOUT_APPROACH
    alarm(0);			/*Make sure we dont get interrupted*/
#endif

    if(still_going++ == 0 || p__is_out_of_memory) { 
        /* If first time through     */
        if (!(p_th_mutex_trylock(p_th_mutexPSRV_LOG))) {
            /* Log statistics before we forget them on restart */
            log_server_stats();
            close_plog();
        }
    }

    /* Close all files except stdin, stdout, and stderr */
    /* which should still be /dev/null, and in_port     */
    /* which we might not be able to reopen             */
    for (f = 3; f < OPEN_MAX; f++) {
	if(f != in_port) (void) close(f);
    }

    /*dsargv now built at startup in set_restart_params */

    dsargv[dsargc++] = "dirsrv"; 

    /* Port to listen on */
    if(in_port >= 0) {
	dsargv[dsargc++] = in_port_str;
    }
    if(portname) {
	dsargv[dsargc++] = "-p";
	dsargv[dsargc++] = portname;
    }	

    if(estring) {
	dsargv[dsargc++] = "-E";
	dsargv[dsargc++] = estring;
    }

    if(fcount > 0) {
	dsargv[dsargc++] = fcount_str;
    }

    dsargv[dsargc++] = "-h";
    dsargv[dsargc++] = hostname;

    dsargv[dsargc++] = "-S";
    dsargv[dsargc++] = shadow;

    dsargv[dsargc++] = "-T";
    dsargv[dsargc++] = pfsdat;     /* storage */

    if(root && *root) {
	dsargv[dsargc++] = "-r";
	dsargv[dsargc++] = root;
    }

    if(aftpdir && *aftpdir) {   
	dsargv[dsargc++] = "-f";
	dsargv[dsargc++] = aftpdir;
    }

    if(afsdir && *afsdir) {   
	dsargv[dsargc++] = "-a";
	dsargv[dsargc++] = afsdir;
    }

    /* logfile if changed on command line */
    if(logfile_arg) {           
	dsargv[dsargc++] = "-L";
	dsargv[dsargc++] = logfile_arg;
    }

    dsargv[dsargc++] = NULL;

#ifdef POSIX_SIGNALS
    {
	sigset_t set;
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, (sigset_t *)NULL);
    }
#else
    sigsetmask(0);
#endif  /*POSIX_SIGNALS*/

    /* Restart the server specifying the appropriate flags */
    execv(P_DIRSRV_BINARY,dsargv);

    /* If we get here, the exec failed */
    /* Oops - cant write to log since we closed it ! */
#ifdef OLDSWA
    plog(L_FAILURE,NOREQ,
#else
    fprintf(stderr,
#endif
         "***Failure - Couldn't restart server %s (%s) - execv exited with \
error # %d, error name %s:  - aborting***",
         P_DIRSRV_BINARY, estring ? estring : "", errno, unixerrstr()
         );
    exit(1);
}

