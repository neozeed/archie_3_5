/*  
 *  Input is a sequence of numbers, one per line.  Non-negative numbers are
 *  heights, negative numbers are starts.  The absolute value of a start,
 *  minus one, is an offset into the text file.
 *  
 *  Output is a paged PaTrie.
 *  
 *  We use the top two bits of each trie node to store its type (start, skip
 *  or super).  A super node is the block number, in the output file, of the
 *  subtrie corresponding to the node.
 *  
 *  We assume that node values are stored in 32 bit integers.
 */

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "patrie.h"
#include "bits.h"
#include "defs.h"
#include "levels.h"
#include "page.h"
#include "int_stack.h"
#include "ptr_stack.h"
#include "timing.h"
#include "trailer.h"
#include "utils.h"


#define BITS_IN_NODE(node) ((node).type == SKIP_NODE ? config->bitsPerSkip : config->bitsPerStart)
#define MAKE_START(config, i) ((i) |= (1 << config->bitsPerStart))
#define N_CHILD_NODES(node) ((node).type == SKIP_NODE ? 2 : 0)


struct paged_stats {
  unsigned long pagedSize;      /* size of paged PaTrie file */
  unsigned long blockSize;      /* size of the pages */
  unsigned long levsPerBlock;   /* # levels of the trie per block */
  int skipBits;                 /* # bits per skip */
  int startBits;                /* # bits per start */
  struct _patrie_timing pagedTime;
};


/*  
 *  If an output page can hold k levels then the buffer must be k levels of M
 *  nodes, where M = N + 2^(k-1), and N is the maximum number of nodes that
 *  can fit in an output page.  The extra term is to ensure we can fit the
 *  contents of one output page plus one k-level subtrie.
 */
struct node_buffer {
  int nBitsUsed;                /* # bits used by counts and nodes in the buffer */
  int nBitsAllNodes;            /* # bits used by all nodes the in buffer */
  
  int nAllTCounts;              /* # tCount entries used by all nodes */
  int nPenTCounts;              /* # tCount entries used by all but the last subtrie */
  int *tCount;                  /* tCount entries */

  int prevIdx;                  /* firstTCountIdx from previous buffer (for performance) */

  int *nAllNodes;               /* sum of # nodes in each level of all subtries */
  int *nPenNodes;               /* sum of # nodes in each level of all but the last subtrie */
  Node **level;                 /* pointers to the beginning of levels */
  Node *node;                   /* pointer to all the nodes of the buffer */

  /* Duplicated here for convenience.  (Once set it never changes.) */

  int nBitsInPage;
};


/*  
 *  For each output page (a set of levels) count the number of links
 *  entering, from the top, all the pages to the left.
 */
struct top_count {
  int navail;                   /* number of available entries in nLeftLinks buffer */
  int nused;                    /* number of used entries in nLeftLinks buffer */
  int firstPage;                /* page number of the first output page on the current level */
  int *nLeftLinks;
};


static void readTriePage(struct patrie_config *config, int pageNum, struct trie_single_level *level);


/*  
 *
 *
 *                           Debugging functions.
 *
 *
 */  


static void debugCheckTCounts(struct top_count *tc)
{
  int i;
  
  for (i = 1; i < tc->nused; i++) {
    ASSERT(tc->nLeftLinks[i] >= tc->nLeftLinks[i-1]);
  }
}


/*  
 *  Set the `used' field of the current level to the number of nodes in the
 *  current page.  Assume the last valid node is a disk block pointer or a
 *  level terminator.  The last node is included in the count.
 */
static void setNodesUsed(struct trie_single_level *level)
{
  int i;

  for (i = 0; level->node[i].type != BLKNO_NODE && level->node[i].type != TERM_NODE; i++) {
    continue;
  }

  level->used = i + 1;
}


static void dumpTrieNode(FILE *fp, Node node)
{
  switch (node.type) {
  case SKIP_NODE:  fprintf(fp, "%d\n", node.value);  break;
  case START_NODE: fprintf(fp, "-%d\n", node.value); break;
  case BLKNO_NODE:                                   break;
  case TERM_NODE:                                    break;
  default:         fprintf(fp, "???\n");             break;
  }
}


