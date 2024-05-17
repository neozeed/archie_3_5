#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include "bits.h"
#include "case.h"
#include "defs.h"
#include "patrie.h"
#include "search.h"
#include "state.h"
#include "utils.h"


#define IS_START(off, src) (((src) + (off) / CHAR_BIT) \
                            & (1 << (CHAR_BIT - ((off) % CHAR_BIT) - 1)))
#define IS_SKIP(bitOff) ( ! IS_START(bitOff))
#define SKIP_BIT 0


enum child {LEFT, RIGHT};

/*
 *  The following map represent the mapping of search types with patrie types
 *  in terms of case and accent sensitivity.
 *  SearchMap [Search-type] [Patrie-build-type];
 */

#define MATCHMAP_SZ 4

int MatchMap[ MATCHMAP_SZ ][ MATCHMAP_SZ ] = {
  {CMP_CIAI, CMP_NULL, CMP_NULL, CMP_NULL},
  {CMP_CIAS, CMP_CIAS, CMP_NULL, CMP_NULL},
  {CMP_CSAI, CMP_NULL, CMP_CSAI, CMP_NULL},
  {CMP_CSAS, CMP_CSAI, CMP_CIAS, CMP_CSAS}
};

static void getNodeAt(struct patrie_config *config, int off,
                      char *src, int *isSkip, unsigned long *val);

static int  keyTypeMatch(struct patrie_config *cf,
                           const char *key, long offset, int type);

static int  keyMatchesIdxType(struct patrie_config *cf,
                           const char *key, long offset);

static int  keyMatchesIdxSearchType(struct patrie_config *cf,
                                    int stype, int ptype,
                                    const char *key, long offset);


/*  
 *
 *
 *                           Debugging routines.
 *
 *
 */  

static void printNode(FILE *fp, struct patrie_node *node)
{
  fprintf(fp, "%lu (%s), level %d, height %d, pos %d, inode %d, enode %d, tnode %d, page %d.\n",
          node->val, node->is_skip ? "skip" : "start", node->lev, node->height,
          node->pos, node->inode, node->enode, node->tnode, node->pageNum);
}


static void printPage(struct patrie_config *config, struct patrie_page *page, int nChild)
{
  int i, off;
  struct patrie_node node;

  fprintf(stderr, "bCount %d, firstChildPage %d, nTCounts %d, page %d\n",
          page->bCount, page->firstChildPage, page->nTCounts, page->pageNum);
  fprintf(stderr, "tCounts:");
  for (i = 0; i < page->nTCounts; i++) fprintf(stderr, " %d", page->tCount[i]);
  fprintf(stderr, "\n");

  off = page->nodeBitOff;
  for (i = 0; i < config->levelsPerPage && nChild > 0; i++) {
    int j, n = 0;
    for (j = 0; j < nChild; j++) {
      getNodeAt(config, off, page->bits, &node.is_skip, &node.val);
      fprintf(stderr, node.is_skip ? " (%d)" : " [%d]", node.val);
      off += node.is_skip ? config->bitsPerSkip + 1 : config->bitsPerStart + 1;
      n += node.is_skip ? 2 : 0;
    }
    nChild = n;
    fprintf(stderr, "\n");
  }
}


/*
 *
 *
 *                            Internal routines.
 *
 *
 */


/*  
 *  Return the value of the n'th bit of `src'.  Bit 0 is the most significant
 *  bit.
 *  
 *  Assume `n' is within the range of the key.
 */
static int bit(int n, const char *src)
{
  int chpos = n / CHAR_BIT;
  int bmask = 1 << (CHAR_BIT - 1 - (n % CHAR_BIT));

  return (src[chpos] & bmask) != 0;
}


