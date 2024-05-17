/*  
 *  Usage:
 *  
 *  chartab '<charlist>'
 *  
 *  Create a table of 256 values, with all entries, except those listed in
 *  the argument, set to zero.  Non-zero entries are set to one.
 */  

#ifdef __STDC__
# include <stdlib.h>
#else
# include <memory.h> /* for memset() */
#endif
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "all.h"


#define CLEAR 0
#define LLEN  8 /* number of items per line of output */
#define SET   1


static char *read_chars();
static char *tail();
static int backslash();
static int get_octal_char();
static void print_tab();


char *prog;
char tab[256];


int main(ac, av)
  int ac;
  char **av;
{
  char *p;
  int val;
  
  prog = tail(av[0]);
  switch (ac)
  {
  case 1:
    p = read_chars(stdin);
    break;

  case 2:
    p = av[1];
    break;

  default:
    fprintf(stderr, "Usage: %s [<char-list>]\n", prog);
    exit(1);
  }

  memset(tab, CLEAR, sizeof tab);
  for (; *p; p++)
  {
    if (*p != '\\')
    {
      val = *p;
    }
    else
    {
      if ((val = backslash(&p)) < 0)
      {
        exit(1);
      }
    }

    tab[val] = SET;
  }

  print_tab(tab, sizeof tab, LLEN);
  exit(0);
}


static char *read_chars(fp)
  FILE *fp;
{
  char *nl;
  static char c[1024 + 2]; /* 256 times 4 chars (\xxx) plus newline and nul */

  c[0] = '\0';
  c[1024] = '\0';
  fgets(c, sizeof c, fp);
  if ((nl = strchr(c, '\n')))
  {
    *nl = '\0';
  }
  else if (c[1024] != '\n')
  {
    fprintf(stderr, "%s: input line too long! (sorry...)\n", prog);
    return 0;
  }

  return c;
}


static char *tail(path)
  char *path;
{
  char *p = strrchr(path, '/');
  return p ? p+1 : path;
}


static int backslash(p)
  char **p;
{
  char *cp = *p + 1;
  char c = *cp;
  int ret = -1;

  if (isdigit(c))
  {
    if ((ret = get_octal_char(cp)) >= 0)
    {
      *p = cp + 2;
    }
    else
    {
      fprintf(stderr, "%s: non-octal number, `%3s', follows backslash.\n",
              prog, cp);
    }
  }
  else
  {
    switch (c)
    {
    case 'a' : ret = '\007' ; break; /* ANSI */
    case 'b' : ret = '\b'   ; break;
    case 'f' : ret = '\f'   ; break;
    case 'n' : ret = '\n'   ; break;
    case 'r' : ret = '\r'   ; break;
    case 't' : ret = '\t'   ; break;
    case 'v' : ret = '\v'   ; break;
    case '0' : ret = '\0'   ; break;
    case '\\': ret = '\\'   ; break;
    case '\'': ret = '\''   ; break;
    case '\"': ret = '\"'   ; break;
    case '?' : ret = '\077' ; break; /* ANSI */

    default:
      fprintf(stderr, "%s: bad character `\\%03o' after backslash.\n",
              prog, (unsigned)c);
      break;
    }
  }

  return ret;
}


static int get_octal_char(s)
  char *s;
{
  int ret = -1;
  
  if (s[0] >= '0' && s[0] <= '3' &&
      s[1] >= '0' && s[1] <= '7' &&
      s[2] >= '0' && s[2] <= '7')
  {
    sscanf(s, "%3o", &ret);
  }

  return ret;
}


static void print_tab(tab, sz, llen)
  char *tab;
  int sz;
  int llen;
{
  int i;
  
  for (i = 0; i < sz; i++)
  {
    if (i % llen == 0) putchar('\n');
    printf("%d, ", tab[i]);
  }
  putchar('\n');
}