static void printTrieNode(FILE *fp, Node node)
{
  switch (node.type) {
  case SKIP_NODE:  fprintf(fp, "(%d)", node.value); break;
  case START_NODE: fprintf(fp, ".%d.", node.value); break;
  case BLKNO_NODE: fprintf(fp, "[%d]", node.value); break;
  case TERM_NODE:  fprintf(fp, "T");                break;
  default:         fprintf(fp, "???");              break;
  }
}


/*  
 *  Print all the nodes in `level->node'.
 */
static void dumpAllTrieNodes(FILE *fp, struct trie_single_level *level)
{
  int i;

  for (i = 0; i < level->used; i++) {
    dumpTrieNode(fp, level->node[i]);
  }
}


/*  
 *  Print all the nodes in `level->node'.
 */
static void printAllTrieNodes(FILE *fp, struct trie_single_level *level)
{
  int i;

  if (level->used > 0) {
    printTrieNode(fp, level->node[0]);
    for (i = 1; i < level->used; i++) {
      fprintf(fp, " "); printTrieNode(fp, level->node[i]);
    }
  }
}


static void dumpTriePage(FILE *fp, void *l)
{
  struct trie_single_level *level = l;
  
  dumpAllTrieNodes(fp, level);
}


static void printTriePage(FILE *fp, void *l)
{
  struct trie_single_level *level = l;
  
  fprintf(fp, "\nfirstPageNum %d, pageNum %d, used %d, nextNode %d\n",
          level->firstPageNum, level->pageNum, level->used, level->nextNode);
  fprintf(fp, "\tNodes: ");
  printAllTrieNodes(fp, level);
  fprintf(fp, "\n");
}


/*  
 *  Debugging.
 */
void dumpTrieLevels(struct patrie_config *config, int verbose, int nlev, struct trie_single_level **level)
{
  int i;
  
  fprintf(stderr, "-------------------- Trie levels --------------------\n");
  
  for (i = 0; i < nlev; i++) {
    int currPage = level[i]->firstPageNum;
    int nnodes;
    
    if (verbose) fprintf(stderr, "\nLevel %d\n\n", i + 1);

    do {
      readTriePage(config, currPage, level[i]);

      setNodesUsed(level[i]);
      level[i]->pageNum = currPage;

      nnodes = level[i]->used;
      dumpTriePage(stderr, level[i]);
      currPage = level[i]->node[nnodes-1].value;
    } while (level[i]->node[nnodes-1].type != TERM_NODE);
  }
}


/*  
 *  Call this once the entire trie is on disk.
 */
static void printTrieLevels(struct patrie_config *config, int nlev, struct trie_single_level **level)
{
  int i;
  
  fprintf(stderr, "\n-------------------- Trie levels --------------------\n");
  
  for (i = 0; i < nlev; i++) {
    int currPage = level[i]->firstPageNum;
    int nnodes;
    
    fprintf(stderr, "\nLevel %d\n\n", i);

    do {
      readTriePage(config, currPage, level[i]);

      setNodesUsed(level[i]);
      level[i]->pageNum = currPage;

      nnodes = level[i]->used;
      printTriePage(stderr, level[i]);
      currPage = level[i]->node[nnodes-1].value;
    } while (level[i]->node[nnodes-1].type != TERM_NODE);
  }
}


/*  
 *
 *
 *                           Stats functions.
 *
 *
 */  


static void printStats(struct paged_stats s)
{
  fprintf(stderr, "# paged_file_size page_size levels_per_page skip_bits start_bits paged_time\n");
  fprintf(stderr, "paged %lu %lu %lu %d %d %.3f\n",
          s.pagedSize, s.blockSize, s.levsPerBlock, s.skipBits, s.startBits,
          _patrieTimingDiff(&s.pagedTime));
}


/*  
 *
 *
 *                           Internal functions.
 *
 *
 */    


static void addTopCount(struct top_count *tc, int nLeftLinks)
{
  ASSERT(tc->navail + tc->nused > 0);
  
  if (tc->navail == 0) {
    tc->navail = tc->nused;
    ASSERT((tc->nLeftLinks = realloc(tc->nLeftLinks, 2 * tc->nused * sizeof *tc->nLeftLinks)));
  }

  tc->nLeftLinks[tc->nused] = nLeftLinks;

  tc->nused++; tc->navail--;
}


/*  
 *  If n is the value of bCount for a particular page, then we return the
 *  index, into tc->nLeftLinks, of the first tCount required by the page.
 *  
 *  In the special case where tc->nLeftLinks contains one entry, INT_MAX, we
 *  return -1, indicating that the page does not require any tCount entries.
 */
