#ifdef __STDC__
# include <stdlib.h>
#else
# include <memory.h>
#endif
#include <string.h>
#include "sub.h"


/*  
 *  Substitute, in `str', all occurences of `src' with `replc'.
 *  
 *  Make two passes over the data: first determine how much space will be
 *  needed, then allocate the space and copy the source string, making the
 *  substitutions.
 */    

char *strsub(src, str, replc)
  const char *src;
  const char *str;
  const char *replc;
{
  char *d;
  char *dst;
  const char *match;
  const char *s;
  unsigned int n;

  s = src;
  n = strlen(src) + 1;
  while ((match = strstr(s, str)))
  {
    n += strlen(replc) - strlen(str);
    s = match + strlen(str);
  }

  if ( ! (dst = malloc(n)))
  {
    return 0;
  }

  s = src;
  d = dst;
  while ((match = strstr(s, str)))
  {
    strncpy(d, s, match - s);
    d += match - s;
    strcpy(d, replc);
    d += strlen(replc);
    
    s = match + strlen(str);
  }

  strcpy(d, s);                 /* copy any trailing characters */

  return dst;
}
