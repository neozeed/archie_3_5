#ifdef __STDC__
#include <unistd.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <signal.h>

#include "ardp.h"
#include "debug.h"
#include "error.h"
#include "extern.h"
#include "lang.h"
#include "macros.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "prosp.h"

#include "signals.h"
#include "tellwait.h"
#include "vars.h"

#include "protos.h"


/*  
 *  Fri Apr 16 00:46:12 EDT 1993 - wheelan
 *  
 *  I've noticed weirdness with autologout when run from the command line: it
 *  doesn't pick up keystrokes when the user is in the pager, causing them to
 *  be logged out even though they haven't been idle.  Typing doesn't seem to
 *  change the modification type of /dev/tty.
 */  


static void child_handle PROTO((int sig));
static void on_hup PROTO((int sig));


/*  
 *  Upon catching an alarm signal, die and take everybody with you.
 */  

void catch_alarm(sig)
  int sig;
{
  const char *max_idle_time_str;
  struct stat sbuf;
  time_t max_idle_time;

  d4fprintf(stderr, "%s: catch_alarm: (%ld) start at %s.\n",
            prog, (long)getpid(), now());

  /* NOTE: may have to make "autologout" un-unsettable. */

  if ((max_idle_time_str = get_var(V_AUTOLOGOUT)))
  {
    max_idle_time = MIN_TO_SEC(atoi(max_idle_time_str));
  }
  else
  {
    error(A_INTERR, curr_lang[241], curr_lang[246]);
    return;
  }

  if (fstat(0, &sbuf) != -1)
  {
    time_t time_been_idle;

    time_been_idle = time((time_t *)0) - sbuf.st_atime;
    d4fprintf(stderr, "%s: catch_alarm: (%ld) %ld seconds idle (%ld left, %ld max) at %s.\n",
              prog, (long)getpid(), (long)time_been_idle,
              (long)(max_idle_time - time_been_idle), (long)max_idle_time, now());
    if (time_been_idle < max_idle_time)
    {
      alarm((unsigned)(max_idle_time - time_been_idle));
      d4fprintf(stderr,
                "%s: catch_alarm: (%ld) reset alarm to %ld seconds at %s.\n",
                prog, (long)getpid(),
                (long)(max_idle_time - time_been_idle), now());
    }
    else
    {
      printf(curr_lang[244]);
      fflush(stdout);
      d4fprintf(stderr,
                "%s: catch_alarm: (%ld) sending HUP to our process group at %s.\n",
                prog, (long)getpid(), now());
#if !defined(AIX) && !defined(SOLARIS)
      kill(-getpgrp(0), SIGHUP);
#else
      kill(-getpgrp(), SIGHUP);
#endif
    }
  }
}


/*  
 *  This is only called from the child.
 */  

static void child_handle(sig)
  int sig;
{
  char signame[32];

  switch (sig)
  {
  case SIGHUP:
    strcpy(signame, "HUP");
    break;

  case SIGINT:
    strcpy(signame, "INT");
    break;

  case SIGQUIT:
    strcpy(signame, "QUIT");
    break;

  default:
    sprintf(signame, "#%d", sig);
    error(A_ERR, "child_handle", "stray signal %d -- child exiting.", sig);
  }
  /*bug: wipes out _all_ our requests; change if we allow async. requests*/
  ardp_abort(NOREQ);
  d4fprintf(stderr, "%s: child_handle: (%ld) caught %s at %s -- exiting.\n",
            prog, (long)getpid(), signame, now());
  /*  
   *  bug! This causes all sort of problems, as the default action of
   *  SIGUSR<n> is to kill the process.  This signal can be sent even if
   *  we're not using the `tell_' functions.
   */  
  tell_parent(getppid());
  exit(sig);
}


/*  
 *  Children will call this.
 */  

void child_sigs()
{
  ppc_signal(SIGALRM, SIG_IGN);	/* Just to be paranoid */
  ppc_signal(SIGINT, child_handle);
  ppc_signal(SIGQUIT, child_handle);
  ppc_signal(SIGPIPE, SIG_DFL);
}


static void on_hup(sig)
  int sig;
{
  d4fprintf(stderr, "%s: on_hup: (%ld) caught HUP at %s -- exiting.\n",
            prog, (long)getpid(), now());
  exit(0);
}


/*  
 *  The parent will call this.
 */  

void parent_sigs()
{
  no_tell_wait();               /* Don't die on SIGUSR1 & SIGUSR2 */
  
  ppc_signal(SIGALRM, catch_alarm);

  /* Ignore these */

  ppc_signal(SIGHUP, on_hup);
  ppc_signal(SIGPIPE, SIG_IGN);
  ppc_signal(SIGINT, SIG_IGN);
  ppc_signal(SIGQUIT, SIG_IGN);
}


/*  
 *  This is taken from ppc/lib/os_indep.c.  It probably ought to be renamed
 *  and moved into libarchie...
 */  

/*  
 *  From Stevens' "Advanced Programming in the UNIX Environment".
 *
 *  This will ensure we get reasonable SIGCHLD semantics.
 */  
Sigfunc *ppc_signal(sig, fn)
  int sig;
  Sigfunc *fn;
{
  struct sigaction act, oact;

  act.sa_handler = fn;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  if (sig == SIGALRM)
  {
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT; /* SunOS */
#endif
  }
  else
  {
#ifdef SA_RESTART
    act.sa_flags |= SA_RESTART; /* SVR4, 4.3+BSD */
#endif
  }
  if (sigaction(sig, &act, &oact) < 0)
  {
    return SIG_ERR;
  }
  return oact.sa_handler;
}
