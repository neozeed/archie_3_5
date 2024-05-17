/*  
 *  Usage: test-access <access-file>
 *
 *  <access-file> is the name of a file containing host or network addresses,
 *  one per line.
 *
 *  After the list of allowed addresses is loaded stdin is read for a list of
 *  host or network addresses, one per line, to be checked against the table.
 */  
#ifdef __STDC__
# include <stdlib.h>
#endif
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "host_access.h"
#include "net.h"
#include "str.h"
#include "all.h"


const char *prog;
int debug;


int main(ac, av)
  int ac;
  char **av;
{
  char host[32];
  int n;
  
  prog = av[0];
  if (ac != 2)
  {
    efprintf(stderr, "Usage: %s <access-file>\n", prog);
    exit(1);
  }
  
  if ((n = load_host_acl(av[1])) != -1)
  {
    efprintf(stderr, "%s: loaded %d hosts.\n", logpfx(), n);
  }
  else
  {
    efprintf(stderr, "%s: load_host_acl() failed.\n", logpfx());
    exit(1);
  }
  
  while (fgets(host, sizeof host, stdin))
  {
    struct in_addr in;
    
    strterm(host, '\n');
    in = net_addr(host);
    printf("%s may%s connect.\n", host, is_host_allowed(in) ? "" : " not");
  }

  exit(0);
}
