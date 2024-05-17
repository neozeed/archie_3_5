#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include "defs.h"
#include "utils.h"


#define NCHARS (1 << (CHAR_BIT - 1))


char *chstr(int ch)
{
  static char s[3];
  
  s[0] = '\\'; s[1] = s[2] = '\0';
  
  switch (ch) {
  case '\0': s[1] = '0'; break;
  case '\a': s[1] = 'a'; break;
  case '\b': s[1] = 'b'; break;
  case '\f': s[1] = 'f'; break;
  case '\n': s[1] = 'n'; break;
  case '\r': s[1] = 'r'; break;
  case '\t': s[1] = 't'; break;
  case '\v': s[1] = 'v'; break;
  default:   s[0] = ch;  break;
  }

  return s;
}


char strch(const char *s)
{
  if (s[0] != '\\') {
    return s[0];
  } else {
    switch (s[1]) {
    case '0': return '\0';
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    default:  return s[1];
    }
  }
}


char *stringcopy(const char *s)
{
  char *c;

  if ((c = malloc(strlen(s) + 1))) {
    strcpy(c, s);
  }

  return c;
}


/*  
 *  Copy `src' to `dst' converting all sequences of the form `\c' to the
 *  appropriate character.  Assume `dst' is not the same buffer as `src'.
 *  
 *  Characters are copied until the source string is exhausted or until the
 *  destination string is filled (including final nul).
 */
char *bdequote(const char *src, int dstsz, char *dst)
{
  int i;
  
  for (i = 0; *src && i < dstsz - 1; src++, i++) {
    dst[i] = strch(src);
    if (*src == '\\') src++;
  }

  dst[i] = '\0';
  return dst;
}


int bmpreproc(char *pat, void **state)
{
  int j, k, m, t, t1, q, q1;
  int *d, *f, *skip;            /* d[patlen], f[patlen], skip[NCHARS] */

  m = strlen(pat);

  if ( ! (d = malloc((2 * m + NCHARS) * sizeof(int)))) {
    return 0;
  }

  f = d + m; skip = f + m;

  for (k = 0; k < NCHARS; k++) skip[k] = m;
  for (k = 1; k <= m; k++) {
    d[k-1] = 2 * m - k;
    skip[pat[k-1]] = m - k;
  }

  t = m + 1;

  for (j = m; j > 0; j--, t--) {
    f[j-1] = t;
    while (t <= m && pat[j-1] != pat[t-1]) {
      d[t-1] = MIN(d[t-1], m - j);
      t = f[t-1];
    }
  }

  q = t; t = m + 1 - q; q1 = 1; t1 = 0;

  for (j = 1; j <= t; j++, t1++) {
    f[j-1] = t1;
    while (t1 >= 1 && pat[j-1] != pat[t1-1]) {
      t1 = f[t1-1];
    }
  }

  while (q < m) {
    for (k = q1; k <= q; k++) d[k-1] = MIN(d[k-1], m + q - k);
    q1 = q + 1; q = q + t - f[t-1]; t = f[t-1];
  }
  
  *state = d;

  return 1;
}


char *bmstrstr(char *pat, size_t txtlen, char *txt, void *state)
{
  int *d, *skip;
  int j, k, m;

  m = strlen(pat);
  if (m == 0) return txt;

  d = (int *)state; skip = d + 2 * m;

  for (k = m - 1; k < txtlen; k += MAX(skip[txt[k]], d[j])) {
    for (j = m - 1; j >= 0 && txt[k] == pat[j]; j--) k--;
    if (j == -1) return txt + k + 1;
  }

  return 0;
}


/*  
 *  The returned length doesn't include a terminating nul.
 */  
int vslen(const char *fmt, va_list ap)
{
  static FILE *nfp = 0;
  int len;

  if ( ! nfp && ! (nfp = fopen("/dev/null", "w")))
  {
    fprintf(stderr, "vslen: can't open `/dev/null' for writing.\n");
    return -1;
  }
  len = vfprintf(nfp, fmt, ap);
  return len;
}


/*  
 *  Like sprintf(), but allocate sufficient memory to contain the result.
 *
 *  Return the the number of characters, not including the terminating nul,
 *  written to *buf.  On error set *buf to 0 and return -1.
 *
 *  Note: on a Sun this is about half the speed of sprintf().
 */  
int memsprintf(char **buf, const char *fmt, ...)
{
  char *mem;
  int len;
  va_list ap;

  va_start(ap, fmt);
  if ((len = vslen(fmt, ap)) < 0)
  {
    goto fail;
  }
  if ( ! (mem = malloc(len + 1)))
  {
    goto fail;
  }
  if ((len = vsprintf(mem, fmt, ap)) < 0)
  {
    free(mem);
    goto fail;
  }
  va_end(ap);
  *buf = mem;
  return len;

 fail:
  *buf = 0;
  va_end(ap);
  return -1;
}


/*  
 *  Similar to ANSI realloc(), but we return the new pointer through a
 *  parameter rather than as the result.  The returned value indicates
 *  success or failure.
 */
int _patrieReAlloc(void *buf, size_t sz, void **new)
{
  void *n;

  if (buf) {
    if ((n = realloc(buf, sz))) {
      *new = n;
      return 1;
    } else {
      fprintf(stderr, "%s: _patrieReAlloc: can't allocate %lu bytes: ",
              prog, (unsigned long)sz); perror("realloc");
      return 0;
    }
  } else {
    if ((n = malloc(sz))) {
      *new = n;
      return 1;
    } else {
      fprintf(stderr, "%s: _patrieReAlloc: can't allocate %lu bytes: ",
              prog, (unsigned long)sz); perror("malloc");
      return 0;
    }
  }
}


off_t _patrieFpSize(FILE *fp)
{
  struct stat s;

  if (fstat(fileno(fp), &s) == -1) {
    return -1;
  }

  return s.st_size;
}


void _patrieDebugBreak(void)
{
  return;
}