#if 1
static int firstTCountIdx(struct top_count *tc, int startidx, int n)
{
  int i;
  
  for (i = startidx; tc->nLeftLinks[i] <= n; i++) {
    continue;
  }
  return i - 1;
}

#else

static int firstTCountIdx(struct top_count *tc, int n)
{
  int i;
  
  for (i = 0; tc->nLeftLinks[i] <= n; i++) {
    continue;
  }
  return i - 1;
}
#endif


/*  
 *  If n is the value of `bCount + #links leaving the page' for a particular
 *  page, then we return the index, into tc->nLeftLinks, of the last tCount
 *  required by the page.
 */
#if 1
static int lastTCountIdx(struct top_count *tc, int startidx, int n)
{
  int i;

  for (i = startidx; tc->nLeftLinks[i] < n; i++) {
    continue;
  }
  return i;
}

#else

static int lastTCountIdx(struct top_count *tc, int n)
{
  int i;

  for (i = 0; tc->nLeftLinks[i] < n; i++) {
    continue;
  }
  return i;
}
#endif


static void freeTopCount(struct top_count *tc)
{
  free(tc->nLeftLinks);
  tc->firstPage = -1;
  tc->navail = 0;
  tc->nused = 0;
}


static void initTopCount(struct top_count *tc, int firstPage)
{
#define INIT_ELTS 1000

  ASSERT((tc->nLeftLinks = MALLOC(INIT_ELTS * sizeof *tc->nLeftLinks)));
  tc->navail = INIT_ELTS;
  tc->firstPage = firstPage;
  tc->nused = 0;

#undef INIT_ELTS
}


static void copyTopCount(struct top_count *dst, struct top_count *src)
{
  freeTopCount(dst);
  *dst = *src;
  src->nLeftLinks = 0;
}


/*  
 *  Read the page specified by `pageNum' into `level->node'.
 *  
 *  Note that nextNode() requires that `level->used' have the correct value,
 *  as it interprets 0 to mean the first page has not yet been read in.
 */
static void readTriePage(struct patrie_config *config, int pageNum, struct trie_single_level *level)
{
  ASSERT(fseek(config->levelTrieFP, pageNum * config->levelPageSize, SEEK_SET) != -1);
  ASSERT(fread(level->node, 1, config->levelPageSize, config->levelTrieFP)
         == config->levelPageSize);

  level->pageNum = pageNum;
  setNodesUsed(level);
}


/*  
 *  Set `*node' to the next node in the current trie level.  Return 1 if
 *  successful, 0 if there are no more nodes on the level.
 *  
 *  After exhausting the nodes on a level repeated calls will return 0.
 *  
 *  If lev->used is 0 then the first page has not yet been read in.
 */
static int nextNode(struct patrie_config *config, struct trie_single_level *lev, Node *node)
{
  if (lev->used == 0) {
    readTriePage(config, lev->firstPageNum, lev);
    lev->nextNode = 0;
  }
  
  *node = lev->node[lev->nextNode];
  while (node->type == BLKNO_NODE) {
    readTriePage(config, node->value, lev);
    lev->nextNode = 0;
    *node = lev->node[lev->nextNode];
  }

  if (node->type == TERM_NODE) {
    return 0;
  } else {
    lev->nextNode++;

    if (config->printDebug) {
      if (node->type == START_NODE) {
        config->debugMinStart = MIN(config->debugMinStart, node->value);
        config->debugMaxStart = MAX(config->debugMaxStart, node->value);
      } else {
        config->debugMinHeight = MIN(config->debugMinHeight, node->value);
        config->debugMaxHeight = MAX(config->debugMaxHeight, node->value);
      }
    }
    
#if 0
#warning DEBUGGING!
    if (node->type == SKIP_NODE && node->value > 100000) {
      abort();
    }
#endif
    
    return 1;
  }
}


/*  
 *  Count the number of links entering the current page.
 */
static int linksEnteringPage(struct node_buffer nb, int allNodes)
{
  int nnodes;
  
  nnodes = allNodes ? nb.nAllNodes[0] : nb.nPenNodes[0];
  return nnodes;
}


/*  
 *  Count the number of links leaving the current page.
 */
