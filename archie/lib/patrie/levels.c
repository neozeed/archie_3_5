#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "patrie.h"
#include "defs.h"
#include "int_stack.h"
#include "levels.h"
#include "page.h"
#include "ptr_stack.h"
#include "timing.h"
#include "trailer.h"
#include "utils.h"


struct levels_stats {
  unsigned long levelsSize;
  unsigned long blockSize;
  struct _patrie_timing levelsTime;
};


/*  
 *
 *
 *                           Debugging functions.
 *
 *
 */  

static void debugCheckNodes(void *data, void *arg)
{
  int i;
  struct trie_single_level *l = data;

  for (i = 0; i < l->used; i++) {
    if (l->node[i].type == SKIP_NODE && l->node[i].value > 10000) {
      abort();
    }
  }
}


static void debugCheckStack0(void *s, void *arg)
{
  forEachEltPtrStack(s, debugCheckNodes, arg);
}


static void debugCheckStack(struct ptr_stack *s)
{
  forEachEltPtrStack(s, debugCheckNodes, 0);
}


static void debugCheckStackStack(struct ptr_stack *s)
{
  forEachEltPtrStack(s, debugCheckStack0, 0);
}


/*  
 *
 *
 *                          Statistics functions.
 *
 *
 */  


void printLevelInfo(struct patrie_config *config, int nlevs, struct trie_single_level **level)
{
  int expected, i;
  int totalAll = 0, totalBlocks = 0, totalExpt = 0, totalExt = 0, totalInt = 0, totalLevels = 0;

  fprintf(stderr, "Level   blocks   int nodes   ext nodes   total nodes   expected   %%used\n");
  fprintf(stderr, "-----   ------   ---------   ---------   -----------   --------   -----\n");

  expected = 1;                 /* value for root level must be 1 */

  for (i = 0; i < nlevs; i++) {
    int all = level[i]->statsIntNodes + level[i]->statsExtNodes;

    totalExpt += expected;

    fprintf(stderr, "%5d   %6d   %9d   %9d   %11d   %8d   %5.1f %s\n",
            i,
            level[i]->statsBlocks,
            level[i]->statsIntNodes,
            level[i]->statsExtNodes,
            all,
            expected,
            100 * all / (level[i]->statsBlocks * (config->levelPageSize / sizeof(Node) - 1.0)),
            all == expected ? "" : "*");

    expected = 2 * level[i]->statsIntNodes; /* for next level */

    totalAll += all;
    totalBlocks += level[i]->statsBlocks;
    totalExt += level[i]->statsExtNodes;
    totalInt += level[i]->statsIntNodes;
    totalLevels++;
  }

  fprintf(stderr, "\nGrand totals\n\n");
  fprintf(stderr, "Level   blocks   int nodes   ext nodes   total nodes   expected   %%used\n");
  fprintf(stderr, "-----   ------   ---------   ---------   -----------   --------   -----\n");
  fprintf(stderr, "%5d   %6d   %9d   %9d   %11d   %8d   %5.1f %s\n",
          totalLevels,
          totalBlocks,
          totalInt,
          totalExt,
          totalAll,
          totalExpt,
          100 * totalAll / (totalBlocks * (config->levelPageSize / sizeof(Node) - 1.0)),
          totalAll == totalExpt ? "" : "*");
}


static void printStats(struct levels_stats s)
{
  fprintf(stderr, "# levels_file_size block_size levels_time\n");
  fprintf(stderr, "levels %lu %lu %.3f\n",
          s.levelsSize, s.blockSize, _patrieTimingDiff(&s.levelsTime));
}


/*  
 *
 *
 *                           Internal functions.
 *
 *
 */    


/*  
 *  Return a pointer to the root node in the stack of levels.  The root will
 *  be the first in the array of nodes on the top of the stack.
 */
