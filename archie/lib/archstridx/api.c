/*
 *  bug: when building a new index we should make duplicates of all the files,
 *  except strings.
 */

/*
 *  The strings database consists of a text file, Stridx.Strings, containing
 *  the file names, a patrie, Stridx.Index, that indexes the first `n'
 *  characters of the text, a file, Stridx.Split, containing the value of `n'
 *  (which should be 1 or more) and Stridx.Type, indicating whether the
 *  strings file is indexed by words or by every character.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "all.h"
#include "patrie.h"
#include "archstridx.h"
#include "lock.h"
#include "newstr.h"
#include "re.h"
#include "utils.h"
#include "defs.h"
#include "strsrch.h"

#define BUILD_MAX_MEM 15000000 /* 15Mb */
#define DFLT_TMP_DIR  "/tmp"



/*
 *  The directory containing the temporary files required to build a new
 *  strings file index.  These files include the runs file, and the infix and
 *  levels files.
 */
static const char *const tmpDir = DFLT_TMP_DIR;


/*
 *
 *
 *                            Internal functions
 *
 *
 */


static int read_split(struct arch_stridx_handle *h)
{
  FILE *sfp;
  char *path;

  if ( ! (path = _archMakePath(h->dbdir, "/", SPLIT_FILE))) {
    fprintf(stderr, "%s: read_split: cann't build path to split file.\n", prog);
    return 0;
  }

  if ( ! (sfp = fopen(path, "r"))) {
    fprintf(stderr, "%s: read_split: can't open `%s': ", prog, path); perror("fopen");
    free(path);
    return 0;
  }

  if (fscanf(sfp, "%ld", &h->split) != 1) {
    fprintf(stderr, "%s: read_split: error reading split from `%s'.\n", prog, path);
    if (ferror(sfp)) perror("fscanf");
    fclose(sfp); free(path);
    return 0;
  }

  fclose(sfp); free(path);

  return 1;
}


static int write_split(struct arch_stridx_handle *h)
{
  FILE *sfp;
  char *path;

  if ( ! (path = _archMakePath(h->dbdir, "/", SPLIT_FILE))) {
    fprintf(stderr, "%s: write_split: can't build path to split file.\n", prog);
    return 0;
  }

  if ( ! (sfp = fopen(path, "w"))) {
    fprintf(stderr, "%s: write_split: can't open `%s' with mode `%s': ", prog, path, "w");
    perror("fopen"); free(path);
    return 0;
  }

  if (fprintf(sfp, "%010ld\n", h->split) < 0 || fflush(sfp) == EOF) {
    fprintf(stderr, "%s: write_split: error writing split: \n", prog);
    perror("fprintf or fflush"); fclose(sfp); free(path);
    return 0;
  }

  fclose(sfp); free(path);

  return 1;
}


static int read_type(struct arch_stridx_handle *h)
{
  FILE *fp;
  char *nl, *path;
  char type[50];

  if ( ! (path = _archMakePath(h->dbdir, "/", TYPE_FILE))) {
    fprintf(stderr, "%s: read_type: can't build path to type file.\n", prog);
    return 0;
  }

  if ( ! (fp = fopen(path, "r"))) {
    fprintf(stderr, "%s: read_type: can't open `%s': ", prog, path); perror("fopen");
    free(path);
    return 0;
  }

  if ( ! fgets(type, sizeof type, fp)) {
    fprintf(stderr, "%s: read_type: error reading index type from `%s': ", prog, path);
    if (feof(fp)) fprintf(stderr, "unexpected end of file.\n");
    else perror("fgets");
    free(path); fclose(fp);
    return 0;
  }

  if ((nl = strchr(type, '\n'))) *nl = '\0';

  if (strcmp(type, INDEX_CHARS_STR) == 0) {
    h->idxType = ARCH_INDEX_CHARS;
  } else if (strcmp(type, INDEX_WORDS_STR) == 0) {
    h->idxType = ARCH_INDEX_WORDS;
  } else {
    h->idxType = ARCH_INDEX_NONE;
    fprintf(stderr, "%s: read_type: unknown type, `%s', in index type file.\n",
            prog, type);
    free(path); fclose(fp);
    return 0;
  }

  free(path); fclose(fp);

  return 1;
}


static int write_type(struct arch_stridx_handle *h)
{
  FILE *fp;
  char *path, *type;

  if ( ! (path = _archMakePath(h->dbdir, "/", TYPE_FILE))) {
    fprintf(stderr, "%s: write_type: can't build path to type file.\n", prog);
    return 0;
  }

  if ( ! (fp = fopen(path, "w"))) {
    fprintf(stderr, "%s: write_type: can't open `%s' with mode `%s': ", prog, path, "w");
    perror("fopen"); free(path);
    return 0;
  }

  if        (h->idxType == ARCH_INDEX_CHARS) {
    type = INDEX_CHARS_STR;
  } else if (h->idxType == ARCH_INDEX_WORDS) {
    type = INDEX_WORDS_STR;
  } else {
    fprintf(stderr, "%s: write_type: unknown index type (%d).\n", prog, h->idxType);
    free(path); fclose(fp);
    return 0;
  }

  if (fprintf(fp, "%s\n", type) < 0 || fflush(fp) == EOF) {
    fprintf(stderr, "%s: write_type: error writing type: \n", prog);
    perror("fprintf or fflush"); free(path); fclose(fp);
    return 0;
  }

  free(path); fclose(fp);

  return 1;
}


/*
 *  The words to be indexed will be separated by a special separator
 *  character.  We assume that two of these characters do not occur
 *  consecutively.
 */
static void index_words(void *run,
                        void (*mark)(void *run, size_t indexPoint),
                        size_t tlen, const char *text, void *arg)
{
  int i;
  int *pnl = arg;
  int prevnl = *pnl;

  for (i = 0; i < tlen; i++) {
    if (prevnl) {
      mark(run, i);
      prevnl = 0;
    } else if (text[i] == KEYSEP_CH) {
      prevnl = 1;
    }
  }

  *pnl = prevnl;
}


/*
 *  As much as possible, restore the strings list database to its state prior
 *  to the last call to archOpenStrIdx().
 */
void rollback(struct arch_stridx_handle *h)
{
#warning UNFINISHED
}


/*
 *  Remove all database files in the directory `dbdir'.
 */
static int destroy_db_files(const char *dbdir)
{
  if (_archUnlinkFile(dbdir, APPEND_FILE)  &&
      _archUnlinkFile(dbdir, IDX_NEW_FILE) &&
      _archUnlinkFile(dbdir, IDX_FILE)     &&
      _archUnlinkFile(dbdir, SPLIT_FILE)   &&
      _archUnlinkFile(dbdir, STRINGS_FILE) &&
      _archUnlinkFile(dbdir, TYPE_FILE)) {
    return 1;
  } else {
    fprintf(stderr, "%s: destroy_db_files: can't clean out database directory `%s'.\n",
            prog, dbdir);
    return 0;
  }
}