static int linksLeavingPage(struct node_buffer nb, int allNodes, int nLevs)
{
  int i, leaving, nnodes;
  
  nnodes = allNodes ? nb.nAllNodes[nLevs-1] : nb.nPenNodes[nLevs-1];

  for (i = 0, leaving = 0; i < nnodes; i++) {
    leaving += N_CHILD_NODES(nb.level[nLevs-1][i]);
  }

  return leaving;
}


static void nodeBufferFree(struct node_buffer *nb)
{
  free(nb->tCount);
  free(nb->nAllNodes);
  free(nb->nPenNodes);
  free(nb->level);
  free(nb->node);
}


static void nodeBufferInit(struct patrie_config *config, int nLevs, int nBitsInPage, struct node_buffer *nb)
{
  int i;
  int npl;                      /* max # nodes per level of the buffer */
  int npp;                      /* max # nodes per output page */

  npp = nBitsInPage / MIN(config->bitsPerStart + 1, config->bitsPerSkip + 1);
  npl = npp + (1 << (nLevs - 1));

  nb->nBitsUsed = 0;
  nb->nBitsAllNodes = 0;

  nb->nAllTCounts = 0;
  nb->nPenTCounts = 0;

  ASSERT((nb->tCount = MALLOC((npp + 1) * sizeof(int))));

  ASSERT((nb->nAllNodes = MALLOC(nLevs * sizeof(int))));
  ASSERT((nb->nPenNodes = MALLOC(nLevs * sizeof(int))));

  ASSERT((nb->level = MALLOC(nLevs * sizeof(Node *))));
  ASSERT((nb->node = MALLOC(nLevs * npl * sizeof(Node))));

  nb->prevIdx = 0;

  for (i = 0; i < nLevs; i++) {
    nb->nAllNodes[i] = 0;
    nb->nPenNodes[i] = 0;
    nb->level[i] = nb->node + i * npl;
  }

  nb->nBitsInPage = nBitsInPage; /* for convenience... */
}


/*  
 *  Count the number of bits used by the current contents of the node buffer.
 */
static int bitsInNodeBuffer(struct patrie_config *config, int nLevs, struct top_count *childTopCount, struct node_buffer *nb)
{
  int nBits;
  
  nBits = 0;
  if (nb->nAllNodes[0] > 0) {
    nBits += nb->nBitsAllNodes;

    /*  
     *  Add the number of bits used by the counts.
     */
    
    nBits += BITS_IN_BCOUNT + BITS_IN_PAGENUM + BITS_IN_CCOUNT;
    nBits += nb->nAllTCounts * BITS_IN_TCOUNT;
  }

  return nBits;
}


/*  
 *  Append successive subtries, of `nLevs' levels and rooted at the next
 *  available node of `subtrie[0]', into `nodeBuf' until the number of nodes
 *  in the buffer is greater than or equal to the number of nodes that can
 *  fit in an output page.  (We also stop copying nodes into the buffer if we
 *  run out of nodes at the top level of the subtrie.)
 *  
 *  This routine should be called repeatedly until the number of nodes in the
 *  buffer is less then the maximum number of nodes in an output page, at
 *  which point we have run out of nodes at the top of the subtrie.
 */
/*  
 *  Add more subtries to the buffer until we have enough to fill an output
 *  page.  We have enough when the relation
 *  
 *  #Bcount + #M + M * (#Tcount + #pageIdx) + N * #node >= #page
 *  
 *  is true.  Note that if the left hand side is strictly greater than the
 *  right we have one subtrie too many (for the current page) in the buffer.
 */
static int nodeBufferLoad(struct patrie_config *config, struct trie_single_level **subtrie,
                          int nLevs, int bCount, struct top_count *childTopCount,
                          struct node_buffer *nb)
{
  int firstIdx, lastIdx, nLeaving;
  int i, j, nChild;
  Node node;

  nb->nBitsUsed = bitsInNodeBuffer(config, nLevs, childTopCount, nb);

  /*  
   *  If we're called with no more nodes available (i.e. the next node read
   *  should be a terminal node) then bCount will equal the last element in
   *  childTopCount, causing firstTCountIdx to overrun the array.  Thus we
   *  require that all childTopCounts have INT_MAX as their final element.
   */
  
  firstIdx = firstTCountIdx(childTopCount, nb->prevIdx, bCount);
  lastIdx = firstIdx;

