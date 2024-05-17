#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "patrie.h"
#include "defs.h"
#include "init.h"
#include "levels.h"
#include "page.h"
#include "sort.h"
#include "timing.h"


/*  
 *
 *
 *                          Statistics functions.
 *
 *
 */  


static void printStats(struct patrie_config *cf, struct _patrie_timing t)
{
  fprintf(stderr, "# skip_nodes start_nodes total_nodes max_skip max_height\n");
  fprintf(stderr, "stats %lu %lu %lu %lu %lu %.3f\n",
          cf->statsIntNodes,
          cf->statsExtNodes,
          cf->statsNodesOutput,
          cf->statsMaxSkip,
          cf->statsMaxHeight,
          _patrieTimingDiff(&t));
}


/*  
 *
 *
 *                           External functions.
 *
 *
 */


/*  
 *  We should require only that tmpFP point to a seekable file.  Also, the
 *  last four bytes of the file should indicate the page size, and the last
 *  page should contain information such as bit lengths, etc. and a pointer
 *  to the root page.
 *  
 *  Values should be in network byte order to make the index portable across
 *  architectures.  (Actually, we probably ought to define our own ordering,
 *  as PCs may not have network byte order routines.)
 */
/*  
 *  The last page should contain:
 *  
 *    a magic cookie indicating that this is a valid paged trie
 *    the page size (last four bytes)
 *    the number of bits per start & skip nodes
 *    the number of trie levels per page? (or is this derivable?)
 */      
int patrieBuild(struct patrie_config *cf)
{
  struct _patrie_timing timing;

  if (cf->printStats || cf->printDebug) {
    _patrieZeroStats(cf);
    _patrieTimingStart(&timing);
  }

  if (cf->doInfix && ! _patrieInfixBuild(cf)) {
    fprintf(stderr, "%s: patrieBuild: patrieInfixBuild() failed.\n", prog);
    return 0;
  }

  if (cf->doLevels && ! _patrieLevelBuild(cf)) {
    fprintf(stderr, "%s: patrieBuild: patrieLevelBuild() failed.\n", prog);
    return 0;
  }

  if (cf->doPaged && ! _patriePagedBuild(cf)) {
    fprintf(stderr, "%s: patrieBuild: patriePagedBuild() failed.\n", prog);
    return 0;
  }

  if (cf->printStats) {
    _patrieTimingEnd(&timing);
    printStats(cf, timing);
  }
  
  if (cf->printDebug) {
    fprintf(stderr, "\nLoaded %lu nodes, wrote %lu\n",
            cf->debugNodesLoaded, cf->debugNodesWritten);
  }

  return 1;
}