/*
 *  Rewrite `*start' to be the offset of the beginning of the string into
 *  which it currently points.  That is, if the start points to the `o' in
 *  `\nzot\n', set it to point to the `z'.
 */
static int patrie_fix_one_start(FILE *tfp, unsigned long *start)
{
  int c;
  unsigned long st = *start - 1;

  /*
   *  Go to the specified offset, less one.  Since we're only called on
   *  substring matches we know the character at offset `*start' won't be a
   *  key separator, so we can begin with the one before it.
   */

  if (fseek(tfp, st, SEEK_SET) == -1) {
    fprintf(stderr, "%s: patrie_fix_one_start: error seeking to offset %lu in text file: ",
            prog, st); perror("fseek");
    return 0;
  }

  /*
   *  Read backwards through the file, looking for a key separator.
   */

  while ((c = getc(tfp)) != KEYSEP_CH) {
    if (fseek(tfp, --st, SEEK_SET) == -1) {
      fprintf(stderr, "%s: patrie_fix_one_start: error seeking to offset %lu in text file: ",
              prog, st); perror("fseek");
      return 0;
    }
  }

  *start = st + 1;              /* since `st' corresponds to the key separator */

  return 1;
}


/*
 *  Rewrite each element of the `start' array to be the offset of the
 *  beginning of the string into which it currently points.
 */
static int patrie_fix_starts(FILE *tfp, unsigned long nhits, unsigned long start[])
{
  unsigned long i;

  for (i = 0; i < nhits; i++) {
    if ( ! patrie_fix_one_start(tfp, &start[i])) {
      fprintf(stderr, "%s: patrie_fix_starts: can't fix up start %lu (value %lu).\n",
              prog, i, start[i]);
      return 0;
    }
  }

  return 1;
}


/*
 *  Rewrite `*start' to be the offset of the beginning of the string into
 *  which it currently points.  That is, if the start points to the `o' in
 *  `\nzot\n', set it to point to the `z'.
 */
static void ui_fix_one_start(const char *text, unsigned long *start)
{
  while (text[*start] != KEYSEP_CH) {
    --(*start);
  }
  (*start)++;
}


/*
 *  Rewrite each element of the `start' array to be the offset of the
 *  beginning of the string into which it currently points.
 */
static void ui_fix_starts(const char *text, unsigned long nhits, unsigned long start[])
{
  unsigned long i;

  for (i = 0; i < nhits; i++) {
    ui_fix_one_start(text, start + i);
  }
}


/*
 *  Remove duplicate values in the `start' array (which is assumed to be
 *  sorted).  The number of unique starts is returned through `uniq'.
 */
static void remove_duplicates(unsigned long n, unsigned long start[], unsigned long *uniq)
{
  int i, j;

  *uniq = 0;

  if (n == 0) return;

  i = 0; j = 1;
  while (j < n) {
    if (start[i] == start[j]) {
      j++;
    } else {
      start[++i] = start[j++];
    }
  }

  *uniq = i + 1;
}


/*
 *  Comparison function for sorting an array of starts.
 */
static int start_compare(const void *s0, const void *s1)
{
  unsigned long a = *(unsigned long *)s0;
  unsigned long b = *(unsigned long *)s1;

  if (a == b) return 0;
  else return a < b ? -1 : 1;
}


/*
 *  Remove duplicate starts from the list of hits.  The starts are sorted to
 *  gather together identical values.  The number of unique values is returned
 *  through `uniq'.
 */
static void remove_duplicate_starts(unsigned long n, unsigned long start[], unsigned long *uniq)
{
  *uniq = 0;

  if (n == 0) return;

  qsort(start, n, sizeof start[0], start_compare);
  remove_duplicates(n, start, uniq);
}


/*
 *  Search the PaTrie index for substrings matching `key'.  The `start' array
 *  will hold the offsets of the matching strings, and the number of strings
 *  found will be returned through `nhits'.  This routine ensures that all the
 *  returned starts are unique.
 */
static int patrie_sub_search(struct arch_stridx_handle *h,
                             const char *key,
                             unsigned long startAdj,
                             unsigned long maxhits,
                             unsigned long *nhits,
                             unsigned long *start)
{
  size_t i;
  unsigned long hits, tothits = 0;

  if ( ! patrieSearchSub(h->cf, key, h->caseAccentSens, maxhits, &hits, start, &h->pState)) {
    fprintf(stderr, "%s: patrie_sub_search: error searching index for `%s'.\n",
            prog, key);
    return 0;
  }

  /*
   *  Compensate for the potential key separator prepended to the search key.
   */

  for (i = 0; i < hits; i++) {
    start[i] += startAdj;
  }

  /*
   *  Rewrite each start to point to its initial character, then remove
   *  duplicate starts.
   */

  if ( ! patrie_fix_starts(patrieGetTextFP(h->cf), hits, start + tothits)) {
    fprintf(stderr, "%s: patrie_sub_search: error fixing up match offsets.\n", prog);
    return 0;
  }

  remove_duplicate_starts(hits, start + tothits, &hits);

  tothits += hits;

  /*
   *  bug: even though patrieGetMoreStarts() shouldn't load more starts if
   *  patrieSearchSub() failed to find _any_ matches, we still probably ought
   *  to check that here, just to make it clear that we shouldn't continue
   *  looking.
   */

  if ( ! archGetMoreMatches(h, maxhits - tothits, &hits, start)) {
    fprintf(stderr, "%s: patrie_sub_search: can't get more matches.\n", prog);
    return 0;
  }

  tothits += hits;

  *nhits = tothits;

  return 1;
}


/*
 *  Scan the unindexed text buffer for the substring given by `key'.  The
 *  search is started at the last place we left off, `h->uState'.  This
 *  routine handles the removal of duplicate starts, and ensures that either
 *  `maxhits' starts will be returned, or that no more text remains to be
 *  searched.
 */
