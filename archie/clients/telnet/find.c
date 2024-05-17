#ifdef __STDC__
#include <stdlib.h>
#endif
#ifdef AIX
#include <sys/select.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "alarm.h"
#include "arch_query.h"
#include "archie.h"
#include "argv.h"
/*#include "database.h"*/
#include "defines.h"
#include "error.h"
#include "extern.h"
#include "files.h" /* for tail() in library */
#include "find.h"
#include "fork_wait.h"
#include "generic_find.h"
#include "get_types.h"
#include "lang.h"
#include "macros.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "mode.h"
#include "pager.h"
#include "prosp.h"
#include "signals.h"
#include "vars.h"

#include "protos.h"


/*
 *
 *
 *                                 Internal routines.
 *
 *
 */


static int do_matches PROTO((const char *arg, FILE *ofp, int nsrchs, int prt_status));


/*  
 *  Return values:
 *  
 *  0 : no matches or failure (e.g. from Prospero)
 *  1 : success, with results
 */  

/*
 *  bug: server variable is ignored, sort type is not yet implemented.
 */
static int do_matches(arg, ofp, nsrchs, prt_status)
  const char *arg;
  FILE *ofp;
  int nsrchs;
  int prt_status;
{
  int ret = 0;
  struct arch_query *aq;

  ptr_check(arg, const char, curr_lang[110], 0);
  ptr_check(ofp, FILE, curr_lang[110], 0);

  if ( ! (aq = queryNew())) {
    error(A_ERR, "do_matches", "can't perform search for `%s'", arg);
    return 0;
  }
  
  querySetKey(aq, arg);
  querySetSearchType(aq, get_var(V_SEARCH));
  querySetSortOrder(aq, get_var(V_SORTBY));
  querySetOutputFormat(aq, get_var(V_OUTPUT_FORMAT));
  querySetMaxHits(aq, atoi(get_var(V_MAXHITS)),
                  atoi(get_var(V_MAXMATCH)), atoi(get_var(V_MAXHITSPM)));
  if (is_set(V_MATCH_DOMAIN)) {
    querySetDomainMatches(aq, get_var(V_MATCH_DOMAIN));
  }
  if (is_set(V_MATCH_PATH)) {
    querySetPathMatches(aq, get_var(V_MATCH_PATH));
  }
  
  if ( ! queryPerform(aq)) {
    error(A_ERR, "do_matches", "search failed");
  } else {
    queryPrintResults(aq, ofp);
    ret = 1;
  }

  queryFree(&aq);

  return ret;
}


/*
 *
 *                             External routines
 *
 */

int anonftp_find(ac, av, ofp)
  int ac;
  char **av;
  FILE *ofp;
{
  int ret = 0;

  ptr_check(av, char *, curr_lang[117], 0);
  ptr_check(ofp, FILE, curr_lang[117], 0);

  if (ac < 2)
  {
    printf(curr_lang[53], av[0]);
    return 0;
  }

  mode_truncate_fp(ofp);
  fprint_srch_restrictions(stdout);
  fflush(stdout);
  switch (fork_me(spin(), &ret))
  {
  case (enum forkme_e)INTERNAL_ERROR:
    return 0;

  case CHILD:
    if (ac == 2)
    {
      ret = do_matches(av[1], ofp, ac-1, spin());
    }
    else
    {
      int i;

      for (i = 1; i < ac; i++)
      {
        fprintf(ofp, curr_lang[31], av[i]);
        ret |= do_matches(av[i], ofp, ac-1, spin());
      }
    }
    fork_return(ret);
    break;

  case PARENT:
    set_alarm();
    break;

  default:
    error(A_INTERR, curr_lang[117], curr_lang[48]);
    return 0;
  }

  return ret;
}


int find_it(ac, av, ofp)
  int ac;
  char **av;
  FILE *ofp;
{
  int ret = 0;

  ptr_check(av, char *, curr_lang[117], 0);
  ptr_check(ofp, FILE, curr_lang[117], 0);

#ifdef MULTIPLE_COLLECTIONS
  if ( ! is_set(V_COLLECTIONS))
  {
#endif

    ret = anonftp_find(ac, av, ofp);

#ifdef MULTIPLE_COLLECTIONS
  }
  else
  {
    char **dbs;
    int maxhdrs = atoi(get_var(V_MAXHDRS));
    int maxdocs = atoi(get_var(V_MAXDOCS));
    int ndbs;
    
    if (argvify(get_var(V_COLLECTIONS), &ndbs, &dbs))
    {
      ret = generic_find(maxhdrs, maxdocs, ndbs, dbs, ac-1, &av[1], ofp);
      argvfree(dbs);
    }
  }
#endif

  return ret;
}
