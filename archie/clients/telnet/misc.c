#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <netinet/in.h>
#include <time.h>
#include <pwd.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "client_defs.h"
#include "defines.h"
#include "error.h"
#include "extern.h"
#include "lang.h"
#include "macros.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "mode.h"
#include "prosp.h"
#include "times.h" /* cvt_to_inttime() */
#include "unixcompat.h"
#include "vars.h"

#include "protos.h"

static const char *getstr PROTO((const char *s, int len));
static const char *skip_blanks PROTO((const char *s));
static char *make_lcase PROTO((char *s));
static int char_in_str PROTO((int c, const char *s));
static int rstrspn PROTO((const char *s, const char *span));
static void fmt PROTO((FILE *ofp, const char *s, int indent, int len));


/*  
 *  Return the ASCII value of a Prospero attribute.
 */  

const char *attr_str(vl, astr)
  VLINK vl;
  const char *astr;
{
  PATTRIB p;

  for (p = vl->lattrib; p; p = p->next)
  {
    if ( ! strcmp(astr, p->aname))
    {
      return p->value.sequence->token;
    }
  }
  return curr_lang[179];
}


/* ------------------------------------------------------------------------ */

/*  
 *  Test whether the character, `c', is in the set of characters, `s'.
 *  Return 1 if it is, 0 otherwise.
 *  
 *  If `c' is '\0', then 0 will be returned.
 */  

static int char_in_str(c, s)
  int c;
  const char *s;
{
  if (c == '\0') return 0;
  do
  {
    if (c == *s) return 1;
  }
  while (*++s);
  return 0;
}


/*  
 *  Like `strspn', but in reverse.
 *  
 *  Return the length of the longest sequence of characters, starting from
 *  the end of `s' and proceeding to the left, which all belong to the set
 *  `scan'.
 */  

static int rstrspn(s, span)
  const char *s;
  const char *span;
{
  const char *e;
  int count = 0;

  e = s + strlen(s) - 1;
  while (e >= s && char_in_str(*e, span))
  {
    count++; --e;
  }
  return count;
}


int bracketstr(s, start, end)
  const char *s;
  const char **start;
  const char **end;
{
  *start = s + strspn(s, WHITE_SPACE);
  *end = s + strlen(s) - rstrspn(s, WHITE_SPACE);
  return 1;
}


