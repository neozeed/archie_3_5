/*  
 *  Repeatedly test the searching of patrie indices.
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
#include "utils.h"


#define MAX_CHAR 255
#define MIN_CHAR 2          /* Must be at least 2 */

#define MAXHITS 500
#define MAXKEYLEN 50
#define MAXTRIES MAX(1, tsz / 8)


char *prog;
int debug = 0;


/*  
 *  Return a number in the range [min, max).
 */
static long randRange(long min, long max)
{
  long r = max - min;
  return r ? min + (rand() % (max - min)) : 0;
}


static int cmpstart(void *a, void *b)
{
  unsigned long x = *(unsigned long *)a;
  unsigned long y = *(unsigned long *)b;

  return x == y ? 0 : (x < y ? -1 : 1);
}


/*
 *  Search for some strings known to be in the text.
 *  
 *  Do it by searching for random chunks grabbed from the text.
 */
static int srch_exist(struct patrie_config *cf, size_t n, size_t tsz, char *tbuf)
{
  size_t i;

  for (i = 0; i < n; i++) {
    char key[MAXKEYLEN + 1];
    char *s = tbuf + randRange(0, tsz - 1);
    size_t j;
    size_t len = randRange(1, MIN(MAXKEYLEN, tbuf + tsz - s));
    unsigned long hits;
    unsigned long start[MAXHITS];

    strncpy(key, s, len); key[len] = '\0';

    ASSERT(patrieSearchSub(cf, key, 1, MAXHITS, &hits, start, 0));
    ASSERT(hits > 0);

    qsort(start, hits, sizeof start[0], cmpstart);

    /* Verify the starts. */
    
    for (j = 0, s = tbuf; j < hits; j++, s++) {
      ASSERT(s = strstr(s, key));
      ASSERT(start[j] == s - tbuf);
    }

    /* Check that we didn't miss one. */

    if (hits < MAXHITS) {
      ASSERT(strstr(s, key) == 0);
    }
  }

  return 1;
}


/*
 *  Similar to srch_exist(), but we insert, into the key, a character known to
 *  not be in the text.  [It might be better to generate keys, of valid
 *  characters, that don't appear in the text, but that seems harder...]
 */
static int srch_nonexist(struct patrie_config *cf, size_t n, size_t tsz, char *tbuf)
{
  size_t i;

  for (i = 0; i < n; i++) {
    char key[MAXKEYLEN + 1];
    char *s = tbuf + randRange(0, tsz - 1);
    size_t len = randRange(1, MIN(MAXKEYLEN, tbuf + tsz - s));
    unsigned long hits;
    unsigned long start[MAXHITS];

    strncpy(key, s, len); key[len] = '\0';
    key[randRange(0, len - 1)] = MIN_CHAR - 1;

    ASSERT(patrieSearchSub(cf, key, 1, MAXHITS, &hits, start, 0));
    ASSERT(hits == 0);
  }

  return 1;
}


static void sigint(int signo)
{
  exit(0);
}


static void printCount(long tsz, long count)
{
  if (tsz < 50) {
    if (count % 100 == 0) fprintf(stderr, "  %ld\r", count);
  } else if (tsz < 500) {
    if (count % 10 == 0) fprintf(stderr, "  %ld\r", count);
  } else {
    fprintf(stderr, "  %ld\r", count);
  }
}


static void usage(void)
{
#define P(opt, desc) fprintf(stderr, "\t%-10s # " desc "\n", opt);

  fprintf(stderr, "Usage: %s [-debug] [-help] <text-file> <patrie-file>\n", prog);

  fprintf(stderr, "\n");
  P("-debug",    "print debugging information");
  P("-help",     "print this message");
  fprintf(stderr, "\n");

  exit(1);

#undef P
}


int main(int ac, char **av)
{
  FILE *pfp, *tfp;
  char *tbuf;
  int caseSens = 1;
  size_t n, tsz;
  struct patrie_config *cf;

  prog = av[0];

  while (av++, --ac) {
    if      (ARG_IS("-case"))  caseSens = 0;
    else if (ARG_IS("-debug")) debug = 1;
    else if (ARG_IS("-help"))  usage();
    else                       break;
  }
  
  if (ac != 2) {
    usage();
  }

  tfp = openFile(av[0], "r");
  pfp = openFile(av[1], "rb");

  if ( ! patrieAlloc(&cf)) {
    fprintf(stderr, "%s: initPatrie: error from patrieAlloc().\n", prog);
    exit(1);
  }

  patrieSetPagedFP(cf, pfp);
  patrieSetTextFP(cf, tfp);

  patrieSetCaseSensitive(cf, caseSens);

  if (debug) {
    patrieSetDebug(cf, 1);
    patrieSetStats(cf, 1);
    patrieSetAssert(cf, 1);
  }
  
  tsz = _patrieFpSize(tfp);

  if ( ! (tbuf = malloc(tsz+1))) {
    fprintf(stderr, "%s: can't allocate %lu bytes for text buffer: ",
            prog, (unsigned long)tsz); perror("malloc");
    exit(1);
  }

  if (fread(tbuf, 1, tsz, tfp) != tsz) {
    fprintf(stderr, "%s: can't read %lu bytes from text file.\n",
            prog, (unsigned long)tsz); perror("fread");
    exit(1);
  }
    
  signal(SIGINT, sigint);

  /*
   *  Calculate the number of searches to perform.
   */
  
  n = MAX(1, tsz / 8);

  /*
   *  Search the index for a lot of existing substrings.
   */

  srch_exist(cf, n, tsz, tbuf);

  /*
   *  Search the index for a lot of non-existant substrings.
   */

  srch_nonexist(cf, n, tsz, tbuf);

  free(tbuf);
  patrieFree(&cf);
  fclose(pfp); fclose(tfp);

  exit(0);
}
