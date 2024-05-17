#include <stdio.h>
#include "Test.h"


char *prog;


static void getPair(FILE *fp0, FILE *fp1, unsigned long n, long *l0, long *l1)
{
  if (fread(l0, sizeof *l0, 1, fp0) == 0) {
    if (fread(l1, sizeof *l1, 1, fp1) != 0) {
      fprintf(stderr, "%s: file 0 ended early at long #%lu (from 0).\n", prog, n);
      exit(1);
    }
    exit(0);
  }

  if (fread(l1, sizeof *l1, 1, fp1) == 0) {
    fprintf(stderr, "%s: file 1 ended early at long #%lu (from 0).\n", prog, n);
    exit(1);
  }
}

    
static void usage(void)
{
  fprintf(stderr, "Usage: %s [-help] <infix-trie-0> <infix-trie-1>\n", prog);
  fprintf(stderr, "\n");
  fprintf(stderr, "\t-help\t# print this message\n");
  fprintf(stderr, "\n");
  exit(1);
}


int main(int ac, char **av)
{
  FILE *fp0, *fp1;
  long l0, l1, prev;
  unsigned long n = 0;
  
  prog = av[0];

  while (av++, --ac) {
    if (ARG_IS("-help")) usage();
    else                 break;
  }

  if (ac != 2) usage();

  fp0 = openFile(av[0], "rb");
  fp1 = openFile(av[1], "rb");

  getPair(fp0, fp1, n, &l0, &l1);
  n++;

  while (1) {
    prev = l0;
    getPair(fp0, fp1, n, &l0, &l1);

    if (l0 != l1) {
      fprintf(stderr, "%s: %ld != %ld at long %lu (from 0).\n", prog, l0, l1, n);
      fprintf(stderr, "%s: previous value was %ld.\n", prog, prev);
      exit(1);
    }

    n++;
  }

  exit(0);
}
