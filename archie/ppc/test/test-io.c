#include <stdio.h>
#include "ansi.h"
#include "defs.h"
#include "io.h"
#include "all.h"


const char *prog;


main(ac, av)
  int ac;
  char **av;
{
  prog = av[0];
  while ( ! feof(stdin) && ! ferror(stdin))
  {
    char *l;
    int badterm;

    l = getline(stdin, &badterm);
    if (l)
    {
      efprintf(stderr, "Line is `%s'.\n", l);
      free(l);
    }
    else
    {
      efprintf(stderr, "getline() failed.\n");
    }
    efprintf(stderr, "Line termination was %s.\n\n", badterm ? "bad" : "okay");
  }
}
