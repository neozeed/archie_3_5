#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Test.h"
#include "defs.h"
#include "patrie.h"


void usage(void)
{
  fprintf(stderr,
          "Usage: %s [-batch] [-caseacc <n>] [-chunk <n>] [-dbacc i|s] [-dbcase i|s] "
          "[-help] [-levels <n>] [-maxhits <n>] [-pagesize <n>] [-quiet] "
          "<text-file> <paged-trie>\n", prog);
  fprintf(stderr, "\n");
  fprintf(stderr, "\t%-10s # read from stdin, don't prompt, print starts only\n", "-batch");
  fprintf(stderr, "\t%-10s # indicates case and accent sensitivity of the search\n", "-caseacc");
  fprintf(stderr, "\t%-10s # `i' or `s' indicates case sensitivity of the database\n", "-dbcase");
  fprintf(stderr, "\t%-10s # `i' or `s' for accent insensitive or accent sensitive build", "-dbacc");
  fprintf(stderr, "\t%-10s # return results in chunks of <n> hits\n", "-chunk");
  fprintf(stderr, "\t%-10s # print this message\n", "-help");
  fprintf(stderr, "\t%-10s # assume <n> trie levels per page\n", "-levels");
  fprintf(stderr, "\t%-10s # return at most <n> hits\n", "-maxhits");
  fprintf(stderr, "\t%-10s # assume a page size of <n> bytes\n", "-pagesize");
  fprintf(stderr, "\t%-10s # don't print stats, or debugging information\n", "-quiet");
  fprintf(stderr, "\n");
  exit(1);
}


char *prog;


int main(int ac, char **av)
{
  char *dbcase = 0, *key;
  char *dbacc = 0;
  int batch = 0, lpp, pps, quiet = 0;
  int caseacc = 3;                  /* exact */
  struct patrie_config *config;
  struct patrie_state *state;
  unsigned long chunk, maxhits = 100;
  unsigned long *start;

  prog = av[0];
  
  chunk = maxhits;

  patrieAlloc(&config);
  
  while (av++, --ac) {
    if      (ARG_IS("-batch"))    batch = 1;
    else if (ARG_IS("-caseacc"))     NEXTINT(caseacc);
    else if (ARG_IS("-chunk"))    NEXTINT(chunk);
    else if (ARG_IS("-dbcase"))   NEXTSTR(dbcase);
    else if (ARG_IS("-dbacc"))   NEXTSTR(dbacc);
    else if (ARG_IS("-help"))     usage();
    else if (ARG_IS("-levels"))   {NEXTINT(lpp); patrieSetLevelsPerPage(config, lpp);}
    else if (ARG_IS("-maxhits"))  NEXTINT(maxhits);
    else if (ARG_IS("-pagesize")) {NEXTINT(pps); patrieSetPagedPageSize(config, pps);}
    else if (ARG_IS("-quiet"))    quiet = 1;
    else                          break;
  }

  if (ac != 2) {
    patrieFree(&config);
    usage();
  }

  if( caseacc > 3 ){
    fprintf(stderr, "%s: invalid value for `-case' parameter.\n", prog);
    patrieFree(&config);
    usage();
  }
  patrieSetDebug(config, ! quiet);
  patrieSetStats(config, ! quiet);
  patrieSetAssert(config, 1);
  
  patrieSetTextFP(config, openFile(av[0], "rb"));
  patrieSetPagedFP(config, openFile(av[1], "rb"));
  
  if (dbcase) {
    if ((strcmp(dbcase, "i") == 0) && (strcmp(dbacc, "i") == 0))
       patrieSetCaseSensitive(config, 0);
    else if  ((strcmp(dbcase, "i") == 0) && (strcmp(dbacc, "s") == 0))
             patrieSetCaseSensitive(config, 1);
    else if  ((strcmp(dbcase, "s") == 0) && (strcmp(dbacc, "i") == 0))
             patrieSetCaseSensitive(config, 2);
    else if  ((strcmp(dbcase, "s") == 0) && (strcmp(dbacc, "s") == 0))
             patrieSetCaseSensitive(config, 3);
    else {
      fprintf(stderr, "%s: invalid value for `-dbcase' or `-dbacc' parameter.\n", prog);
      patrieFree(&config);
      usage();
    }
  }

  ASSERT((start = malloc(chunk * sizeof *start)));

  if ( ! batch) printf("Key: ");
  while ((key = getSearchKey())) {
    unsigned long hits;

    if ( ! patrieSearchSub(config, key, caseacc, chunk, &hits, start, &state)) {
      printf("*** search failure ***.\n");
    } else {
      unsigned long tothits = 0;
      
      while (hits > 0 && tothits < maxhits) {
        tothits += hits;
        
        if (batch) {
          printStarts(patrieGetTextFP(config), hits, start);
        } else {
          printStrings(patrieGetTextFP(config), key, hits, start);
        }

        if ( ! patrieGetMoreStarts(config, chunk, &hits, start, state)) {
          fprintf(stderr, "%s: error from patrieGetMoreStarts().\n", prog);
          break;
        }
      }
      patrieFreeState(&state);
    }

    if ( ! batch) printf("Key: ");
  }

  free(start);
  fclose(patrieGetTextFP(config)); fclose(patrieGetPagedFP(config));
  patrieFree(&config);

  exit(0);
}
