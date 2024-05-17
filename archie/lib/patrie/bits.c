#include <limits.h>
#include "bits.h"


#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif


/*  
 *  Assume non-overlapping bit strings and valid arguments.
 *  
 *  We number the bits in a byte 76543210 (MSB first).
 *  
 *  bug: should number them 0-7 to conform to bit offset nomenclature
 */
void copyBits(int nBits, int srcOffset, char *src, int dstOffset, char *dst)
{
  char *d, *s;
  int m;                      /* mask width, in bits */
  int n, n0, n1;
  static unsigned char mask[] = { 0, 1, 3, 7, 15, 31, 63, 127, 255 };

  d = dst + dstOffset / CHAR_BIT;
  s = src + srcOffset / CHAR_BIT;

  /*  
   *  Positive n means we shift left by n bits, otherwise we shift right by
   *  -n bits.
   */
  
  n = (srcOffset % CHAR_BIT) - (dstOffset % CHAR_BIT);

  if (n > 0) {
    n0 = n;
    n1 = CHAR_BIT - n0;
    while (1) {
      if (nBits == 0) break;
      m = MIN(nBits, CHAR_BIT - n0);
      *d = (*d & ~(mask[m] << n0)) | ((*s << n0) & (mask[m] << n0));
      s++; nBits -= m;

      if (nBits == 0) break;
      m = MIN(nBits, CHAR_BIT - n1);
      *d = (*d & ~(mask[m] >> n1)) | ((*s >> n1) & (mask[m] >> n1));
      d++; nBits -= m;
    }
  } else {
#warning NOT IMPLEMENTED
    return;
    
    n0 = -n0;
    n1 = CHAR_BIT - n0;
    while (1) {
    }
  }
}


/*  
 *  Assume nBits is 32 and that srcOffset and dstOffset are multiples of 32.
 */
void copyBits32(int nBits, int srcOffset, char *src, int dstOffset, char *dst)
{
  char *d, *s;

  d = dst + dstOffset / CHAR_BIT; s = src + srcOffset / CHAR_BIT;
  memcpy(d, s, 32 / CHAR_BIT);
}
