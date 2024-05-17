#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "archstridx.h"
#include "Test.h"
#ifdef MEMADVISE
# include "memadv.h"
#endif


#define MB(x) ((x) * 1024 * 1024)


char *dbdir  = "/tmp";
char *in_strings = "Foo\nblah\nfoo\nzorklefeeder\n.\nOo\n";
char *in_append = "bla\noo\nOO\nblah\n";
char *prog;
int verbose = 1;


static void quit(int ret)
{
#ifdef MEMADVISE
  MaReportFdLeaks(MA_ERROR_FD);
#endif
  exit(ret);
}


static int isPlainFile(FILE *fp)
{
  struct stat s;

  if (fstat(fileno(fp), &s) == -1) {
    fprintf(stderr, "%s: isPlainFile: can't stat file: ", prog); perror("fstat");
    return 0;
  }

  return S_ISREG(s.st_mode);
}


static FILE *writeFile(const char *s)
{
  FILE *fp;

  if ( ! (fp = tmpfile())) {
    fprintf(stderr, "%s: writeFile: can't create temporary input file: ", prog);
    perror("tmpfile");
    quit(1);
  }

  if (fwrite(s, 1, strlen(s), fp) == 0) {
    fprintf(stderr, "%s: writeFile: can't write strings to temporary file: ", prog);
    perror("fwrite");
    quit(1);
  }

  if (fflush(fp) == EOF) {
    fprintf(stderr, "%s: writeFile: can't flush strings to temporary file: ", prog);
    perror("fflush");
    quit(1);
  }

  rewind(fp);

  return fp;
}


static void append(struct arch_stridx_handle *h)
{
  FILE *fp;
  char line[1024];
  
  if (isPlainFile(stdin)) {
    fp = stdin;
  } else {
    fp = writeFile(in_append);
  }

  if ( ! archOpenStrIdx(h, dbdir, ARCH_STRIDX_APPEND)) {
    quit(1);
  }

  while (fgets(line, sizeof line, fp)) {
    char *nl;
    unsigned long start;
    
    if ((nl = strchr(line, '\n'))) *nl = '\0';

    if (archAppendKey(h, line, &start)) {
      if (verbose) printf("%s at %lu.\n", line, start);
    } else {
      archCloseStrIdx(h); fclose(fp);
      quit(1);
    }
  }

  if ( ! archUpdateStrIdx(h)) {
    archCloseStrIdx(h); fclose(fp);
    quit(1);
  }

  archPrintStats(h);

  fclose(fp);
}


static void build(struct arch_stridx_handle *h)
{
  if ( ! archOpenStrIdx(h, dbdir, ARCH_STRIDX_BUILD)) {
    quit(1);
  }

  if ( ! archBuildStrIdx(h)) {
    quit(1);
  }
}


static void create(struct arch_stridx_handle *h, int idxChars)
{
  FILE *fp;
  char line[1024];

  if (isPlainFile(stdin)) {
    fp = stdin;
  } else {
    fp = writeFile(in_strings);
  }
  
  if ( ! archCreateStrIdx(h, dbdir,
                          idxChars ? ARCH_INDEX_CHARS : ARCH_INDEX_WORDS)) {
    quit(1);
  }

  while (fgets(line, sizeof line, fp)) {
    char *nl;
    unsigned long start;
    
    if ((nl = strchr(line, '\n'))) *nl = '\0';

    if (archAppendKey(h, line, &start)) {
      if (verbose) printf("%s at %lu.\n", line, start);
    } else {
      archCloseStrIdx(h); fclose(fp);
      quit(1);
    }
  }

  if ( ! archUpdateStrIdx(h)) {
    archCloseStrIdx(h); fclose(fp);
    quit(1);
  }

  archPrintStats(h);

  fclose(fp);
}


