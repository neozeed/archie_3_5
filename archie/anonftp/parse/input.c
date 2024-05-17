#include <stdio.h>
#include "defines.h"
#include "utils.h"
#include "input.h"
#include "error.h"
#include "lang_parsers.h"

static int linenum = 0 ;    /* number of input lines currently read */
static int skipline = 0 ;   /* 1 -> return without reading a line */

int
init_line_num()
{
linenum = 0;
}

int
get_line(l, len, fp)
  char *l ;
  int len ;
  FILE *fp ;
{
  ptr_check(l, char, "get_line", 0) ;
  ptr_check(fp, FILE, "get_line", 0) ;

  if(skipline)
  {
    skipline = 0 ;
    return 1 ;
  }
  else
  {
    if(fgets(l, len, fp) == (char *)0)
    {
      if(feof(fp))
      {
        return 0 ;
      }
      else
      {

	/* "Error from fgets()") */

	error(A_ERR, "get_line", INPUT_001);
        return 0 ;
      }
    }
    else
    {
      nuke_nl(l) ;
      linenum++ ;
      return 1 ;
    }
  }
  return 1 ;                    /* Not reached, but keeps gcc -Wall happy */
}


int
line_num()
{
  return linenum ;
}


void
skip_line()
{
  skipline = 1 ;
}
