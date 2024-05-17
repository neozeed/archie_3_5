/*
  Do what fprintf does, but also recognize %m, which causes the error
  message associated with 'errno' to be substituted.

  E.g.

    if(open("foo", O_RDONLY) == -1)
    {
      efprintf(stderr, "%s: open(\"foo\", O_RDONLY): %m\n", prog);
    }
*/

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef __STDC__
# include <stdlib.h>
# include <unistd.h>
#else
# include <malloc.h>
#endif
#include "ansi_compat.h"
#include "defs.h"
#include "error.h"
#include "protos.h"
#include "all.h"


extern char *sys_errlist[];
extern int sys_nerr;


static const char *esub proto_((const char *s, int e));
static const char *fpm proto_((const char *s));


/*  
 *  Return a pointer to the first valid occurrence of "%m" in 's'.  (I.e. it
 *  correctly handles "%%m").
 *  
 *  If none is found return the NULL pointer.
 */  

static const char *fpm(s)
  const char *s;
{
  const char *b = 0;
  const char *p;
  int pct = 0;

  for(p = s; *p != '\0'; ++p)
  {
    if(pct)
    {
      if(*p == '%' || *p != 'm')
      {
        pct = 0;
        b = (char *)0;
      }
      else
      {
        return b;
      }
    }
    else
    {
      if(*p == '%')
      {
        pct = 1;
        b = p;
      }
    }
  }

  return (char *)0;
}


/*  
 *  Return a pointer to a string identical to 's', but with all valid
 *  occurrences of "%m" expanded to the error string corresponding to 'e'.
 */  

static const char *esub(s, e)
  const char *s;
  int e;
{
  static char *bad = "<unknown error>";
  char *estr;
  char *news;
  const char *oldp;
  const char *p;
  int elen;
  int mem;
  int n ;


  /*  
   *  Pass 1: count the number of %m's to determine how much memory to
   *  malloc.
   */  
  estr = errno < 0 || errno >= sys_nerr ? bad : sys_errlist[errno];
  elen = strlen(estr);

  n = 0;
  p = s;
  while((p = fpm(p)) != (char *)0)
  {
    ++n;
    ++p;
  }

  mem = strlen(s) + 1 - (n * 2) + (n * elen);
  if((news = malloc((unsigned)mem)) == (char *)0)
  {
    return (char *)0;
  }
  *news = '\0';

  /*  
   *  Pass 2: substitute the error string for all valid %m's.
   */  
  oldp = p = s;
  while((p = fpm(oldp)) != (const char *)0)
  {
    strncat(news, oldp, p - oldp);
    strcat(news, estr);
    oldp = p + 2;
  }
  strcat(news, oldp);

  return news;
}


int copy_to_stderr(fp)
  FILE *fp;
{
  return debug && fp != stderr;
}


int efprintf(FILE *fp, const char *fmt, ...)
{
  const char *s;
  int r;
  va_list ap;

  va_start(ap, fmt);
  if((s = esub(fmt, errno)) == (char *)0)
  {
    s = fmt;
  }
  r = vfprintf(fp, s, ap); fflush(fp);
  if (copy_to_stderr(fp))
  {
    vfprintf(stderr, s, ap);
  }
  va_end(ap);
  if(s != fmt)
  {
    free((char *)s);
  }

  return r;
}


int error(const char *file, const char *fmt, ...)
{
  FILE *fp;
  const char *s;
  int r;
  va_list ap;

  if ( ! (fp = fopen(file, "a+")))
  {
    return 0;
  }
  if ( ! (s = esub(fmt, errno)))
  {
    s = fmt;
  }

  va_start(ap, fmt);
  r = vfprintf(fp, s, ap);
  va_end(ap);

  fclose(fp);
  if(s != fmt)
  {
    free((char *)s);
  }

  return r;
}
