#ifdef __STDC__
# include <stdlib.h> /* free() */
#endif
#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "error.h"
#include "results.h"
#include "strhash.h"
#include "all.h"


static void call proto_((int (*fn) OFUN_PROTO, FILE *fp, VLINK v, TOKEN args));


/*  
 *  Set up `fn's argument list, then call it.
 */  
static void call(fn, fp, v, args)
  int (*fn) OFUN_PROTO;
  FILE *fp;
  VLINK v;
  TOKEN args;
{
  TOKEN tend = 0;
  TOKEN tsav = 0;

  /* check for null `args' just in case... */
  if (args && args->token)
  {
    tend = args;
    while (tend->next && tend->next->token)
    {
      tend = tend->next;
    }
    tsav = tend->next;
    tend->next = 0;
  }

  fn(fp, v, args);

  if (tsav)
  {
    tend->next = tsav;
  }
}

  
/*
 *
 *
 *                             External routines
 *
 *
 */

#define HTABSZ 64


static struct hash_table *htab;


void printContents()
{
  print_hash_table(stderr, htab);
}


int registerOutputFunction(cmd, fn)
  const char *cmd;
  int (*fn) OFUN_PROTO;
{
  if ( ! htab && ! (htab = new_hash_table(HTABSZ)))
  {
    return 0;
  }
    
  return insert(strdup(cmd), (void *)fn, htab);
}


/*  
 *  Run through the various commands, calling the appropriate output
 *  functions.
 */  
void displayResults(fp, res)
  FILE *fp;
  struct Result *res;
{
  TOKEN t;
  int (*ofn) OFUN_PROTO;
  
  t = res->commands;
  while (t)
  {
    const char *s = t->token;
    
    if ((ofn = (int (*)OFUN_PROTO)get_value(s, htab)))
    {
      call(ofn, fp, res->res, t->next);
    }
    else
    {
      efprintf(stderr, "%s: displayResults: no output function for `%s'.\n",
               logpfx(), s);
    }

    /* Skip to next command.  (I.e. skip over argument terminator) */
    while (t)
    {
      if (t->token)
      {
        t = t->next;
      }
      else
      {
        t = t->next;
        break;
      }
    }
  }
}


void cancelOutputFunctions()
{
  if (htab)
  {
    free_hash_table(htab, free, 0);
  }
}