static int initPageBuffer(struct patrie_config *config)
{
  char *diskPage;
  int *tcounts;
  int n;
  struct patrie_page *triePage;
    
  if ( ! (triePage = malloc(sizeof *triePage))) {
    fprintf(stderr, "%s: initPageBuffer: can't malloc() %lu bytes for trie page buffer:",
            prog, (unsigned long)sizeof *triePage); perror("");
    return 0;
  }
  n = config->pagedPageSize * BITS_IN_TCOUNT / (config->bitsPerSkip + 1);
  if ( ! (tcounts = malloc(n))) {
    fprintf(stderr, "%s: initPageBuffer: can't malloc() %d bytes for child t-counts:",
            prog, n); perror("");
    free(triePage);
    return 0;
  }
  if ( ! (diskPage = malloc(config->pagedPageSize))) {
    fprintf(stderr, "%s: initPageBuffer: can't malloc() %d bytes for page buffer:",
            prog, config->pagedPageSize); perror("");
    free(triePage); free(tcounts);
    return 0;
  }
  
  triePage->tCount = tcounts;
  triePage->bits = diskPage;

  config->triePage = triePage;

  return 1;
}


/*  
 *  Return the type and value of the node starting at a bit offset of `off'
 *  into `src'.  The first bit gives the type; following bits (the exact
 *  number depends on the type) contain the value.
 */
static void getNodeAt(struct patrie_config *config, int off, char *src, int *isSkip, unsigned long *val)
{
#if 1
  /*
   *  Take advantage of the (current) fact that all values are 32 bit
   *  quantities and lie on 32 bit boundaries.  This approximately doubles the
   *  speed of searching, compared with the other branch of the #if.
   */

  unsigned long v;

  v = *(unsigned long *)(src + off / CHAR_BIT);

  *isSkip = (v >> 31) == SKIP_BIT;
  *val = v & ~(1 << 31); /* mask out the type */

#else
  
  int b, is_skip;
  unsigned long v;

  is_skip = bit(off, src) == SKIP_BIT;
  b = is_skip ? config->bitsPerSkip : config->bitsPerStart;

  /*
   *  bug: HACK until regular bit copy works (+ 0 will become + 1)
   */
  
  copyBits32(b, off + 0, src, 0, (char *)&v); 
  v &= ~(1 << 31); /* mask out the type */
  
  *isSkip = is_skip;
  *val = v;
#endif
}


static void loadPage(struct patrie_config *config, long pageNum)
{
  int bitOff, i;
  struct patrie_page *page = config->triePage;
  
  ASSERT(fseek(config->pagedTrieFP, pageNum * config->pagedPageSize, SEEK_SET) != -1);
  ASSERT(fread(page->bits, 1, config->pagedPageSize, config->pagedTrieFP));
  
  bitOff = 0;

  copyBits32(BITS_IN_BCOUNT, bitOff, page->bits, 0, (char *)&page->bCount);
  bitOff += BITS_IN_BCOUNT;

  copyBits32(BITS_IN_PAGENUM, bitOff, page->bits, 0, (char *)&page->firstChildPage);
  bitOff += BITS_IN_PAGENUM;

  copyBits32(BITS_IN_CCOUNT, bitOff, page->bits, 0, (char *)&page->nTCounts);
  bitOff += BITS_IN_CCOUNT;

  for (i = 0; i < page->nTCounts; i++) {
    copyBits32(BITS_IN_TCOUNT, bitOff, page->bits, 0, (char *)(page->tCount + i));
    bitOff += BITS_IN_TCOUNT;
  }

  page->nodeBitOff = bitOff;

#if 1
  page->pageNum = pageNum;
#else
#warning HACK
  page->pageNum = ftell(config->pagedTrieFP) / config->pagedPageSize - 1;
#endif
}

  
/*  
 *  Replace the contents of `*node' with either its left or right child
 *  (determined by the value of `child').
 */
static void getTrieChild(struct patrie_config *config, enum child child, struct patrie_node *node)
{
  int childLevel;
  int childPosition;            /* always >= 1 */
  int height;
  struct patrie_page *page = config->triePage;
  
  childLevel = node->lev + 1;
  childPosition = 2 * node->inode;
  height = node->height;

  if (child == LEFT) --childPosition;

  while ( ! (node->lev == childLevel && node->inode + node->enode == childPosition)) {
    if (node->inode + node->enode == node->tnode) {
      /* this node is the last node on the current level */
      node->lev++;
      node->tnode = 2 * node->inode;
      node->height++;
      node->inode = 0;
      node->enode = 0;

      height++;
    }

    node->pos += node->is_skip ? config->bitsPerSkip + 1 : config->bitsPerStart + 1;
    getNodeAt(config, node->pos, page->bits, &node->is_skip, &node->val);
    
    if (node->is_skip) node->inode++;
    else node->enode++;
  }

  if (node->is_skip) {
    node->height = height + node->val;
  }
}


