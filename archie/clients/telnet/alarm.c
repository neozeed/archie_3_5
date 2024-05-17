#ifdef __STDC__
#include <stdlib.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include "protos.h"
#include "debug.h"
#include "defines.h"
#include "error.h"
#include "extern.h"
#include "lang.h"
#include "macros.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "vars.h"


/*  
 *  Routines to do autologout stuff.
 */  


int set_alarm()
{
  const char *val;
  int mins;

  if ( ! (val = get_var(V_AUTOLOGOUT)))
  {
    error(A_INTERR, "set_alarm", curr_lang[1]);
    return 0;
  }
  else if ((mins = atoi(val)) == 0)
  {
    return 0;
  }
  else
  {
    alarm(MIN_TO_SEC(mins));
    d4fprintf(stderr, "%s: set_alarm: (%ld) alarm set to %d seconds at %s.\n",
              prog, (long)getpid(), MIN_TO_SEC(mins), now());
    return 1;
  }
}


int unset_alarm()
{
  alarm((unsigned)0);
  d4fprintf(stderr, "%s: set_alarm: (%ld) alarm unset at %s.\n",
            prog, (long)getpid(), now());
  return 1;
}
