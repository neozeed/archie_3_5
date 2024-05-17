/*  
 *  Keep a table of network addresses allowed to connect.
 */  

#ifdef __STDC__
# include <stdlib.h>
# include <string.h> /* memcmp() */
#else
# include <memory.h> /* memcmp() */
# include <search.h>
#endif
#include <stdio.h>
#include "error.h"
#include "host_access.h"
#include "misc.h"
#include "net.h"
#include "protos.h"
#include "str.h"
#include "all.h"


#define TSZ 4096


static int all_allowed = 1;
static int nelts = 0;
static struct in_addr acl[TSZ]; /* bug: make dynamic */


static int cmp_in_addr proto_((char *a, char *b));


static int cmp_in_addr(a, b)
  char *a;
  char *b;
{
  return memcmp(a, b, sizeof acl[0]);
}


int is_host_allowed(host)
  struct in_addr host;
{
  struct in_addr net;

  if (all_allowed)
  {
    return 1;
  }

  /*  
   *  First check whether this host's network is allowed access, and if
   *  not, check whether this particular host has been granted access.
   */  
  net = network(host);
  return (!!bsearch((char *)&net, (char *)&acl[0], (unsigned)nelts,
                    sizeof net, (int (*)())cmp_in_addr) ||
          !!bsearch((char *)&host, (char *)&acl[0],
                    (unsigned)nelts, sizeof net, (int (*)())cmp_in_addr));
}


/*  
 *  Load the ACL list from the file `aclfile'.
 *  
 *  Return the number of hosts loaded into the ACL table, or -1 on an error.
 */
int load_host_acl_file(aclfile)
  const char *aclfile;
{
  FILE *afp;
  int ret;
  
  all_allowed = 0;
  if ( ! (afp = fopen(aclfile, "r")))
  {
    efprintf(stderr, "%s: load_host_acl_file: can't open `%s': %m.\n",
             logpfx(), aclfile);
    return -1;
  }

  ret = load_host_acl_fp(afp);

  fclose(afp);
  return ret;
}


/*  
 *  Load an ACL list from the stream `afp'.
 *  
 *  Return the number of hosts loaded into the ACL table, or -1 on an error.
 */    
int load_host_acl_fp(afp)
  FILE *afp;
{
  char host[64];
  
  all_allowed = 0;
  for (nelts = 0; nelts < TSZ; nelts++)
  {
    host[0] = '\0';
    if ( ! fgets(host, sizeof host, afp))
    {
      if (ferror(afp))
      {
        efprintf(stderr, "%s: load_host_acl_fp: fgets() failed: %m.\n", logpfx());
        fclose(afp);
        return -1;
      }
      break;
    }

    strterm(host, '\n');
    acl[nelts] = net_addr(host);
  }

  if ( ! feof(afp))
  {
    efprintf(stderr, "%s: load_host_acl_fp: WARNING! Network ACL table full (%d)!\n",
             logpfx(), nelts);
  }
  
  /* Sort the table to prepare it for binary searching. */
  qsort((char *)&acl[0], nelts, sizeof acl[0], (int (*)())cmp_in_addr);

  return nelts;
}
