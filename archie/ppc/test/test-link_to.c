#ifdef __STDC__
# include <stdlib.h>
#endif
#include <stdio.h>
#include "all.h"
#include "waisattrib.h"


const char *prog;
int debug;


int main(ac, av)
  int ac;
  char **av;
{
  prog = av[0];
  if (ac == 3)
  {
    char *desc;
    char *path;
    char *type;
    char *prec[64];
    int n;
    
    n = strnsplit(av[2], prec, 64, 0);
    prec[n] = 0;

    if ( ! link_to(av[1], (const char **)prec, &type, &path, &desc))
    {
      fprintf(stderr, "%s: link_to() failed.\n", prog);
    }
    else
    {
      printf("%s: chose (%s, %s, %s).\n", prog, type, path, desc);
      free(desc); free(path); free(type);
    }
  }

  return 0;
}
