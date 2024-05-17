/*  
 *  Keep a table of HTTP paths, or Gopher selector string to allow remapping
 *  and redirection.
 */

#ifdef __STDC__
# include <stdlib.h>
#else
# include <search.h>
#endif
#include <stdio.h>
#include "error.h"
#include "misc.h"
#include "protos.h"
#include "redirect.h"
#include "str.h"
#include "all.h"


#define TSZ 1024


static int nelts = 0;
struct remap {
  char *src;
  char *dst;
} remap[TSZ];                   /* bug: make dynamic */


static int cmp_strs(a, b)
  struct remap *a;
  struct remap *b;
{
  return strcmp(a->src, b->src);
}


const char *redirection(src)
  const char *src;
{
  struct remap *r;
  struct remap s;

  if (nelts == 0)
  {
    return 0;
  }
  
  s.src = src;
  if ((r = (struct remap *)bsearch((char *)&s, (char *)&remap[0], (unsigned)nelts,
                                   sizeof remap[0], (int (*)())cmp_strs)))
  {
    return r->dst;
  }

  return 0;
}


/*  
 *  Load the redirection file from `redfile'.
 *  
 *  Return the number of entries loaded into the table, or -1 on an error.
 */
int load_redirection_file(redfile)
  const char *redfile;
{
  FILE *rfp;
  int ret;
  
  if ( ! (rfp = fopen(redfile, "r")))
  {
    efprintf(stderr, "%s: load_redirection_file: can't open `%s': %m.\n",
             logpfx(), redfile);
    return -1;
  }

  ret = load_redirection_fp(rfp);

  fclose(rfp);
  return ret;
}


/*  
 *  Load an ACL list from the stream `afp'.
 *  
 *  Return the number of hosts loaded into the ACL table, or -1 on an error.
 */    
int load_redirection_fp(rfp)
  FILE *rfp;
{
  char dst[512];
  char line[512];
  char src[512];
  
  for (nelts = 0; nelts < TSZ; nelts++)
  {
    if ( ! fgets(line, sizeof line, rfp))
    {
      if (ferror(rfp))
      {
        efprintf(stderr, "%s: load_redirect_fp: fgets() failed: %m.\n", logpfx());
        fclose(rfp);
        return -1;
      }
      break;
    }

    if (sscanf(line, "%s %s", src, dst) == 2)
    {
      remap[nelts].src = strdup(src);
      remap[nelts].dst = strdup(dst);
    }
    else
    {
      efprintf(stderr, "%s: load_redirect_fp: WARNING! Bad redirect line `%s': skipping.\n",
               logpfx(), line);
      nelts--;
    }
  }

  if ( ! feof(rfp))
  {
    efprintf(stderr, "%s: load_redirect_fp: WARNING! Redirect table full (%d)!\n",
             logpfx(), nelts);
  }
  
  /* Sort the table to prepare it for binary searching. */
  qsort((char *)&remap[0], nelts, sizeof remap[0], (int (*)())cmp_strs);

  return nelts;
}
