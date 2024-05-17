#ifdef __STDC__
# include <stdlib.h>
#endif
#include <stdio.h>
#include "aprint.h"
#include "ppc.h"


void print_sites(ofp, v, print_as_dir, here)  /* bajan */
  FILE *ofp;
  VLINK v;
  int print_as_dir;
  const Where here;
{
  char *gophstr;

  for (; v; v = v->next)
  {
    if((gophstr = get_gopher_info(v)))
    {
      efprintf(ofp, "%s\r\n", gophstr);
    }
    else
    {
      efprintf(stderr, "%s: print_sites: get_gopher_info() failed on `%s:%s'.",
               logpfx(), v->host, v->hsoname);
    }

    free(gophstr);
  }
}
