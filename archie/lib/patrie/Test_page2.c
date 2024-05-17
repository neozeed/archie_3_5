#include <sys/types.h>
#include <sys/wait.h>
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
          "Usage: %s [-case i|s] [-help] [-levels <#>] [-lps <#>] [-maxmem <#>] "
          "[-pad <#>] [-pps <#>] [-sortdir <path>] [-unique <#>] "
          "<text-file> <levs-tmp-file> <paged-trie>\n",
          prog);
  fprintf(stderr, "\n");

  P("-case",     "`i' or `s' for case insensitive or case sensitive build");
  P("-help",     "print this message");
  P("-levels",   "number of levels per (final) trie page");
  P("-lps",      "number bytes per (intermediate) page");
  P("-maxmem",   "approximate upper limit on amount of memory to use in sorting");
  P("-pad",      "number of characters to ensure uniqueness of strings");
  P("-pps",      "number bytes per (final) trie page");
  P("-prof",     "generate useful profiling information for both parent and child");
  P("-sortdir",  "store temporary files under this directory");

  fprintf(stderr, "\n");
  exit(1);

#undef P
}


int main(int ac, char **av)
{
  FILE *ifp, *ofp;
  char *dbcase = 0, *sortdir;
  int lpp, lps, pps;
  int prof = 0;
  pid_t pid;
  size_t maxmem, padlen;
  struct patrie_config *config;

  prog = av[0];
  
  patrieAlloc(&config);
  
  patrieSetDebug(config, 1);
  patrieSetStats(config, 1);
  patrieSetAssert(config, 1);
  
  while (av++, --ac) {
    if      (ARG_IS("-case"))     NEXTSTR(dbcase);
    else if (ARG_IS("-help"))     usage();
    else if (ARG_IS("-levels"))   {NEXTINT(lpp);     patrieSetLevelsPerPage(config, lpp);}
    else if (ARG_IS("-lps"))      {NEXTINT(lps);     patrieSetLevelPageSize(config, lps);}
    else if (ARG_IS("-maxmem"))   {NEXTINT(maxmem);  patrieSetSortMaxMem(config, maxmem);}
    else if (ARG_IS("-pad"))      {NEXTINT(padlen);  patrieSetSortPadLen(config, padlen);}
    else if (ARG_IS("-pps"))      {NEXTINT(pps);     patrieSetPagedPageSize(config, pps);}
    else if (ARG_IS("-prof"))     prof = 1;
    else if (ARG_IS("-sortdir"))  {NEXTSTR(sortdir); patrieSetSortTempDir(config, sortdir);}
    else                          break;
  }

  if (ac != 3) usage();

  if (dbcase) {
    if      (strcmp(dbcase, "i") == 0) patrieSetCaseSensitive(config, 0);
    else if (strcmp(dbcase, "s") == 0) patrieSetCaseSensitive(config, 1);
    else {
      fprintf(stderr, "%s: invalid value for `-case' parameter.\n", prog);
      usage();
    }
  }

  switch (pid = fpFork(&ifp, &ofp)) {
  case -1:
    exit(1);
    break;

    /*  
     *  Child: read the infix trie from the parent and create the paged trie.
     */
  case 0:
    fclose(ofp);

    patrieSetCreateInfix(config, 0, 0, 0);
    patrieSetInfixFP(config, ifp);
    patrieSetLevelFP(config, openTemp(av[1], "w+b"));
    patrieSetPagedFP(config, openFile(av[2], "wb"));

    patrieBuild(config);

    fclose(patrieGetInfixFP(config));
    fclose(patrieGetLevelFP(config));
    fclose(patrieGetPagedFP(config));
    exit(0);
    break;
    
    /*  
     *  Parent: create the infix trie from the text file, and write it to the
     *  child.
     */
  default:
    fclose(ifp);

    patrieSetCreateLevels(config, 0);
    patrieSetCreatePaged(config, 0);
    patrieSetTextFP(config, openFile(av[0], "rb"));
    patrieSetInfixFP(config, ofp);

    patrieBuild(config);

    fclose(patrieGetInfixFP(config));
    fclose(patrieGetTextFP(config));
    wait(0);
    break;
  }
  
  if (prof) rename("gmon.out", "gmon.out.child");

  exit(0);
}
