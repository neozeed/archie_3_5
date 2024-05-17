/*  
 *  Based on Stevens.
 */  

#include <signal.h>
#include <sys/wait.h>
#include "ansi_compat.h"
#include "debug.h"
#include "error.h"
#include "extern.h"
#include "signals.h"
#include "tellwait.h"

#include "protos.h"

static volatile int sigflag; /* set nonzero by signal handler */
static sigset_t newmask;
static sigset_t oldmask;
static sigset_t zeromask;


static void sig_usr PROTO((int signo));


/*  
 *  One signal handler for SIGUSR1 and SIGUSR2
 */  
static void sig_usr(signo)
  int signo;
{
  d5fprintf(stderr, "%s (%ld): sig_usr: caught signal `%d'.\n",
            prog, (long)getpid(), signo);
  sigflag = 1;
  return;
}


int no_tell_wait()
{
  int ret = 1;
  
  /* reset signal mask to original value */
  if (sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)0) < 0)
  {
    error(A_SYSERR, "wait_parent", "error resetting signal mask");
  }
  if (ppc_signal(SIGUSR1, SIG_IGN) == SIG_ERR)
  {
    error(A_SYSERR, "tell_wait", "error ignoring SIGUSR1"); /*FFF*/
    ret = 0;
  }
  if (ppc_signal(SIGUSR2, SIG_IGN) == SIG_ERR)
  {
    error(A_SYSERR, "tell_wait", "error ignoring SIGUSR2"); /*FFF*/
    ret = 0;
  }

  return ret;
}


int tell_wait()
{
#ifndef PROFILE
  sigflag = 0;
  if (ppc_signal(SIGUSR1, sig_usr) == SIG_ERR)
  {
    error(A_SYSERR, "tell_wait", "error setting SIGUSR1 signal handler"); /*FFF*/
    return 0;
  }
  if (ppc_signal(SIGUSR2, sig_usr) == SIG_ERR)
  {
    error(A_SYSERR, "tell_wait", "error setting SIGUSR2 signal handler"); /*FFF*/
    return 0;
  }
  if (ppc_signal(SIGCHLD, sig_usr) == SIG_ERR)
  {
    error(A_SYSERR, "tell_wait", "error setting SIGCHLD signal handler"); /*FFF*/
    return 0;
  }

  sigemptyset(&zeromask);

  sigemptyset(&newmask);
  sigaddset(&newmask, SIGUSR1);
  sigaddset(&newmask, SIGUSR2);
  sigaddset(&newmask, SIGCHLD);
  /* block SIGUSR1 and SIGUSR2, and save current signal mask */
  if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
  {
    error(A_SYSERR, "tell_wait", "error blocking SIGUSR1 and SIGUSR2");
    return 0;
  }
#endif
  return 1;

}


/*  
 *  Tell the parent to resume; called by child.
 */  
void tell_parent(pid)
  pid_t pid;
{
#ifndef PROFILE
  d5fprintf(stderr, "%s (%ld): tell_parent: kill(%ld, SIGUSR2).\n",
            prog, (long)getpid(), (long)pid);
  kill(pid, SIGUSR2);           /* tell parent we're done */
#endif
}


/*  
 *  Wait for signal from parent; called by child.
 */  
void wait_parent()
{
#ifndef PROFILE
  while (sigflag == 0)
  {
    sigsuspend(&zeromask);      /* and wait for parent */
  }

  d5fprintf(stderr, "%s (%ld): wait_parent: resuming.\n",
            prog, (long)getpid());

  sigflag = 0;
  /* reset signal mask to original value */
  if (sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)0) < 0)
  {
    error(A_SYSERR, "wait_parent", "error resetting signal mask");
  }
#endif
}


/*  
 *  Tell the child to resume; called by parent.
 */  
void tell_child(pid)
  pid_t pid;
{
#ifndef PROFILE
  d5fprintf(stderr, "%s (%ld): tell_child: kill(%ld, SIGUSR1).\n",
            prog, (long)getpid(), (long)pid);
  kill(pid, SIGUSR1);           /* tell child we're done */
#endif
}


/*  
 *  Wait for signal from child; called by parent.
 */  
void wait_child()
{
#ifndef PROFILE
  while (sigflag == 0)
  {
    sigsuspend(&zeromask);      /* and wait for child */
  }

  d5fprintf(stderr, "%s (%ld): wait_child: resuming.\n",
            prog, (long)getpid());

  sigflag = 0;
  /* reset signal mask to original value */
  if (sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)0) < 0)
  {
    error(A_SYSERR, "wait_child", "error resetting signal mask");
  }
#endif
}
