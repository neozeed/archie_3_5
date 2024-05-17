#ifdef __STDC__
# include <stdlib.h>
#endif
#include <limits.h>
#include <string.h>
#include "all.h"
#include "error.h"
#include "misc.h"
#include "pattrib.h"
#include "protos.h"
#include "psearch.h"
#include "str.h"
#include "token.h"
#include "waisattrib.h"


/*  
 *  Look for the next tagged Prospero contents attribute having the WAIS
 *  attribute name `tagname'.  Return a pointer to Prospero attribute.
 */    
PATTRIB nextAttrWithTag(tagname, cat)
  const char *tagname;
  PATTRIB cat;
{
  PATTRIB at;
  
  for (at = cat; (at = nextAttr(CONTENTS, at)); at = at->next)
  {
    const char *atname;

    if ((atname = nth_token_str(at->value.sequence, 1)) &&
        strcasecmp(tagname, atname) == 0)
    {
      break;
    }
  }

  return at;
}


const char *getTagValue(name, pat)
  const char *name;
  PATTRIB pat;
{
  PATTRIB at;
  
  for (at = pat; at; at = at->next)
  {
    if (tkmatches(at->value.sequence, "TAGGED", name, (char *)0))
    {
      return nth_token_str(at->value.sequence, 2);
    }
  }

  return 0;
}


/*  
 *  `val' should be of the form
 *  
 *  <type> <epath> <desc> [<type> <epath> <desc> ...]
 *  
 *  where <type> is one of `TEXT', `IMAGE', `AUDIO', `VIDEO', `HTML' or
 *  `MENU' (this subject to change), <epath> is the usual encoded path and
 *  <desc> is a double quoted string.
 *  
 *  Precedence for `ferretd' should be "VIDEO IMAGE AUDIO HTML TEXT".  For
 *  `weaseld' it is "IMAGE TEXT" (in which case the image had better be a GIF
 *  file!).
 *  
 *  bug: this routine just plain sucks; come up with a better way...
 */          

int link_to(val, prec, contype, encpath, descrip)
  const char *val;
  const char *prec[];
  char **contype;
  char **encpath;
  char **descrip;
{
#define TYPE 0
#define PATH 1
#define DESC 2  
#define SWAP(curr, best)           \
  do {                             \
    int i;                         \
    for (i = 0; i < 3; i++)        \
    {                              \
      if (best[i]) free(best[i]);  \
      best[i] = curr[i];           \
    }                              \
  } while (0)

  char *best[3];
  char *curr[3];
  const char *rest;
  const char *s;
  int besti = INT_MAX;
  int n;

  s = val;
  best[TYPE] = best[PATH] = best[DESC] = 0;

  while ((n = qwstrnsplit(s, curr, 3, &rest)) == 3)
  {
    int i;

    /* find current type in precedence list */
    for (i = 0; prec[i]; i++)
    {
      if (strcasecmp(curr[TYPE], prec[i]) == 0) break;
    }
    if ( ! prec[i]) /* unknown type */
    {
      efprintf(stderr, "%s: link_to: unknown type `%s' in `%s'.\n",
               logpfx(), curr[TYPE], val);
    }
    else
    {
      /* Is the current type preferable to the best so far? */
      if (i < besti)
      {
        besti = i;
        SWAP(curr, best);
      }
    }

    s = rest;
  }

  if (besti == INT_MAX)
  {
    return 0;
  }
  else
  {
    *contype = best[TYPE];
    *encpath = best[PATH];
    *descrip = best[DESC];
    return 1;
  }
  
#undef TYPE
#undef PATH
#undef DESC
#undef SWAP
}
