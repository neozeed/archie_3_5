/*  
 *  The purpose of these routines is to provide (fairly) consistent behaviour
 *  across different OSes (i.e. to fix some SVR4 braindamage).
 */

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
#include "os_indep.h"


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


int max_open_files()
{
#define MAX_OPEN_GUESS 256

  int max = 0;

  errno = 0;
  if ((max = sysconf(_SC_OPEN_MAX)) < 0) {
    max = MAX_OPEN_GUESS;
  }

  return max;

#undef MAX_OPEN_GUESS
}
