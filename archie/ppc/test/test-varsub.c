#include <stdio.h>
#ifdef __STDC__
# include <stdlib.h>
#endif
#include "varsub.h"


int main(ac, av)
  int ac;
  char **av;
{
  char *dst;
  char cmp[512];
  char line[512];
  char replc[512];
  char src[512];
  char str[512];

  while (fgets(line, sizeof line, stdin) != (char *)0)
  {
    if (*line == '#') continue;
    
    if (sscanf(line, "%s %s %s %s", src, str, replc, cmp) != 4)
    {
      fprintf(stderr, "%s: bad input line `%s'.\n", av[0], line);
      exit(1);
    }

    dst = varsub(src, str, replc);
    if (strcmp(dst, cmp) != 0) abort();
    free(dst);
  }

  exit(0);
}