static void regex(struct arch_stridx_handle *h)
{
  char line[1024];
  
  if ( ! archOpenStrIdx(h, dbdir, ARCH_STRIDX_SEARCH)) {
    quit(1);
  }

  printf("Key: "); fflush(stdout);
  while (fgets(line, sizeof line, stdin)) {
    char *nl, *str;
    int i;
    unsigned long nhits, start[100];
    
    if ((nl = strchr(line, '\n'))) *nl = '\0';

    /*
     *  Case sensitive regular expression.
     */
    
    if ( ! archSearchRegex(h, line, 1 /* case sens */, 20, &nhits, start)) {
      fprintf(stderr, "%s: regex: error from archSearchRegex().\n", prog);
      archCloseStrIdx(h);
      quit(1);
    }

    if (nhits > 0) {
      printf("\n\t### Case sensitive regular expression matches ###\n\n");
      for (i = 0; i < nhits; i++) {
        if ( ! archGetString(h, start[i], &str)) {
          fprintf(stderr, "%s: regex: error from archGetString().\n", prog);
          archCloseStrIdx(h);
          quit(1);
        }
        printf("\t\t`%s'\n", str);
        free(str);
      }
    }

    /*
     *  Case insensitive regular expression.
     */
    
    if ( ! archSearchRegex(h, line, 0 /* case insens */, 20, &nhits, start)) {
      fprintf(stderr, "%s: regex: error from archSearchRegex().\n", prog);
      archCloseStrIdx(h);
      quit(1);
    }

    if (nhits > 0) {
      printf("\n\t### Case insensitive regular expression matches ###\n\n");
      for (i = 0; i < nhits; i++) {
        if ( ! archGetString(h, start[i], &str)) {
          fprintf(stderr, "%s: regex: error from archGetString().\n", prog);
          archCloseStrIdx(h);
          quit(1);
        }
        printf("\t\t`%s'\n", str);
        free(str);
      }
    }

    printf("\nKey: "); fflush(stdout);
  }
    
  if ( ! archUpdateStrIdx(h)) {
    archCloseStrIdx(h);
    quit(1);
  }
}


static void search(struct arch_stridx_handle *h)
{
  char line[1024];
  
  if ( ! archOpenStrIdx(h, dbdir, ARCH_STRIDX_SEARCH)) {
    quit(1);
  }

  /*  
   *  For each key the user enters peform three searches.
   */
  
  printf("Key: "); fflush(stdout);
  while (fgets(line, sizeof line, stdin)) {
    char *nl, *str;
    int i;
    unsigned long nhits, start[100];
    
    if ((nl = strchr(line, '\n'))) *nl = '\0';

    /*  
     *  Case sensitive exact search.
     */
    
    if ( ! archSearchExact(h, line, 1 /* case sens */, 20, &nhits, start)) {
      fprintf(stderr, "%s: search: error from archSearchExact().\n", prog);
      archCloseStrIdx(h);
      quit(1);
    }

    if (nhits > 0) {
      printf("\n\t### Case sensitive exact matches ###\n\n");
      for (i = 0; i < nhits; i++) {
        if ( ! archGetString(h, start[i], &str)) {
          fprintf(stderr, "%s: search: error from archGetString().\n", prog);
          archCloseStrIdx(h);
          quit(1);
        }
        printf("\t\t`%s'\n", str);
        free(str);
      }
    }
    
    /*  
     *  Case insensitive exact search.
     */
    
    if ( ! archSearchExact(h, line, 0 /* case insens */, 20, &nhits, start)) {
      fprintf(stderr, "%s: search: error from archSearchExact().\n", prog);
      archCloseStrIdx(h);
      quit(1);
    }

    if (nhits > 0) {
      printf("\n\t### Case insensitive exact matches ###\n\n");
      for (i = 0; i < nhits; i++) {
        if ( ! archGetString(h, start[i], &str)) {
          fprintf(stderr, "%s: search: error from archGetString().\n", prog);
          archCloseStrIdx(h);
          quit(1);
        }
        printf("\t\t`%s'\n", str);
        free(str);
      }
    }
    
    /*  
     *  Case sensitive substring search.
     */
    
    if ( ! archSearchSub(h, line, 1 /* case sens */, 20, &nhits, start)) {
      fprintf(stderr, "%s: search: error from archSearchSub().\n", prog);
      archCloseStrIdx(h);
      quit(1);
    }

    if (nhits > 0) {
      printf("\n\t### Case sensitive substring matches ###\n\n");
      for (i = 0; i < nhits; i++) {
        if ( ! archGetString(h, start[i], &str)) {
          fprintf(stderr, "%s: search: error from archGetString().\n", prog);
          archCloseStrIdx(h);
          quit(1);
        }
        printf("\t\t`%s'\n", str);
        free(str);
      }
    }
    
    /*  
     *  Case insensitive substring search.
     */
    
    if ( ! archSearchSub(h, line, 0 /* case insens */, 20, &nhits, start)) {
      fprintf(stderr, "%s: search: error from archSearchSub().\n", prog);
      archCloseStrIdx(h);
      quit(1);
    }

    if (nhits > 0) {
      printf("\n\t### Case insensitive substring matches ###\n\n");
      for (i = 0; i < nhits; i++) {
        if ( ! archGetString(h, start[i], &str)) {
          fprintf(stderr, "%s: search: error from archGetString().\n", prog);
          archCloseStrIdx(h);
          quit(1);
        }
        printf("\t\t`%s'\n", str);
        free(str);
      }
    }

    printf("\nKey: "); fflush(stdout);
  }

  if ( ! archUpdateStrIdx(h)) {
    archCloseStrIdx(h);
    quit(1);
  }
}


