#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "defs.h"
#include "patrie.h"
#include "utils.h"


/*  
 *  Return the substring of the text file beginning at `offset'.  The string
 *  is terminated by (i.e. does not contain) any of the characters in `stop'.
 *  
 *  Upon success a nul terminated malloc()ed buffer is returned, containing a
 *  copy of the string.  A null pointers is returned if an error occurs.
 */
char *patrieGetSubstring(struct patrie_config *cf, long offset, const char *stop)
{
#ifndef BSZ
#define BSZ 64
#endif
  
  char *b, *buf = 0;
  size_t bsz = BSZ, len, n, obsz = 0;

  if (fseek(cf->textFP, offset, SEEK_SET) == -1) {
    fprintf(stderr, "%s: patrieGetText: error seeking to offset %ld: ", prog, offset);
    perror("fseek");
    return 0;
  }

  while (1) {
    if ( ! _patrieReAlloc(buf, bsz + 1, (void **)&buf)) {
      free(buf);
      return 0;
    }

    b = buf + obsz;

    n = fread(b, 1, bsz - obsz, cf->textFP);
    b[n] = '\0';

    if (n == 0) {
      if (feof(cf->textFP)) {
        return buf;             /* substring runs to end of file */
      } else {
        fprintf(stderr, "%s: patrieGetText: error reading %lu bytes from text file: ",
                prog, (unsigned long)bsz); perror("fread");
        free(buf);
        return 0;
      }
    }

    len = strcspn(b, stop);
    if (b[len] != '\0') {
      b[len] = '\0';
      return buf;
    }

    obsz = bsz;
    bsz *= 2;
  }

#undef BSZ
}
