#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "Test.h"
#include "patrie.h"
#include "defs.h"
#include "page.h"


char *prog;


static void usage(void)
{
#define P(opt, desc) fprintf(stderr, "\t%-10s # " desc "\n", opt);

  fprintf(stderr,
          "Usage: %s [-case i|s] [-acc i|s] [-debug] [-help] "
          "[-keepdist] [-levels <#>] [-lps <#>] [-maxmem <#>] "
          "[-only sort|levels|paged] [-pad <#>] [-pps <#>] "
          "[-stats] [-tmpdir <path>] "
          "<text-file> <infix-trie> <tmp-file> <paged-trie>\n",
          prog);
  fprintf(stderr, "\n");

  P("-case",     "`i' or `s' for case insensitive or case sensitive build");
  P("-acc",      "`i' or `s' for accent insensitive or accent sensitive build");
  P("-debug",    "print debugging information");
  P("-help",     "print this message");
  P("-keepdist", "don't remove the temporary sort files");
  P("-levels",   "number of levels per (final) trie page");
  P("-lps",      "number bytes per (intermediate) page");
  P("-maxmem",   "approximate upper limit on amount of memory to use in sorting");
  P("-only",     "`sort', `levels' or `paged'");
  P("-pad",      "number of characters to ensure uniqueness of strings");
  P("-pps",      "number bytes per (final) trie page");
  P("-stats",    "print stats on file sizes, times, etc.");
  P("-tmpdir",   "store temporary files under this directory");

  fprintf(stderr, "\n");
  exit(1);

#undef P
}


int main(int ac, char **av)
{
  char *dbcase = 0, *only = 0, *tmpdir = 0;
  char *dbacc = 0;
  int kdist = 0, lpp, lps, pps;
  size_t maxmem, padlen;
  struct patrie_config *config;

  prog = av[0];
  
  patrieAlloc(&config);
  
  patrieSetAssert(config, 1);
  
  while (av++, --ac) {
    if      (ARG_IS("-case"))     NEXTSTR(dbcase);
    else if (ARG_IS("-acc"))      NEXTSTR(dbacc);
    else if (ARG_IS("-debug"))    patrieSetDebug(config, 1);
    else if (ARG_IS("-help"))     usage();
    else if (ARG_IS("-keepdist")) kdist = 1;
    else if (ARG_IS("-levels"))   {NEXTINT(lpp);     patrieSetLevelsPerPage(config, lpp);}
    else if (ARG_IS("-lps"))      {NEXTINT(lps);     patrieSetLevelPageSize(config, lps);}
    else if (ARG_IS("-maxmem"))   {NEXTINT(maxmem);  patrieSetSortMaxMem(config, maxmem);}
    else if (ARG_IS("-only"))     NEXTSTR(only);
    else if (ARG_IS("-pad"))      {NEXTINT(padlen);  patrieSetSortPadLen(config, padlen);}
    else if (ARG_IS("-pps"))      {NEXTINT(pps);     patrieSetPagedPageSize(config, pps);}
    else if (ARG_IS("-stats"))    patrieSetStats(config, 1);
    else if (ARG_IS("-tmpdir"))   {NEXTSTR(tmpdir);  patrieSetSortTempDir(config, tmpdir);}
    else                          break;
  }

  if (ac != 4) {
    patrieFree(&config);
    usage();
  }
  
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

#if 0
  if (dbcase) {
    if      (strcmp(dbcase, "i") == 0) patrieSetCaseSensitive(config, 0);
    else if (strcmp(dbcase, "s") == 0) patrieSetCaseSensitive(config, 1);
    else {
      fprintf(stderr, "%s: invalid value for `-case' parameter.\n", prog);
      patrieFree(&config);
      usage();
    }
  }
#endif

  if ( ! only) {
    patrieSetTextFP(config, openFile(av[0], "rb"));
    patrieSetInfixFP(config, openFile(av[1], "w+b"));
    patrieSetLevelFP(config, openTemp(av[2], "w+b"));
    patrieSetPagedFP(config, openFile(av[3], "wb"));
  } else {
    if (strcmp(only, "levels") == 0) {
      patrieSetCreateInfix(config, 0, 0, kdist);
      patrieSetCreatePaged(config, 0);
      patrieSetInfixFP(config, openFile(av[1], "rb"));
      patrieSetLevelFP(config, openFile(av[2], "w+b"));

    } else if (strcmp(only, "paged") == 0) {
      patrieSetCreateInfix(config, 0, 0, kdist);
      patrieSetCreateLevels(config, 0);
      patrieSetLevelFP(config, openFile(av[2], "rb"));
      patrieSetPagedFP(config, openFile(av[3], "wb"));

    } else if (strcmp(only, "sort") == 0) {
      patrieSetCreateLevels(config, 0);
      patrieSetCreatePaged(config, 0);
      patrieSetTextFP(config, openFile(av[0], "rb"));
      patrieSetInfixFP(config, openFile(av[1], "w+b"));

    } else {
      patrieFree(&config);
      usage();
    }
  }

  patrieBuild(config);
  
  {
    FILE *fp;
    
    if ((fp = patrieGetInfixFP(config))) fclose(fp);
    if ((fp = patrieGetLevelFP(config))) fclose(fp);
    if ((fp = patrieGetPagedFP(config))) fclose(fp);
  }

  patrieFree(&config);
  
  exit(0);
}