static void usage(void)
{
  fprintf(stderr, "Usage: %s "
          "[-dbdir <database-dir>] "
          "[-load <load-factor>] "
          "[-maxmem <#-bytes>] "
          "[-quiet] "
          "[-tmpdir <build-temp-dir>] "
          "-append | -build | -create | -regex | -search | -wordcreate\n", prog);
  quit(1);
}


int main(int ac, char **av)
{
  char *tmpdir = "/tmp";
  enum {TEST_NOTHING, TEST_APPEND, TEST_BUILD, TEST_CREATE,
        TEST_REGEX, TEST_SEARCH, TEST_WCREATE} test;
  double lf = -1.0;
  size_t maxmem = MB(10);
  struct arch_stridx_handle *h;

  prog = av[0];
  test = TEST_NOTHING;
  
  while (av++, --ac) {
    if      (strcmp(av[0], "-append") == 0)      test = TEST_APPEND;
    else if (strcmp(av[0], "-build")  == 0)      test = TEST_BUILD;
    else if (strcmp(av[0], "-create") == 0)      test = TEST_CREATE;
    else if (strcmp(av[0], "-dbdir")  == 0)      NEXTSTR(dbdir);
    else if (strcmp(av[0], "-load")   == 0)      NEXTDBL(lf);
    else if (strcmp(av[0], "-quiet")  == 0)      verbose = 0;
    else if (strcmp(av[0], "-maxmem") == 0)      NEXTUINT(maxmem);
    else if (strcmp(av[0], "-regex")  == 0)      test = TEST_REGEX;
    else if (strcmp(av[0], "-search") == 0)      test = TEST_SEARCH;
    else if (strcmp(av[0], "-tmpdir") == 0)      NEXTSTR(tmpdir);
    else if (strcmp(av[0], "-wordcreate") == 0)  test = TEST_WCREATE;
    else                                         usage();
  }
  
  if (test == TEST_NOTHING) usage();
  
  if ( ! ((h = archNewStrIdx()))) {
    quit(1);
  }

  if (lf != -1.0) {
    archSetHashLoadFactor(h, lf);
  }

  if ( ! archSetBuildMaxMem(h, maxmem) ||
       ! archSetTempDir(h, tmpdir)) {
    archFreeStrIdx(&h);
    quit(1);
  }

  switch (test) {
  case TEST_APPEND:  append(h);    break;
  case TEST_BUILD:   build(h);     break;
  case TEST_CREATE:  create(h, 1); break;
  case TEST_REGEX:   regex(h);     break;
  case TEST_SEARCH:  search(h);    break;
  case TEST_WCREATE: create(h, 0); break;
  case TEST_NOTHING: break;     /* not reached */
  }
  
  archCloseStrIdx(h);
  archFreeStrIdx(&h);

  quit(0);
}
