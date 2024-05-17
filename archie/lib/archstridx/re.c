#include <sys/types.h> /* for AIX */
#include <sys/mman.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "all.h"
#include "archstridx.h"
#include "re.h"
#include "utils.h"


/*
 *  The interface to these regular expression routines is a botch, but what
 *  can you do?
 */

static jmp_buf env;

#define INIT       unsigned char *sp = instring;
#define GETC()     (*sp++)
#define PEEKC()    (*sp)
#define UNGETC(c)  (--sp)
#define RETURN(c)  return (c);
#define ERROR(c)   longjmp(env, (c))

#include <regexp.h>


/*
 *  The number of bytes in each chunk of text.
 */
#define TSZ (64 * 1024)


static void regerr(const char *re, int err)
{
  const char *errstr;

  switch (err) {
  case 11: errstr = "range endpoint too large";               break;
  case 16: errstr = "bad number";                             break;
  case 25: errstr = "\\ digit out of range";                  break;
  case 36: errstr = "illegal or missing delimiter";           break;
  case 41: errstr = "no remembered search string";            break;
  case 42: errstr = "\\( \\) imbalance";                      break;
  case 43: errstr = "too many \\(";                           break;
  case 44: errstr = "more than two numbers given in \\{ \\}"; break;
  case 45: errstr = "} expected after \\";                    break;
  case 46: errstr = "first number exceeds second in \\{ \\}"; break;
  case 49: errstr = "[] imbalance";                           break;
  case 50: errstr = "regular expression too long";            break;
  default:
    fprintf(stderr, "%s: regerr: unknown error %d in `%s'.\n", prog, err, re);
    return;
  }
  
  fprintf(stderr, "%s: regerr: %s in `%s'.\n", prog, errstr, re);
}


struct inbuf {
  char txt[TSZ];
  size_t boff;                  /* offset of first byte in buffer */
  size_t noff;                  /* offset of next byte to be read */
  char *end;                    /* pointer to last valid byte, plus one */
  char *sep;                    /* pointer to last key separator in buffer */
};


/*
 *  Initialize an inbuf structure.  Calls to `fillBuf' will set `boff' to the
 *  current value of `noff', so we store any saved state in `noff'.
 */
static void initBuf(size_t off, struct inbuf *b)
{
  b->end = b->sep = b->txt + sizeof b->txt;
  b->boff = 0;
  b->noff = off;
}


/*
 *  Fill a buffer with a chunk of text.
 *  
 *  Upon entry, there may be a partial string at the end of the buffer that
 *  hasn't yet been examined.  We copy any such string to the beginning of the
 *  buffer, then fill the rest of the buffer with the succeeding text.
 *  
 *  We update various fields to indicate the status of the buffer: `end'
 *  points to one past the last valid character in the buffer; `boff' is the
 *  offset, into the strings file, corresponding to the first character in the
 *  buffer, and `noff' is the offset of the first character that will be read
 *  on the next call to this function (i.e. `boff' plus the number of
 *  characters in the buffer).
 */
static int fillBuf(int fd, struct inbuf *b)
{
  char *dst = b->txt;
  char *src = b->sep + 1;
  int n;
  
  /*
   *  If there are any trailing bytes, from the previous call, shift them to
   *  the front of the buffer.
   */
  
  while (src < b->end) {
    *dst++ = *src++;
  }
    
  if ((n = read(fd, dst, b->end - dst)) < 1) {
    if (n == -1) {
      fprintf(stderr, "%s: fillBuf: error filling text buffer: ",
              prog); perror("read");
    }
    return 0;
  }

  b->end = dst + n;

  b->boff = b->noff;

  /*
   *  Set b->sep to point to the last key separator in the buffer.  We don't
   *  want to try to match the regular expression against the last string, as
   *  it probably is not complete.
   *  
   *  (Put a sentinel at b->txt[0] to ensure we don't run past the beginning
   *  of the buffer.)
   */

  {
    char c = b->txt[0];

    b->txt[0] = '\n';
    for (b->sep = b->end - 1; *b->sep != KEYSEP_CH; b->sep--) {
      continue;
    }
    if (b->sep == b->txt && c != KEYSEP_CH) {
      fprintf(stderr, "%s: fillBuf: no key separator in buffer!\n", prog);
      return 0;
    }
    b->txt[0] = c;
  }

  b->noff += b->sep - b->txt + 1;

  return 1;
}


/*
 *  The regular expression search is done by reading in chunks of strings file
 *  into a buffer, and for each chunk setting the key separators to nul, then
 *  checking the resulting strings against the regular expression.  (For
 *  performance reasons, we set the key separators to nul and perform the
 *  comparison in a single pass through the buffer.)
 *  
 *  bug: the `case_sens' flag is ignored.  Comparisons are case sensitive.
 */
int _archRegexSearch(struct arch_stridx_handle *h,
                     const char *key,
                     int case_sens,
                     unsigned long maxhits,
                     unsigned long *nhits,
                     unsigned long start[])
{
  int err;
  long strsSize;

  *nhits = 0;
  h->srchType = SRCH_REGEX;

  if (key[0] == '\0' || maxhits == 0) {
    return 1;
  }

  if ((strsSize = _archFileSizeFP(h->strsFP)) == -1) {
    fprintf(stderr, "%s: _archRegexSearch: can't get size of strings file.\n", prog);
    return 0;
  }

  if (h->rState >= strsSize) {
    /*
     *  We're at the end of the strings file, so there are no more matches.
     */
    return 1;
  }

  if ((err = setjmp(env)) != 0) {
    regerr(key, err);
  } else {
    char *p;
    char cre[512];              /* compiled regular expression */
    size_t n = 0;
    struct inbuf b;

    if (fseek(h->strsFP, h->rState, SEEK_SET) == -1) {
      fprintf(stderr, "%s: _archRegexSearch: can't seek to offset %ld in %lu byte strings file.\n",
              prog, (unsigned long)h->rState, strsSize);
      return 1;
    }

    compile(key, cre, cre + sizeof cre, '\0');

    initBuf(h->rState, &b);
    while (n < maxhits && fillBuf(fileno(h->strsFP), &b)) {
      p = b.txt;
      
      /*
       *  For each string in the buffer set its terminating key separator to
       *  nul, then compare it with the regular expression.
       *  
       *  `b.sep' points to the nul after the last _complete_ string in the
       *  buffer.  (There may be a partial string after it.)
       */
      
      while (n < maxhits && p < b.sep) {
        char *sep = p;

        while (*sep != '\n') {
          sep++;
        }
        *sep = '\0';
        
        if (step(p, cre)) {
          start[n++] = b.boff + (p - b.txt);
        }

        p = sep + 1;
      }
    }

    /*
     *  Set the state to the offset, into the strings file, of the next string
     *  to check against the regular expression.
     *  
     *  `p' points to the first character, in the text buffer `b.txt', of the
     *  next string to check.  `b.boff' is the offset, in the strings file, of
     *  the first character in the text buffer.
     */
    
    h->rState = b.boff + (p - b.txt);
    *nhits = n;
  }

  return 1;
}