  while (nb->nBitsUsed < nb->nBitsInPage &&
         nextNode(config, subtrie[0], &node)) {
    /*  
     *  Reset counts for the number of nodes in each level of all but the
     *  (new) last subtrie.
     */
    
    for (i = 0; i < nLevs; i++) nb->nPenNodes[i] = nb->nAllNodes[i];
    
    /*  
     *  Add the root node to the buffer.
     */

    nb->level[0][nb->nAllNodes[0]] = node;
    nb->nAllNodes[0]++; config->debugNodesLoaded++;
    nb->nBitsAllNodes += BITS_IN_NODE(node) + 1;

    /*  
     *  Add all descendants of the root node.
     */
    
    nChild = N_CHILD_NODES(node);

    for (i = 1; i < nLevs && nChild > 0; i++) {
      int n = 0;
      for (j = 0; j < nChild; j++) {
        ASSERT(nextNode(config, subtrie[i], &node));
        nb->level[i][nb->nAllNodes[i]] = node;
        nb->nAllNodes[i]++; config->debugNodesLoaded++;
        nb->nBitsAllNodes += BITS_IN_NODE(node) + 1;
        n += N_CHILD_NODES(node);
      }
      nChild = n;
    }

    /*  
     *  Count the number of tCounts used by the page.
     */  

    nb->nPenTCounts = nb->nAllTCounts;
    
    nLeaving = 0;
    for (i = 0; i < nb->nAllNodes[nLevs-1]; i++) {
      nLeaving += N_CHILD_NODES(nb->level[nLevs-1][i]);
    }

    if (nLeaving > 0) {
      ASSERT(firstIdx >= 0);
      lastIdx = lastTCountIdx(childTopCount, lastIdx, bCount + nLeaving);
      nb->nAllTCounts = lastIdx - firstIdx + 1;
      ASSERT(nb->nAllTCounts >= 0);
    }

    nb->nBitsUsed = bitsInNodeBuffer(config, nLevs, childTopCount, nb);
  } /* while */

  if (nb->nAllTCounts > 0) {
    for (i = firstIdx; i <= lastIdx; i++) {
      nb->tCount[i-firstIdx] = childTopCount->nLeftLinks[i];
    }
  }

  if (firstIdx > nb->prevIdx) {
    nb->prevIdx = firstIdx;
  }

  return node.type != TERM_NODE;
}


/*  
 *  Prepare for reading a new set of levels by resetting all counts in the
 *  node buffer.
 */
static void nodeBufferReset(struct patrie_config *config, struct node_buffer *nb)
{
  int i;

  nb->nBitsUsed = 0;
  nb->nBitsAllNodes = 0;
  
  nb->nAllTCounts = 0;
  nb->nPenTCounts = 0;

  nb->prevIdx = 0;

  for (i = 0; i < config->levelsPerPage; i++) {
    nb->nAllNodes[i] = 0;
    nb->nPenNodes[i] = 0;
  }
}


/*  
 *  Remove the subtries that have been copied to the output page.
 */
static void nodeBufferShift(struct patrie_config *config, int nLevs, int bCount,
                            struct top_count *childTopCount, struct node_buffer *nb)
{
  if (nb->nBitsUsed <= nb->nBitsInPage) {
    nodeBufferReset(config, nb);
  } else {
    int nLeaving;
    int i, j;
  
    nb->nBitsUsed = 0;          /* allowed: value set when loading buffer */
    nb->nBitsAllNodes = 0;
    
    nb->nAllTCounts = 0;
    nb->nPenTCounts = 0;
    
    for (i = 0; i < nLevs; i++) {
      for (j = 0; j < nb->nAllNodes[i] - nb->nPenNodes[i]; j++) {
        nb->level[i][j] = nb->level[i][j+nb->nPenNodes[i]];
        nb->nBitsAllNodes += BITS_IN_NODE(nb->level[i][j]) + 1;
      }
      nb->nAllNodes[i] -= nb->nPenNodes[i];
      nb->nPenNodes[i] = 0;
    }

    nLeaving = 0;
    for (i = 0; i < nb->nAllNodes[nLevs-1]; i++) {
      nLeaving += N_CHILD_NODES(nb->level[nLevs-1][i]);
    }

    if (nLeaving > 0) {
      int firstIdx, lastIdx;

      firstIdx = firstTCountIdx(childTopCount, nb->prevIdx, bCount);
      lastIdx = lastTCountIdx(childTopCount, firstIdx, bCount + nLeaving);
      nb->nAllTCounts = lastIdx - firstIdx + 1;

      ASSERT(nb->nAllTCounts > 0);

      for (i = firstIdx; i <= lastIdx; i++) {
        nb->tCount[i-firstIdx] = childTopCount->nLeftLinks[i];
      }

      nb->prevIdx = firstIdx;
    }

    nb->nBitsUsed = bitsInNodeBuffer(config, nLevs, childTopCount, nb);
  }
}


