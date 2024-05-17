#include <sys/types.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>


#define NDIST (1000/CHAR_BIT+1)


#define BITS_IN_BCOUNT  32
#define BITS_IN_CCOUNT  32    /* count of th number of child pages */
#define BITS_IN_PAGENUM 32
#define BITS_IN_TCOUNT  32

#define HEIGHT_NODE 0
#define SKIP_NODE   HEIGHT_NODE
#define START_NODE  1
#define BLKNO_NODE  2
#define TERM_NODE   3

/* types of comparisons.  Case and Accent sensitivity. */

#define CMP_CIAI 0   /* 00 */
#define CMP_CIAS 1   /* 01 */
#define CMP_CSAI 2   /* 10 */
#define CMP_CSAS 3   /* 11 */
#define CMP_NULL -1  /* No comparison specified */

#define CMP_CASE_SENS 2
#define CMP_ACCENT_SENS 1

#define ASSERT(cond) \
do { \
  errno = 0; \
  if ( ! (cond)) { \
    fprintf(stderr, "%s:%d: failed assertion `%s'.\n", __FILE__, __LINE__, #cond); \
    if (errno != 0) perror("system call"); \
    abort(); \
    exit(1); \
  } \
} while (0)
/* 1 if c0 and c1 differ only in case, 0 otherwise */
#define DIFFER_IN_CASE_ONLY(c0, c1) (isalpha(c0) && ((unsigned char)((c0) ^ (c1)) == 0x20))
#if 1
# define MALLOC(n) (malloc(n))
#else
# define MALLOC(n) (calloc(1, (n)))
#endif
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#if 0
#define TOLOWER(c) tolower((unsigned char)(c))
#else
#define TOLOWER(c) (_patrieChLowerCase[(int)(unsigned char)(c)])
#endif

/*  
 *  bug: this structure is a crock.  We should make Node a long, and get rid
 *  of terminal nodes and block number nodes.  Blocks of single trie level
 *  nodes should be prefixed by the number of valid nodes in the block and
 *  the number of the next block.  Trie_single_level should be made opaque
 *  and be given an API.
 */
typedef struct node {
  unsigned int type  :2;
  unsigned int value :30;
} Node;


typedef int s32;          /* 32 bit signed integer */
typedef unsigned int u32; /* 32 bit unsigned integer */


/*  
 *  bug: In order to make the index portable between architectures we should
 *  store the data with the most significant byte first (i.e. big endian,
 *  network byte order).
 */  

struct patrie_page {
  int bCount;                   /* # of links leaving pages to our left */
  int nTCounts;                 /* # of child pages */
  int firstChildPage;           /* page offset of the first child page */
  int *tCount;                  /* t-count for each child page */
  int nodeBitOff;               /* bit offset (from 0) of first node's type bit */
  char *bits;                   /* pointer to _all_ the page's bits */
  int pageNum;                  /* the offset of the current page */
};

struct patrie_config {
  /*  
   *  User settable fields.
   */
  
  int doInfix;                  /* create the infix trie from the text input */
  int doInfixDistrib;           /* do distribution phase of sorting */
  int doInfixMerge;             /* do merge phase of sorting */
  int doLevels;                 /* create levels file from the infix input */
  int doPaged;                  /* create the paged trie from the levels */

  FILE *infixTrieFP;            /* the trie as skips and starts, in infix order */
  FILE *levelTrieFP;            /* the trie as a series of pages in level order */
  FILE *pagedTrieFP;            /* the final paged trie */
  FILE *textFP;

  int buildCaseAccentSens;      /* build trie to be: accent    case
                                            00 = 0 : insens    insens
                                            01 = 1 : insens    sens
                                            10 = 2 : sens      insens
                                            11 = 3 : sens      sens  */

  int levelPageSize;            /* number of bytes in a page of the levels file */
  int pagedPageSize;            /* number of bytes in a page of the final file */
  int levelsPerPage;            /* maximum number of trie levels in a page of the final file */
  int bitsPerSkip;              /* # bits required to hold a skip value, not including type bit */
  int bitsPerStart;             /* # bits required to hold a start value, not including type bit */

  int assertCheck;              /* 1: do assertion checking; 0: don't. */
  int printStats;               /* 1: print statistics; 0: don't. */
  int printDebug;               /* 1: print debugging information; 0: don't. */

  /* bug: should take args (e.g. read/write, errno)? */
  int (*allocError)(void);
  int (*ioError)(void);

  /*  
   *  For the sorting phase of building the trie.
   */

  const char *sortTempDir;
  int sortKeep;
  int infixKeepTemp;            /* don't remove temporary files, if 1 */
  long sortStart;               /* only consider sistrings starting from this offset: -1 is BOF */
  long sortEnd;                 /* ignore sistrings at, and after, this offset: -1 is EOF */
  size_t sortMaxMem;            /* max. amount of memory to allocate for buffers, etc. */
  size_t sortPadLen;            /* amount to pad to end of buffer */
  void *indexArg;               /* user argument to index points function */
  void (*setIndexPoints)(void *run,
                         void (*mark)(void *run, size_t indexPoint),
                         size_t tlen, const char *text, void *arg);

  /* Comparison functions for sorting and searching. */

  int (*strcomp)(const char *s0, const char *s1);
  long (*diffbit)(const char *str0, const char *str1);
  
  /*  
   *  Statistics gathering.
   */

  unsigned long statsMaxHeight; /* init. to 0 */
  unsigned long statsMaxSkip;   /* init. to 0 */
  unsigned long statsExtNodes;  /* init. to 0 */
  unsigned long statsIntNodes;  /* init. to 0 */
  unsigned long statsNodesOutput; /* init. to 0 */
  
  unsigned long statsSortCmpDA; /* # comparisons req. disk accesses */
  unsigned long statsSortHgtDA; /* # height calcs. req. disk accesses */
  unsigned long statsSortNumCmp; /* # comparisons */
  unsigned long statsSortNumHgt; /* # height calculations */
  unsigned long statsSortNumSs; /* # sistrings */

  unsigned long statsSortHgtDist[NDIST];

  /*  
   *  Debugging.
   */
  
  unsigned long debugMaxHeight;
  unsigned long debugMaxStart;
  unsigned long debugMinHeight;
  unsigned long debugMinStart;
  unsigned long debugNodesLoaded; /* init. to 0 */
  unsigned long debugNodesWritten; /* init. to 0 */

  /*  
   *  These fields are for internal use only.
   */

  size_t trailerSize;
  unsigned long nextAvailPage;  /* init. to 0 */
  struct patrie_page *triePage;
};


struct trie_single_level {
  int firstPageNum;             /* location of first disk page of this level */
  int pageNum;                  /* write the nodes to the pageNum'th (0, 1, ...) disk block */
  int used;                     /* how many nodes in this page have been filled */
  int nextNode;                 /* used by nextNode(): the next node to return */
  Node *node;                   /* an array the size of a disk block */

  /* Stats */
  
  int statsExtNodes;
  int statsIntNodes;
  int statsBlocks;
};


extern char *prog;
