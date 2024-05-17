#ifdef __STDC__
#include <stdlib.h>
#endif
#include <stdio.h>
#include <string.h>
#include "alarm.h"
#include "argv.h"
#include "error.h"
#include "extern.h"
#include "fork_wait.h"
#include "generic_find.h"
#include "lang.h"
#include "macros.h"
#include "misc_ansi_defs.h"
#include "mode.h"
#include "pager.h"
#include "prosp.h"
#include "style_lang.h"
#include "vars.h"

#include "protos.h"

/*  
 *
 *
 *                             Internal routines
 *
 *
 */  

static int find PROTO((int maxhdrs, int maxdocs, int ndbs, char **dbs, int nsrchs,
                       char **srchs, FILE *ofp, int prt_status));
static int fprint_item PROTO((FILE *ofp, VLINK item, int ostyle));


static int fprint_item(ofp, item, ostyle)
  FILE *ofp;
  VLINK item;
  int ostyle;
{
  PATTRIB attrib;

  for(attrib = item->lattrib; attrib; attrib = attrib->next)
  {
    if (strncmp(attrib->aname, "WAIS", 4) != 0)
    {
      char attr[128];

      sprintf(attr, "%c%s: ", toupper(attrib->aname[0]), attrib->aname + 1);
      fmtprintf(ofp, attr, attrib->value.sequence->token, 80);
    }
  }

  return 1;
}


static int find(maxhdrs, maxdocs, ndbs, dbs,
                nsrchs, srchs, ofp, prt_status)
  int maxhdrs;
  int maxdocs;
  int ndbs;
  char **dbs;
  int nsrchs;
  char **srchs;
  FILE *ofp;
  int prt_status;
{
  int ret = 0;
  struct aquery arq;

  aq_init(&arq);

  if ( ! (arq.dbs = argvFlattenSep(ndbs, dbs, 0, ndbs, ";")))
  {
    return 0;
  }
  if ( ! (arq.string = argvFlattenSep(nsrchs, srchs, 0, nsrchs, ";")))
  {
    free(arq.dbs);
    return 0;
  }

  arq.flags = AQ_NOSORT;
  arq.host = get_var(V_SERVER);
  arq.maxhits = atoi(get_var(V_MAXHITS));
  arq.maxmatch = arq.maxhitpm = arq.maxhits; /* bug? */
  arq.offset = 0;
  arq.query_type = AQ_GENERIC;
  
  if (archie_query(&arq, prt_status) > 0)
  {
    perrmesg((char *)0, 0, (char *)0);    /*bug: check*/
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
      for (; r; r = r->next)
      {
        fputs("\n", ofp);
        fprint_item(ofp, r, ostyle);
        fputs("\n", ofp);
      }

      ret = 1;
      /* bug: should free up VLINK structs here? */
      vllfree(arq.results);
    }
  }

  free(arq.dbs);
  free(arq.string);
  return ret;
}


/*  
 *
 *
 *                             External routines
 *
 *
 */  


/*  
 *  Return values:
 *  
 *  0 - no matches or error (in arguments, from Prospero, etc.)
 *  
 *  1 - success
 */    
int generic_find(maxhdrs, maxdocs, ndbs, dbs, nsrchs, srchs, ofp)
  int maxhdrs;
  int maxdocs;
  int ndbs;
  char **dbs;
  int nsrchs;
  char **srchs;
  FILE *ofp;
{
  int ret;

  ptr_check(dbs, char *, "generic_find", 0);
  ptr_check(srchs, char *, "generic_find", 0);
  ptr_check(ofp, FILE, "generic_find", 0);

  mode_truncate_fp(ofp);
  switch (fork_me(spin(), &ret))
  {
  case (enum forkme_e)INTERNAL_ERROR:
    return 0;

  case CHILD:
    /*bug: should check for no matches _at_all_*/
    ret = find(maxhdrs, maxdocs, ndbs, dbs, nsrchs, srchs, ofp, spin());
    fork_return(ret);
    break;

  case PARENT:
    set_alarm();
    break;

  default:
    error(A_INTERR, "generic_find", curr_lang[48]);
    return 0;
  }

  return ret;
}