static Node *rootNode(struct ptr_stack *levels)
{
  struct trie_single_level *l;

  l = topPtrStack(levels);
  return &l->node[0];
}


void _patrieFreeSingleLevel(struct trie_single_level **l)
{
  struct trie_single_level *level = *l;

  free(level->node);
  free(level);
  *l = 0;
}


void _patrieNewSingleLevel(struct patrie_config *config, struct trie_single_level **l)
{
  Node *nodes;
  struct trie_single_level *level;
  
  ASSERT((nodes = MALLOC(config->levelPageSize)));
  ASSERT((level = MALLOC(sizeof *level)));
  
  level->firstPageNum = -1;
  level->pageNum = -1;
  level->used = 0;
  level->nextNode = 0;
  level->node = nodes;

  /*
   *  Initialize the nodes to stop Memory Advisor from complaining.
   */

  memset(level->node, CHAR_MAX, config->levelPageSize);

  /* Stats */
  
  level->statsIntNodes = 0;
  level->statsExtNodes = 0;
  level->statsBlocks = 0;
  
  *l = level;
}


void appendLevels(struct patrie_config *config, struct trie_single_level *dst, struct trie_single_level *src)
{
  int copy;
  int n = config->levelPageSize / sizeof(Node);
  
  /*  
   *  Append as many nodes as possible to the destination level.
   */

  copy = MIN(n - dst->used - 1, src->used);
  memcpy(&dst->node[dst->used], &src->node[0], copy * sizeof dst->node[0]);
  dst->used += copy;

  /*  
   *  Shift remaining source nodes to the beginning of the page.
   */

  memmove(&src->node[0], &src->node[copy], (src->used - copy) * sizeof src->node[0]);
  src->used -= copy;

  /*  
   *  Update the stats for this level.
   */
  
  dst->statsBlocks += src->statsBlocks;
}


static void writeTriePage(struct patrie_config *config, struct trie_single_level *level)
{
  int i;
    
  if (level->pageNum == -1) {
    /*  
     *  This is the first page, on this level, to be written.
     */
    
    level->pageNum = config->nextAvailPage++;
    level->firstPageNum = level->pageNum;
  }

  ASSERT(fseek(config->levelTrieFP, level->pageNum * config->levelPageSize, SEEK_SET) != -1);
  ASSERT(fwrite(level->node, 1, config->levelPageSize, config->levelTrieFP)
         == config->levelPageSize);
  
  /*  
   *  Update the stats for this level.
   */  

  level->statsBlocks++;
  for (i = 0; i < level->used; i++) {
    if (level->node[i].type == SKIP_NODE) level->statsIntNodes++;
    else if (level->node[i].type == START_NODE) level->statsExtNodes++;
  }
    
  level->used = 0;
  memset(level->node, 0, config->levelPageSize);
}


static void nodeAppend(Node node, struct trie_single_level *level)
{
  level->node[level->used] = node;
  level->used++;
}


/*  
 *  Append the nodes of the right trie level to those of the left trie level.
 *  
 *  If the right trie level is entirely in memory then append as many nodes
 *  as possible to the left trie level.  If we couldn't get them all, we
 *  write the left page to disk and call ourself again.
 *  
 *  If the right trie level has disk pages then we append, to the left trie
 *  level, a pointer to the first disk block of the right trie level, write
 *  the left page to disk, and copy the right page to the left page.
 *  
 *  Let, N = g_levelPageSize / sizeof(Node), be the number of nodes that can
 *  fit in a disk page.  We consider a page to be full when the number of
 *  nodes in it is equal to N - 1.  (This allows us to append a final node
 *  pointing to the page's successor, or a node terminating this level.)
 */
