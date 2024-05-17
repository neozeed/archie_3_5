/*  
 *  Input has the form:
 *  
 *  0 start height start height ... start 0
 *  
 *  We want to find the minimum and maximum values of the starts and heights,
 *  so we ignore the initial and final zeros.
 */

#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>


#define TRY(expr) \
do{if(!(expr)){fprintf(stderr,"%s: %s failed.\n",prog,#expr);exit(1);}}while(0)


static char *prog;


static void usage(void)
{
  fprintf(stderr, "Usage: %s <infix-trie>\n", prog);
  exit(1);
}


int main(int ac, char **av)
{
  FILE *fp;
  int i, n;
  long height, start;
  long statsMaxStart = LONG_MIN, statsMinStart = LONG_MAX;
  long statsMaxHeight = LONG_MIN, statsMinHeight = LONG_MAX;
  struct stat sbuf;

  prog = av[0];

  if (ac != 2) usage();

  TRY(fp = fopen(av[1], "rb"));
  TRY(fstat(fileno(fp), &sbuf) != -1);
  
  n = (sbuf.st_size / sizeof height - 3) / 2;

  TRY(fread(&height, sizeof height, 1, fp) > 0); /* height initial 0 */
  for (i = 0; i < n; i++) {
    TRY(fread(&start, sizeof start, 1, fp) > 0);
    start = -start - 1;
    if (start < statsMinStart) statsMinStart = start;
    if (start > statsMaxStart) statsMaxStart = start;

    TRY(fread(&height, sizeof height, 1, fp) > 0);
    if (height < statsMinHeight) statsMinHeight = height;
    if (height > statsMaxHeight) statsMaxHeight = height;
  }

  TRY(fread(&start, sizeof start, 1, fp) > 0);
  start = -start - 1;
  if (start < statsMinStart) statsMinStart = start;
  if (start > statsMaxStart) statsMaxStart = start;

  printf("Read %d actual values.\n", 2 * n + 1);
  printf("\tmin. start: %ld, max. start: %ld\n", statsMinStart, statsMaxStart);
  printf("\tmin. height: %ld, max. height: %ld\n", statsMinHeight, statsMaxHeight);

  fclose(fp);
  exit(0);
}
