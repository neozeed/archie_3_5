#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef SOLARIS
#include <regexpr.h>
#endif
#include "strings_index.h"
#include "typedef.h"
#include "ar_search.h"
#include "plog.h"

status_t ar_exact_match(s_req, index_list, strings_hash )
   search_req_t	*s_req;
   index_t	*index_list;
   file_info_t	*strings_hash;

{
   datum string_search, string_data;

   string_search.dptr = s_req -> search_str;
   string_search.dsize = strlen( s_req -> search_str) + 1;

   string_data = dbm_fetch(strings_hash -> fp_or_dbm.dbm, string_search);

   if(dbm_error(strings_hash -> fp_or_dbm.dbm))
      return(ERROR);

   if(string_data.dptr == (char *) NULL)
      return(ERROR);

   memcpy(&index_list[0], string_data.dptr, string_data.dsize);

   index_list[1] = -1;

   return(A_OK);
}

status_t ar_regex_match(search_req, index_list, strings_idx, strings)
   search_req_t	*search_req;
   index_t	*index_list;
   file_info_t	*strings_idx;
   file_info_t  *strings;
   
{
#ifndef SOLARIS
   extern	 char* re_comp PROTO((char *));
   extern	 int   re_exec PROTO(( char *));
#endif

   char		 *strings_idx_end;
   strings_idx_t *strings_idx_rec;
   int		 recno;
   int		 count;
   int		 checkcount = 0;
#ifdef SOLARIS
   char		 *re;
#endif   

#ifndef SOLARIS
   if((search_req -> error_string = re_comp(search_req -> search_str)) != (char *) NULL){
      return(ERROR);
   }
#else
   if((re = compile(search_req -> search_str, (char *) NULL, (char *) NULL)) == (char *) NULL){
      search_req -> error_string = strdup("Error in regular expression");
      goto thend;
   }
#endif

   strings_idx_rec = (strings_idx_t *) strings_idx -> ptr + search_req -> orig_offset;
   strings_idx_end = strings_idx -> ptr + strings_idx -> size;

   if(strings_idx_rec >= (strings_idx_t *) strings_idx_end){
      search_req -> error_string = strdup("Invalid offset given. Please try again");
      goto thend;
   }

   for(recno = 0, count=0;
       ((char *) strings_idx_rec < strings_idx_end) && (count != search_req -> maxmatch);
       strings_idx_rec++, recno++, checkcount++){

       if(strings_idx_rec -> strings_offset < 0){

	  plog(L_DIR_ERR, NOREQ, "Likely corrupt database. Strings index at rec %d has offset %d", recno, strings_idx_rec -> strings_offset);
	  continue;
       }
       

#ifndef SOLARIS
       switch(re_exec((strings -> ptr) + strings_idx_rec -> strings_offset)){

	  case 1: /* String matched */

	      index_list[count++] = recno;
	      break;

	  case 0:
	      break;

	  case -1:
	      search_req -> error_string = strdup("Internal error");
	      index_list[++count] = -1;
	      return(ERROR);
	      break;
       }
#else
       switch(step(((strings -> ptr) + strings_idx_rec -> strings_offset), re)){

	 case 0:
	    break;

	 default: /* String matched */
	      index_list[count++] = recno;
	      break;
       }
#endif

       if(checkcount > 1000){

	 ardp_accept();
	 checkcount = 0;
       }
   }

   /* End of List */

   index_list[count] = -1;

   /* set curr_offset to zero if we fell of the end of the file otherwise
      set it to current strings_idx record number */


   if((char *) strings_idx_rec >= strings_idx_end)
      search_req -> curr_offset = 0;
   else
      search_req -> curr_offset = ((char *) strings_idx_rec - strings_idx_end) / sizeof(strings_idx_t);


#ifdef SOLARIS
   if(re)
      free(re);
#endif      

   return(A_OK);

thend:

#ifdef SOLARIS
   if(re)
      free(re);
#endif      

   return(ERROR);

}
	      

   
