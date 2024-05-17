#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Test.h"
#include "patrie.h"
#include "defs.h"
#include "levels.h"
#include "page.h"
#include "sort.h"


char *prog;


void usage(void)
{
  fprintf(stderr, "Usage: %s [-case i|s] [-help] [-keep] [-maxmem <n>] [-only dist|merge] "
          "[-pad <n>] [-tmpdir <path>] [-unique <n>] <text-file> <infix-trie>\n", prog);
  fprintf(stderr, "\n");
  fprintf(stderr, "\t-case\t# `i' or `s' for case insensitive or case sensitive build\n");
  fprintf(stderr, "\t-help\t# print this message\n");
  fprintf(stderr, "\t-keep\t# don't remove temporary files\n");
  fprintf(stderr, "\t-maxmem\t# approximate upper limit on amount of memory to use\n");
  fprintf(stderr, "\t-only\t# `dist' or `merge'\n");
  fprintf(stderr, "\t-pad\t# must be enough to ensure uniqueness of strings\n");
  fprintf(stderr, "\t-tmpdir\t# store temporary files under this directory\n");
  fprintf(stderr, "\n");
  exit(1);
}


int main(int ac, char **av)
{
  char *dbcase = 0, *only = 0, *tempdir;
  int keeptemp = 0;
  size_t maxmem, padlen;
  struct patrie_config *config;
  
  prog = av[0];
  
  patrieAlloc(&config);
  
  patrieSetDebug(config, 1);
  patrieSetStats(config, 1);
  patrieSetAssert(config, 1);
  
  while (av++, --ac) {
    if      (ARG_IS("-case"))   NEXTSTR(dbcase);
    else if (ARG_IS("-help"))   usage();
    else if (ARG_IS("-keep"))   keeptemp = 1;
    else if (ARG_IS("-maxmem")) {NEXTINT(maxmem);  patrieSetSortMaxMem(config, maxmem);}
    else if (ARG_IS("-only"))   NEXTSTR(only);
    else if (ARG_IS("-pad"))    {NEXTINT(padlen);  patrieSetSortPadLen(config, padlen);}
    else if (ARG_IS("-tmpdir")) {NEXTSTR(tempdir); patrieSetSortTempDir(config, tempdir);}
    else                        break;
  }

  if (ac != 2) usage();

  if (dbcase) {
    if      (strcmp(dbcase, "i") == 0) patrieSetCaseSensitive(config, 0);
    else if (strcmp(dbcase, "s") == 0) patrieSetCaseSensitive(config, 1);
    else {
      fprintf(stderr, "%s: invalid value for `-case' parameter.\n", prog);
      usage();
    }
  }

  if (only) {
    if      (strcmp(only, "dist") == 0)  patrieSetSortMerge(config, 0);
    else if (strcmp(only, "merge") == 0) patrieSetSortDistribute(config, 0);
    else                                 usage();
  }

  patrieSetTextFP(config, openFile(av[0], "rb"));
  patrieSetInfixFP(config, openFile(av[1], "w+b"));
  
  _patrieInfixBuild(config);

  fclose(patrieGetTextFP(config)); fclose(patrieGetInfixFP(config));
  
  exit(0);
}