static void getPagedTrieChild(struct patrie_config *config, enum child child, struct patrie_node *node)
{
  struct patrie_page *page = config->triePage;

  /*  
   *  If the page currently in memory is inconsistent with the current node we
   *  must reload the correct page.  This might occur when doing a depth first
   *  traversal of a subtrie.  For example, if the traversal crosses a page
   *  boundary, then we unwind back to the original node, the current page
   *  will be inconsistent with the node.
   *  
   *  bug: HACK.
   */

  if (node->pageNum != page->pageNum) {
    loadPage(config, node->pageNum);
  }
  
  /*  
   *  Is the child of the current node on a different page?  (node->lev == 1
   *  for the root node.)
   */
  
  if (node->lev % config->levelsPerPage != 0) {
    getTrieChild(config, child, node);
  } else {
    int childPosition;          /* always >= 1 */
    int i;

    childPosition = node->inode * 2 + page->bCount;
    if (child == LEFT) --childPosition;

    /* we know tCount[0] will be less... */
    for (i = 1; page->tCount[i] < childPosition; i++) {
      continue;
    }

    childPosition -= page->tCount[i-1];
    node->lev++;
    node->tnode = page->tCount[i] - page->tCount[i-1];
    node->height++;
    node->inode = 0; node->enode = 0;

    loadPage(config, page->firstChildPage + i - 1);

    node->pos = page->nodeBitOff;

    getNodeAt(config, node->pos, page->bits, &node->is_skip, &node->val);
    if (node->is_skip) node->inode++; else node->enode++;
    node->pageNum = page->pageNum; /* bug: HACK */

    /*
     *  bug: merge above code with below?; do while
     */

    while (node->enode + node->inode < childPosition) {
      node->pos += node->is_skip ? config->bitsPerSkip + 1 : config->bitsPerStart + 1;
      getNodeAt(config, node->pos, page->bits, &node->is_skip, &node->val);
      if (node->is_skip) node->inode++; else node->enode++;
    }

    if (node->is_skip) node->height += node->val;
  }
}


/*  
 *  Initialize `*node' to the root of the trie.
 */
static void getTrieRoot(struct patrie_config *config, struct patrie_node *node)
{
  struct patrie_page *page = config->triePage;
  long pageno;
  
  /*
   *  bug: this becomes -2 when informational page is appended.  [Actually,
   *       the informational page is probably not a good idea.  Just having a
   *       length, followed by that many bytes is probably better.
   */
  
  pageno = _patrieFpSize(config->pagedTrieFP) / config->pagedPageSize - 1;
  loadPage(config, pageno);

  getNodeAt(config, page->nodeBitOff, page->bits, &node->is_skip, &node->val);
  node->pageNum = page->pageNum; /* bug: HACK */

  node->lev = 1;
  node->pos = page->nodeBitOff;
  node->tnode = 1;
  node->height = node->val - 1;
  if (node->is_skip) {
    node->inode = 1; node->enode = 0;
  } else {
    node->inode = 0; node->enode = 1;
  }
}


/*  
 *  Return 1 if `key' can be found in the text file, starting at `offset',
 *  otherwise return 0.  The comparison is case sensitive.
 *  
 *  Note:
 *  
 *      On SunOS-4.1.4 getc() seems to follow the ANSI standard of applying
 *      the cast `(int)(unsigned char)' to the character just read.  This is
 *      despite the man page saying: "getc() returns the next character (that
 *      is, byte) from the named input stream, as an integer."  Anyway,
 *      casting it back to a char should work in all cases.
 */
static int keyMatches(struct patrie_config *cf, const char *key, long offset)
{
  int c;
  
  ASSERT(fseek(cf->textFP, (long)offset, SEEK_SET) != -1);

  while ((c = getc(cf->textFP)) != EOF) {
    if (*key == '\0') return 1; /* exhausted key before finding mismatch */
    if (*key != (char)c) return 0;
    key++;
  }

  /*  
   *  EOF is true, so return true only if we've just exhausted the key.
   */

  return *key == '\0';
}


