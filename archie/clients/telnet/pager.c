#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __STDC__
#include <unistd.h>
#endif
#ifndef AIX
#include <sys/termios.h>
#else
#include <termio.h>
#endif
#ifdef __STDC__
#include <stdlib.h>
#endif
#include "defines.h"
#include "debug.h"
#include "error.h"
#include "extern.h"
#include "files.h" /* for tail() in library */
#include "fork_wait.h"
#include "lang.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "pager.h"
#include "prosp.h"
#include "terminal.h" /*bug: debugging*/
#include "vars.h"

#include "protos.h"

extern char **environ;


/*  
 *  Exec the pager.
 */  

int pager(file_name)
  const char *file_name;
{
  FILE *ifp;
  int ret = 0;

  if ( ! (ifp = fopen(file_name, "r")))
  {
    error(A_SYSERR, curr_lang[111], curr_lang[214]);
    sleep(2);
  }
  else
  {
    ret = pager_fp(ifp);
    fclose(ifp);
  }

  return ret;
}


/*  
 *  Exec the pager, directing `fp' to its stdin.
 *  
 *  **** WARNING! ****
 *  
 *  Since the child invokes an external program its return value is zero upon
 *  success and non-zero upon failure.  It is up to the parent (see the case
 *  label PARENT) to reverse this.
 */            
int pager_fp(fp)
  FILE *fp;
{
  int ret = 1;

  ptr_check(fp, FILE, "pager_fp", 0);

  rewind_fp(fp);
  if ( ! fempty(fp))
  {
    switch (fork_me(0, &ret))
    {
    case (enum fork_me_e)INTERNAL_ERROR:
      ret = 0;
      break;
      
    case CHILD:
      {
        const char *ppath;

        if ( ! (ppath = get_var(V_PAGER_PATH)))
        {
          error(A_INTERR, "pager_fp", curr_lang[216], V_PAGER_PATH);
        }
        else
        {
          const char *p = get_var(V_PAGER_OPTS);
          
          /*
           *  bug: if we're running setuid root the pager will not be able to
           *  open /dev/tty.  However, in this case we just happen to know
           *  that our version of `less' will fall back to using file
           *  descriptor 2 (?!).
           */
          dup2(0, 2);
          
          dup2(fileno(fp), 0);

          /* less doesn't like an empty string as an argument */
          if (p && ! *p) p = 0;

          if (getuid() != 0)
          {
            DBG(uids("pager_fp: non-chroot: "); sleep(5););
            execlp(ppath, tail(ppath), p, (char *)0, environ);
            error(A_SYSERR, "pager_fp", curr_lang[217], ppath);
          }
          else
          {
            /*  
             *  From the pager path we need the directory to which to
             *  chroot() and the name of the pager executable.
             */  
            DBG(uids("pager_fp: pre-chroot_for_exec: "););
            if ( ! chroot_for_exec("pager"))
            {
              error(A_SYSERR, "pager_fp", "can't change to '%s'", "pager"); /*FFF*/
              exit(1);          /* paranoid or cautious? :-) */
            }
            DBG(uids("pager_fp: post-chroot_for_exec: "); sleep(5););
            /* bug! */
            {
              const char *q;
              ppath = (q = strchr(ppath, '/')) ? q+1 : ppath;
            }
            execlp(ppath, tail(ppath), p, (char *)0);
            error(A_SYSERR, "pager_fp", curr_lang[217], ppath);
          }
        }
      }
      fork_return(ret);

    case PARENT:
      ret = ! ret; /* Convert exit status to return value. */
      break;

    default:
      error(A_INTERR, curr_lang[152], curr_lang[48]);
      ret = 0;
    }
  }

  return ret;
}