/* ------------------------------------------------------------------------ */

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
    fputc('\n', ofp);
  }
  else
  {
    last = getstr(s, len - indent);
    for (p = s; *p && p <= last; p++) fputc(*p, ofp);
    fputc('\n', ofp);

    while (*last)
    {
      if ( ! *(s = skip_blanks(last + 1)))
      {
        return;
      }

      last = getstr(s, len - indent);
      for (i = 0; i < indent; i++) fputc(' ', ofp);
      for (p = s; *p && p <= last; p++) fputc(*p, ofp);
      fputc('\n', ofp);
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
  fputs(pfix, ofp);
  fmt(ofp, sfix, (int)strlen(pfix), len);
}


/* ------------------------------------------------------------------------ */

#define QCHAR '"'

/*  
 *  If a string both begins and ends with a quote character it is assumed to
 *  be a quoted string, and they are both stripped off, otherwise the string
 *  remains unchanged.  (Note: a single quote is left unchanged.)
 */  

int dequote(qs)
  char *qs;
{
  char *end;
  int i;
  int len;

  ptr_check(qs, char, curr_lang[181], 0);

  if (*qs == '\0') return 1;
  len = strlen(qs);
  if (len == 1) return 1; /* implies " is left unchanged */
  end = qs + len - 1; /* end point to the last character */
  if (*qs == QCHAR && *end == QCHAR)
  {
    for(i = 0; i < len - 1; i++) qs[i] = qs[i+1];
    qs[i-1] = '\0';
  }
  return 1;
}


void dir_file(path, dir, file)
  const char *path;
  char **dir;
  char **file;
{
  const char *sl;

  if ( ! (sl = strrchr(path, '/')))
  {
    if (dir) *dir = strdup(".");
    if (file) *file = strdup(path);
  }
  else
  {
    if (dir) *dir = strndup(path, sl - path);
    if (file) *file = strdup(sl+1);
  }
}


void fprint_srch_restrictions(fp)
  FILE *fp;
{
  int first = 1;

  if (is_set(V_SEARCH))
  {
    fprintf(fp, curr_lang[320], get_var(V_SEARCH));
    first = 0;
  }
  if (is_set(V_MATCH_DOMAIN))
  {
    fprintf(fp, curr_lang[321], first ? "# " : ", ", get_var(V_MATCH_DOMAIN));
    first = 0;
  }
  if (is_set(V_MATCH_PATH))
  {
    fprintf(fp, curr_lang[322], first ? "# " : ", ", get_var(V_MATCH_PATH));
    first = 0;
  }
  if ( ! first)
  {
    fputs(".\n", fp);
    fflush(fp);
  }
}


const char *get_home_dir_by_name(name)
  const char *name;
{
  struct passwd *p = getpwnam(name);

  if (p)
  {
    return p->pw_dir;
  }
  else
  {
    return (const char *)0;
  }
}


const char *get_home_dir_by_uid(uid)
  int uid;
{
  struct passwd *p = getpwuid(uid);

  if (p)
  {
    return p->pw_dir;
  }
  else
  {
    return (const char *)0;
  }
}


char *head(path, buf, bufsize)
  const char *path;
  char *buf;
  int bufsize;
{
  const char *s = strrchr(path, '/');

  if ( ! s)
  {
    *buf = '\0';
    return buf;
  }
  else
  {
    int l = min(bufsize, s - path);

    if (l == 0)
    {
      strcpy(buf, curr_lang[98]);
    }
    else
    {
      strncpy(buf, path, (unsigned)l);
      buf[l] = '\0';
    }
    return buf;
  }
}


char *make_lcase(s)
  char *s;
{
  char *p;
  
  for (p = s; *p; p++)
  {
    if (isascii(*p) && isupper(*p)) *p = tolower(*p);
  }
  return s;
}


const char *now()
{
  static char tbuf[64];
  time_t c = time((time_t *)0);
  struct tm *t = localtime(&c);
  strftime(tbuf, sizeof tbuf, "%T", t);
  return tbuf;
}


/*  
 *  Replace the newline in 'line' (if it occurs) with '\0'.
 */  

void nuke_newline(line)
  char *line;
{
  char *nl;

  if ( ! line)
  {
    error(A_INTERR, curr_lang[182], curr_lang[183]);
    return;
  }
  if ((nl = strrchr(line, '\n')))
  {
    *nl = '\0';
  }
}


int fempty(fp)
  FILE *fp;
{
  struct stat s;

  if (fstat(fileno(fp), &s) == -1)
  {
    error(A_SYSERR, "fempty", "fstat() failed on fd %d", fileno(fp)); /*FFF*/
    return 1; /*bug: what else?*/
  }
  return s.st_size == 0;
}


int fprint_file(ofp, fname, quietly)
  FILE *ofp;
  const char *fname;
  int quietly;
{
  FILE *ifp;

  if ( ! fname)
  {
    error(A_INTERR, "fprint_file", "file name argument is null");
    return 0;
  }

  if ((ifp = fopen(fname, curr_lang[44])))
  {
    int ret = fprint_fp(ofp, ifp);
    fclose(ifp);
    return ret;
  }
  else if ( ! quietly)
  {
    error(A_SYSERR, "fprint_file", "fopen() failed to open `%s'", fname);
  }

  return 0;
}


int fprint_fp(ofp, ifp)
  FILE *ofp;
  FILE *ifp;
{
  char line[INPUT_LINE_LEN];
  
  rewind_fp(ifp);
  while (fgets(line, sizeof line, ifp))
  {
    if (fputs(line, ofp) == EOF)
    {
      error(A_SYSERR, "fprint_fp", "fputs() failed"); /* FFF */
      return 0;
    }
  }
  fflush(ofp);
  if (ferror(ifp))
  {
    error(A_SYSERR, "fprint_fp", "error from fgets()"); /* FFF */
    return 0;
  }

  return 1;
}


/* ------------------------------------------------------------------------ */

/*  
 *  Like strftime(), but pass it a Prospero time string, instead.
 */  

int prosp_strftime(buf, bufsize, fmt, prosp_time)
  char *buf;
  int bufsize;
  const char *fmt;
  const char *prosp_time;
{
  struct tm *tlocal;
  time_t tt;

  ptr_check(buf, char, curr_lang[190], 0);
  ptr_check(fmt, const char, curr_lang[190], 0);
  ptr_check(prosp_time, const char, curr_lang[190], 0);

  if ( ! (tt = cvt_to_inttime(prosp_time, 0 /* time in GMT */)))
  {
    derror((A_INTERR, curr_lang[108], curr_lang[191], prosp_time));
    strcpy(buf, curr_lang[192]);
    return 0;
  }
  tlocal = localtime(&tt);
  return strftime(buf, bufsize, fmt, tlocal);
}


/* ------------------------------------------------------------------------ */

int mode_truncate_fp(fp)
  FILE *fp;
{
  fflush(fp);
  return current_mode() == M_EMAIL ? 1 : truncate_fp(fp);
}


int rewind_fp(fp)
  FILE *fp;
{
  fflush(fp);
  rewind(fp);
  return 1;
}


int spin()
{
  return current_mode() != M_EMAIL && is_set(V_STATUS);
}


int truncate_fp(fp)
  FILE *fp;
{
  if (ftruncate(fileno(fp), (off_t)0) == -1)
  {
    error(A_SYSERR, "truncate_fp", "ftruncate() failed"); /*FFF*/
    return 0;
  }
  return 1;
}


/*  
 *  Rewrite 'str' in place so that all leading and trailing whitespace is
 *  removed and all other whitespace is replaced with a single space.
 *  
 *  Return -1 upon an error, otherwise return the number of "words" in the
 *  string.  (A word is a sequence of non-whitespace characters.)
 */  

char *squeeze_whitespace(str)
  char *str;
{
  char *rd;
  char *wr;
  int in_word;

  if ( ! str)
  {
    error(A_INTERR, curr_lang[198], curr_lang[199]);
    return (char *)0;
  }

  rd = wr = str;
  while (isspace((int)*rd)) rd++; /* skip leading whitespace */
  in_word = 1;

  while (*rd != '\0')
  {
    if (isspace((int)*rd))
    {
      rd++;
      in_word = 0;
    }
    else
    {
      if ( ! in_word)
      {
	in_word = 1;
        *wr++ = ' ';
      }
      *wr++ = *rd++;
    }
  }

  *wr = '\0';
  return str;
}


/*  
 *  Translate each `fromc' in `s' to a `toc'.
 */  

char *strxlate(s, fromc, toc)
  char *s;
  int fromc;
  int toc;
{
  char *p;

  for (p = s; *p; p++)
  {
    if (*p == fromc)
    {
      *p = toc;
    }
  }
  return s;
}


#ifndef P5_WHATIS
/*  
 *  The following two routines implement the case-sensitive and case-
 *  insensitive substring search.
 */  

#define TABLESIZE 256

static char *pat = (char *)0;
static int nocase;
static int plen;
static int plen_1;
static int skiptab[TABLESIZE];

int initskip(pattern, len, ignore_case)
  const char *pattern;
  int len;
  int ignore_case;
{
  int i;

  plen = len;
  plen_1 = plen - 1;

  nocase = ignore_case;

  if (pat == (char *) 0)
  {
    if ((pat = (char *) malloc((unsigned) (plen + 1))) == (char *) 0)
    {
      error(A_ERR, curr_lang[200], curr_lang[201],
	    plen + 1);
      return 0;
    }
  }
  else
  {
    if ((pat = (char *) realloc(pat, (unsigned) (plen + 1))) == (char *) 0)
    {
      error(A_ERR, curr_lang[200], curr_lang[202],
	    plen + 1);
      return 0;
    }
  }

  memcpy(pat, pattern, (unsigned)(plen + 1));

  if (ignore_case) make_lcase(pat);

  for (i = 0; i < TABLESIZE; i++)
    skiptab[i] = plen;

  for (i = 0; i < plen; i++)
  {
    skiptab[(int)*(pat + i)] = plen - 1 - i;
  }

  return 1;
}


int strfind(txt, tlen)
  const char *txt;
  int tlen;
{
  int pc;
  int skip;
  int tc;
  unsigned char tchar;

  if (tlen < plen)
  return 0;

  pc = tc = plen_1;

  do
  {
    tchar = *(txt + tc);
    if (nocase)
    {
      if (isascii((int) tchar) && isupper((int) tchar))
      {
        tchar = (char) tolower((int) tchar);
      }
    }

    if (tchar == (unsigned char /*why aren't they the same?*/)*(pat + pc))
    {
      --pc;
      --tc;
    }
    else
    {
      skip = skiptab[tchar];
      tc += (skip < plen_1 - pc) ? plen : skip;
      pc = plen_1;
    }
  }
  while (pc >= 0 && tc < tlen);

  return pc < 0;
}
#endif


int install_term(ac, av)
  int ac;
  char **av;
{
  if (ac < 2)
  {
    printf(curr_lang[203], av[0]);
    return 0;
  }
  else
  {
    char term[256];
    int i;

    term[0] = term[1] = '\0'; /*bug: kludge*/
    for(i = 1; i < ac; i++)
    {
      strcat(term, curr_lang[57]);
      strcat(term, av[i]);
    }

    return set_var(V_TERM, term + 1); /*bug: kludge*/
  }
}


/* ------------------------------------------------------------------------ */


/*  
 *  Two routines to convert between strings and their corresponding values,
 *  as defined by the StrVal structure.
 */  

int strtoval(str, val, sv)
  const char *str;
  int *val;
  StrVal *sv;
{
  StrVal *s = sv;

  if ( ! str || ! val || ! sv)
  {
    return 0;
  }

  for (s = sv; s->str != (const char *)0; s++)
  {
    if (strcmp(str, s->str) == 0)
    {
      *val = s->val;
      return 1;
    }
  }
  return 0;
}


int valtostr(val, str, sv)
  int val;
  const char **str;
  StrVal *sv;
{
  StrVal *s = sv;

  if ( ! str || ! sv)
  {
    return 0;
  }

  for (s = sv; s->str != (const char *)0; s++)
  {
    if (s->val == val)
    {
      *str = s->str;
      return 1;
    }
  }
  return 0;
}


int chroot_for_exec(dir)
  const char *dir;
{
  return (regain_root() &&
          chdir(dir) != -1 &&
          chroot(".") != -1 &&
          setuid(getuid()) != -1);
}


/*  
 *  Actually, just checks whether or not our effective uid is the same as our
 *  real uid.  (I.e. may fail if we are suid foo, and are started by foo.)
 */  
int we_are_suid()
{
  return getuid() != geteuid();
}