/*  
 *  Copy nodes from the buffer to the output page.  Also set the counts.
 */
static void fillPage(struct patrie_config *config, int useAllNodes, int nLevs, int bCount,
                     struct top_count *tc, struct node_buffer *nb, char *page)
{
  int i, currOff = 0;
  int nTCounts;
  int childOffset;

  /*
   *  Initialize the page to stop Memory Advisor from complaining.
   */

  memset(page, CHAR_MAX, config->pagedPageSize);

  copyBits32(BITS_IN_BCOUNT, 0, (char *)&bCount, currOff, page);
  currOff += BITS_IN_BCOUNT + BITS_IN_PAGENUM;

  /* How many child pages do we have? */

  if (useAllNodes) {
    nTCounts = nb->nAllTCounts;
  } else {
    nTCounts = nb->nPenTCounts;
  }

  /*
   *  bug: define type u32 for consistent sizes
   */
  
  copyBits32(BITS_IN_CCOUNT, 0, (char *)&nTCounts, currOff, page);
  currOff += BITS_IN_CCOUNT;

  /*  
   *  If this page has children set the page number of the first child, and
   *  copy the necessary tCounts.
   */

  if (nTCounts > 0) {
    int n;
    
    childOffset = firstTCountIdx(tc, nb->prevIdx, bCount);
    n = tc->firstPage + childOffset;
    copyBits32(BITS_IN_PAGENUM, 0, (char *)&n, BITS_IN_BCOUNT, page);

    for (i = 0; i < nTCounts; i++) {
      copyBits32(BITS_IN_TCOUNT, 0, (char *)&tc->nLeftLinks[i + childOffset], currOff, page);
      currOff += BITS_IN_TCOUNT;
    }
  }
  
  for (i = 0; i < nLevs; i++) {
    int j, nCopy;

    nCopy = useAllNodes ? nb->nAllNodes[i] : nb->nPenNodes[i];
        
    for (j = 0; j < nCopy; j++) {
      unsigned long val = nb->level[i][j].value;
      if (nb->level[i][j].type == START_NODE) MAKE_START(config, val);
      copyBits32(BITS_IN_NODE(nb->level[i][j]) + 1, 0, (char *)&val, currOff, page);
      currOff += BITS_IN_NODE(nb->level[i][j]) + 1;
    }
    
    config->debugNodesWritten += nCopy;
  }
}


static int pageTooSmall(struct patrie_config *cf)
{
  unsigned long nodes_mx;       /* maximum # nodes to try to fit in page */
  unsigned long nodes_ub;       /* upper bound on # nodes in page */
  unsigned long overhead;       /* upper bound on # nodes in page */

  /*
   *  bug: should include a bound, rather than a guess, on the number of
   *  tcounts in the page.
   */
  overhead = (BITS_IN_BCOUNT + BITS_IN_CCOUNT + BITS_IN_PAGENUM +
              20 * BITS_IN_BCOUNT) / CHAR_BIT;

  if (cf->levelsPerPage == 0) {
    fprintf(stderr,
            "%s: pageTooSmall: at least one trie level per page is required.\n",
            prog);
    return 1;
  }
  
  /*
   *  The index PaTrie page size is definitely too small if the following
   *  relation holds:
   *  
   *  ((2^k)-1) * sizeof node + overhead > pagesize
   *      or (approximately)
   *  2^k > ((pagesize - overhead) / sizeof node) - 1
   */

  nodes_mx = 1 << (cf->levelsPerPage - 1);
  nodes_ub = (cf->pagedPageSize - overhead) /
             (MAX(cf->bitsPerSkip, cf->bitsPerStart) / CHAR_BIT) - 1;

  if (cf->levelsPerPage >= sizeof nodes_mx * CHAR_BIT) {
    fprintf(stderr, "%s: pageTooSmall: too many trie levels for page.\n",
            prog);
    return 1;
  }

  if (nodes_mx > nodes_ub) {
    fprintf(stderr, "%s: pageTooSmall: page can't hold potentially %lu nodes.\n",
            prog, nodes_mx - 1);
    return 1;
  }

  return 0;
}



