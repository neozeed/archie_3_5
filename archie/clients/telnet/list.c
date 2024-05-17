#include <string.h>
#include <stdio.h>
#include "alarm.h"
#include "ansi_compat.h"
#include "archie.h"
#include "client_defs.h"
#include "defines.h"
#include "error.h"
#include "extern.h"
#include "fork_wait.h"
#include "lang.h"
#include "list.h"
#include "macros.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "mode.h"
#include "pager.h"
#include "prosp.h"
#include "signals.h"
#include "style_lang.h"
#include "vars.h"

#include "protos.h"

/*
 *
 *
 *                                 Internal routines.
 *
 *
 */


static int do_list PROTO((char *arg, FILE *ofp, int nsrchs, int prt_status));
static int fprint_list_item PROTO((FILE *ofp, VLINK item, int style));


/*
  verbose
    binkley.cs.mcgill.ca                       132.206.51.9      02:13 10 Jan 1993
*/

static int fprint_list_item(ofp, item, style)
  FILE *ofp;
  VLINK item;
  int style;
{
  char hostname[128];
  char tstr[64];

  ptr_check(ofp, FILE, curr_lang[108], 0);
  if ( ! item)
  {
    error(A_INTERR, curr_lang[108], curr_lang[147]);
    return 0;
  }

  aq_lhost(item, hostname, sizeof hostname);

  switch (style)
  {
  case MACHINE:
    fprintf(ofp, curr_lang[148], hostname, attr_str(item, curr_lang[101]));
    break;

  case TERSE:                   /*bug: unfinished*/
    fprintf(ofp, curr_lang[148], hostname, attr_str(item, curr_lang[103]));
    break;
    
  case URL:
  case SILENT:
  case VERBOSE:                 /*bug: unfinished*/
    prosp_strftime(tstr, sizeof tstr, curr_lang[99], attr_str(item, curr_lang[101]));
    fprintf(ofp, curr_lang[149], hostname, attr_str(item, curr_lang[103]), tstr);
    break;

  default:
    error(A_INTERR, curr_lang[150], curr_lang[109], style);
    return 0;
  }
  return 1;
}


/*  
 *  Do sorting by attribute other than site name (which the server does,
 *  anyway)?
 */  

static int do_list(arg, ofp, nsrchs, prt_status)
  char *arg;
  FILE *ofp;
  int nsrchs;
  int prt_status;
{
  char tmpstring[REG_EX_LEN];
  char srch_str[REG_EX_LEN];	/* copy search string into this for call */
  int ret = 0;
  struct aquery arq;

  srch_str[0] = '\0';
  if (*arg != '^') /* bug: kludge, kludge!!! RE tells server it's a list command */
  {
    strcpy(srch_str, curr_lang[151]);
  }
  strcat(srch_str, arg);

  sprintf(tmpstring,"(%s)", srch_str);
  strcpy(srch_str, tmpstring);
  
  aq_init(&arq);
  arq.host = get_var(V_SERVER);
  arq.string = srch_str;
  arq.query_type = AQ_SITELIST;
  arq.flags = AQ_NOSORT;

  if (archie_query(&arq, prt_status) != PSUCCESS)
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
        fprint_list_item(ofp, r, ostyle);
      }

      /* bug: should free up VLINK structs here? */
      vllfree(arq.results);
      ret = 1;
    }
  }

  return ret;
}



/*
 *
 *
 *                                  External routines.
 *
 *
 */


int list_it(ac, av, ofp)
  int ac;
  char **av;
  FILE *ofp;
{
  int ret = 0;

  ptr_check(av, char *, curr_lang[152], 0);
  ptr_check(ofp, FILE, curr_lang[152], 0);

  mode_truncate_fp(ofp);
  switch (fork_me(spin(), &ret))
  {
  case (enum forkme_e)INTERNAL_ERROR:
    return 0;

  case CHILD:
    if (ac == 1 || ac == 2)
    {
      ret = do_list(ac == 1 ? ".*" : av[1], ofp, 1, spin());
    }
    else
    {
      int i;

      for (i = 1; i < ac; i++)
      {
        fprintf(ofp, curr_lang[31], av[i]);
        ret |= do_list(av[i], ofp, ac-1, spin());
      }
    }
    fork_return(ret);
    break;

  case PARENT:
    set_alarm();
    break;

  default:
    error(A_INTERR, curr_lang[152], curr_lang[48]);
    return 0;
  }

  return ret;
}
