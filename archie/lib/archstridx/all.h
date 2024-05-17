#define APPEND_FILE    "Stridx.Append"
#define IDX_NEW_FILE   "Stridx.Index.new"
#define IDX_FILE       "Stridx.Index"
#define LOCK_FILE      "Stridx.Lock"
#define SPLIT_FILE     "Stridx.Split"
#define STRINGS_FILE   "Stridx.Strings"
#define TYPE_FILE      "Stridx.Type"

#define ARCH_INDEX_NONE 0

#define INDEX_CHARS_STR "chars"
#define INDEX_WORDS_STR "words"

#define DIRSEP_CH  '/'
#define KEYSEP_CH  '\n'
#define KEYSEP_STR "\n"


enum search_type {
  SRCH_EXACT = -99, SRCH_REGEX, SRCH_SUBSTR, SRCH_UNSET
};


struct arch_stridx_handle {
  int mode;                     /* how the database was opened */

  char *dbdir;                  /* directory containing database files */
  char *tmpdir;                 /* directory containing temporary files for build */
  size_t buildMaxMem;           /* amt. of memory to use for index build */
  
  FILE *strsFP;                 /* file of keys (i.e. strings list) */
  FILE *idxFP;                  /* main index file */
  FILE *nidxFP;                 /* new index file */
  FILE *infxFP;                 /* temp. infix file */
  FILE *levFP;                  /* temp. levels file */

  int idxType;                  /* indexed by character or word */
  int prevnl;                   /* used by function to index on words */

  void *uiMem;                  /* chunk of memory containing unindexed text */
  char *uiText;                 /* pointer to key separator preceding unindexed text */
  size_t uiLen;                 /* strlen(uiText) */
  size_t uState;                /* index of where to resume searching */

  size_t rState;                /* like uState, but for regex searching */

  char *key;                    /* search key state */
  int caseAccentSens;           /* case sensitivity of current search state */

  struct patrie_config *cf;     /* config structure for the patrie index */
  struct patrie_state *pState;  /* state of last search done through `cf' */

  enum search_type srchType;    /* type of search, for saving state */

  long split;                   /* see note below */
  long next;                    /* offset of next string to be appended (i.e. file size) */

  float loadFactor;             /* save value until we can set it */
  struct new_string_cache *nsc;

  struct arch_lock *lock;

  /* Statistics */

  long numKeysProcessed;        /* # of calls to archAppendKey() */
  long numNewKeysAdded;         /* # of news keys appended to the strings file */
};

/*  
 *  ABOUT THE SPLIT.  The split is the offset, into the strings file, of the
 *  first character, after the initial key separator, not included in the
 *  PaTrie index.  A value of 1 indicates no index exists and that the entire
 *  strings file should be read into the unindexed text buffer.  A value equal
 *  to the size of the strings file means that the entire strings file has
 *  been indexed and that no strings need be read in.  Finally, for any other
 *  value we read in strings starting from an offset of `split - 1'.  We
 *  subtract one so that the first character of the text buffer is a key
 *  separator; which is required to do exact searches and word searches in the
 *  buffer.  Thus, if `nl' is the address of the first key separator at, or
 *  before, the match, then the start corresponding to that match is:
 *  
 *  split + (nl - uiText) - 1 [%] + 1 [*]
 *  
 *  which becomes:
 *  
 *  split + (nl - uiText)
 *  
 *  [%] compensate for the split corresponding to the _second_ character
 *  (after the initial key separator) of the unindexed text buffer.
 *  
 *  [*] to skip the key separator (pointed to by `nl') just before the match.
 */


extern char *prog;