/*  
 *
 *
 *                           External functions.
 *
 *
 */


/*  
 *  Pass # levels per page and page size as arguments?
 */
/*  
 *  The maximum number of nodes that can fit in a page is:
 *  
 *  N =
 *  
 *  Starting from the next node at the current level, read k levels of trie
 *  into a buffer.  Try packing this trie into the current page.  If we can't
 *  we write the page (with the appropriate counts) and try again with the
 *  same buffer.  We reload the buffer only after we've successfully packed
 *  its contents into a page.  (A complete binary tree of k levels has
 *  (2^k)-1 nodes.)
 */
#if 0
/*  
 *  Note: nodeBuffer should keep track of the number of nodes in the last
 *  level of each subtrie.
 */    
for each set of L levels {
  currLinksIn = [];
  linksLeavingLeft = 0
  pageNum = 0
  while we can [gather as many subtries as will fit in a page] {
    set S to be the number of subtries to put in the page (All or All-1)

    put linksLeavingLeft into the page

    pack S subries into the page
    write the page
    pageNum++

    L = the number of links leaving this page
    linksLeavingLeft += L

    L = the number of links entering the page (i.e. S)
    append {pageNum, L} to currLinksIn

    shift buffer left by S subtries

    childLinksIn = currLinksIn
    currLinksIn = []
  }
}
#endif