static int uitext_sub_search(struct arch_stridx_handle *h,
                             const char *key,
                             unsigned long startAdj,
                             unsigned long maxhits,
                             unsigned long *nhits,
                             unsigned long start[])
{
  int moreToSearch = 1;
  unsigned long hits, tothits = 0;
  struct funcs funs;

  if (h->uState >= h->uiLen - 1) {
    return 1;
  }

  /*
   *  Loop, searching for matches, until we've found as many as were
   *  requested, or until we've exhausted the text.
   */

  switch (h->caseAccentSens) {
  case 0:
    funs.lowerCase = (tolower);
    funs.upperCase = (toupper);
    funs.noAccent = (NoAccent);
    funs.withAccent = (identical);
    funs.toComp = (_patrieUStrCiAiCmp);
    break;
  case 1:
    /*
     *  Case Insensitive - Accent Sensitive
     */
    funs.lowerCase = (tolower);
    funs.upperCase = (toupper);
    funs.noAccent = (identical);
    funs.withAccent = (identical);
    funs.toComp = (_patrieUStrCiAsCmp);
    break;
  case 2:
    /*
     *  Case Sensitive - Accent Insensitive
     */
    funs.lowerCase = (identical);
    funs.upperCase = (identical);
    funs.noAccent = (NoAccent);
    funs.withAccent = (identical);
    funs.toComp = (_patrieUStrCsAiCmp);
    break;
  case 3:
    /*
     *  Case Sensitive - Accent Sensitive
     */
    funs.lowerCase = (identical);
    funs.upperCase = (identical);
    funs.noAccent = (identical);
    funs.withAccent = (identical);
    funs.toComp = (_patrieUStrCsAsCmp);
    break;
  }
  while (tothits < maxhits && moreToSearch) {

    _archBMHGeneralSearch(h->uiText + h->uState, h->uiLen - h->uState, key,
                          strlen(key), maxhits - tothits, &hits, start + tothits,
                          funs);

/*
 *  (h->caseSens ?  _archBMHSearch : _archBMHCaseSearch)(h->uiText +
 *    h->uState, h->uiLen - h->uState, key, strlen(key), maxhits -
 *    tothits, &hits, start + tothits);
 */

    if (hits == 0) {
      moreToSearch = 0;
    } else {
      size_t i, newstate;

      /*
       *  The new state should point to the beginning of the next _word_ in
       *  the unindexed text.  (If one word contains multiple instances of the
       *  key, we only want to return it once.)  Look for the beginning of the
       *  next word, starting one character past the last hit.
       */

      {
        unsigned long lasthit = h->uState + start[tothits + hits - 1];

        for (i = lasthit + 1; h->uiText[i] != KEYSEP_CH; i++) {
          continue;
        }
        newstate = i + 1;
      }

      /*
       *  Make starts relative to the beginning of the unindexed text buffer.
       *
       *  `startAdj' will be 1 if the first character of the key is a key
       *  separator, otherwise it will be 0.
       */

      for (i = tothits; i < tothits + hits; i++) {
        start[i] += h->uState + startAdj;
      }

      ui_fix_starts(h->uiText, hits, start + tothits);
      remove_duplicates(hits, start + tothits, &hits);

      /*
       *  Make starts relative to the beginning of the strings file,
       *  compensating for the initial key separator in the buffer.
       */

      for (i = tothits; i < tothits + hits; i++) {
        start[i] += h->split - 1;
      }

      h->uState = newstate;

      tothits += hits;
    }
  }

  *nhits = tothits;

  return 1;
}


/*
 *  Load the unindexed part of the strings file into memory.
 *
 *  Since the strings file doesn't have a terminating nul we allocate one more
 *  byte than is actually required (by mmap()) to store it.  We use valloc()
 *  to get the memory, since mmap() requires page alignment.  Mmap() also
 *  requires that the offset into the file, at which to start mapping, be page
 *  aligned, so we need a separate pointer into the mapped memory to indicate
 *  where to start searching.  (I.e. It is unlikely that the key separator
 *  preceding the split will fall on a page boundary.)
 *
 *  Here's a picture using the nomenclature of the routine.  (File and Memory
 *  are divided into page sized regions.)
 *
 *
 *                           split-1
 *                          |
 *                          | split
 *                          ||
 *  <---------------------- || ------- fsz --------------->
 *  <----- off ----->       vv
 *  +---------------+---------------+---------------+-----+
 *  |               |       ..      |               |     |
 *  |               |       ..      |               |     |
 *  +---------------+---------------+---------------+-----+
 *      File                ..                            .
 *                          ..                            .
 *                  <------ .. --------------- msz ------ . --------->
 *                  <------ .. -------- tsz -------------->
 *                  +---------------+---------------+----------------+
 *                  |       ..      |               |     .          |
 *                  |       ..      |               |     .          |
 *                  +---------------+---------------+----------------+
 *      Memory      ^       ^                             ^^
 *                  |       |                             ||
 *                  uiMem   uiText                        | uiMem+tsz
 *                                                        |
 *                                                         uiMem+tsz-1
 *
 *
 *  bug: do the in-core string search routines actually require a terminating
 *  nul?
 */
static int load_unindexed_text(FILE *fp, struct arch_stridx_handle *h)
{
  char *mem;
  long fsz, off, tsz;
  static long psz = 0;

  if ((fsz = _archFileSizeFP(fp)) == -1) {
    fprintf(stderr, "%s: load_unindexed_text: can't get size of strings file.\n", prog);
    return 0;
  }

  /*
   *  If the split is equal to the file size then the entire strings file has
   *  been indexed.
   */

  if (fsz == h->split) {
    return 1;
  }

  if (psz == 0 && (psz = _archSystemPageSize()) == -1) {
    fprintf(stderr, "%s: load_unindexed_text: can't get system page size.\n", prog);
    return 0;
  }

  /*
   *  `off' is the offset, into the strings file, of the first byte in the
   *  page containing the key separator preceding the split.  (Therefore it's
   *  multiple of the page size.)
   */

  off = (h->split - 1) - ((h->split - 1) % psz);

  /*
   *  `tsz' is the amount of memory required to hold just the text.
   */

  tsz = fsz - off;

  {
    int flags = MAP_SHARED;
    int prot = PROT_READ;

    if ((mem = mmap(0, tsz, prot, flags, fileno(fp), off)) == (char *)-1) {
      fprintf(stderr, "%s: load_unindexed_text: can't map file: ", prog);
      perror("mmap");
      return 0;
    }
  }

  /*
   *  `uiText' points to the key separator just before the split, and `uiText
   *  + uiLen - 1' is the offset of the last valid character.
   */
  
  h->uiMem = mem;
  h->uiText = (char *)h->uiMem + ((h->split - 1) - off);
  h->uiLen = (char *)h->uiMem + tsz - h->uiText;

  return 1;
}


/*
 *  Open a temporary file, in `dir', with `mode'.  The file is unlink()ed.
 */
static int tempOpen(const char *dir, const char *mode, FILE **fp)
{
  FILE *f;
  char *path, *dirsep;

  /*
   *  If tempnam() is given a bogus directory name, it will silently convert
   *  it to its default (e.g. /var/tmp).  We'll check that its return value is
   *  what we're looking for...
   */

  if ( ! (path = tempnam(dir, "infx"))) {
    fprintf(stderr, "%s: tempOpen: can't get temporary file name: ", prog); perror("tempnam");
    return 0;
  }

  if ( ! (dirsep = strrchr(path, DIRSEP_CH))) {
    fprintf(stderr, "%s: tempOpen: missing `%c' in temporary file name `%s'.\n",
            prog, DIRSEP_CH, path);
    free(path);
    return 0;
  }

  if (strncmp(dir, path, dirsep - path) != 0) {
    fprintf(stderr, "%s: tempOpen: can't create file in `%s': "
            "check existence or permissions of directory.\n", prog, dir);
    free(path);
    return 0;
  }

  if ( ! (f = fopen(path, mode))) {
    fprintf(stderr, "%s: tempOpen: can't open `%s' with mode `%s': ", prog, path, mode);
    perror("fopen"); free(path);
    return 0;
  }

  if (unlink(path) == -1) {
    fprintf(stderr, "%s: tempOpen: can't unlink temporary file `%s': ", prog, path);
    perror("unlink"); free(path);
    return 0;
  }

  free(path);
  *fp = f;

  return 1;
}


