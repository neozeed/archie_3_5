#include <malloc.h>
#include <string.h>
#include "error.h"
#include "extern.h"
#include "lang.h"
#include "macros.h"
#include "misc_ansi_defs.h"
#include "rmem.h"


typedef enum
{
  AV_OUT_STR,
  AV_IN_STR,
  AV_IN_QUOTE,
  AV_Q_IN_STR,
  AV_Q_IN_QUOTE,
  AV_DONE,
  AV_ERROR
} State;


/*  
 *
 *                                     Internal routines
 *
 */  

#define INIT_ARGV_ELTS 16
#define INIT_STR_ELTS 32


static char **end_current_str PROTO((char *s, char **av));
static char **init_argv PROTO((void));
static char *insert_char PROTO((int c, char *s));
static char *new_str PROTO((void));


static char *insert_char(c, s)
  int c;
  char *s;
{
  char ch = c;
  char *ts = rappend(s, &ch, char);

  if (ts) return ts;
  else
  {
    rfree(s);
    return (char *)0;
  }
}


static char **end_current_str(s, av)
  char *s;
  char **av;
{
  char *ts = insert_char('\0', s);

  if (ts) return (char **)rappend(av, &ts, char *);
  else
  {
    rfree(s);
    return (char **)0;
  }
}


static char **init_argv()
{
  char **av;

  if ((av = rmalloc(INIT_ARGV_ELTS, char *)))
  {
    av[0] = (char *)0;
    return (char **)av;
  }
  else
  {
    error(A_SYSERR, curr_lang[65]);
    return (char **)0;
  }
}


static char *new_str()
{
  char *s = rmalloc(INIT_STR_ELTS, char);

  if (s) return s;
  else
  {
    error(A_SYSERR, curr_lang[66], curr_lang[67], INIT_STR_ELTS);
    return (char *)0;
  }
}


/*  
 *
 *                                  External routines
 *
 */  


/*bug: generic argvFindStrCmp()*/
int argvFindStrCase(ac, av, start, end, str)
  int ac;
  char **av;
  int start;
  int end;
  const char *str;
{
  int i;

  for (i = start; i <= end && i < ac; i++)
  {
    if (strcasecmp(av[i], str) == 0) return i;
  }
  return -1;
}


/*bug: start > ac or all args empty could cause problems */
char *argvFlattenSep(ac, av, start, end, sep)
  int ac;
  char **av;
  int start;
  int end;
  const char *sep;
{
  char *t = (char *)0;
  int i;
  int len = 0;
  int seplen = strlen(sep);

  ptr_check(av, char *, curr_lang[68], (char *)0);

  for (i = start; i <= end && i < ac; i++)
  {
    len += strlen(av[i]) + seplen;
  }
  if ( ! (t = malloc(len+1))) /*bug? +1 for nul of final strcat */
  {
    error(A_SYSERR, curr_lang[68], curr_lang[69], len);
    return (char *)0;
  }
  else
  {
    t[0] = '\0';
    for (i = start; i <= end && i < ac; i++)
    {
      strcat(t, av[i]);
      strcat(t, sep);
    }
    t[len - 1] = '\0';
    return (char *)t;
  }
}


int argvReplaceStr(ac, av, idx, newstr)
  int ac;
  char **av;
  int idx;
  const char *newstr;
{
  char *s;

  if (idx >= ac) return 0;
  
  s = rmalloc((unsigned)strlen(newstr) + 1, char);
  if ( ! s)
  {
    return 0;
  }
  else
  {
    strcpy(s, newstr);
    rfree(av[idx]);
    av[idx] = s;
    return 1;
  }
}


/*bug: start > ac or all args empty could cause problems */
char *argvflatten(ac, av, start)
  int ac;
  char **av;
  int start;
{
  return argvFlattenSep(ac, av, start, ac, " ");
}


void argvfree(av)
  char **av;
{
  int i;

  for(i = 0; i <= last_index(av); i++)
  {
    rfree(av[i]);
    av[i] = (char *)0;
  }
  rfree(av);
}


#define try(x, t, v, e) \
do { if ((t = (e))) { v = t; } else { error(A_ERR, curr_lang[70], curr_lang[71], x); state = ERROR; } } while (0) 

  
int argvify(str, argc, argv)
  const char *str;
  int *argc;
  char ***argv;
{
  State state = AV_OUT_STR;
  const char *p;
  char *ts;
  char *s = (char *)0;
  char **av;
  char **tav;
  int done = 0;

  ptr_check(str, const char, curr_lang[70], 0);
  ptr_check(argc, int, curr_lang[70], 0);
  ptr_check(argv, char **, curr_lang[70], 0);

  *argc = 0;
  *argv = (char **)0;
  try(curr_lang[72], tav, av, init_argv());
  p = str;
  while ( ! done)
  {
    switch (state)
    {
    case AV_OUT_STR:
      switch (*p)
      {
      case ' ':
      case '\t':
        break;

      case '\\':
        state = AV_Q_IN_STR;
        try(curr_lang[73], ts, s, new_str());
        break;

      case '"':
        state = AV_IN_QUOTE;
        try(curr_lang[73], ts, s, new_str());
        break;

      case '\0':
        state = AV_DONE;
        break;

      default:
        state = AV_IN_STR;
        try(curr_lang[73], ts, s, new_str());
        try(curr_lang[74], ts, s, insert_char(*p, s));
        break;
      }
      break;
        
    case AV_IN_STR:
      switch (*p)
      {
      case ' ':
      case '\t':
        state = AV_OUT_STR;
        try(curr_lang[75], tav, av, end_current_str(s, av));
        break;

      case '\\':
        state = AV_Q_IN_STR;
        break;

      case '"':
        state = AV_IN_QUOTE;
        break;

      case '\0':
        state = AV_DONE;
        try(curr_lang[75], tav, av, end_current_str(s, av));
        break;

      default:
        state = AV_IN_STR;
        try(curr_lang[74], ts, s, insert_char(*p, s));
        break;
      }
      break;

    case AV_IN_QUOTE:
      switch (*p)
      {
      case '\\':
        state = AV_Q_IN_QUOTE;
        break;

      case '"':
        state = AV_IN_STR;
        break;

      case '\0':
        state = AV_ERROR;
        break;

      default:
        state = AV_IN_QUOTE;
        try(curr_lang[74], ts, s, insert_char(*p, s));
        break;
      }
      break;

    case AV_Q_IN_STR:
      if ( ! *p) state = AV_ERROR;
      else
      {
        state = AV_IN_STR;
        try(curr_lang[74], ts, s, insert_char(*p, s));
      }
      break;

    case AV_Q_IN_QUOTE:
      if ( ! *p) state = AV_ERROR;
      else
      {
        state = AV_IN_QUOTE;
        try(curr_lang[74], ts, s, insert_char(*p, s));
      }
      break;

    case AV_DONE:
    case AV_ERROR:
    default:
      done = 1;
      break;
    }

    p++;
  }

  switch (state)
  {
  case AV_DONE:
    *argv = av;
    *argc = last_index(av) + 1;
    return 1;

  case AV_ERROR:
    if (s) rfree(s);
    argvfree(av);
    return 0;

  default:
    error(A_INTERR, curr_lang[70], curr_lang[76], (int)state);
    return 0;
  }
}