/*  
 *  Return 1 if `key' can be found in the text file, starting at `offset',
 *  otherwise return 0.  The comparison is case insensitive.
 *  
 *  Note:
 *  
 *      See the note for keyMatches().  In our case the TOLOWER macro
 *      casts its arguments, making any other casts superfluous.
 */
static int keyMatchesCase(struct patrie_config *cf, const char *key, long offset)
{
  int c;
  
  ASSERT(fseek(cf->textFP, (long)offset, SEEK_SET) != -1);

  while ((c = getc(cf->textFP)) != EOF) {
    if (*key == '\0') return 1; /* exhausted key before finding mismatch */
    if (TOLOWER(*key) != TOLOWER(c)) return 0;
    key++;
  }

  /*  
   *  EOF is true, so return true only if we've just exhausted the key.
   */

  return *key == '\0';
}

/*  
 *  Return 1 if `key' can be found in the text file, starting at `offset',
 *  otherwise return 0.  The comparison is accent insensitive.
 *  
 *  Note:
 *  
 *      See the note for keyMatches().  In our case the TOLOWER macro
 *      casts its arguments, making any other casts superfluous.
 */
static int keyMatchesAccent(struct patrie_config *cf, const char *key, long offset)
{
  int c;
  
  ASSERT(fseek(cf->textFP, (long)offset, SEEK_SET) != -1);

  while ((c = getc(cf->textFP)) != EOF) {
    if (*key == '\0') return 1; /* exhausted key before finding mismatch */
    if (NoAccent(*key) != NoAccent(c)) return 0;
    key++;
  }

  /*  
   *  EOF is true, so return true only if we've just exhausted the key.
   */

  return *key == '\0';
}


/*  
 *  Return 1 if `key' can be found in the text file, starting at `offset',
 *  otherwise return 0.  The comparison is accent and case insensitive.
 *  
 *  Note:
 *  
 *      See the note for keyMatches().  
 */
static int keyMatchesCiAi(struct patrie_config *cf, const char *key, long offset)
{
  int c;
  char c0, c1;

  ASSERT(fseek(cf->textFP, (long)offset, SEEK_SET) != -1);

  while ((c = getc(cf->textFP)) != EOF) {
    if (*key == '\0') return 1; /* exhausted key before finding mismatch */
    c0 = NoAccent(*key); c0 = TOLOWER(c0);
    c1 = NoAccent(c); c1 = TOLOWER(c1);
    if ((c0) != (c1)) return 0;
    key++;
  }

  /*  
   *  EOF is true, so return true only if we've just exhausted the key.
   */

  return *key == '\0';
}

/*
 *  For consistency reasons, map the new function-names to their
 *  equivelant old function-names.
 */
static int keyMatchesCiAs(struct patrie_config *cf, const char *key, long offset)
{
  return keyMatchesCase( cf, key, offset );
}

static int keyMatchesCsAi(struct patrie_config *cf, const char *key, long offset)
{
  return keyMatchesAccent( cf, key, offset );
}

static int keyMatchesCsAs(struct patrie_config *cf, const char *key, long offset)
{
  return keyMatches( cf, key, offset );
}


/*
 *  
 *  
 *      Some functions used for matching keys but with respect to the type
 *      of the patrie-build and the type of search.  The type is
 *      case/accent determined.
 *  
 *  
 *  
 */


/*
 *  returns 1 if the key matches the word starting at "offset".  The
 *  matching is done with respect to the "type" provided.  The type
 *  specifies case and accent sesitivity.
 */
static int keyTypeMatch(struct patrie_config *cf,
                        const char *key, long offset, int type)
{
  int keymatch = 0;

  switch( type ){
  case CMP_CIAI:
    keymatch = keyMatchesCiAi(cf, key, offset);
    break;
  case CMP_CIAS:
    keymatch = keyMatchesCiAs(cf, key, offset);
    break;
  case CMP_CSAI:
    keymatch = keyMatchesCsAi(cf, key, offset);
    break;
  case CMP_CSAS:
    keymatch = keyMatchesCsAs(cf, key, offset);
    break;
  case CMP_NULL:
  default:
    keymatch = 0;
    break;
  }

  return keymatch;
}

