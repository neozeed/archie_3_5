#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>


#ifdef __STDC__
# define INLINE
#else
# define INLINE inline
#endif


/*  
 *  L is the number of bits in a long.
 */
#define L (sizeof(long) * CHAR_BIT)

/*  
 *  MASK(n, s) generates n bits of ones, left shifted by s bits.  It assumes
 *  two's complement arithmetic.
 */
#define MASK(n, s) (((1 << (n)) - 1) << (s))


static long mask(size_t bwidth, size_t shift)
{
  return MASK(bwidth, shift);
}


/*  
 *  Copy the `n' bits, at an offset of `off' bits from `src', into `dst'.
 */
INLINE void getlong(const long *src, size_t n, size_t off, long *dst)
{
  const long *l;                /* pointer to first long to hold destination bits */
  size_t b;                     /* bit offset from `*l' */

  l = src + off / L; b = off % L;

  if (b + n <= L) {
    /*  
     *  The bits fit entirely within one long.
     */
    size_t s = L - n - b;       /* right shift */
    *dst = (*l >> s) & MASK(n, 0);
  } else {
    /*  
     *  The bits are spread across l and l+1.
     */
    size_t m = L - b;           /* mask width for least significant bits */
    *dst = (*l & MASK(m, 0)) | ((*(l+1) >> (L - n)) & MASK(n - m, m));
  }
}


/*  
 *  Copy the `n' least significant bits of `src' to an offset of `off' bits
 *  into `dst'.
 */
INLINE void setlong(long src, size_t n, size_t off, long *dst)
{
  long *l;                      /* pointer to first long to hold destination bits */
  size_t b;                     /* bit offset from `*l' */

  l = dst + off / L; b = off % L;

  if (b + n <= L) {
    /*  
     *  The bits fit entirely within one long.
     */
    size_t s = L - n - b;       /* left shift */
    *l = (*l & ~MASK(n, s)) | ((src << s) & MASK(n, s));
  } else {
    /*  
     *  The bits are spread across l and l+1.
     */
    size_t m = L - b;           /* mask width for least significant bits */
    *l = (*l & ~MASK(m, 0)) | (src & MASK(m, 0));
    *(l+1) = src << (L - n);
  }
}


#define OSZ 1000
#define PSZ (OSZ * sizeof(long) * CHAR_BIT)


static long expd[OSZ];   /* pack[] expanded into array of longs; should equal orig[] */
static long orig[OSZ];   /* original set of bits */
static long pack[PSZ];   /* original bits packed into array of longs */


static jmp_buf jbuf;


void sigint(int signo)
{
  longjmp(jbuf, 1);
}


int main(void)
{
  size_t mw;                    /* mask width */
  size_t lng;                   /* loop counter over orig and bit1 */
  volatile size_t cnt = 0;

  if (setjmp(jbuf) != 0) {
    printf("Did %lu iterations.\n", (unsigned long)cnt);
    exit(0);
  }
  signal(SIGINT, sigint);

  while (1) {
    for (mw = 1; mw < L; mw++) {
      /*  
       *  Fill `orig' with n-bit random numbers of the current mask length.
       *  Use the highest order bits, just to be safe...
       */
      
      for (lng = 0; lng < OSZ; lng++) {
        orig[lng] = (mrand48() >> (L - mw)) & MASK(mw, 0);
      }

      /*  
       *  Pack the original longs.
       */
      
      for (lng = 0; lng < OSZ; lng++) {
        setlong(orig[lng], mw, mw * lng, pack);
      }

      /*  
       *  Expand the packed bits.
       */
      
      for (lng = 0; lng < OSZ; lng++) {
        getlong(pack, mw, mw * lng, &expd[lng]);
      }

      /*  
       *  Compare the original longs with the expanded bits.
       */
      
      if (memcmp(orig, expd, OSZ * sizeof(long)) != 0) {
        abort();
      }

      cnt++;
    }
  }
  
  exit(0);
}
