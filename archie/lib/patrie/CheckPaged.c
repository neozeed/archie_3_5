#include <stdio.h>
#include "Test.h"


char *prog;


int checkBottomUp(struct patrie_config *config)
{
  fprintf(stderr, "%s: checkBottomUp: not yet implemented.\n", prog);
  return 0;
}


int checkTopDown(struct patrie_config *config)
{
  loadPage(config, -1, SEEK_END); /* root page */
  
}


void usage(void)
{
  fprintf(stderr, "Usage: %s [-bo] [-help] [-td] <paged-trie>\n", prog);
  fprintf(stderr, "\n");
  fprintf(stderr, "\t-bo\t# check the trie from the bottom up\n");
  fprintf(stderr, "\t-help\t# print this message\n");
  fprintf(stderr, "\t-pps\t# number bytes per (final) trie page\n");
  fprintf(stderr, "\n-td\t# check the trie from the top down\n");
  fprintf(stderr, "\n");
  exit(1);
}


int main(int ac, char **av)
{
  int ret, topDown = 1;
  struct patrie_config config;

  patrieAlloc(&config);

  config.printDebug = 1;
  config.printStats = 1;
  config.assertCheck = 1;
  
  while (av++, --ac) {
    if      (ARG_IS("-bo"))   topDown = 0;
    else if (ARG_IS("-help")) usage();
    else if (ARG_IS("-pps"))  NEXTINT(config.pagedPageSize);
    else if (ARG_IS("-td"))   topDown = 1;
    else                      break;
  }

  if (ac != 1) usage();

  ASSERT((config.triePage = malloc(sizeof(struct patrie_page))));
  config.pagedTrieFP = openFile(av[1], "rb");
  
  ret = topDown ? checkTopDown(&config) : checkBottomUp(&config);

  exit( ! ret);
}
