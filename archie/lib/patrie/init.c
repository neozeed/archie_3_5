#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "case.h"
#include "defs.h"
#include "init.h"
#include "patrie.h"
#include "sort.h"
#include "utils.h"


/*  
 *  Default values.
 */

#define PAGED_PAGE_SIZE 8192 /* number of bytes per paged (final) file page */
#define LEVEL_PAGE_SIZE 1024 /* number of bytes per level (temp) file page */
#define LEVELS_PER_PAGE 8    /* number of trie levels per final page */
#define BITS_PER_SKIP   31   /* to match the node structure */
#define BITS_PER_START  31   /* to match the node structure */
#define SORT_MAX_MEM    10000000 /* 10Mb */
#define SORT_PAD_LEN    (8 * 1024) /* 64K bits; amt. held by height vars */
#define SORT_TEMP_DIR   "/tmp"


static char *tempDir = SORT_TEMP_DIR;


void _patrieZeroStats(struct patrie_config *c)
{
  c->statsMaxHeight = 0;
  c->statsMaxSkip = 0;
  c->statsExtNodes = 0;
  c->statsIntNodes = 0;
  c->statsNodesOutput = 0;

  c->statsSortCmpDA = 0;
  c->statsSortHgtDA = 0;
  c->statsSortNumCmp = 0;
  c->statsSortNumHgt = 0;
  c->statsSortNumSs = 0;

  memset(c->statsSortHgtDist, 0, sizeof c->statsSortHgtDist);

  c->debugMaxHeight = 0;
  c->debugMaxStart = 0;
  c->debugMinHeight = ULONG_MAX;
  c->debugMinStart = ULONG_MAX;
  c->debugNodesLoaded = 0;
  c->debugNodesWritten = 0;
}


/*  
 *  Allocate a configuration structure and initialize it to reasonable
 *  values.
 */
int patrieAlloc(struct patrie_config **config)
{
  struct patrie_config *c;

  if ( ! (c = MALLOC(sizeof *c))) {
    fprintf(stderr, "%s: patrieAlloc: can't allocate %lu bytes for config structure: ",
            prog, (unsigned long)sizeof *c); perror("malloc");
    return 0;
  }

  c->doLevels = 1;
  c->doPaged = 1;
  c->doInfix = 1;
  c->doInfixDistrib = 1;
  c->doInfixMerge = 1;

  c->infixTrieFP = 0;
  c->levelTrieFP = 0;
  c->pagedTrieFP = 0;
  c->textFP = 0;

  c->buildCaseAccentSens = 3;  /* 3='11' : case and accent sensitive.
                                * Used to be = 1 (case sensitive)
                                */

  c->levelPageSize = LEVEL_PAGE_SIZE;
  c->pagedPageSize = PAGED_PAGE_SIZE;

  c->levelsPerPage = LEVELS_PER_PAGE;

  /*
   *  bug: should these be initialized, or just set by infix phase?
   */
  
  c->bitsPerSkip = BITS_PER_SKIP;
  c->bitsPerStart = BITS_PER_START;

  c->assertCheck = 0;
  c->printStats = 0;
  c->printDebug = 0;

  c->allocError = 0;
  c->ioError = 0;

  c->sortTempDir = tempDir;
  c->sortKeep = 0;
  c->infixKeepTemp = 0;
  c->sortStart = -1;
  c->sortEnd = -1;
  c->sortMaxMem = SORT_MAX_MEM;
  c->sortPadLen = SORT_PAD_LEN;
  c->indexArg = 0;
  c->setIndexPoints = _patrieSetIndexPoints;

#ifdef HAVE_UNSIGNED_STRCMP
  c->strcomp = strcmp;
#else
  c->strcomp = _patrieUStrCmp;
#endif

  c->diffbit = _patrieDiffBit;

  _patrieZeroStats(c);
  
  c->trailerSize = 0;
  c->nextAvailPage = 0;
  c->triePage = 0;

  *config = c;

  return 1;
}


void patrieFree(struct patrie_config **config)
{
  struct patrie_config *c = *config;
  
  if (c->sortTempDir && c->sortTempDir != tempDir) {
    free((char *)c->sortTempDir);
  }

  if (c->triePage) {
    if (c->triePage->tCount) free(c->triePage->tCount);
    if (c->triePage->bits) free(c->triePage->bits);
    free(c->triePage);
  }

  free(c);
  *config = 0;
}


FILE *patrieGetInfixFP(struct patrie_config *config)
{
  return config->infixTrieFP;
}


FILE *patrieGetLevelFP(struct patrie_config *config)
{
  return config->levelTrieFP;
}


FILE *patrieGetPagedFP(struct patrie_config *config)
{
  return config->pagedTrieFP;
}


FILE *patrieGetTextFP(struct patrie_config *config)
{
  return config->textFP;
}


