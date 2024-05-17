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


static int do_domains PROTO((FILE *ofp, int prt_status));
static int fprint_domains_item PROTO((FILE *ofp, VLINK item, int style));
  

/*
  verbose
    binkley.cs.mcgill.ca                       132.206.51.9      02:13 10 Jan 1993
*/

static int fprint_domains_item(ofp, item, style)
  FILE *ofp;
  VLINK item;
  int style;
{
  char hostname[128];

  ptr_check(ofp, FILE, curr_lang[108], 0);
  if ( ! item)
  {
    error(A_INTERR, curr_lang[108], curr_lang[147]);
    return 0;
  }

  switch (style)
  {
  case MACHINE:
    fprintf(ofp, curr_lang[148], hostname, attr_str(item, curr_lang[101]));
    break;

  case TERSE:                   /*bug: unfinished*/
    fprintf(ofp, "%-40s %s\n", item -> lattrib -> value.sequence -> next -> next -> token,
				     item -> lattrib -> value.sequence -> token);
    break;

  case SILENT:
  case VERBOSE:                 /*bug: unfinished*/
    fprintf(ofp, "%-15s %-20s %s\n", item -> lattrib -> value.sequence -> token,
				     item -> lattrib -> value.sequence -> next -> next -> token,
				     item -> lattrib -> value.sequence -> next -> token);


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
 *  
 *  Return 0 if no results were produced.
 */      
static int do_domains(ofp, prt_status)
  FILE *ofp;
  int prt_status;
{
  int ret = 0;
  struct aquery arq;

  aq_init(&arq);
  arq.host = get_var(V_SERVER);
  arq.query_type = AQ_DOMAINS;
  arq.flags = AQ_NOSORT;

  if (archie_query(&arq, prt_status) != PSUCCESS)
  {
    perrmesg((char *)0, 0, (char *)0);
  }
  else
  {
    VLINK r = arq.results;
    int ostyle = TERSE;

    if ( ! r)                   /*bug: check no matches => r == 0*/
    {
      puts(curr_lang[115]); /*bug? new remark?*/
    }
    else
    {
      strtoval(get_var(V_OUTPUT_FORMAT), &ostyle, style_list);

      fputs("\nDomains supported by this server:\n\n", ofp); /*FFF*/
      for (; r; r = r->next)
      {
        fprint_domains_item(ofp, r, ostyle);
      }

      ret = 1;
      /* bug: should free up VLINK structs here? */
      vllfree(arq.results);
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


/*  
 *  Return 0 if no output was produced, else non-zero.
 */  
int domains_it(ac, av, ofp)
  int ac;
  char **av;
  FILE *ofp;
{
  int ret;

  ptr_check(av, char *, curr_lang[152], 0);
  ptr_check(ofp, FILE, curr_lang[152], 0);

  mode_truncate_fp(ofp);
  switch (fork_me(spin(), &ret))
  {
  case (enum forkme_e)INTERNAL_ERROR:
    return 0;

  case CHILD:
    ret = do_domains(ofp, spin());
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
