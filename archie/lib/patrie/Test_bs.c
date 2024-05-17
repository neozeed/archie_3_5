/*  
 *  Repeatedly test the building and searching of patrie indices.
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

#define INFIX_FILE "/tmp/Test_bs.infix"
#define LEVEL_FILE "/tmp/Test_bs.levs"
#define PAGED_FILE "/tmp/Test_bs.page"
#define TEXT_FILE  "/tmp/Test_bs.text"


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


/*  
 *  Fill the buffer with `tsz' random characters.  Append a nul to the
 *  buffer, which is tsz+1 bytes in size.
 */
static void randText(size_t tsz, char *tbuf)
{
  size_t i;
  
  for (i = 0; i < tsz; i++) {
    tbuf[i] = randByte();
  }
  tbuf[i] = '\0';
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
static int srch_exist(struct patrie_config *cf, size_t tsz, char *tbuf)
{
  size_t i;

  for (i = 0; i < MAXTRIES; i++) {
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
 *  Similar to srch_exist(), but we insert, into the key, a character known
 *  not to be in the text.  [It might be better to generate keys, of valid
 *  characters, that don't appear in the text, but that seems harder...]
 */
static int srch_nonexist(struct patrie_config *cf, size_t tsz, char *tbuf)
{
  size_t i;

  for (i = 0; i < MAXTRIES; i++) {
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


static void initPatrie(struct patrie_config **cf,
                       int dbg, int caseSens, int keepSort,
                       FILE *ifp, FILE *lfp, FILE *pfp, FILE *tfp)
{
  if ( ! patrieAlloc(cf)) {
    fprintf(stderr, "%s: initPatrie: error from patrieAlloc().\n", prog);
    exit(1);
  }

  patrieSetCaseSensitive(*cf, caseSens);
  patrieSetCreateInfix(*cf, 1, 0, keepSort);

  if (dbg) {
    patrieSetDebug(*cf, 1);
    patrieSetStats(*cf, 1);
    patrieSetAssert(*cf, 1);
  }
  
  patrieSetInfixFP(*cf, ifp);
  patrieSetLevelFP(*cf, lfp);
  patrieSetPagedFP(*cf, pfp);
  patrieSetTextFP(*cf, tfp);
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


static void sigint(int signo)
{
  done = 1;
}


static void usage(void)
{
#define P(opt, desc) fprintf(stderr, "\t%-10s # " desc "\n", opt);

  fprintf(stderr, "Usage: %s [-atsz <file-lenght>] [-debug]\n", prog);

  fprintf(stderr, "\n");
  P("-atsz",     "average text file size");
  P("-case",     "build case insensitive indices");
  P("-debug",    "print debugging information");
  P("-help",     "print this message");
  P("-keep",     "keep the temporary sort files");
  fprintf(stderr, "\n");

  exit(1);

#undef P
}


int main(int ac, char **av)
{
  FILE *ifp, *lfp, *pfp, *tfp;
  int caseSens = 1, keepSort = 0;
  long avgtsz = AVG_TEXT_SZ, count = 0;
  struct patrie_config *cf;

  prog = av[0];

  while (av++, --ac) {
    if      (ARG_IS("-atsz"))  NEXTLONG(avgtsz);
    else if (ARG_IS("-case"))  caseSens = 0;
    else if (ARG_IS("-debug")) debug = 1;
    else if (ARG_IS("-help"))  usage();
    else if (ARG_IS("-keep"))  keepSort = 1;
    else                       break;
  }
  
  if (ac != 0) {
    usage();
  }

  ifp = openFile(INFIX_FILE, "w+b");
  lfp = openFile(LEVEL_FILE, "w+b");
  pfp = openFile(PAGED_FILE, "w+b");
  tfp = openFile(TEXT_FILE,  "w+");

  signal(SIGINT, sigint);

  while ( ! done) {
    char *tbuf;
    long tsz = randTextSize(avgtsz);

    initPatrie(&cf, debug, caseSens, keepSort, ifp, lfp, pfp, tfp);

    while ( ! (tbuf = malloc(tsz+1))) {
      fprintf(stderr, "%s: can't allocate %lu bytes for text buffer (sleeping): ",
              prog, (unsigned long)tsz); perror("malloc");
      sleep(60);
    }

    /*  
     *  Fill the text buffer with random data.
     */

    randText(tsz, tbuf);

    while ((fwrite(tbuf, 1, tsz, tfp)) == 0) {
      fprintf(stderr, "%s: error writing text file (sleeping): ", prog);
      perror("fwrite");
      sleep(60);
    }
    fflush(tfp);

    rewind(tfp);

    /*  
     *  Build a patrie from the random text.
     */

    if ( ! patrieBuild(cf)) {
      fprintf(stderr, "%s: error building patrie.\n", prog);
      exit(1);
    }

    /*  
     *  Search the index for a lot of existing substrings.
     */

    srch_exist(cf, tsz, tbuf);

    /*  
     *  Search the index for a lot of non-existant substrings.
     */

    srch_nonexist(cf, tsz, tbuf);

    /*  
     *  Prepare for another round...
     */

    free(tbuf);
    patrieFree(&cf);

    rewind(ifp); rewind(lfp);
    rewind(pfp); rewind(tfp);

    ftruncate(fileno(ifp), 0); ftruncate(fileno(lfp), 0);
    ftruncate(fileno(pfp), 0); ftruncate(fileno(tfp), 0);

    if (debug) {
      fprintf(stderr, "%s: ---------------------------------------------------------------\n",
              prog);
    } else {
      printCount(avgtsz, count++);
    }
  }

  fprintf(stderr, "\n");

  exit(0);
}
