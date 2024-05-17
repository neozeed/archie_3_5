#include <malloc.h>
#ifdef AIX
#include <stdlib.h>
#endif
#include "error.h"
#include "input.h"
#include "lang.h"
#include "macros.h"
#include "misc_ansi_defs.h"

#include "protos.h"

#define m(x) fprintf(stderr, #x curr_lang[141])

#define DFLT_LINE_LEN 81


void add_history(line)
  const char *line;
{
}


int new_hist_context()
{
  return 0;
}


char *readline(prompt)
  const char *prompt;
{
  char *curpos;
  char *end;
  char *l;
  char *line = malloc(DFLT_LINE_LEN);
  int lsize = DFLT_LINE_LEN;
  int newsize;

  if ( ! line)
  {
    error(A_SYSERR, curr_lang[142], curr_lang[143], DFLT_LINE_LEN);
    return (char *)0;
  }
  curpos = line;
  end = line + DFLT_LINE_LEN;
  fputs(prompt, stdout);
  while (1)
  {
    while (curpos < end)
    {
      switch (fread(curpos, 1, 1, stdin))
      {
      case -1:
        error(A_SYSERR, curr_lang[142], curr_lang[144]);
        free(line);
        return (char *)0;

      case 0:
        free(line);
        return (char *)0;
      
      case 1:
        if (*curpos == '\n')
        {
          *curpos = '\0';
          return line;
        }
        curpos++;
        break;

      default:
        error(A_SYSERR, curr_lang[142], curr_lang[145]);
        free(line);
        return (char *)0;
      }
    }
    /*
      If we get here then we need to get some more space.
    */
    if ( ! (l = realloc(line, (unsigned)newsize = lsize + DFLT_LINE_LEN)))
    {
      error(A_SYSERR, curr_lang[142], curr_lang[146], newsize);
      free(line);
      return (char *)0;
    }
    else
    {
      line = l;
      curpos = line + lsize;
      lsize = newsize;
      end = line + lsize;
    }
  }
  /* NOT REACHED */
}


int set_hist_context(n)
  int n;
{
  return 0;
}