void patrieSetAssert(struct patrie_config *config, int bool)
{
  config->assertCheck = bool;
}


#ifdef HAVE_UNSIGNED_STRCASECMP
extern int strcasecmp(const char *, const char *);
#endif


/*  
 *  We should also include (as well):
 *  
 *  - the comparison function passed to qsort()
 *  
 *  - the bit difference function
 */  
void patrieSetCaseSensitive(struct patrie_config *config, int bool)
{
#ifdef HAVE_UNSIGNED_STRCMP
#define STRCMP strcmp
#else
#define STRCMP _patrieUStrCmp
#endif
#ifdef HAVE_UNSIGNED_STRCASECMP
#define STRCASECMP strcasecmp
#else
#define STRCASECMP _patrieUStrCaseCmp
#endif
  
  switch((config->buildCaseAccentSens = bool)){

  case CMP_CIAI:
    config->strcomp = _patrieUStrCiAiCmp;
    config->diffbit = _patrieDiffBitCiAi;
    break;
  case CMP_CIAS:
    config->strcomp = _patrieUStrCiAsCmp;
    config->diffbit = _patrieDiffBitCiAs;
    break;
  case CMP_CSAI:
    config->strcomp = _patrieUStrCsAiCmp;
    config->diffbit = _patrieDiffBitCsAi;
    break;
  case CMP_CSAS:
  default:
    config->strcomp = _patrieUStrCsAsCmp;
    config->diffbit = _patrieDiffBitCsAs;
    break;
  }


#if 0
  if ((config->buildCaseAccentSens = bool)) {
    config->strcomp = STRCMP;
    config->diffbit = _patrieDiffBit;
  } else {
    config->strcomp = STRCASECMP;
    config->diffbit = _patrieDiffBitCase;
  }
#endif
#undef STRCMP
#undef STRCASECMP
}


void patrieSetCreateInfix(struct patrie_config *config, int bool, int keepInfix, int keepInfixTemp)
{
  config->doInfix = bool;
  config->sortKeep = keepInfix;
  config->infixKeepTemp = keepInfixTemp;
}


void patrieSetCreateLevels(struct patrie_config *config, int bool)
{
  config->doLevels = bool;
}


void patrieSetCreatePaged(struct patrie_config *config, int bool)
{
  config->doPaged = bool;
}


void patrieSetDebug(struct patrie_config *config, int bool)
{
  config->printDebug = bool;
}


void patrieSetIndexPointsFunction(struct patrie_config *config,
                                  void (*fn)(void *run,
                                             void (*mark)(void *run, size_t indexPoint),
                                             size_t tlen, const char *text, void *arg),
                                  void *arg)
{
  config->setIndexPoints = fn;
  config->indexArg = arg;
}


void patrieSetInfixFP(struct patrie_config *config, FILE *fp)
{
  config->infixTrieFP = fp;
}


void patrieSetLevelFP(struct patrie_config *config, FILE *fp)
{
  config->levelTrieFP = fp;
}


void patrieSetLevelPageSize(struct patrie_config *config, size_t lps)
{
  config->levelPageSize = lps;
}


void patrieSetLevelsPerPage(struct patrie_config *config, unsigned int lpp)
{
  config->levelsPerPage = lpp;
}


void patrieSetPagedFP(struct patrie_config *config, FILE *fp)
{
  config->pagedTrieFP = fp;
}


void patrieSetPagedPageSize(struct patrie_config *config, size_t pps)
{
  config->pagedPageSize = pps;
}


void patrieSetSortDistribute(struct patrie_config *config, int bool)
{
  config->doInfixDistrib = bool;
}


void patrieSetSortMaxMem(struct patrie_config *config, size_t mem)
{
  config->sortMaxMem = mem;
}


void patrieSetSortMerge(struct patrie_config *config, int bool)
{
  config->doInfixMerge = bool;
}


void patrieSetSortPadLen(struct patrie_config *config, size_t pad)
{
  config->sortPadLen = pad;
}


int patrieSetSortTempDir(struct patrie_config *config, const char *dir)
{
  const char *t;

  if ( ! (t = stringcopy(dir))) {
    fprintf(stderr, "%s: patrieSetSortTempDir: can't allocate memory for copy of directory name: ",
            prog); perror("stringcopy");
    return 0;
  }
  
  config->sortTempDir = t;
  return 1;
}


void patrieSetStats(struct patrie_config *config, int bool)
{
  config->printStats = bool;
}


void patrieSetTextFP(struct patrie_config *config, FILE *fp)
{
  config->textFP = fp;
}


#if 0
/*
 *  bug: make the infix build take a starting offset and text length?
 */
void patrieSetTextEnd(struct patrie_config *config, long off)
{
}
#endif


void patrieSetTextStart(struct patrie_config *config, long off)
{
  config->sortStart = off;
}
