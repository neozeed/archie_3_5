#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "prosp.h"
#include "protos.h"
#include "token.h"


/*  
 *  Convert a list of TOKENs to a null terminated array of character
 *  pointers.
 */    
char **tkarray(tok)
  TOKEN tok;
{
  TOKEN t;
  char **mem;
  int i;
  int n;
  
  for (t = tok, n = 0; t; t = t->next, n++)
  { continue; }

  if ( ! (mem = (char **)malloc(sizeof (char *) * (n + 1))))
  {
    return 0;
  }

  for (i = 0, t = tok; i < n; i++, t = t->next)
  {
    mem[i] = t->token;
  }
  mem[i] = 0;

  return mem;
}


/*  
 *  Check whether succesive elements in the list of TOKENs match the null
 *  terminated series of string arguments.
 */  
int tkmatches(TOKEN tok, ...)
{
  const char *s;
  va_list ap;

  if ( ! tok) return 0;
  
  va_start(ap, tok);
  s = va_arg(ap, const char *);
  while (tok && s)
  {
    if (strcasecmp(tok->token, s) != 0)
    {
      va_end(ap);
      return 0;
    }
    tok = tok->next;
    s = va_arg(ap, const char *);
  }
  va_end(ap);

  return !s;                   /* Only fail if some arguments remain. */
}
