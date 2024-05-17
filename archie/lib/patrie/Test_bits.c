/*  
 *
 *
 *  Test that copyBits: (1) copies the right bits, and (2) doesn't modify
 *  surrounding bits.
 *
 *
 */

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "bits.h"


#define LONG (sizeof(unsigned long))
#define NBITS (LONG * CHAR_BIT)


/*  
 *  Print the most significant bit first.
 *  
 *  Assume buf is at least NBITS+1 bytes long.
 */
char *bitString(unsigned long n, int zeroCh, int oneCh, char *buf)
{
  int i;
  
  for (i = 0; i < NBITS; i++) {
    buf[i] = (n & (1 << (NBITS - i - 1))) ? oneCh : zeroCh;
  }
  buf[i] = '\0';
  return buf;
}


int main(int ac, char **av)
{
  char dst[LONG], src[LONG];
  int i, verbose = 0;
  int dstOffset, nBits, srcOffset;
  unsigned long ansLong, dstLong, srcLong;
  unsigned long mask[NBITS+1];  /* 0, 1, 3, 7, ... 2^k - 1, ... */

  if (ac == 2 && strcmp(av[1], "-verbose") == 0) verbose = 1;

  for (i = 0; i < NBITS; i++) mask[i] = ~(~0 << i); mask[i] = ~0;

  if (verbose) {
    printf("Mask values:\n\n");
    for (i = 0; i <= NBITS; i++) {
      char s[NBITS+1];
      printf("\t%2d\t%s\n", i, bitString(mask[i], '.', '1', s));
    }
    printf("\n");
  }
  
  nBits = 0; dstOffset = 0; srcOffset = 0;
  for (nBits = 0; nBits <= NBITS; nBits++) {
    for (srcOffset = 0; srcOffset + nBits <= NBITS; srcOffset++) {
      for (dstOffset = 0; dstOffset + nBits <= NBITS; dstOffset++) {

#warning TEMPORARY HACK
        /* filter out the cases we can't yet handle */
        if (nBits == 0 || (srcOffset % CHAR_BIT) <= (dstOffset % CHAR_BIT)) {
          continue;
        }

        /*  
         *
         *
         *  One bits against a zero field.
         *
         *
         */

        srcLong = mask[nBits] << (NBITS - srcOffset - nBits);
        ansLong = mask[nBits] << (NBITS - dstOffset - nBits);
        memcpy(src, &srcLong, LONG);
        memset(dst, 0, LONG);
        
        if (verbose) {
          char ansStr[NBITS+1], srcStr[NBITS+1];
          
          printf("%d/%d/%d\t%s -> %s\n",
                 nBits, srcOffset, dstOffset,
                 bitString(srcLong, '.', '1', srcStr),
                 bitString(ansLong, '.', '1', ansStr));
        }

        copyBits(nBits, srcOffset, src, dstOffset, dst);

        memcpy(&dstLong, dst, LONG);
        if (dstLong != ansLong) {
          if ( ! verbose) {
            printf("%d/%d/%d failed.\n", nBits, srcOffset, dstOffset);
          } else {
            char s[NBITS+1];
            printf("%d/%d/%d\t%s -> %s\n\n",
                   nBits, srcOffset, dstOffset,
                   "             FAILED             ",
                   bitString(dstLong, '.', '1', s));
          }
        }

        /*  
         *
         *
         *  Zero bits against a one field.
         *
         *
         */

        srcLong = ~(mask[nBits] << (NBITS - srcOffset - nBits));
        ansLong = ~(mask[nBits] << (NBITS - dstOffset - nBits));
        memcpy(src, &srcLong, LONG);
        memset(dst, ~0, LONG);
        
        if (verbose) {
          char ansStr[NBITS+1], srcStr[NBITS+1];
          
          printf("%d/%d/%d\t%s -> %s\n",
                 nBits, srcOffset, dstOffset,
                 bitString(srcLong, '.', '1', srcStr),
                 bitString(ansLong, '.', '1', ansStr));
        }

        copyBits(nBits, srcOffset, src, dstOffset, dst);

        memcpy(&dstLong, dst, LONG);
        if (dstLong != ansLong) {
          if ( ! verbose) {
            printf("%d/%d/%d failed.\n", nBits, srcOffset, dstOffset);
          } else {
            char s[NBITS+1];
            printf("%d/%d/%d\t%s -> %s\n\n",
                   nBits, srcOffset, dstOffset,
                   "             FAILED             ",
                   bitString(dstLong, '.', '1', s));
          }
        }

        printf("\n");
      }
    }
  }

  return 0;
}