static void *mergeSingleLevel(void *conf, void *leftLev, void *rightLev)
{
  Node blkPtr;
  struct patrie_config *config = conf;
  struct trie_single_level *left = leftLev, *right = rightLev;

#if 0
  if right trie is entirely in memory then {
    move as much as possible to left trie
    if right trie not empty then {
      write out left trie page
      move as much as possible to left trie (i.e. move everything)
    }
  } else {
    append right.firstPageNum to left trie page
    write out left trie page
    move as much as possible to left trie (i.e. move everything)
  }

  return left trie
#endif
  
  if (right->firstPageNum == -1) {
    appendLevels(config, left, right);
    if (right->used > 0) {
      blkPtr.type = BLKNO_NODE; blkPtr.value = config->nextAvailPage++;
      nodeAppend(blkPtr, left);
      writeTriePage(config, left);
      left->pageNum = blkPtr.value; /* prepare for next write */
      appendLevels(config, left, right);
    }
  } else {
    blkPtr.type = BLKNO_NODE; blkPtr.value = right->firstPageNum;
    nodeAppend(blkPtr, left);
    writeTriePage(config, left);
    left->pageNum = right->pageNum;
    appendLevels(config, left, right);
  }

  /*  
   *  Stats.
   */

  left->statsIntNodes += right->statsIntNodes;
  left->statsExtNodes += right->statsExtNodes;
  
  /*  
   *  We can free up the right trie level: there's nothing left in it.  Also,
   *  note that the left trie level has space for at least one node (i.e. the
   *  pointer to the next page).
   */

  _patrieFreeSingleLevel(&right);
  
  return left;
}


/*  
 *  
 *  If the `node' is a height convert it to a skip relative to the current
 *  height.
 *  
 *  (See footnote 2 in Algorithm 5.)
 *  
 */
static void cvtHeightToSkip(struct patrie_config *config, int currHeight, Node *node)
{
  if (node->type == HEIGHT_NODE) {
    config->statsMaxHeight = MAX(config->statsMaxHeight, node->value); /* stats */

    ASSERT(node->value > currHeight);

    node->value = node->value - currHeight - 1;
    node->type = SKIP_NODE;

    config->statsMaxSkip = MAX(config->statsMaxSkip, node->value); /* stats */
  }
}


/*  
 *  Return the result of merging the stacks `left' and `right'.  `left' will
 *  contain the merger of the two stacks, while `right' will be empty (thus
 *  available for deletion).  Before returning, a single level containing the
 *  node `root' will be pushed on the new stack.
 */
static struct ptr_stack *mergeLevelStacks(struct patrie_config *config, Node root, struct ptr_stack *left, struct ptr_stack *right)
{
  struct trie_single_level *level;
  
  mergePtrStack(left, right, mergeSingleLevel, config);
  freePtrStack(&right); /* NEW */
  _patrieNewSingleLevel(config, &level);
  nodeAppend(root, level);
  pushPtrStack(left, level);

  return left;
}


/*  
 *  Return a stack containing a single trie level, containing the node
 *  `root'.
 */
static struct ptr_stack *createLevelStack(struct patrie_config *config, Node root)
{
  struct ptr_stack *levStack;
  struct trie_single_level *level;
  
  _patrieNewSingleLevel(config, &level);
  nodeAppend(root, level);
  newPtrStack(&levStack);
  pushPtrStack(levStack, level);

  return levStack;
}


static Node readNode(struct patrie_config *config)
{
  Node ret;
  int val;

  ASSERT(fread(&val, sizeof val, 1, config->infixTrieFP) != 0);

  ret.value = abs(val);
  if (val < 0) {
    ret.type = START_NODE;

    /* Debugging */
    
    config->debugMinStart = MIN(config->debugMinStart, ret.value);
    config->debugMaxStart = MAX(config->debugMaxStart, ret.value);
  } else {
    ret.type = HEIGHT_NODE;

    /* Debugging */
    
    config->debugMinHeight = MIN(config->debugMinHeight, ret.value);
    config->debugMaxHeight = MAX(config->debugMaxHeight, ret.value);
  }

  return ret;
}


