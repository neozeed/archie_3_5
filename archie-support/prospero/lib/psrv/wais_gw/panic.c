/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.
*/

/*---------------------------------------------------------------------------*/

#include <ctype.h>

#include "panic.h"
#include "waislog.h"
#include "cutil.h"

#include <pfs.h>                /* for stcopyr() */
#include <perrno.h>

extern int fault_count;
/*----------------------------------------------------------------------*/

void
panic(char* formatString,...)
/* something bad and unexpected has happened, print out a log message 
   and abort the program in such a way that it can be debugged.
 */
{
  va_list ap;			

  char msg[2000];		

  va_start(ap,formatString);	
  vsprintf(msg,formatString,ap);
#if !defined(IN_RMG) 
#if   !defined(PFS_THREADS)
  waislog(WLOG_HIGH,WLOG_ERROR,msg);
#else
#error	NO WAY TO LOG - DONT COMPILE LIKE THIS
#endif
#else
  p_err_string = stcopyr(p_err_string, msg);
  /* restart_server(fault_count, msg); */
#endif
  va_end(ap);			

#ifdef THINK_C
  Debugger();
#endif

  abort();
}


/*----------------------------------------------------------------------*/