/*
 *  returns 1 if the key matches, 0 otherwise. The comparison is done
 *  with regard to the case/accent sesitivity of index-build.  The type of
 *  key Match peformed should match the index-build type.
 */
static int keyMatchesIdxType(struct patrie_config *cf,
                             const char *key,
                             long offset)
{
  int keymatch = 0;

  keymatch = keyTypeMatch( cf, key, offset, cf->buildCaseAccentSens );
  return keymatch;
}

/*
 *  returns 1 if the key matches, 0 otherwise. The matching is done
 *  according to the MatchMap[][] table.  This table defines which
 *  keyMatch to use with regard to the type of search and the type of
 *  index-build (case/accent sensitive/insensitive).
 */
static int keyMatchesIdxSearchType(struct patrie_config *cf,
                                  int stype,
                                  int ptype,
                                  const char *key,
                                  long offset)
{
  int type, keymatch = 0;

  if( stype > MATCHMAP_SZ || ptype > MATCHMAP_SZ ){
    type = CMP_NULL;
  }else{
    type = MatchMap[ stype ][ ptype ];
  }
  keymatch = keyTypeMatch( cf, key, offset, type );
  return keymatch;
}

#if 0
/*
 *  The PaTrie traversal algorithm uses an explicit stack.  The resulting C
 *  code is based on the following series of transformations.
 */

T(s)
{
  while ( ! Empty(s)) {
    n = pop(s)
    if (Ext(n)) {
      output n
    } else {
      push(n.right, s)
      push(n.left, s)
    }
  }
}


T(s)
{
  while ( ! Empty(s)) {
    n = pop(s)
  CONT:
    if (Ext(n)) {
      output n
    } else {
      push(n.right, s)
      n = n.left
      goto CONT
    }
  }
}


T(s)
{
  if (Empty(s)) return
  n = pop(s)

  while (1) {
    if (Ext(n)) {
      output n
      if (Empty(s)) return
      n = pop(s)
    } else {
      push(n.right, s)
      n = n.left
    }
  }
}
#endif


static int load_all_starts(struct patrie_config *cf,
                      unsigned long maxhits,
                      unsigned long *nhits,
                      unsigned long start[],
                      struct patrie_state *state)
{
  size_t n = 0;
  struct patrie_node node;

  *nhits = 0;
  if (maxhits == 0 ||
      _patrieStateEmpty(state)) {
    return 1;
  }

  node = _patrieStatePop(state);

  while (1) {
    if (node.is_skip) {
      struct patrie_node child = node;

      getPagedTrieChild(cf, RIGHT, &child);
      _patrieStatePush(child, state);

      getPagedTrieChild(cf, LEFT, &node);
    } else {
      start[n++] = node.val - 1;
      if (n >= maxhits || _patrieStateEmpty(state)) {
        *nhits = n;
        return 1;
      }
      node = _patrieStatePop(state);
    }
  }
}


/*
 *  This routine will be called if the case sensitivity of the index differs
 *  from that of the search.  That is, all the starts must be checked against
 *  the search key.
 */
static int load_matching_starts(struct patrie_config *cf,
                                const char *key,
                                unsigned long maxhits,
                                unsigned long *nhits,
                                unsigned long start[],
                                struct patrie_state *state)
{
  size_t n = 0;
  struct patrie_node node;

  *nhits = 0;
  if (maxhits == 0 ||
      _patrieStateEmpty(state)) {
    return 1;
  }

  node = _patrieStatePop(state);

  while (1) {
    if (node.is_skip) {
      struct patrie_node child = node;

      getPagedTrieChild(cf, RIGHT, &child);
      _patrieStatePush(child, state);

      getPagedTrieChild(cf, LEFT, &node);
    } else {
      /*
       *  Must check type of comparison for the patrie against the type of
       *  search being performed.
       */

      if (keyMatchesIdxSearchType(cf, state->matchCaseAccSens,
                                  cf->buildCaseAccentSens,
                                  key, node.val - 1)) {
        start[n++] = node.val - 1;
      }

      if (n >= maxhits || _patrieStateEmpty(state)) {
        *nhits = n;
        return 1;
      }

      node = _patrieStatePop(state);
    }
  }
}


