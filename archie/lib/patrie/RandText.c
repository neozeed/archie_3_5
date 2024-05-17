/*  
 *  Create a text file of random characters.
 */

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Test.h"
#include "defs.h"
#include "patrie.h"


#define AVG_TEXT_SZ 100000

#define MAX_CHAR 255
#define MIN_CHAR 2          /* Must be at least 2 */

#define MAXHITS 500
#define MAXKEYLEN 50
#define MAXTRIES MAX(1, tsz / 8)


char *prog;
int debug = 0;
volatile int done = 0;


/*  
 *  Return a number in the range [min, max).
 */
static long randRange(long min, long max)
{
  long r = max - min;
  return r ? min + (rand() % r) : min;
}


/*  
 *  Return a random, non-nul character.
 */
static char randByte(void)
{
  return randRange(MIN_CHAR, MAX_CHAR);
}


/*  
 *  Return a random number near `avgsz'.
 */
static long randTextSize(long avgsz)
{
  return avgsz + randRange(-avgsz / 10, avgsz / 10);
}


static void usage(void)
{
#define P(opt, desc) fprintf(stderr, "\t%-10s # " desc "\n", opt);

  fprintf(stderr, "Usage: %s [-atsz <file-lenght>] [-help]\n", prog);

  fprintf(stderr, "\n");
  P("-atsz",     "average text file size");
  P("-help",     "print this message");
  fprintf(stderr, "\n");

  exit(1);

#undef P
}


int main(int ac, char **av)
{
  long tsz;
  long avgtsz = AVG_TEXT_SZ;
  size_t i;

  prog = av[0];

  while (av++, --ac) {
    if      (ARG_IS("-atsz"))  NEXTLONG(avgtsz);
    else if (ARG_IS("-help"))  usage();
    else                       break;
  }
  
  tsz = randTextSize(avgtsz);

  for (i = 0; i < tsz; i++) {
    if (putchar(randByte()) == EOF) {
      fprintf(stderr, "%s: ", prog); perror("putchar");
      exit(1);
    }
  }
  
  exit(0);
}