/*
 *  Open the database for appending to an existing strings list.
 */
static int open_append(struct arch_stridx_handle *h)
{
  if ( ! _archFileOpen(h->dbdir, STRINGS_FILE, "a+", 1, &h->strsFP)) {
    fprintf(stderr, "%s: open_append: can't open strings file `%s/%s'.\n",
            prog, h->dbdir, STRINGS_FILE);
    return 0;
  }

  if ( ! read_split(h)) {
    fprintf(stderr, "%s: open_append: can't read the split value.\n", prog);
    return 0;
  }

  if ( ! (h->lock = _archLockShared(h->dbdir, LOCK_FILE))) {
    fprintf(stderr, "%s: open_append: can't get shared lock.\n", prog);
    return 0;
  }

  /*
   *  Open the index file and the strings file for reading.  It may be that
   *  the index file doesn't exist; if the database has recently been
   *  created, for example.  In this case just free the patrie structure for
   *  this index and continue.
   */

  if ( ! patrieAlloc(&h->cf)) {
    fprintf(stderr, "%s: open_append: can't allocate PaTrie structure.\n", prog);
    _archUnlock(&h->lock);
    return 0;
  }

  if ( ! _archFileOpen(h->dbdir, IDX_FILE, "rb", 0, &h->idxFP)) {
    fprintf(stderr, "%s: open_append: can't open index file `%s/%s'.\n",
            prog, h->dbdir, IDX_FILE);
    _archUnlock(&h->lock);
    return 0;
  } else if ( ! h->idxFP) {
    patrieFree(&h->cf);
  } else {
    patrieSetTextFP(h->cf, h->strsFP);
    patrieSetPagedFP(h->cf, h->idxFP);
    /* bug: this info should be in the database */
    patrieSetCaseSensitive(h->cf, 0);
  }

  _archUnlock(&h->lock);

  /*
   *  How the database was indexed will determine how we handle searching
   *  for exact keys.
   */

  if ( ! read_type(h)) {
    fprintf(stderr, "%s: open_append: can't determine how database was indexed.\n", prog);
    return 0;
  }

  if ( ! (h->nsc = _archNewStringCache(h->dbdir))) {
    fprintf(stderr, "%s: open_append: can't create new string cache.\n", prog);
    return 0;
  }

  if (h->loadFactor > 0) {
    (void)_archStringCacheLoadFactor(h->nsc, h->loadFactor);
  }

  if ( ! _archInitStringCache(h->nsc, h->strsFP, h->split)) {
    fprintf(stderr, "%s: open_append: can't initialize new strings cache.\n", prog);
    return 0;
  }

  h->next = _archFileSizeFP(h->strsFP);

  return 1;
}


/*
 *  Open the files necessary for building a new PaTrie index over the strings
 *  file.
 */
static int open_build(struct arch_stridx_handle *h)
{
  if ( ! patrieAlloc(&h->cf)) {
    fprintf(stderr, "%s: open_build: can't allocate PaTrie structure.\n", prog);
    return 0;
  }

  if ( ! read_type(h)) {
    fprintf(stderr, "%s: open_build: can't read type.\n", prog);
    return 0;
  }

  switch (h->idxType) {
  case ARCH_INDEX_CHARS:
    break;

  case ARCH_INDEX_WORDS:
    h->prevnl = 0;
    patrieSetIndexPointsFunction(h->cf, index_words, &h->prevnl);
    break;

  default:                      /* read_type() should catch this one. */
    fprintf(stderr, "%s: open_build: unknown database type (%d).\n", prog, h->idxType);
    return 0;
  }

  /* bug: this info should be in the database */
  patrieSetCaseSensitive(h->cf, 0);

  /*
   *  Open all the files we need to build the new index.
   */

  if ( ! (_archFileOpen(h->dbdir, STRINGS_FILE, "r", 1, &h->strsFP) &&
          tempOpen(h->tmpdir, "w+b", &h->infxFP) &&
          tempOpen(h->tmpdir, "w+b", &h->levFP) &&
          _archFileOpen(h->dbdir, IDX_NEW_FILE, "wb", 0, &h->nidxFP))) {
    fprintf(stderr, "%s: open_build: can't open new index files.\n", prog);
    rollback(h);
    return 0;
  }

  patrieSetTextFP(h->cf, h->strsFP);
  patrieSetInfixFP(h->cf, h->infxFP);
  patrieSetLevelFP(h->cf, h->levFP);
  patrieSetPagedFP(h->cf, h->nidxFP);

  /*
   *  bug: we should ensure that, at the time of checking the size, the file
   *  ends with a key separator.
   */
  h->split = _archFileSizeFP(patrieGetTextFP(h->cf));

  return 1;
}


/*
 *  Create a new strings list index.  Any existing database files will be
 *  removed.
 */
static int open_create(struct arch_stridx_handle *h)
{
  /*
   *  Ensure there are no existing database files hanging around.
   */

  if ( ! destroy_db_files(h->dbdir)) {
    fprintf(stderr, "%s: open_create: error cleaning up database directory.\n", prog);
    return 0;
  }

  /*
   *  Open a strings file in the appropriate directory and write an initial
   *  key separator to it.
   */

  if ( ! _archFileOpen(h->dbdir, STRINGS_FILE, "w", 1, &h->strsFP)) {
    fprintf(stderr, "%s: open_create: can't open strings file `%s/%s'.\n",
            prog, h->dbdir, STRINGS_FILE);
    return 0;
  }

  if (fputs(KEYSEP_STR, h->strsFP) == EOF) {
    fprintf(stderr, "%s: open_create: can't initialize strings list with a key separator: ",
            prog); perror("fputs");
    return 0;
  }

  /*
   *  Create the `split' file.  It contains the index, into the strings file,
   *  of the start of the unindexed strings.
   */

  h->split = 1;                 /* points past initial key separator */
  h->next = 1;                  /* after initial key separator */

  if ( ! write_split(h)) {
    fprintf(stderr, "%s: open_create: can't write split file.\n", prog);
    return 0;
  }

  /*
   *  Create the `type' file.  It indicates whether the database is indexed
   *  on word boudaries, or on every character.
   */

  if ( ! write_type(h)) {
    fprintf(stderr, "%s: open_create: can't write type file.\n", prog);
    return 0;
  }

  if ( ! (h->nsc = _archNewStringCache(h->dbdir))) {
    fprintf(stderr, "%s: open_create: can't create new string cache.\n", prog);
    return 0;
  }

  if (h->loadFactor > 0) {
    (void)_archStringCacheLoadFactor(h->nsc, h->loadFactor);
  }

  return 1;
}


/*
 *  Open the database for searching.
 */