/*
 *  Search for `key' in the PaTrie index.
 *  
 *  There are four cases we must consider, depending on the case sensitivity
 *  of the search and that of the underlying index.
 *  
 *  - search case sensitive, index case sensitive: use original search key,
 *  only check one substring with a case sensitive string compare.
 *  
 *  - search case sensitive, index case insensitive: use original search key,
 *  check all substrings with a case sensitive string compare.
 *  
 *  - search case insensitive, index case sensitive: not yet implemented.
 *  
 *  - search case insensitive, index case insensitive: lower case of search
 *  key, only check one substring with a case insensitive string compare.
 *  
 *  Return 0 if an error prevented us from performing the search, otherwise
 *  return 1.
 */
static int search(struct patrie_config *cf,
                  const char *key,
                  struct patrie_node *node,
                  struct patrie_state *state)
{
  int nKeyBits;
  
  nKeyBits = strlen(key) * CHAR_BIT;
  
  getTrieRoot(cf, node);

  if (cf->printDebug) {
    printNode(stderr, node);
  }

  while (node->is_skip) {
    if (nKeyBits <= node->height) {
      /*  
       *  We've checked all the bits we can.  Return with `*node' set to the
       *  node at which we stopped.
       */
      return 1;
    }

    if (cf->printDebug) {
      int b = bit(node->height, key);
      fprintf(stderr, "bit %d of key is %d: %s.\n", node->height, b, b ? "RIGHT" : "LEFT");
    }

    if (bit(node->height, key) == 0) {
      getPagedTrieChild(cf, LEFT, node);
      if (cf->printDebug) {
        printNode(stderr, node);
      }
    } else {
      getPagedTrieChild(cf, RIGHT, node);
      if (cf->printDebug) {
        printNode(stderr, node);
      }
    }
  }
  
  return 1;
}


/*
 *  This routine is similar to patrieSearchSub(), but instead of searching for
 *  `key' it returns more starts based on the stack used to traverse the
 *  PaTrie.  That is, it returns starts from where it left off, after the
 *  previous call to patrieSearchSub() or patrieGetMoreMatches().
 */
int patrieGetMoreStarts(struct patrie_config *cf,
                        unsigned long maxhits,
                        unsigned long *nhits,
                        unsigned long start[],
                        struct patrie_state *state)
{
  *nhits = 0;

  if (maxhits == 0 || _patrieStateEmpty(state)) return 1;

  if (cf->triePage == 0 && ! initPageBuffer(cf)) {
    fprintf(stderr, "%s: patrieGetMoreStarts: can't initialize PaTrie page buffer.\n", prog);
    return 0;
  }

  if ((state->matchCaseAccSens == cf->buildCaseAccentSens)) {
    if (state->firstMatched && ! load_all_starts(cf, maxhits, nhits, start, state)) {
      fprintf(stderr, "%s: patrieGetMoreStarts: can't load all starts.\n", prog);
      return 0;
    }
  } else {
    if ( ! load_matching_starts(cf, state->key, maxhits, nhits, start, state)) {
      fprintf(stderr, "%s: patrieGetMoreStarts: can't load matching starts.\n", prog);
      return 0;
    }
  }
  
  return 1;
}


/*
 *  Search the PaTrie index for `key'.  Return, through `start', at most
 *  `maxhits' results.  `*nhits' holds the actual number of results returned.
 *  If `matchCaseAccSens' is 0 then the case of letters is ignored when searching.
 *  
 *  If `search_state' is not null then the state is reset, and the new state,
 *  following the search, is returned through the pointer.  The existing state
 *  is always freed, whether or not the routine returns an error.
 */
int patrieSearchSub(struct patrie_config *cf,
                    const char *key,
                    int matchCaseAccSens,
                    unsigned long maxhits,
                    unsigned long *nhits,
                    unsigned long start[],
                    struct patrie_state **search_state)
{
  char *k;
  struct patrie_node node;
  struct patrie_state *state;

  if ( ! matchCaseAccSens && cf->buildCaseAccentSens) {
    fprintf(stderr, "%s: patrieSearchSub: case and accent insensitive search but case or accent sensitive index "
            "combination is not implemented.\n", prog);
    return 0;
  }

