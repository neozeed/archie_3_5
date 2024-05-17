#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/errno.h>
#ifdef __STDC__
#include <stdlib.h>
#endif
#include "debug.h"
#include "defines.h"
#include "error.h"
#include "extern.h"
#include "fork_wait.h"
#include "lang.h"
#include "misc_ansi_defs.h"
#include "signals.h"
#include "tellwait.h"
#include "unixcompat.h"
#include "vars.h"

#include "protos.h"

#ifndef DEBUG
#define SLEEP (5 /* tenths of a second */ * (100000))
#else
#define SLEEP (20 /* tenths of a second */ * (100000))
#endif


#if 1
static char spintab[] = { '=', 'O' };
#else
static char spintab[] = { '/', '-', '\\', '|' }; /* boring spinner... */
#endif


static int sit_and_spin PROTO((int *status, int prt_status));


static int sit_and_spin(status, prt_status)
  int *status;
  int prt_status;
{
#ifdef PROFILE
  return 1;
#else
  if ( ! prt_status)
  {
    return wait(status);
  }
  else
  {
    int i = 0;
    int rv;

    wait_child();
    fputs(curr_lang[118], stdout);
    while ((rv = waitpid(-1, status, WNOHANG)) == 0)
    {
      putchar('\b'); putchar(spintab[i]);
      fflush(stdout);
      i = (i + 1) % (sizeof spintab / sizeof spintab[0]);
      u_sleep(SLEEP);
    }
    putchar('\n');
    return rv;
  }
#endif
}


/*  
 *  Fork and return who we are, parent or child.  In the case of the parent
 *  wait for the child to finish, unless 'is_background' is set.  In the case
 *  of the child catch some signals.
 */  

Forkme fork_me(prt_status, ret)
  int prt_status;
  int *ret;
{
  int child_pid;
  int cstat;
  int rpid;

#ifdef PROFILE
  child_pid = 0;
#else
  if (prt_status) tell_wait();
  child_pid = fork();
#endif

  switch (child_pid)
  {
  case -1:                      /* error */
    perror(curr_lang[119]);
    return (enum forkme_e)INTERNAL_ERROR;

  case 0:                       /* child */
#ifdef CHILD_DEBUG
    fprintf(stderr, "%s: fork_me: child #%ld; hit return to continue: ",
            prog, (long)getpid());
    while (getchar() != '\n');
#endif
    child_sigs();
    return CHILD;

  default:                      /* parent */
#ifndef PROFILE
    if ( ! prt_status) no_tell_wait();
#endif
    rpid = sit_and_spin(&cstat, prt_status);
    if (WIFSIGNALED(cstat))     /*bug: what if child stops?*/
    {
      *ret = 0;
      error(A_ERR, curr_lang[120], curr_lang[323], WTERMSIG(cstat));
    }
    else if (rpid == -1)
    {
      *ret = 0;
      error(A_SYSERR, curr_lang[120], curr_lang[121]);
      return (enum forkme_e)INTERNAL_ERROR;
    }
    *ret = WEXITSTATUS(cstat);
    return PARENT;
  }
}