int _patriePagedBuild(struct patrie_config *cf)
{
  char *page;
  int debugCount = 0;
  int i, nlevs;
  int pageLevel;
  int maxBitsInPage;
  int outputPageNum = 0;        /* bug? make 1, leaving room for dummy page? */
  struct node_buffer nb;
  struct paged_stats stats;
  struct top_count childTCount, currTCount;
  struct trie_single_level **trie;

  if (cf->printStats) {
    _patrieTimingStart(&stats.pagedTime);
  }

  if (pageTooSmall(cf)) {
    fprintf(stderr, "%s: _patriePagedBuild: page size too small.\n", prog);
    return 0;
  }

#if 0
  _patrieReadTrailer(cf, cf->levelTrieFP);
#endif

  ASSERT(fseek(cf->levelTrieFP, -(long)(sizeof nlevs + cf->trailerSize), SEEK_END) != -1);
  ASSERT(fread(&nlevs, sizeof nlevs, 1, cf->levelTrieFP));
  ASSERT((trie = MALLOC(nlevs * sizeof *trie)));
  ASSERT(fseek(cf->levelTrieFP, -(1 + nlevs) * sizeof nlevs, SEEK_END) != -1);
  for (i = 0; i < nlevs; i++) {
    _patrieNewSingleLevel(cf, &trie[i]);
    ASSERT(fread(&trie[i]->firstPageNum, sizeof trie[0]->firstPageNum, 1, cf->levelTrieFP));
  }
    
  if (cf->printDebug) {
    cf->debugMinHeight = cf->debugMinStart = ULONG_MAX;
    cf->debugMaxHeight = cf->debugMaxStart = 0;
  }

  /*  
   *  Set up pages for use by nextNode().
   *  
   *  bug: maybe we should load the first page of each level so that
   *  nextNode() doesn't require the `used' field.  (Also, readTriePage()
   *  wouldn't have to set it.)
   */

  for (i = 0; i < nlevs; i++) {
    trie[i]->used = 0;          /* denotes an unfilled buffer */
    trie[i]->nextNode = 0;
  }
  
  maxBitsInPage = cf->pagedPageSize * CHAR_BIT;
  ASSERT((page = MALLOC(cf->pagedPageSize)));

  initTopCount(&childTCount, -1);
  addTopCount(&childTCount, INT_MAX); /* special case */
  nodeBufferInit(cf, cf->levelsPerPage, maxBitsInPage, &nb);

  pageLevel = nlevs / cf->levelsPerPage;
  if (nlevs % cf->levelsPerPage != 0) pageLevel++;

  for (i = pageLevel - 1; i >= 0; i--) {
    int bCount, tCount;
    int copyLevs;               /* number of levels to copy into a page */
    int currLev;
    
    bCount = 0; tCount = 0;
    initTopCount(&currTCount, outputPageNum);

    currLev = i * cf->levelsPerPage;
    copyLevs = MIN(cf->levelsPerPage, nlevs - currLev);

    nodeBufferLoad(cf, &trie[currLev], copyLevs, bCount, &childTCount, &nb);

    if (cf->printDebug) {
      fprintf(stderr, "\n");
      fprintf(stderr,
              "%3d/%-3d  Page#  Entering  Leaving  bCount  tCounts  Loaded  Written  dbgCnt\n",
              currLev, currLev + copyLevs - 1);
      fprintf(stderr,
              "%7s  -----  --------  -------  ------  -------  ------  -------  ------\n\n",
              " ");
    }
    
    while (nb.nBitsUsed > 0) {
      int useAllNodes;
      
      useAllNodes = nb.nBitsUsed <= maxBitsInPage;

      /*  
       *  Before the current page is written out:
       *  
       *  bCount is the number of _links_ leaving pages to the left of the
       *  current (to be written) page.
       *  
       *  tCount is the number of links entering pages to the left of the
       *  current (to be written) page.
       *  
       *  outputPageNum is the number (0, 1, 2, ...) of the next page to
       *  write.
       *  
       *  currTCount[i] (0 <= i <= N) is the number of links entering pages
       *  to the left of the i'th page (from 0) in the current set of pages.
       *  (N is the number of pages already written on this level.)
       *  
       *  childTCount is currTCount from the previous (level below) set of
       *  pages.
       */  
      
      fillPage(cf, useAllNodes, copyLevs, bCount, &childTCount, &nb, page);
      ASSERT(fwrite(page, 1, cf->pagedPageSize, cf->pagedTrieFP) == cf->pagedPageSize);
      outputPageNum++;

      if (cf->printDebug) {
        fprintf(stderr, "%7s  %5d  %8d  %7d  %7d  %6d  %6lu  %7lu  %6d\n", " ",
                outputPageNum - 1,
                linksEnteringPage(nb, useAllNodes),
                linksLeavingPage(nb, useAllNodes, copyLevs),
                bCount,
                useAllNodes ? nb.nAllTCounts : nb.nPenTCounts,
                cf->debugNodesLoaded,
                cf->debugNodesWritten,
                debugCount
                );
        cf->debugNodesWritten = 0;
        cf->debugNodesLoaded = 0;
      }

      /*  
       *  Update the count of nodes leaving pages to the left of the current
       *  page (i.e. the new page to be filled), as well as nodes entering
       *  pages to the left.
       */  
      
      bCount += linksLeavingPage(nb, useAllNodes, copyLevs);
      
#if 0
#warning DEBUGGING!
      debugCheckTCounts(&currTCount);
#endif

      addTopCount(&currTCount, tCount);

#if 0
#warning DEBUGGING!
      debugCheckTCounts(&currTCount);
#endif

      tCount += linksEnteringPage(nb, useAllNodes);

      nodeBufferShift(cf, copyLevs, bCount, &childTCount, &nb);
      nodeBufferLoad(cf, &trie[currLev], copyLevs, bCount, &childTCount, &nb);

      debugCount++;
    }

    addTopCount(&currTCount, tCount);
    addTopCount(&currTCount, INT_MAX); /* prevernt overruns in nodeBufferLoad */
    
    copyTopCount(&childTCount, &currTCount);
    nodeBufferReset(cf, &nb);
  }

#if 0
  _patrieWriteTrailer(cf, cf->pagedTrieFP);
#endif
  fflush(cf->pagedTrieFP);

  for (i = 0; i < nlevs; i++) {
    _patrieFreeSingleLevel(&trie[i]);
  }
  free(trie);
  free(page);
  freeTopCount(&childTCount);
  freeTopCount(&currTCount);
  nodeBufferFree(&nb);

  if (cf->printStats) {
    _patrieTimingEnd(&stats.pagedTime);

    stats.pagedSize = _patrieFpSize(cf->pagedTrieFP);
    stats.blockSize = cf->pagedPageSize;
    stats.levsPerBlock = cf->levelsPerPage;
    stats.skipBits = cf->bitsPerSkip;
    stats.startBits = cf->bitsPerStart;

    printStats(stats);
  }

  if (cf->printDebug) {
    fprintf(stderr, "patriePagedBuild:\n");
    fprintf(stderr, "\tMinimum height: %lu, maximum height: %lu\n",
            cf->debugMinHeight, cf->debugMaxHeight);
    fprintf(stderr, "\tMinimum start: %lu, maximum start: %lu\n",
            cf->debugMinStart, cf->debugMaxStart);
  }

  return 1;
}
