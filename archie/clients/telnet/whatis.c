#include <stdio.h>
#include <sys/types.h>
#ifdef MMAP
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include "alarm.h"
#include "ansi_compat.h"
#include "client_defs.h"
#include "client_structs.h"
/*#include "database.h"*/
#include "defines.h"
#include "error.h"
#include "extern.h"
#include "fork_wait.h"
#include "lang.h"
#include "macros.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "mode.h"
#include "pager.h"
#include "prosp.h"
#include "style_lang.h"
#include "terminal.h"
#include "vars.h"
#include "whatis.h"

#include "protos.h"

static int get_whatis_list PROTO((FILE *ofp, const char *srch_str, int nsrchs, int scr_width));


/*  
 *  Search through the "whatis" database and find the requested substring
 */     

static int get_whatis_list(ofp, srch_str, nsrchs, scr_width)
  FILE *ofp;
  const char *srch_str;
  int nsrchs;
  int scr_width;
{
#ifdef P5_WHATIS
  SpinStart spin;
  int email = current_mode() == M_EMAIL;
  int ret = 0;
  struct aquery arq;

  aq_init(&arq);
  arq.host = get_var(V_SERVER);
  arq.string = srch_str;
  arq.query_type = AQ_WHATIS;
  arq.flags = AQ_NOSORT;

  /*bug: we should fork_me() for this */
  if (archie_query(&arq, spin()) != PSUCCESS)
  {
    perrmesg((char *)0, 0, (char *)0);
  }
  else
  {
    VLINK r = arq.results;
    int ostyle = VERBOSE;

    if ( ! r)                   /*bug: check no matches => r == 0*/
    {
      fputs(curr_lang[115], nsrchs == 1 ? stdout : ofp);
    }
    else
    {
      strtoval(get_var(V_OUTPUT_FORMAT), &ostyle, style_list);

      fputs(curr_lang[28], ofp);
      for (; r; r = r->next)
      {
        fprint_whatis_item(ofp, r, ostyle);
      }

      ret = 1;
      /* bug: should free up VLINK structs here? */
      vllfree(arq.results);
    }
  }

  return ret;

#else

  FILE *what_fp;
  char des_line[MAX_STRING_LEN];
  int found = 0;

  ptr_check(ofp, FILE, curr_lang[311], 0);
  ptr_check(srch_str, const char, curr_lang[311], 0);

  if ( ! (what_fp = fopen(get_var(V_WHATIS_FILE), curr_lang[44])))
  {
    error(A_SYSERR, curr_lang[311], curr_lang[312],
          get_var(V_WHATIS_FILE));
    return 0;
  }

  while (fgets(des_line, sizeof(des_line), what_fp))
  {
    initskip(srch_str, (int)strlen(srch_str), 1);
    if (strfind(des_line, (int)strlen(des_line)))
    {
      char *end;
      char *start;
      char *ws;

      found = 1;

      /*  
       *  Knock off any leading or trailing white space.
       */  
      nuke_newline(des_line);
      bracketstr(des_line, (const char **)&start, (const char **)&end); /* sigh */
      *(end + 1) = '\0';

      ws = strpbrk(start, "\t");
      if ( ! ws)
      {
        fputs(start, ofp); fputs(curr_lang[28], ofp);
      }
      else
      {
        char c = *ws;
        char *nw; /* next word */

        nw = ws + strspn(ws, WHITE_SPACE);
        *ws = '\0';
        if (nw != ws) /* there was more after the white space */
        {
          char pfix[INPUT_LINE_LEN];
          
          sprintf(pfix, "%-25s ", start);
          fmtprintf(ofp, pfix, nw, get_cols());
        }
        else
        {
          fputs(start, ofp); fputs(curr_lang[28], ofp);
        }
        *ws = c;
      }
    }
  }

  if ( ! found)
  {
    printf(curr_lang[314], srch_str);
  }

  return found;
#endif
}


int whatis_it(ac, av, ofp)
  int ac;
  char **av;
  FILE *ofp;
{
  int ret = 0;

  ptr_check(av, char *, curr_lang[49], 0);
  ptr_check(ofp, FILE, curr_lang[49], 0);

  if (ac != 2)
  {
    printf(curr_lang[46], av[0]);
    return 0;
  }

  mode_truncate_fp(ofp);
  /*bug: this should take multiple strings*/
#ifdef P5_WHATIS
  ret = get_whatis_list(ofp, av[1], 1, get_cols());
#else  
  switch (fork_me(0, &ret))
  {
  case (enum forkme_e)INTERNAL_ERROR:
    break;

  case CHILD:
    ret = get_whatis_list(ofp, av[1], 1, get_cols());
    fork_return(ret);
    break;

  case PARENT:
    set_alarm();
    break;

  default:
    error(A_INTERR, curr_lang[49], curr_lang[48]);
    break;
  }
#endif

  return ret;
}
