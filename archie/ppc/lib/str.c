#ifdef __STDC__
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#ifdef VARARGS
# include <varargs.h>
#else
# include <stdarg.h>
#endif
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "misc.h"
#include "protos.h"
#include "str.h"
#include "all.h"


static int cin proto_((int c, const char *cset));


#define CQUOTE '\\'
#define SQUOTE '"'
#define WHITESPACE " \t"
  

/*  
 *  Rewrite the string `s', replacing character pairs of the form \c with the
 *  single character c.  If a \ appears at the end of the string, and is not
 *  quoted, then it is dropped.
 */        
static char *char_dequote(s)
  char *s;
{
  if (*s)
  {
    char *dst = s;
    char *src = s;

    do
    {
      if (*src == CQUOTE) src++;
    }
    while ((*dst++ = *src++));
  }

  return s;
}


static int find_quotes(s, open, close)
  const char *s;
  const char **open;
  const char **close;
{
  const char *c;
  const char *o = s;

  while (*o && *o != SQUOTE) o++;
  if ( ! *o) return 0;

  c = o+1;
  while (*c)
  {
    if (*c == SQUOTE)
    {
      *open = o; *close = c;
      return 1;
    }
    if (*c == CQUOTE && *++c == '\0')
    {
      return 0;
    }
    c++;
  }

  return 0;
}

  
int cin(c, cset)
  int c;
  const char *cset;
{
  if (c == 0) return 0;
  else return !! strchr(cset, c);
}


char *strend(s)
  char *s;
{
  while (*s++);
  return s-1;
}


char *strncopy(s1, s2, n)
  char *s1;
  const char *s2;
  int n;
{
  strncpy(s1, s2, n);
  s1[n] = '\0';
  return s1;
}


int strrspn(s, cset)
  char *s;
  const char *cset;
{
  int l;

  if ( ! *s)
  {
    l = 0;
  }
  else
  {
    char c = *s;
    char *e = strend(s);
    char *p = e;

    *s = '\0';
    for (p = e - 1; cin(*p, cset); p--);
    if (*p) l = e - p - 1;
    else if (cin(c, cset)) l = e - p;
    else l = e - p - 1;
    *s = c;
  }

  return l;
}


#ifdef VARARGS
char *strsdup(va_alist)
  va_dcl
{
  char *mem = 0;
  const char *p;
  const char *s;
  int len = 0;
  va_list ap;

  if (s)
  {
    p = s;
    len += strlen(s) + 1; /* +1 for the final nul */

    va_start(ap);
    for (p = va_arg(ap, const char *); p; p = va_arg(ap, const char *))
    {
      len += strlen(p);
    }
    va_end(ap);

    if ((mem = malloc(len)))
    {
      strcpy(mem, s);
      va_start(ap);
      for (p = va_arg(ap, const char *); p; p = va_arg(ap, const char *))
      {
        strcat(mem, p);
      }
      va_end(ap);
    }
  }

  return mem;
}

#else

char *strsdup(const char *s, ...)
{
  char *mem = 0;
  va_list ap;

  if (s)
  {
    const char *p;
    int len = 0;

    len = strlen(s);
    va_start(ap, s);
    while ((p = va_arg(ap, const char *)))
    {
      len += strlen(p);
    }
    va_end(ap);
    if ((mem = malloc(len + 1)))
    {
      strcpy(mem, s);
      va_start(ap, s);
      while ((p = va_arg(ap, const char *)))
      {
        strcat(mem, p);
      }
      va_end(ap);
    }
  }

  return mem;
}
#endif


/*  
 *  If the initial string in `s' is the same as `skip' return a pointer to
 *  the character after the match, otherwise return NULL.
 */  
char *strskip(s, skip)
  char *s;
  const char *skip;
{
  for (; *s && *s == *skip; s++, skip++);
  return *skip ? 0 : s;
}


char *strterm(str, c)
  char *str;
  int c;
{
  char *s;

  for (s = str; *s; s++)
  {
    if (*s == c)
    {
      *s = '\0';
      break;
    }
  }

  return str;
}


char *tr(s, src, dst)
  char *s;
  int src;
  int dst;
{
  for (; *s; s++)
  {
    if (*s == src) *s = dst;
  }

  return s;
}


char *trimright(s, tchars)
  char *s;
  const char *tchars;
{
  *(s + strcspn(s, tchars)) = '\0';
  return s;
}


/*  
 *  Duplicate the n'th word of `s'.  An `n' of 0 corresponds to the
 *  first word.
 */  
