#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "defs.h"
#include "patrie.h"
#include "trailer.h"


#define TR_ELTS 8 /* # elements in trailer array */


int _patrieReadTrailer(struct patrie_config *config, FILE *fp)
{
  int i;
  long here;
  unsigned long checksum = 0, cs;
  unsigned long trail[TR_ELTS];

  here = ftell(fp);
  
  config->trailerSize = sizeof trail;

  ASSERT(fseek(fp, -config->trailerSize, SEEK_END) != -1);
  ASSERT(fread(trail, sizeof trail[0], TR_ELTS, fp));

  cs = trail[6]; trail[6] = 0;

  for (i = 0; i < TR_ELTS; i++) {
    checksum += trail[i];
  }

  if (checksum != cs) {
    fprintf(stderr, "%s: _patrieReadTrailer: bad checksum in trailer.\n", prog);
    ASSERT(fseek(fp, here, SEEK_SET) != -1);
    return 0;
  }

  /* bug: -1 should go */
  config->bitsPerStart = trail[0] - 1;
  config->bitsPerSkip = trail[1] - 1;
  config->levelsPerPage = trail[2];
  config->pagedPageSize = trail[3];
  config->buildCaseAccentSens = trail[4];

  ASSERT(fseek(fp, here, SEEK_SET) != -1);

  return 1;
}


/*  
 *  Write a descriptive trailer to the paged trie file.  It contains the
 *  following information, starting at the end of the file, each of which is
 *  32 bits (unsigned long for now):
 */  
/*
 *  #bytes of trailer (including this count)
 *  trailer checksum (calculated with this value 0)
 *  byte order (currently unused, set to 0)
 *  case sensitive index (0: insensitive, 1: sensitive)
 *  page size (in bytes)
 *  #levels per page
 *  #bits per skip node (including 1 bit for node type)
 *  #bits per start node (including 1 bit for node type)
 */  
void _patrieWriteTrailer(struct patrie_config *config, FILE *fp)
{
  int i;
  unsigned long checksum = 0;
  unsigned long trail[TR_ELTS];

  /* bug: +1 should go */
  trail[0] = config->bitsPerStart + 1;
  trail[1] = config->bitsPerSkip + 1;
  trail[2] = config->levelsPerPage;
  trail[3] = config->pagedPageSize;
  trail[4] = config->buildCaseAccentSens;
  trail[5] = 0;                 /* byte order (unused) */
  trail[6] = 0;                 /* checksum */
  trail[7] = sizeof trail;      /* # bytes in trailer */

  for (i = 0; i < TR_ELTS; i++) {
    checksum += trail[i];
  }

  trail[6] = checksum;

  ASSERT(fwrite(trail, sizeof trail[0], TR_ELTS, fp));
}