static int open_search(struct arch_stridx_handle *h)
{
  FILE *fp;

  /*
   *  Create a patrie structure for the index.
   */

  if ( ! patrieAlloc(&h->cf)) {
    fprintf(stderr, "%s: open_search: can't allocate PaTrie structure.\n", prog);
    return 0;
  }

  /* bug: this info should be in the database */
  patrieSetCaseSensitive(h->cf, 0);

  /*
   *  Open the strings file.  It is required to exist.
   */

  if ( ! _archFileOpen(h->dbdir, STRINGS_FILE, "r", 1, &h->strsFP)) {
    fprintf(stderr, "%s: open_search: can't open strings file `%s/%s'.\n",
            prog, h->dbdir, STRINGS_FILE);
    return 0;
  }

  patrieSetTextFP(h->cf, h->strsFP);

  if ( ! (h->lock = _archLockShared(h->dbdir, LOCK_FILE))) {
    fprintf(stderr, "%s: open_search: can't get shared lock.\n", prog);
    return 0;
  }

  /*
   *  Open the patrie index.  If the strings database has recently been
   *  created this file may not exist, in which case we just carry on.
   */

  if (_archFileOpen(h->dbdir, IDX_FILE, "rb", 0, &fp) && fp) {
    h->idxFP = fp;
    patrieSetPagedFP(h->cf, h->idxFP);
  } else {
    patrieFree(&h->cf);
  }

  if ( ! read_split(h)) {
    fprintf(stderr, "%s: open_search: can't read split.\n", prog);
    _archUnlock(&h->lock);
    return 0;
  }

  _archUnlock(&h->lock);

  /*
   *  How the database was indexed will determine how we handle substring
   *  searching in the unindexed text.
   */

  if ( ! read_type(h)) {
    fprintf(stderr, "%s: open_search: can't determine how database was indexed.\n", prog);
    return 0;
  }

  /*
   *  Load, into a buffer, that part of the text that is unindexed.
   */

  if ( ! load_unindexed_text(h->strsFP, h)) {
    fprintf(stderr, "%s: open_search: can't load unindexed text into buffer.\n", prog);
    return 0;
  }

  return 1;
}


/*
 *
 *
 *                            External functions
 *
 *
 */


/*
 *  If the key does not already exist in the strings file, append it, then add
 *  it to the temporary database of newly added strings.
 */
int archAppendKey(struct arch_stridx_handle *h, const char *key, unsigned long *start)
{
  unsigned long found, st;

  if (key[0] == '\0') {
    fprintf(stderr, "%s: archAppendKey: key is the empty string!\n", prog);
    return 0;
  }
  
  /*
   *  Ensure the key doesn't contain the separator character used in the
   *  strings file.
   */

  if (KEYSEP_CH != '\0' && strchr(key, KEYSEP_CH)) {
    fprintf(stderr, "%s: archAppendKey: error -- key `%s' contains the key separator!\n",
            prog, key);
    return 0;
  }

  h->numKeysProcessed++;

  if ( ! _archKeyInStringCache(h->nsc, key, &found, &st)) {
    fprintf(stderr, "%s: _archAddStringCache: error looking for key in string cache.\n", prog);
    return 0;
  }

  if (found) {
    *start = st;
    return 1;
  }

  /*
   *  The key is not in the cache; check the patrie index.
   */

  if (h->cf) {
    if ( ! archSearchExact(h, key, 3, 1, &found, &st)) {
      fprintf(stderr, "%s: archAppendKey: error looking for key `%s' in index.\n",
              prog, key);
      rollback(h);
      return 0;
    } else if (found) {
      *start = st;
      return 1;
    }
  }

  /*
   *  The key wasn't found anywhere.  Append it to the strings list and
   *  insert it into the cache.
   */

  if ( ! _archAddKeyToStringCache(h->nsc, key, h->next)) {
    fprintf(stderr, "%s: _archAddKeyToStringCache: error adding key to cache.\n", prog);
    rollback(h);
    return 0;
  }

  /*
   *  Seek to the end of the strings file to ensure the write (fputs) isn't
   *  immediately preceded by a read.  This is required for a file opened for
   *  update.  For more information, see (for example) the section describing
   *  `fopen' in Harbison & Steele's "C: A Reference Manual".
   */
  
  if (fseek(h->strsFP, 0, SEEK_END) == -1) {
    fprintf(stderr, "%s: archAppendKey: error seeking to end of strings file: ", prog);
    perror("fseek");
    rollback(h);
    return 0;
  }
  
  if (fputs(key, h->strsFP) == EOF || fputs(KEYSEP_STR, h->strsFP) == EOF) {
    fprintf(stderr, "%s: archAppendKey: error appending key `%s' to strings list: ",
            prog, key); perror("fputs");
    rollback(h);
    return 0;
  }

  *start = h->next;
  h->next += strlen(key) + 1; /* +1 for the key separator */
  h->numNewKeysAdded++;

  return 1;
}


/*
 *  Build a new strings database.  The patrie index will cover the entire text
 *  file.  The index is created in new files, which replace the existing files
 *  only if there are no errors.
 */
int archBuildStrIdx(struct arch_stridx_handle *h)
{
  char *curridx = 0, *newidx = 0;

  patrieSetSortMaxMem(h->cf, h->buildMaxMem);

  if ( ! patrieSetSortTempDir(h->cf, h->tmpdir)) {
    fprintf(stderr, "%s: archBuildStrIdx: error setting PaTrie temporary directory.\n", prog);
    rollback(h);
    return 0;
  }

  /*
   *  Build the index.
   */

  if ( ! patrieBuild(h->cf)) {
    fprintf(stderr, "%s: archBuildStrIdx: error building index.\n", prog);
    rollback(h);
    return 0;
  }

  /*
   *  bug:
   *
   *  We need a protocol to ensure that one of these is true:
   *
   *  - the database is consistent
   *
   *  - the database is inconsistent, but it can be detected and repaired
   *
   *  This should also hold should the repair fail.
   *
   *  Also, the split file should contain the size of the strings file, so
   *  that it can be truncated as part of the rollback.  Fsync()?
   */

  if ( ! ((curridx = _archMakePath(h->dbdir, "/", IDX_FILE)) &&
          (newidx = _archMakePath(h->dbdir, "/", IDX_NEW_FILE)))) {
    fprintf(stderr, "%s: archBuildStrIdx: can't build paths to index files.\n", prog);
    rollback(h);
    if (curridx) free(curridx);
    return 0;
  }

  if ( ! (h->lock = _archLockExclusive(h->dbdir, LOCK_FILE))) {
    fprintf(stderr, "%s: archBuildStrIdx: can't get exclusive lock.\n", prog);
    return 0;
  }

  /*
   *  Replace the old index with the new one.
   */

  if (rename(newidx, curridx) == -1) {
    fprintf(stderr, "%s: archBuildStrIdx: error installing new index file: ", prog);
    perror("rename");
    rollback(h);
    _archUnlock(&h->lock);
    free(curridx); free(newidx);
    return 0;
  }

  if ( ! write_split(h)) {
    fprintf(stderr, "%s: archBuildStrIdx: can't write split.\n", prog);
    rollback(h);
    _archUnlock(&h->lock);
    return 0;
  }

  _archUnlock(&h->lock);

  free(curridx); free(newidx);

  return 1;
}