int _patrieLevelBuild(struct patrie_config *cf)
{
  Node currNode;
  int debugLoopCount = 0;
  int done = 0;
  struct int_stack *heightStack;
  struct levels_stats stats;
  struct ptr_stack *trie;
  struct ptr_stack *trieStack;

  if (cf->printStats) {
    _patrieTimingStart(&stats.levelsTime);
  }

#if 0
  _patrieReadTrailer(cf, cf->infixTrieFP);
#endif

  rewind(cf->infixTrieFP);

  if (cf->printDebug) {
    cf->debugMinHeight = cf->debugMinStart = ULONG_MAX;
    cf->debugMaxHeight = cf->debugMaxStart = 0;
  }

  newIntStack(&heightStack);
  newPtrStack(&trieStack);

  currNode = readNode(cf);
  pushIntStack(heightStack, currNode.value);
  currNode = readNode(cf);

  while ( ! done) {
    if (currNode.type == START_NODE) {
      /* current node is a start */
      pushPtrStack(trieStack, createLevelStack(cf, currNode));
      currNode = readNode(cf);
    } else if (currNode.value == 0 && topIntStack(heightStack) == 0) {
      done = 1;
      (void)popIntStack(heightStack);
    } else if (topIntStack(heightStack) < currNode.value) {
      pushIntStack(heightStack, currNode.value);
      currNode = readNode(cf);
    } else {
      do {
        Node newNode;
        struct ptr_stack *left, *newTrie, *right;

        right = popPtrStack(trieStack);
        left = popPtrStack(trieStack);

        newNode.value = popIntStack(heightStack);
        newNode.type = HEIGHT_NODE;

        cvtHeightToSkip(cf, newNode.value, rootNode(left));
        cvtHeightToSkip(cf, newNode.value, rootNode(right));

        newTrie = mergeLevelStacks(cf, newNode, left, right);

        pushPtrStack(trieStack, newTrie);
        debugLoopCount++;
      } while ( ! (topIntStack(heightStack) <= currNode.value));
    }
  }

  trie = popPtrStack(trieStack);

  ASSERT(isEmptyIntStack(heightStack));
  ASSERT(isEmptyPtrStack(trieStack));

  freeIntStack(&heightStack);
  freePtrStack(&trieStack);
  
  {
    int *first;
    int i, nlevs;
    Node term;
    
    term.type = TERM_NODE; term.value = 0;

    nlevs = sizePtrStack(trie);
    ASSERT((first = MALLOC((nlevs + 1) * sizeof *first)));

    for (i = 0; ! isEmptyPtrStack(trie); i++) {
      struct trie_single_level *l = popPtrStack(trie);
      nodeAppend(term, l);
      writeTriePage(cf, l);
      first[i] = l->firstPageNum;
      _patrieFreeSingleLevel(&l);
    }
    first[i] = nlevs;           /* store the number of levels last */

    ASSERT(fwrite(first, sizeof *first, nlevs + 1, cf->levelTrieFP));

    fflush(cf->levelTrieFP);
    freePtrStack(&trie); free(first);
  }

#if 0
  _patrieWriteTrailer(cf, cf->levelTrieFP);
#endif

  if (cf->printStats) {
    _patrieTimingEnd(&stats.levelsTime);

    stats.levelsSize = _patrieFpSize(cf->levelTrieFP);
    stats.blockSize = cf->levelPageSize;

    printStats(stats);
  }

  if (cf->printDebug) {
#if 0
    printLevelInfo(cf, *nlevs, *levels);
#endif

    fprintf(stderr, "patrieLevelBuild:\n");
    fprintf(stderr, "\tMinimum height: %lu, maximum height: %lu\n",
            cf->debugMinHeight, cf->debugMaxHeight);
    fprintf(stderr, "\tMinimum start: %lu, maximum start: %lu\n",
            cf->debugMinStart, cf->debugMaxStart);
  }

  return 1;
}
