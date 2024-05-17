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
          "Usage: %s [-batch] [-case] [-dbcase i|s] [-help] [-levels <n>] "
          "[-maxhits <n>] [-pagesize <n>] [-quiet] "
          "<text-file> <paged-trie>\n", prog);
  fprintf(stderr, "\n");
  fprintf(stderr, "\t%-10s # read from stdin, don't prompt, print starts only\n", "-batch");
  fprintf(stderr, "\t%-10s # perform a case insensitive search\n", "-case");
  fprintf(stderr, "\t%-10s # `i' or `s' indicates case sensitivity of the database\n", "-dbcase");
  fprintf(stderr, "\t%-10s # print this message\n", "-help");
  fprintf(stderr, "\t%-10s # assume <n> trie levels per page\n", "-levels");
  fprintf(stderr, "\t%-10s # assume a page size of <n> bytes\n", "-pagesize");
  fprintf(stderr, "\t%-10s # don't print stats, or debugging information\n", "-quiet");
  fprintf(stderr, "\n");
  exit(1);
}


char *prog;


int main(int ac, char **av)
{
  char *dbcase = 0, *key;
  int batch = 0, lpp, maxhits = 100, pps, quiet = 0;
  int casesrch = 1;
  struct patrie_config *config;
  struct patrie_state *state;
  unsigned long *starts;
  unsigned long hits;

  prog = av[0];
  
  patrieAlloc(&config);
  patrieAllocState(&state);
  
  while (av++, --ac) {
    if      (ARG_IS("-batch"))    batch = 1;
    else if (ARG_IS("-case"))     casesrch = 0;
    else if (ARG_IS("-dbcase"))   NEXTSTR(dbcase);
    else if (ARG_IS("-help"))     usage();
    else if (ARG_IS("-levels"))   {NEXTINT(lpp); patrieSetLevelsPerPage(config, lpp);}
    else if (ARG_IS("-maxhits"))  NEXTINT(maxhits);
    else if (ARG_IS("-pagesize")) {NEXTINT(pps); patrieSetPagedPageSize(config, pps);}
    else if (ARG_IS("-quiet"))    quiet = 1;
    else                          break;
  }

  if (ac != 2) usage();

  patrieSetDebug(config, ! quiet);
  patrieSetStats(config, ! quiet);
  patrieSetAssert(config, 1);
  
  patrieSetTextFP(config, openFile(av[0], "rb"));
  patrieSetPagedFP(config, openFile(av[1], "rb"));
  
  if (dbcase) {
    if      (strcmp(dbcase, "i") == 0) patrieSetCaseSensitive(config, 0);
    else if (strcmp(dbcase, "s") == 0) patrieSetCaseSensitive(config, 1);
    else {
      fprintf(stderr, "%s: invalid value for `-case' parameter.\n", prog);
      usage();
    }
  }

  ASSERT((starts = malloc(maxhits * sizeof *starts)));

  if ( ! batch) printf("Key: ");
  while ((key = getSearchKey())) {
    if ( ! patrieSearchSub(config, key, casesrch, maxhits, &hits, starts, &state)) {
      printf("*** search failure ***.\n");
    } else {
      if (batch) {
        printStarts(patrieGetTextFP(config), hits, starts);
      } else {
        printStrings(patrieGetTextFP(config), key, hits, starts);
      }
    }

    patrieResetState(state);

    if ( ! batch) printf("Key: ");
  }

  free(starts);
  fclose(patrieGetTextFP(config)); fclose(patrieGetPagedFP(config));
  patrieFree(&config); patrieFreeState(&state);

  exit(0);
}