void archCloseStrIdx(struct arch_stridx_handle *h)
{
  if (h->dbdir) free(h->dbdir);
  if (h->tmpdir && h->tmpdir != tmpDir) free(h->tmpdir);

  if (h->strsFP) fclose(h->strsFP);
  if (h->idxFP) fclose(h->idxFP);
  if (h->nidxFP) fclose(h->nidxFP);
  if (h->infxFP) fclose(h->infxFP);
  if (h->levFP) fclose(h->levFP);

  if (h->uiMem) {
    munmap(h->uiMem, h->uiText - (char *)h->uiMem + h->uiLen);
  }

  if (h->key) free(h->key);

  if (h->cf) patrieFree(&h->cf);
  if (h->pState) patrieFreeState(&h->pState);

  if (h->nsc) _archFreeStringCache(&h->nsc);
}


int archCreateStrIdx(struct arch_stridx_handle *h, const char *dbdir, int indexType)
{
  if ( ! (h->dbdir = _archStrDup(dbdir))) {
    fprintf(stderr, "%s: archCreateStrIdx: can't duplicate string `%s'.\n", prog, dbdir);
    archCloseStrIdx(h);
    return 0;
  }

  if (indexType != ARCH_INDEX_CHARS && indexType != ARCH_INDEX_WORDS) {
    fprintf(stderr, "%s: archCreateStrIdx: invalid index type (%d).\n", prog, indexType);
    return 0;
  }

  h->idxType = indexType;

  if ( ! open_create(h)) {
    fprintf(stderr, "%s: archCreateStrIdx: can't open database for creation.\n", prog);
    archCloseStrIdx(h);
    return 0;
  }

  return 1;
}


int archGetIndexedSize(struct arch_stridx_handle *h, long *sz)
{
  if (h->split == -1 && ! read_split(h)) {
    fprintf(stderr, "%s: archGetSplit: can't read split.\n", prog);
    return 0;
  }

  if (h->split == 1) {
    *sz = 0;
  } else {
    *sz = h->split;
  }

  return 1;
}


/*
 *  Depending upon the type of search and how the strings file is indexed (by
 *  character or by word), we may have to rewrite the search key.  The key may
 *  have the key separator character prepended or appended to it, or both.
 */
static int makeSearchKeys(struct arch_stridx_handle *h,
                          const char *key,
                          char **patKey,
                          unsigned long *pAdj,
                          char **uiKey,
                          unsigned long *uAdj)
{
  char *ppfx, *psfx;            /* PaTrie key affixes */
  char *upfx, *usfx;            /* unindexed text key affixes */
  char *pk, *uk;                /* the rewritten keys */

  switch (h->srchType) {
  case SRCH_EXACT:
    ppfx = h->idxType == ARCH_INDEX_CHARS ? KEYSEP_STR : "";
    psfx = KEYSEP_STR;
    *pAdj = h->idxType == ARCH_INDEX_CHARS ? 1 : 0;

    upfx = KEYSEP_STR;
    usfx = KEYSEP_STR;
    *uAdj = 1;

    break;

  case SRCH_REGEX:
    fprintf(stderr, "%s: makeSearchKeys: regex keys are handled elsewhere.\n", prog);
    return 0;
    break;

  case SRCH_SUBSTR:
    ppfx = "";
    psfx = "";
    *pAdj = 0;

    upfx = h->idxType == ARCH_INDEX_WORDS ? KEYSEP_STR : "";
    usfx = "";
    *uAdj = h->idxType == ARCH_INDEX_WORDS ? 1 : 0;

    break;

  case SRCH_UNSET:
    fprintf(stderr, "%s: makeSearchKeys: no search has been done.\n", prog);
    return 0;

  default:
    fprintf(stderr, "%s: makeSearchKeys: unknown search type `%d'.\n", prog, h->srchType);
    return 0;
  }

  if (_archMemSprintf(&pk, "%s%s%s", ppfx, key, psfx) == -1) {
    fprintf(stderr, "%s: makeSearchKeys: can't create PaTrie search key.\n", prog);
    perror("_archMemSprintf");
    return 0;
  }

  if (_archMemSprintf(&uk, "%s%s%s", upfx, key, usfx) == -1) {
    fprintf(stderr, "%s: makeSearchKeys: can't create unindexed text search key.\n", prog);
    perror("_archMemSprintf"); free(pk);
    return 0;
  }

  *patKey = pk;
  *uiKey = uk;

  return 1;
}


int archGetMoreMatches(struct arch_stridx_handle *h,
                       unsigned long maxhits,
                       unsigned long *nhits,
                       unsigned long start[])
{
  char *patKey, *uiKey;
  unsigned long tothits = 0;
  unsigned long patAdj, uiAdj;

  if (maxhits == 0) {
    *nhits = 0;
    return 1;
  }

  /*
   *  bug: treat regular expressions specially.
   */

  if (h->srchType == SRCH_REGEX) {
    return _archRegexSearch(h, h->key, h->caseAccentSens, maxhits, nhits, start);
  }

  if ( ! makeSearchKeys(h, h->key, &patKey, &patAdj, &uiKey, &uiAdj)) {
    fprintf(stderr, "%s: archGetMoreMatches: can't create search keys.\n", prog);
    return 0;
  }

  if (h->uiText && ! uitext_sub_search(h, uiKey, uiAdj, maxhits, &tothits, start)) {
    fprintf(stderr, "%s: archGetMoreMatches: error searching unindexed text for `%s'.\n",
            prog, h->key);
    free(patKey); free(uiKey);
    return 0;
  }

  if (h->cf && tothits < maxhits) {
    unsigned long hits;

    /*
     *  If there's no search state then we have to do a search before we can
     *  get more hits.
     */

    if ( ! h->pState) {
      if ( ! patrie_sub_search(h, patKey, uiAdj, maxhits - tothits, &hits, start + tothits)) {
        fprintf(stderr, "%s: archGetMoreMatches: error searching index for `%s'.\n",
                prog, h->key);
        free(patKey); free(uiKey);
        return 0;
      }

      tothits += hits;
    }

    while (tothits < maxhits) {
      size_t i;

      if ( ! patrieGetMoreStarts(h->cf, maxhits - tothits, &hits, start + tothits, h->pState)) {
        fprintf(stderr, "%s: archGetMoreMatches: error getting more matches.\n", prog);
        return 0;
      }

      if (hits == 0) break;

      for (i = tothits; i < hits; i++) {
        start[i] += patAdj;
      }

      if ( ! patrie_fix_starts(patrieGetTextFP(h->cf), hits, start + tothits)) {
        fprintf(stderr, "%s: archGetMoreMatches: error fixing up match offsets.\n", prog);
        free(patKey); free(uiKey);
        return 0;
      }

      remove_duplicate_starts(hits, start + tothits, &hits);

      tothits += hits;
    }
  }

  free(patKey); free(uiKey);

  *nhits = tothits;

  return 1;
}


