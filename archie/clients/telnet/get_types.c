#include <stdio.h>
#include <string.h>
#include "ar_search.h"
/*#include "database.h"*/
#include "get_types.h"
#include "lang.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "vars.h"


#include "get_types_lang.h" /* protect from translation */


/*  
 *  Check the value of the "sortby" variable and return the appropriate
 *  enumerated type value.  If, for any reason, we get an error we default to
 *  not sorting.
 */  

int get_sort_type()
{
  int val;
  return strtoval(get_var(V_SORTBY), &val, sort_types) ? val : SORT_DBORDER;
}


/*  
 *  Same thing, but for the search type.
 *  
 *  Note: bug: the new default, if things fail, is a subcase search.
 */  

char get_search_type()
{
  int val;
  return strtoval(get_var(V_SEARCH), &val, search_types) ? val : S_SUB_NCASE_STR;
}
