#include <stdio.h>
#include <stdlib.h>
#include "archstridx.h"


char *prog;


int main(int ac, char **av)
{
  int exists = -99, r;
  
  prog = av[0];

  if (ac != 2) {
    fprintf(stderr, "Usage: %s <db-dir>\n", prog);
    exit(1);
  }
  
  r = archStridxExists(av[1], &exists);

  printf("%d %d\n", r, exists);

  exit(0);
}