static int getSearchTypeChar(struct arch_stridx_handle *h, char *stype)
{
  switch (h->srchType) {
  case SRCH_EXACT:  *stype = 'E'; break;
  case SRCH_REGEX:  *stype = 'R'; break;
  case SRCH_SUBSTR: *stype = 'S'; break;
  default:
    fprintf(stderr, "%s: getSearchTypeChar: invalid search type `%d'.\n", prog, h->srchType);
    return 0;
  }

  return 1;
}


int archGetStateString(struct arch_stridx_handle *h, char **str)
{
  char *ss;
  char stype;

  if ( ! getSearchTypeChar(h, &stype)) {
    fprintf(stderr, "%s: archGetStateString: error setting search type in state.\n", prog);
    return 0;
  }

  /*
   *  bug: this is a kludgy way of saving space, if there's no PaTrie state.
   */

  if ( ! h->pState) {
    if (_archMemSprintf(&ss, "%lu %lu %c %d %s",
                   (unsigned long)h->rState, (unsigned long)h->uState,
                   stype, h->caseAccentSens, h->key)
        == -1) {
      fprintf(stderr, "%s: archGetStateString: error allocating space for unindexed state.\n",
              prog); perror("_archMemSprintf");
      return 0;
    }
  } else {
    char *ps;

    if ( ! (ps = patrieGetStateString(h->pState))) {
      fprintf(stderr, "%s: archGetStateString: error getting PaTrie search state string.\n", prog);
      return 0;
    }

    if (_archMemSprintf(&ss, "%lu %lu %c %d %s %s",
                   (unsigned long)h->rState, (unsigned long)h->uState,
                   stype, h->caseAccentSens, h->key, ps)
        == -1) {
      fprintf(stderr, "%s: archGetStateString: can't concatenate search state strings.\n", prog);
      perror("_archMemSprintf"); free(ps);
      return 0;
    }

    free(ps);
  }

  *str = ss;

  return 1;
}


static int setSearchTypeFromChar(char ch, struct arch_stridx_handle *h)
{
  switch (ch) {
  case 'E': h->srchType = SRCH_EXACT ; break;
  case 'S': h->srchType = SRCH_SUBSTR; break;
  case 'R': h->srchType = SRCH_REGEX ; break;
  default:
    fprintf(stderr, "%s: setSearchTypeFromChar: unknown search type `%c'.\n", prog, ch);
    return 0;
  }

  return 1;
}


int archSetBuildMaxMem(struct arch_stridx_handle *h, size_t mem)
{
  h->buildMaxMem = mem;
  return 1;
}


int archSetStateFromString(struct arch_stridx_handle *h, const char *str)
{
  char st;                      /* search type character */
  size_t i;

  if (h->key) {
    free(h->key); h->key = 0;
  }

  if (h->pState) {
    patrieFreeState(&h->pState);
  }

  {
    unsigned long r, u;

    if (sscanf(str, "%lu %lu %c %d", &r, &u, &st, &h->caseAccentSens) != 4) {
      fprintf(stderr, "%s: archSetStateFromString: invalid state before key.\n", prog);
      return 0;
    }

    h->rState = r; h->uState = u;
  }

  if ( ! setSearchTypeFromChar(st, h)) {
    fprintf(stderr, "%s: archSetStateFromString: can't get search type from state string.\n",
            prog);
    return 0;
  }

  /* Skip over the scanned text. */

  for (i = 0; i < 4; i++) {
    str += strspn(str, " "); str += strcspn(str, " ");
  }
  str += strspn(str, " ");

  if ( ! (patrieDequoteKey(str, &h->key, &str))) {
    fprintf(stderr, "%s: archSetStateFromString: can't quote search key.\n", prog);
    return 0;
  }

  if (*str == ' ') {
    return patrieSetStateFromString(str + 1, &h->pState);
  } else {
    return 1;
  }
}


/*
 *  Return a malloc()ed copy of the string at `offset' into the text.
 */
int archGetString(struct arch_stridx_handle *h, long offset, char **str)
{
  char *s;

  if (h->cf && (s = patrieGetSubstring(h->cf, offset, KEYSEP_STR))) {
    *str = s;
    return 1;
  } else if (offset < h->uiLen) {
    char *p = h->uiText + offset - h->split + 1;
    char *nl = _archStrChr(p, KEYSEP_CH);
    return (*str = _archStrNDup(p, nl - p)) != 0;
  } else {
    fprintf(stderr, "%s: archGetString: bad string offset `%ld'.\n", prog, offset);
    return 0;
  }
}


int archGetStringsFileSize(struct arch_stridx_handle *h, long *sz)
{
  if (h->next == -1) {
    *sz = _archFileSizeFP(h->strsFP);
  } else {
    *sz = h->next;
  }

  return 1;
}


void archFreeStrIdx(struct arch_stridx_handle **h)
{
  free(*h);
  *h = 0;
}


struct arch_stridx_handle *archNewStrIdx(void)
{
  struct arch_stridx_handle *h;

  if ( ! (h = malloc(sizeof *h))) {
    fprintf(stderr, "%s: archNewStrIdx: can't allocate %lu bytes: ",
            prog, (unsigned long)sizeof *h); perror("malloc");
  } else {
    h->mode = -1;

    h->dbdir = 0;
    h->tmpdir = tmpDir;
    h->buildMaxMem = BUILD_MAX_MEM;

    h->strsFP = h->idxFP =
    h->nidxFP = h->infxFP = h->levFP = 0;

    h->idxType = ARCH_INDEX_NONE;
    h->prevnl = 0;

    h->uiMem = h->uiText = 0;
    h->uiLen = 0;
    h->uState = 0;

    h->rState = 0;

    h->key = 0;
    h->caseAccentSens = 0;

    h->cf = 0;
    h->pState = 0;

    h->srchType = SRCH_UNSET;

    h->split = h->next = -1;


    h->loadFactor = -1.0;
    h->nsc = 0;

    h->lock = 0;

    h->numKeysProcessed = 0;
    h->numNewKeysAdded = 0;
  }

  return h;
}


int archOpenStrIdx(struct arch_stridx_handle *h, const char *dbdir, int mode)
{
  if ( ! (h->dbdir = _archStrDup(dbdir))) {
    fprintf(stderr, "%s: archOpenStrIdx: can't duplicate string `%s'.\n", prog, dbdir);
    archCloseStrIdx(h);
    return 0;
  }

  h->mode = mode;

  switch (mode) {
  case ARCH_STRIDX_APPEND:
    if ( ! open_append(h)) {
      fprintf(stderr, "%s: archOpenStrIdx: can't open database for appending.\n", prog);
      archCloseStrIdx(h);
      return 0;
    }
    break;

  case ARCH_STRIDX_BUILD:
    if ( ! open_build(h)) {
      fprintf(stderr, "%s: archOpenStrIdx: can't open database for building.\n", prog);
      archCloseStrIdx(h);
      return 0;
    }
    break;

  case ARCH_STRIDX_SEARCH:
    if ( ! open_search(h)) {
      fprintf(stderr, "%s: archOpenStrIdx: can't open database for searching.\n", prog);
      archCloseStrIdx(h);
      return 0;
    }
    break;

  default:
    fprintf(stderr, "%s: archOpenStrIdx: invalid value, %d, for mode argument.\n", prog, mode);
    return 0;
  }

  return 1;
}


