/*
 *  Read in a chunk of text, then ask the user for regular expression for
 *  which to search.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if defined(USE_GNU_RX)
# include "rxposix.h"
#elif defined(USE_BSD_REGEX)
# include "regex.h"
#endif


char *prog;


static char *readFile(char *name, size_t *tlen)
{
  FILE *fp;
  char *t;
  long pgsz;
  size_t mem;
  struct stat s;
  
#ifdef _SC_PAGESIZE

  if ((pgsz = sysconf(_SC_PAGESIZE)) == -1) {
    fprintf(stderr, "%s: readFile: can't get memory page size: ",
            prog); perror("sysconf");
    return 0;
  }

#else

  if ((pgsz = getpagesize()) == -1) {
    fprintf(stderr, "%s: readFile: can't get memory page size: ",
            prog); perror("getpagesize");
    return 0;
  }

#endif

  if ( ! (fp = fopen(name, "r"))) {
    fprintf(stderr, "%s: readFile: can't open `%s': ",
            prog, name); perror("fopen");
    return 0;
  }

  if (fstat(fileno(fp), &s) == -1) {
    fprintf(stderr, "%s: readFile: can't stat `%s': ",
            prog, name); perror("fstat");
    fclose(fp);
    return 0;
  }

  mem = s.st_size + (pgsz - (s.st_size % pgsz)) + 1;

#if 0
  fprintf(stderr, "pgsz %ld, alloc %lu\n", pgsz, (unsigned long)mem);
#endif

  if ( ! (t = valloc(mem))) {
    fprintf(stderr, "%s: readFile: can't allocate %lu bytes: ",
            prog, (unsigned long)mem); perror("valloc");
    fclose(fp);
    return 0;
  }

  t[s.st_size] = '\0';

  {
    int flags = MAP_SHARED | MAP_FIXED;
    int prot = PROT_READ;
    
    if (mmap(t, s.st_size, prot, flags, fileno(fp), 0) == (caddr_t)-1) {
      fprintf(stderr, "%s: readFile: can't map file at location %p: ",
              prog, (void *)t); perror("mmap");
      fclose(fp);
      return 0;
    }
  }
  
#if 1
  fclose(fp);
#endif

  *tlen = s.st_size;

  return t;
}


/*
 *  Return the index, into `txt', of the first newline following `txt[off]'.
 */
static size_t skipWord(char *txt, size_t off)
{
  while (txt[++off] != '\n') {
    continue;
  }

  return off;
}


static void printWordContaining(char *txt, size_t off)
{
  while (txt[off] != '\n') {
    --off;
  }

  off++;

  printf("\t%lu\t", (unsigned long)off);
  while (txt[off] != '\n') {
    putchar(txt[off++]);
  }
  putchar('\n');
}


#if defined(USE_BSD_REGEX)

static void searchRE(char *re, char *txt, size_t tlen)
{
  int err;
  int re_flags = REG_NEWLINE | REG_ICASE | REG_EXTENDED;
  regex_t cre; /* compiled regular expression */

  if ((err = regcomp(&cre, re, re_flags)) != 0) {
    char ebuf[512];

    regerror(err, &cre, ebuf, sizeof ebuf);
    fprintf(stderr, "%s: searchRE: invalid regular expression: %s.\n", prog, ebuf);
  } else {
    regmatch_t rem[1];

    rem[0].rm_so = 0;
    rem[0].rm_eo = tlen;

    while (regexec(&cre, txt, 1, rem, REG_STARTEND) == 0) {
      printWordContaining(txt, rem[0].rm_so);
      rem[0].rm_so = skipWord(txt, rem[0].rm_so);
      rem[0].rm_eo = tlen;
    }
  }

  regfree(&cre);
}


#elif defined(USE_GNU_RX)


static void searchRE(char *re, char *txt, size_t tlen)
{
  int err;
  int re_flags = REG_NEWLINE | REG_ICASE | REG_EXTENDED;
  regex_t cre; /* compiled regular expression */

  if ((err = regcomp(&cre, re, re_flags)) != 0) {
    char ebuf[512];

    regerror(err, &cre, ebuf, sizeof ebuf);
    fprintf(stderr, "%s: searchRE: invalid regular expression: %s.\n", prog, ebuf);
  } else {
    regmatch_t rem[1];
    size_t off = 0;

    while (regexec(&cre, txt + off, 1, rem, 0) == 0) {
      printWordContaining(txt, off + rem[0].rm_so);
      off = skipWord(txt, off + rem[0].rm_so);
    }
  }

  regfree(&cre);
}


#endif


static void usage(void)
{
  fprintf(stderr, "Usage: %s <text-file>\n", prog);
  exit(1);
}


int main(int ac, char **av)
{
  char re[128], *txt;
  size_t tlen;
  
  prog = av[0];
  
  if (ac != 2) usage();

  if ( ! (txt = readFile(av[1], &tlen))) {
    exit(1);
  }

  printf("Key: ");
  while (fgets(re, sizeof re, stdin)) {
    char *nl = strchr(re, '\n');
    if (nl) *nl = '\0';

    if (re[0] != '\0') {
      searchRE(re, txt, tlen);
    }

    printf("Key: ");
  }

  exit(0);
}
