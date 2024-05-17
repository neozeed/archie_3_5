#include <stdio.h>
#include "misc.h"
#include "str.h"
#include "all.h"


const char *prog;
int debug;


main(ac, av)
  int ac;
  char **av;
{
  char *rest;
  char *s[64];
  int r;
  
  prog = av[0];
  while (av++, --ac)
  {
    r = qwstrnsplit(av[0], s, 3, &rest);
    if (r == 0)
    {
      printf("%s: qwstrnsplit() returned 0 on `%s' (rest is `%s').\n",
             prog, av[0], rest);
    }
    else
    {
      int i;
      
      printf("%s: %d substring(s):", prog, r);
      for (i = 0; i < r; i++)
      {
        printf(" `%s'", s[i]);
      }
      printf("; rest is `%s'.\n", rest);
    }
  }

  return 0;
}