  if ( ! patrieAllocState(&state)) {
    fprintf(stderr, "%s: patrieSearchSub: can't allocate search state.\n", prog);
    return 0;
  }

  if (maxhits == 0) {
    if (search_state) {
      *search_state = state;
    } else {
      patrieFreeState(&state);
    }
    return 1;
  }

  if ( ! (k = stringcopy(key))) {
    fprintf(stderr, "%s: patrieSearchSub: can't allocate memory for copy of search key: ",
            prog); perror("stringcopy"); patrieFreeState(&state);
    return 0;
  }

  if ( ! (state->key = stringcopy(key))) {
    fprintf(stderr, "%s: patrieSearchSub: can't allocate memory for copy of search key: ",
            prog); perror("stringcopy"); patrieFreeState(&state);
    return 0;
  }
  state->matchCaseAccSens = matchCaseAccSens;

  if ( cf->buildCaseAccentSens != CMP_CSAS ) {
    int i; for (i = 0; k[i]; i++) {
      switch( cf->buildCaseAccentSens ){
      case CMP_CIAI:
        k[i] = TOLOWER(k[i]);
        k[i] = NoAccent(k[i]);
        break;
      case CMP_CIAS:
        k[i] = TOLOWER(k[i]);
        break;
      case CMP_CSAI:
        k[i] = NoAccent(k[i]);
        break;
      default:
        break;
      }
    }
  }

  if (cf->triePage == 0 && ! initPageBuffer(cf)) {
    fprintf(stderr, "%s: patrieSearchSub: can't initialize PaTrie page buffer.\n", prog);
    free(k); patrieFreeState(&state);
    return 0;
  }

  /*
   *  If there are no errors load_all_starts() is guaranteed to return one
   *  start.
   */
  
  if ( ! (search(cf, k, &node, state) &&
          _patrieStatePush(node, state) &&
          load_all_starts(cf, 1, nhits, start, state))) {
    fprintf(stderr, "%s: patrieSearchSub: can't find first matching start.\n", prog);
    free(k); patrieFreeState(&state);
    return 0;
  }

  /*
   *  Check whether the first start matches the search key.  If not, we can
   *  stop right now: no further matches will be found.  The case sensitivity
   *  of this comparison should be based solely on that of the index.
   *  
   *  The firstMatched field tells patrieGetMoreStarts() whether or not to
   *  blindly load all the remaining starts.  (I.e. If it's called when the
   *  first match failed then it shouldn't load the remaining starts.)
   *  
   *  bug: is this also true for the currently filtered case?
   */

  if ( ! keyMatchesIdxType( cf, key, start[0] ) ) {
    free(k);
    *nhits = 0;
    state->firstMatched = 0;
    if (search_state) {
      *search_state = state;
    } else {
      patrieFreeState(&state);
    }
    return 1;
  }

  state->firstMatched = 1;

  if (matchCaseAccSens == cf->buildCaseAccentSens) {
    /*
     *  Continue blindly loading starts from where we left off.  The
     *  construction of the index guarantees that the remaining starts will
     *  match.
     */
    
    if ( ! load_all_starts(cf, maxhits - 1, nhits, start + 1, state)) {
      fprintf(stderr, "%s: patrieSearchSub: can't load remaining starts.\n", prog);
      free(k); patrieFreeState(&state);
      return 0;
    }

    (*nhits)++;                 /* The first value of 1 got overwritten. */
  } else {
    /*
     *  Throw away the first start and begin again.  The first start can tell
     *  us whether or not to continue, but it may not be what we want
     *  `casewise'.
     */
    
    _patrieStateResetStack(state);

    if ( ! _patrieStatePush(node, state)) {
      fprintf(stderr, "%s: patrieSearchSub: reset search state.\n", prog);
      free(k); patrieFreeState(&state);
      return 0;
    }

    if ( ! load_matching_starts(cf, key, maxhits, nhits, start, state)) {
      fprintf(stderr, "%s: patrieSearchSub: can't load remaining starts.\n", prog);
      free(k); patrieFreeState(&state);
      return 0;
    }
  }    

  if (search_state) {
    *search_state = state;
  } else {
    patrieFreeState(&state);
  }

  free(k);

  return 1;
}

