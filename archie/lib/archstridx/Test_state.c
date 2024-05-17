#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "archstridx.h"


#define DB_DIR     "/tmp"
#define NHITS      20


char *prog;


static void onesearch(const char *key,
                      int (*fn)(struct arch_stridx_handle *h,
                                const char *key,
                                int case_sens,
                                unsigned long maxhits,
                                unsigned long *nhits,
                                unsigned long starts[]),
                      const char *fn_name,
                      int case_sens,
                      const char *title)
{
  char *str;
  const char *dbdir = DB_DIR;
  int i;
  struct arch_stridx_handle *h;
  unsigned long n, hits, start[100];
    
  if ( ! (h = archNewStrIdx())) {
    exit(1);
  }
  
  if ( ! archOpenStrIdx(h, dbdir, ARCH_STRIDX_SEARCH)) {
    exit(1);
  }

  if ( ! fn(h, key, case_sens, 1, &n, start)) {
    fprintf(stderr, "%s: search: error from %s().\n", prog, fn_name);
    archCloseStrIdx(h); archFreeStrIdx(&h);
    exit(1);
  }

  hits = n;

  while (hits < NHITS && n > 0) {
    char *ss;
      
    if ( ! archGetStateString(h, &ss)) {
      fprintf(stderr, "%s: search: error from archGetStateString().\n", prog);
      archCloseStrIdx(h); archFreeStrIdx(&h);
      exit(1);
    }

    /*
     *  Close and reopen the database, as if we were a different invocation of
     *  the program, in order to reset `h'.
     */
    
    archCloseStrIdx(h); archFreeStrIdx(&h);

    if ( ! (h = archNewStrIdx())) {
      exit(1);
    }
  
    if ( ! archOpenStrIdx(h, dbdir, ARCH_STRIDX_SEARCH)) {
      exit(1);
    }

    if ( ! archSetStateFromString(h, ss)) {
      fprintf(stderr, "%s: search: error from archSetStateFromString().\n", prog);
      archCloseStrIdx(h); archFreeStrIdx(&h); free(ss);
      exit(1);
    }

    free(ss);

    if ( ! archGetMoreMatches(h, 1, &n, start + hits)) {
      fprintf(stderr, "%s: search: error from archGetMoreMatches().\n", prog);
      archCloseStrIdx(h); archFreeStrIdx(&h); free(ss);
      exit(1);
    }

    hits += n;
  }
    
  if (hits > 0) {
    printf("\n\t%s\n\n", title);
    for (i = 0; i < hits; i++) {
      if ( ! archGetString(h, start[i], &str)) {
        fprintf(stderr, "%s: search: error from archGetString().\n", prog);
        archCloseStrIdx(h); archFreeStrIdx(&h);
        exit(1);
      }
      printf("\t\t%-7lu\t`%s'\n", start[i], str);
      free(str);
    }
  }
    
  archCloseStrIdx(h);
  archFreeStrIdx(&h);
}


static void search(void)
{
  char line[1024];
  
  /*  
   *  For each key the user enters peform three searches.
   */
  
  printf("Key: "); fflush(stdout);
  while (fgets(line, sizeof line, stdin)) {
    char *nl;
    
    if ((nl = strchr(line, '\n'))) *nl = '\0';

    onesearch(line, archSearchExact, "archSearchExact", 1, "Case sensitive exact");
    onesearch(line, archSearchExact, "archSearchExact", 0, "Case insensitive exact");

    onesearch(line, archSearchSub, "archSearchSub", 1, "Case sensitive substring");
    onesearch(line, archSearchSub, "archSearchSub", 0, "Case insensitive substring");

    onesearch(line, archSearchRegex, "archSearchRegex", 1, "Case sensitive regex");
    onesearch(line, archSearchRegex, "archSearchRegex", 0, "Case insensitive regex");

    printf("\nKey: "); fflush(stdout);
  }
}


int main(int ac, char **av)
{
  prog = av[0];

  search();
  
  exit(0);
}
