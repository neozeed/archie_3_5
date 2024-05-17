/*  
 *  Dump the levels of the trie in the same format as output by O_build.
 */  

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "patrie.h"
#include "defs.h"
#include "levels.h"
#include "page.h"


#define ARG_IS(str)  (strcmp(av[0], str) == 0)
#define NEXTDBL(dbl) do{if(av++,--ac<= 0||sscanf(av[0],"%lf",&dbl)!=1)usage();}while(0)
#define NEXTINT(i)   do{if(av++,--ac<=0||sscanf(av[0],"%d",&i)!=1)usage();}while(0)
#define NEXTSTR(str) do{if(av++,--ac<=0)usage();elsestr=av[0];}while(0)
  

char *prog;


void usage(void)
{
  fprintf(stderr, "Usage: %s <infix-trie> <level-trie>\n", prog);
  exit(1);
}


int main(int ac, char **av)
{
  int nlevs, verbose = 0;
  struct patrie_config config;
  struct trie_single_level **levels;
  
  prog = av[0];
  
  patrieAlloc(&config);
  
  config.printDebug = 0;
  config.printStats = 0;
  config.assertCheck = 1;
  
  while (av++, --ac) {
    if      (ARG_IS("-pagesize")) NEXTINT(config.levelPageSize);
    else if (ARG_IS("-verbose"))  verbose = 1;
    else                          break;
  }

  if (ac != 2) usage();

  if ( ! (config.infixTrieFP = fopen(av[0], "rb")) ||
       ! (config.levelTrieFP = fopen(av[1], "w+b"))) {
    fprintf(stderr, "%s: can't open one or both of `%s', `%s'.\n", prog, av[0], av[1]);
    perror("fopen");
    exit(1);
  }
  
#error fix up
#if 0
  patrieLevelBuild(&config, &nlevs, &levels);
#endif
  dumpTrieLevels(config, verbose, nlevs, levels);

  exit(0);
}