void archPrintStats(struct arch_stridx_handle *h)
{
  if (h->numKeysProcessed == 0) {
    fprintf(stderr, "%s: no keys were processed.\n", prog);
  } else {
    fprintf(stderr, "%s: processed %ld keys.\n", prog, h->numKeysProcessed);
    fprintf(stderr, "%s: %ld (%.1f%%) were new, %ld (%.1f%%) already existed.\n", prog,
            h->numNewKeysAdded, h->numNewKeysAdded / (float)h->numKeysProcessed * 100.0,
            h->numKeysProcessed - h->numNewKeysAdded,
            (h->numKeysProcessed - h->numNewKeysAdded) / (float)h->numKeysProcessed * 100.0);
  }
}


static int saveSearchState(const char *key,
                           int case_acc_sens,
                           enum search_type srchType,
                           struct arch_stridx_handle *h)
{
  if (h->key) free(h->key);
  if ( ! (h->key = _archStrDup(key))) {
    fprintf(stderr, "%s: saveSearchState: can't duplicate key for search state.\n", prog);
    perror("_archStrDup");
    return 0;
  }

  h->caseAccentSens = case_acc_sens;
  h->srchType = srchType;

  h->rState = 0;
  h->uState = 0;
  if (h->pState) {
    patrieFreeState(&h->pState);
  }

  return 1;
}


static int subSearch(struct arch_stridx_handle *h,
                     const char *key,
                     int case_acc_sens,
                     unsigned long maxhits,
                     unsigned long *nhits,
                     unsigned long start[])
{
  char *patKey, *uiKey;
  unsigned long patAdj, uiAdj;
  unsigned long phits = 0;      /* # hits in PaTrie index */
  unsigned long uhits = 0;      /* # hits in unindexed text */

  *nhits = 0;

  /*
   *  Return no matches for the empty string.
   *
   *  bug: Currently, searching for it can screw up some code.  Anyway, the
   *  proper behaviour would be to return the entire strings file, so there's
   *  no great loss...
   */

  if (key[0] == '\0') {
    return 1;
  }

  if ( ! makeSearchKeys(h, key, &patKey, &patAdj, &uiKey, &uiAdj)) {
    fprintf(stderr, "%s: subSearch: can't create search keys.\n", prog);
    return 0;
  }

  /*
   *  First, search the unindexed text for matches.  If we don't find
   *  `maxhits' results then try the PaTrie index.
   */

  if (h->uiText && ! uitext_sub_search(h, uiKey, uiAdj, maxhits, &uhits, start)) {
    fprintf(stderr, "%s: subSearch: error searching unindexed text for `%s'.\n", prog, uiKey);
    free(patKey); free(uiKey);
    return 0;
  }

  if (uhits == maxhits) {
    *nhits = uhits;
    free(patKey); free(uiKey);
    return 1;
  }

  if (h->cf && ! patrie_sub_search(h, patKey, patAdj, maxhits - uhits, &phits, start + uhits)) {
    fprintf(stderr, "%s: subSearch: error searching index for `%s'.\n", prog, patKey);
    free(patKey); free(uiKey);
    return 0;
  }

  free(patKey); free(uiKey);

  *nhits = uhits + phits;

  return 1;
}


int archSearchExact(struct arch_stridx_handle *h,
                    const char *key,
                    int case_acc_sens,
                    unsigned long maxhits,
                    unsigned long *nhits,
                    unsigned long start[])
{
  saveSearchState(key, case_acc_sens, SRCH_EXACT, h);
  return subSearch(h, key, case_acc_sens, maxhits, nhits, start);
}


int archSearchRegex(struct arch_stridx_handle *h,
                    const char *key,
                    int case_acc_sens,
                    unsigned long maxhits,
                    unsigned long *nhits,
                    unsigned long start[])
{
  saveSearchState(key, case_acc_sens, SRCH_REGEX, h);
  return _archRegexSearch(h, key, case_acc_sens, maxhits, nhits, start);
}


int archSearchSub(struct arch_stridx_handle *h,
                  const char *key,
                  int case_acc_sens,
                  unsigned long maxhits,
                  unsigned long *nhits,
                  unsigned long start[])
{
  saveSearchState(key, case_acc_sens, SRCH_SUBSTR, h);
  return subSearch(h, key, case_acc_sens, maxhits, nhits, start);
}


int archSetTempDir(struct arch_stridx_handle *h, const char *newdir)
{
  char *td;

  if ( ! (td = _archStrDup(newdir))) {
    fprintf(stderr, "%s: archSetTempDir: can't allocate memory for directory name `%s': ",
            prog, newdir); perror("_archStrDup");
    return 0;
  }

  if (h->tmpdir && h->tmpdir != tmpDir) free(h->tmpdir);
  h->tmpdir = td;

  return 1;
}


/*
 *  Return, through `*ans', whether `dir'/`name' exists.
 */
static int fileExists(const char *dir, const char *name, int *ans)
{
  char *path;
  int res;
  struct stat s;

  if ( ! (path = _archMakePath(dir, "/", name))) {
    fprintf(stderr, "%s: fileExists: can't make file path.\n", prog);
    return 0;
  }

  {
    int e;
    res = stat(path, &s); e = errno;
    free(path); errno = e;
  }

  if (res != -1) {
    *ans = 1;
    return 1;
  } else {
    if (errno == ENOENT) {
      *ans = 0;
      return 1;
    } else {
      fprintf(stderr, "%s: fileExists: can't get information about `%s/%s': ",
              prog, dir, name); perror("stat");
      return 0;
    }
  }
}


/*
 *  Return, through `*ans', an indication of whether the necessary strings
 *  database files (STRINGS_FILE, SPLIT_FILE and TYPE_FILE) exist.
 *
 *  Valid values of `*ans' are -1: some, but not all, of the files exist, 0:
 *  none of the files exist, and 1: all the files exist.
 */
int archStridxExists(const char *dbdir, int *ans)
{
  int str = 0, spl = 0, typ = 0;

  if ( ! (fileExists(dbdir, STRINGS_FILE, &str) &&
          fileExists(dbdir, SPLIT_FILE, &spl) &&
          fileExists(dbdir, TYPE_FILE, &typ))) {
    fprintf(stderr, "%s: archStridxExists: can't get information on necessary files.\n", prog);
    return 0;
  }

  if (str && spl && typ) {
    *ans = 1;                   /* All files exist. */
  } else if (str || spl || typ) {
    *ans = -1;                  /* Some files exist. */
  } else {
    *ans = 0;                   /* No files exist. */
  }

  return 1;
}


int archUpdateStrIdx(struct arch_stridx_handle *h)
{
  return 1;
}


void archSetHashLoadFactor(struct arch_stridx_handle *h, float lf)
{
  h->loadFactor = lf;
}
