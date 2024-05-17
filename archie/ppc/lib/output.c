#include <stdio.h>
#include "defs.h"
#include "protos.h"
#include "all.h"

static const char *getstr proto_((const char *s, int len));
static const char *skip_blanks proto_((const char *s));
static void fmt proto_((FILE *ofp, const char *s, int indent, int len));


static const char *skip_blanks(s)
  const char *s;
{
  while (*s && *s == ' ') s++;
  return s;
}


/*  
 *  Find the longest substring of `s', whose length is less than or equal to
 *  `len', and whose last character precedes a space or nul.
 *  
 *  Also, skip over leading blanks in `s'.
 */    

static const char *getstr(s, len)
  const char *s;
  int len;
{
  char oc = '\0';
  const char *p;
  const char *sp = s;
  int n = 0;

  for (p = s; *p; p++)
  {
    if (*p == ' ' && oc != ' ') sp = p;
    n++;
    if (n >= len) return sp == s ? p : sp;
    oc = *p;
  }

  return p;
}


static void fmt(ofp, s, indent, len)
  FILE *ofp;
  const char *s;
  int indent;
  int len;
{
  const char *last;
  const char *p;
  int i;

  s = skip_blanks(s);
  if ( ! *s)
  {
    fputs("\r\n", ofp);
  }
  else
  {
    last = getstr(s, len - indent);
    for (p = s; *p && p <= last; p++) fputc(*p, ofp);
    fputs("\r\n", ofp);

    while (*last)
    {
      if ( ! *(s = skip_blanks(last + 1)))
      {
        return;
      }

      last = getstr(s, len - indent);
      for (i = 0; i < indent; i++) fputc(' ', ofp);
      for (p = s; *p && p <= last; p++) fputc(*p, ofp);
      fputs("\r\n", ofp);
    }
  }
}


/*  
 *  Print a string as a series of lines, each of which is indented (i.e.
 *  prefixed by spaces) by a specified amount.  Assume the first line is
 *  already indented.
 */  
void fmtprintf(ofp, pfix, sfix, len)
  FILE *ofp;
  const char *pfix;
  const char *sfix;
  int len;
{
  fputs((char *)pfix, ofp);
  fmt(ofp, sfix, strlen(pfix), len);
}