char *worddup(s, n)
  const char *s;
  int n;
{
  s +=+ strspn(s, WHITESPACE);
  while (n-- > 0)
  {
    s +=+ strcspn(s, WHITESPACE);
    s +=+ strspn(s, WHITESPACE);
  }

  if (*s) return strndup(s, strcspn(s, WHITESPACE));
  else return 0;
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
 *  Split the `src' string into `n' or fewer substrings, each consisting of a
 *  sequence of non-whitespace characters, or a doubly quoted sequence of
 *  characters (which is dequoted).  Substrings are duplicated and pointers
 *  to them are put in successive locations of `dst'.  The number of
 *  substrings processed is returned.
 */      
/*  
 *  bug: assumes no malloc() errors...
 */  
int qwstrnsplit(src, dst, n, remain)
  const char *src;
  char *dst[];
  int n;
  const char **remain;
{
  const char *e;
  const char *s;
  int i;

  e = s = src;
  for (i = 0; i < n; i++)
  {
    s = e + strspn(e, WHITESPACE);
    if (*s == '\0')
    {
      if (remain) *remain = s;
      return i;
    }
    else if (*s != SQUOTE)
    {
      e = s + strcspn(s, WHITESPACE);
      dst[i] = strndup(s, e - s);
    }
    else
    {
      char *cq, *oq;
      
      if ( ! find_quotes(s, &oq, &cq))
      {
        if (remain) *remain = s;
        return i;
      }
      else
      {
        dst[i] = strndup(oq+1, cq - oq - 1);
        char_dequote(dst[i]);
        e = cq + 1;
      }
    }
  }

  if (remain) *remain = e;
  return i;
}


#if 0
/* Now in libarchie/libparchie. */

int splitwhite(src, ac, av)
  const char *src;
  int *ac;
  char ***av;
{
  char **argv;                  /* resulting array of string pointers */
  char *sptr;                   /* pointer to start of current result string */
  int nstrs;                    /* total number of strings */
  int strslen = 0;              /* total length of all strings, not including nuls */

  *ac = 0; *av = 0;

  /*  
   *  Count the number of strings, and the sum of their lengths.
   */  

  {
    int n = 0;
    const char *s = src;
    
    while (1) {
      const char *e;
      int len;
  
      s += strspn(s, WHITESPACE); /* skip leading white space */
      e = s + strcspn(s, WHITESPACE); /* skip over word */
      len = e - s;

      if (len == 0) {
        break;
      }

      s = e;
      n++; strslen += len;
    }

    nstrs = n;
  }
  
  if (nstrs == 0) {
    return 1;
  }

  /*  
   *  Allocate enough memory for the strings (including nul terminators) and
   *  the pointers to them.
   */

  if ( ! (argv = malloc(nstrs * sizeof(char **) + (strslen + nstrs)))) {
    return 0;
  }

  sptr = (char *)(argv + nstrs);

  /*  
   *  Reparse the source string, copying the words into the result, and
   *  setting up pointers to them.
   */

  {
    int n = 0;
    const char *s = src;

    strslen = 0;
    while (1) {
      const char *e;
      int len;

      s += strspn(s, WHITESPACE); /* skip leading white space */
      e = s + strcspn(s, WHITESPACE); /* skip over word */
      len = e - s;

      if (len == 0) {
        break;
      }

      argv[n] = sptr + strslen;
      strncpy(argv[n], s, len); *(argv[n] + len) = '\0';
      strslen++;
    
      s = e;
      n++; strslen += len;
    }
  }

  *ac = nstrs;
  *av = argv;

  return 1;
}
#endif


#if 0
/* CURRENTLY UNUSED */

/*  
 *  Split the `src' string into `n' or fewer substrings, each consisting of a
 *  sequence of non-`splitstr' characters.  Substrings are duplicated and
 *  pointers to them are put in successive locations of `dst'.  The number of
 *  substrings processed is returned.
 */        
/*  
 *  bug: assumes no malloc() errors...
 */  
int strnsplit(splitstr, src, dst, n, remain)
  const char *splitstr;
  const char *src;
  char *dst[];
  int n;
  const char **remain;
{
  const char *e;
  const char *s;
  int i;

  e = s = src;
  for (i = 0; i < n; i++)
  {
    s = e + strspn(e, splitstr);
    if (*s == '\0')
    {
      if (remain) *remain = s;
      return i;
    }
    e = s + strcspn(s, WHITESPACE);
    dst[i] = strndup(s, e - s);
  }

  if (remain) *remain = e;
  return i;
}
#endif


#if 0
/* Now in libarchie/libparchie. */

/*  
 *  Split a string into substrings, based on a set of separator characters.
 *  
 *  Separator characters appearing at the beginning or end of the source
 *  string will result in leading or trailing empty substrings.  Also, a
 *  sequence of N separator characters appearing in the middle of the source
 *  string will result in N-1 empty substrings.
 *
 *  E.g.  `/foo///bar/' becomes `', `foo', `', `', `bar', `'.
 *  
 *  The return value is a pointer to a null terminated array of pointers to
 *  the substrings.  Only one block of memory is allocated, so only the
 *  returned pointer need be freed.
 *  
 *  Zero is returned on an error.
 */  
int strsplit(src, splchs, ac, av)
  const char *src;
  const char *splchs;
  int *ac;
  char ***av;
{
  char *mem;
  char **ptrmem;
  char *strmem;
  char *p, *s;
  int n;
  int memsz;
  int numss;
  int srclen;
  int ptrsz;

  /*  
   *  Determine the number of substring pointers we'll need.
   */  
  for (numss = 0, p = (char *)src; *p; p++)
  {
    if (strchr(splchs, *p)) numss++;
  }
  numss++;

  srclen = strlen(src);
  ptrsz = numss * sizeof (char *);
  memsz = ptrsz + srclen + 1;

  if ( ! (mem = malloc(memsz)))
  {
    return 0;
  }

  ptrmem = (char **)mem;
  strmem = mem + ptrsz;

  memcpy(strmem, src, srclen + 1);

  for (n = 0, s = p = strmem; *p; p++)
  {
    if (strchr(splchs, *p))
    {
      *p = '\0';
      ptrmem[n++] = s;
      s = p + 1;
    }
  }
  ptrmem[n] = s;

  *ac = numss;
  *av = ptrmem;

  return 1;
}

#elif 0
/* Preceding version is now used. */

/*  
 *  Split a string into substrings, based on a set of separator characters.
 *  
 *  Separator characters appearing at the beginning or end of the source
 *  string will result in leading or trailing empty substrings.  Also, a
 *  sequence of N separator characters appearing in the middle of the source
 *  string will result in N-1 empty substrings.
 *
 *  E.g.  `/foo///bar/' becomes `', `foo', `', `', `bar', `'.
 *  
 *  The return value is a pointer to a null terminated array of pointers to
 *  the substrings.  Only one block of memory is allocated, so only the
 *  returned pointer need be freed.
 *  
 *  A null pointer is returned on an error.
 */  
char **strsplit(src, splchs)
  const char *src;
  const char *splchs;
{
  char *mem;
  char **ptrmem;
  char *strmem;
  char *p, *s;
  int n;
  int memsz;
  int numss;
  int srclen;
  int ptrsz;

  /*  
   *  Determine the number of substring pointers we'll need.
   */  
  for (numss = 0, p = (char *)src; *p; p++)
  {
    if (strchr(splchs, *p)) numss++;
  }
  numss++;

  srclen = strlen(src);
  ptrsz = (numss + 1) * sizeof (char *);
  memsz = ptrsz + srclen + 1;

  if ( ! (mem = malloc(memsz)))
  {
    return 0;
  }

  ptrmem = (char **)mem;
  strmem = mem + ptrsz;

  memcpy(strmem, src, srclen + 1);

  for (n = 0, s = p = strmem; *p; p++)
  {
    if (strchr(splchs, *p))
    {
      *p = '\0';
      ptrmem[n++] = s;
      s = p + 1;
    }
  }
  ptrmem[n] = s;
  ptrmem[n+1] = 0;

  return ptrmem;
}
#endif

   
#ifdef VARARGS
int vslen(va_alist)
  va_dcl
{
  const char *fmt;
  int len;
  static FILE *nfp = 0;
  va_list ap;

  if ( ! nfp && ! (nfp = fopen("/dev/null", "w")))
  {
    efprintf(stderr, "%s: vslen: can't open `/dev/null' for writing.\n",
             logpfx());
    return -1;
  }
  va_start(ap);
  fmt = va_arg(ap, const char *);
  len = vfprintf(nfp, fmt, ap);
  va_end(ap);

  return len;
}

#else

#if 1
int vslen(const char *fmt, va_list ap)
{
  static FILE *nfp = 0;
  int len;

  if ( ! nfp && ! (nfp = fopen("/dev/null", "w")))
  {
    efprintf(stderr, "%s: vslen: can't open `/dev/null' for writing.\n",
             logpfx());
    return -1;
  }
  len = vfprintf(nfp, fmt, ap);
  return len;
}

#else

int vslen(const char *fmt, ...)
{
  static FILE *nfp = 0;
  int len;
  va_list ap;

  if ( ! nfp && ! (nfp = fopen("/dev/null", "w")))
  {
    efprintf(stderr, "%s: vslen: can't open `/dev/null' for writing.\n",
             logpfx());
    return -1;
  }
  va_start(ap, fmt);
  len = vfprintf(nfp, fmt, ap);
  va_end(ap);
  return len;
}
#endif
#endif
