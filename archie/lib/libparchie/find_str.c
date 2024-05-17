
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include "debug.h"
#include "misc.h"
#include "db_ops.h"
#include "strings_index.h"
#include "ar_search.h"
#include "srch.h"
#include "error.h"


extern char *prog ;

extern index_t bsrch PROTO((file_info_t *str_idx, index_t idx, /* debugging */ int i)) ;
extern int gen_list_heads PROTO((index_t *match_list, int hits, file_info_t *str_idx)) ;



/*
  Search the strings file for strings matching the pattern given by 'sreq'.  Return, in 
  'match_list', a list of indices into the string index file of those entries pointing to
  the matching strings.

  sreq       -- search request: pointer to information regarding the search
  match_list -- result of search: array of indices into string index file
  str_idx    -- pointer to information about the string index file
  strs       -- pointer to information about the strings file
*/

status_t
ar_sub_match(sreq, match_list, str_idx, strs)
  search_req_t *sreq ;
  index_t *match_list ;
  file_info_t *str_idx ;
  file_info_t *strs ;
{
  extern int getpagesize();

  int hits ;     /* the number of matches found */
  size_t dblen ;
  unsigned char *text ;

  ptr_check(sreq, search_req_t, "ar_sub_match", ERROR) ;
  ptr_check(match_list, index_t, "ar_sub_match", ERROR) ;
  ptr_check(str_idx, file_info_t, "ar_sub_match", ERROR) ;
  ptr_check(strs, file_info_t, "ar_sub_match", ERROR) ;

  if(*sreq->search_str == '\0')
  {
    sreq -> error_string = strdup("empty search string");
    return ERROR ;
  }

  /* BUG: check for 0 max hits */

  if( ! prep(sreq->search_str, strlen(sreq->search_str),
             ! (sreq->curr_type == S_SUB_CASE_STR || sreq -> curr_type == S_SUB_KASE)))
  {
    sreq -> error_string = strdup("error pre-processing search pattern") ;
    return ERROR ;
  }
  text = (unsigned char *)strs->ptr ;
  dblen = strs->size - INIT_STRING_SIZE ;

  if (sreq->curr_type == S_SUB_CASE_STR || sreq -> curr_type == S_SUB_KASE)
  {
    hits = cs_exec(text, dblen, match_list, sreq-> maxmatch);
  }
  else
  {
    hits = ci_exec(text, dblen, match_list, sreq->maxmatch);
  }

  if(hits < 0)
  {
    sreq -> error_string = strdup ("error searching for pattern") ;
    return ERROR ;
  }

  /*
    Rewrite 'match_list' to be indices into the strings index file, corresponding
    to the strings matched.
  */

  if( ! gen_list_heads(match_list, hits, str_idx))
  {
    if(!sreq -> error_string){
       sreq -> error_string = strdup("Internal error while post processing search");
       return ERROR ;
    }
  }

  /* BUG: fill in bits of sreq? */
  return A_OK ;
}


/*
  'match_list' is a list of offsets into the strings file of matching strings.
  Convert it to a list of offsets of the entries in the strings index
  file that point to the heads of the matching strings.  Got it?
*/

int
gen_list_heads(match_list, hits, str_idx)
  index_t *match_list ;
  int hits ;
  file_info_t *str_idx ;
{
  int i ;

  ptr_check(match_list, index_t, "gen_list_heads", 0) ;
  ptr_check(str_idx, file_info_t, "gen_list_heads", 0) ;

  for(i = 0 ; i < hits ; i++)
  {
    match_list[i] = bsrch(str_idx, match_list[i], i+1) ;
  }
  match_list[hits] = (index_t)-1 ; /* BUG: does this element exist?  Is this needed? */
  return 1 ;
}


/*
  We assume that on the first call to this routine (for a particular
  query) the value of str_idx->offset is 0.

  Return the offset, in records, of the entry in the string index file
  corresponding to the string at offset 'idx' in the strings file.
*/

#define MAX_DB_STR_LEN 1024 /* little kludge */

index_t
bsrch(str_idx, idx, i)
  file_info_t *str_idx ;
  index_t idx ;
  int i ;
{
#define base ((strings_idx_t *)str_idx->ptr)

  index_t offset ;
  strings_idx_t *first ;
  strings_idx_t *last ;
  strings_idx_t *mid ;

  ptr_check(str_idx, file_info_t, "bsrch", (index_t)-1) ;

  first = base ;
  last = base + str_idx->size / sizeof(strings_idx_t) - 1 ;

  while(first < last)
  {
    mid = first + (last - first)/2 ;
    if(mid->strings_offset > idx) last = mid - 1 ;
    else if(mid->strings_offset < idx) first = mid + 1 ;
    else
    {
      offset = mid - base ;
      return offset ;
    }
  }
  offset = (first->strings_offset <= idx ? first - base : first - base - 1) ;
  return offset ;

#undef base
}
