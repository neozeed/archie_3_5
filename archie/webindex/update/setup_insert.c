/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


/* Here is the assumption on the format of the input file...

   Imagine that one has the following list of urls

   /services:8000/
   /services:8000/a
   /services:8000/b
   /services:8000/c


   then we will represent it in the following dir structure

   
                                             |   rec_no       parent index
   -----------------------------------------------------------------------
                             services        |    0               -1
   services:                                 |    1                0
                             8000            |    2                0
   8000:                                     |    3                2
                             a               |    4                2
                             b               |    5                2
                             c               |    6                2


   Note that when keys do not use the parent pointer...

*/                             


#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <search.h>
#include <string.h>
#include "typedef.h"

#include "strings_index.h"

#include "core_entry.h"
#include "site_file.h"
#include "header_def.h"

#include "host_db.h"
#include "insert_webindex.h"
#include "error.h"
#include "archie_dbm.h"
#include "lang_webindex.h"
#include "debug.h"
#include "files.h"


/*
 * setup_insert.c This file contains the routines for the first pass over
 * the input data for an insertion into the webindex database.
 */

    

/*
 * count_extra_recs() Read the parser input file and construct the extra
 * number of records that will be added to reflect the parent-entries.
 */


status_t count_extra_recs(in_array, input_size, header_rec)
   char *in_array;		 /* Input parser "array" (mmaped file) */
   int input_size;
   header_t *header_rec;	 /* The input header record where no_recs and site_no_recs are found */
{

  int no_recs;

  no_recs = header_rec->no_recs;

  ptr_check(in_array, char, "count_extra_recs", ERROR);


  /* Check to see if the header_rec->header_flags includes the site_no_recs
   * field. If it does then we don't need to recount the number of extra 
   * records. Otherwise we recount and assign the new number to 
   * header_rec.header_flags.
   */

  /*
     if(HDR_GET_SITE_NO_RECS(header_rec -> header_flags))
     {
     return(A_OK);
     }
     
     * Go through the input one record at a time *
     
     for( curr_recno=0;  byte_offset < input_size && curr_recno < no_recs; curr_recno++){
     
     memcpy(&curr_irec, &in_array[byte_offset], sizeof(parser_entry_t));
     
     if (CSE_IS_DIR(curr_irec.core))
     {
     extra_recs++;
     }else if ( CSE_IS_SUBPATH(curr_irec.core) ||
     CSE_IS_PORT(curr_irec.core) ||
     CSE_IS_NAME(curr_irec.core))
     {
     extra_recs--;
     }
     
     byte_offset +=  sizeof(parser_entry_t) + curr_irec.slen;
     
     * Align it on the next 4 byte boundary. May be NON-PORTABLE*
     
     if( byte_offset & 0x3 )
     byte_offset += (4 - (byte_offset & 0x3));
     
     
     }
     */
  HDR_SET_SITE_NO_RECS(header_rec -> header_flags);
  header_rec -> site_no_recs = no_recs + 1; /*extra_recs*/
  
  return(A_OK);    
}



   

/*
 * setup_insert() Read the parser input file and construct list of records
 * to be modified
 */


status_t setup_insert(strhan,in_array,out_array, input_size, out_rec_no, site_no_recs )
   struct arch_stridx_handle *strhan;
   char *in_array;		 /* Input parser "array" (mmaped file) */
   full_site_entry_t *out_array; /* Output "array" (mmaped output file) */
   int input_size;
   int *out_rec_no;
   int site_no_recs;			 /* Number of input records */
{

  index_t *outlist;             /* Constructed outlist */
  parser_entry_t curr_irec;     /* Current input record */
  int curr_recno;               /* and its record number */
  char *input_string;           /* Current input string */

  index_t tmp_index;
  index_t prev_prnt_dir;
  int curr_prnt_dir;

  int byte_offset = 0;          /* Byte offset of current record */

  int no_recs, web;

  int  *rec_numbers;
  /* Stores the original parent indexes mapped to 
     their new locations after the addition of the parent
     entries.*/

  int extra_recs = 0;

  no_recs = site_no_recs;
  *out_rec_no = 0;

  if((outlist =  (index_t *)malloc( no_recs * sizeof(index_t))) == (index_t) 0){

    /* "Can't malloc() space for outlist" */

    error(A_SYSERR,"setup_insert", INSERT_WEBINDEX_005);
    return(ERROR);
  }


  if ((rec_numbers = (int *)malloc(sizeof(int)*(no_recs)))== (int *) NULL){

    /* "Can't malloc space for integer" */

    error(A_SYSERR, "setup_insert", "Can't malloc space for integer array\n");
    return(ERROR);
  }

  ptr_check(in_array, char, "setup_insert", ERROR);
  ptr_check(out_array, full_site_entry_t, "setup_insert", ERROR);
   
  prev_prnt_dir = (index_t) (-2);
  curr_prnt_dir = (index_t) (-1);
  web = 0;

  /* Go through the input one record at a time */

  for( curr_recno=0; byte_offset < input_size && curr_recno < no_recs; curr_recno++){

    memcpy(&curr_irec, &in_array[byte_offset], sizeof(parser_entry_t));

    if( !(CSE_IS_SUBPATH(curr_irec.core) ||
          CSE_IS_PORT(curr_irec.core)  ||
          CSE_IS_NAME(curr_irec.core)) )
    {
      if ( !web && (curr_irec.core.parent_idx != prev_prnt_dir) )
      {
        extra_recs++; 
        outlist[curr_recno] = curr_prnt_dir;

        CSE_SET_SUBPATH(out_array[curr_recno]);

        if( curr_irec.core.parent_idx > 0 ){
          out_array[curr_recno].core.prnt_entry.strt_2 = 
            rec_numbers[curr_irec.core.parent_idx] ;
          out_array[curr_recno].strt_1 = 
            (index_t)(outlist[ rec_numbers[curr_irec.core.parent_idx] ]);
        }else 
        {
          out_array[curr_recno].core.prnt_entry.strt_2 = 
            (index_t) (-1);
          out_array[curr_recno].strt_1 = (index_t)(-1);
        }
        curr_prnt_dir = curr_recno;
        prev_prnt_dir = curr_irec.core.parent_idx;
        curr_recno++; 
      }
    }else{
      curr_prnt_dir = curr_recno;
      prev_prnt_dir = curr_irec.core.parent_idx;
      web = 1;
    }

    outlist[curr_recno] = curr_prnt_dir;


    /* allocate space for filename string and copy */

    if((input_string = (char *) malloc( curr_irec.slen + 1)) == (char *) NULL){

      /* "Can't malloc space for input string" */

      error(A_SYSERR, "setup_insert", SETUP_INSERT_003);
      free(outlist);
      return(ERROR);
    }

    input_string[0] = '\0';
    strncpy( input_string, (char *) &in_array[byte_offset] + sizeof(parser_entry_t),
            curr_irec.slen);
    input_string[curr_irec.slen] = '\0';
    if( archAppendKey( strhan, input_string, &tmp_index ) ){

      out_array[curr_recno].strt_1 = tmp_index ;
      /*  error(A_INFO,"setup_insert","%d:%s at %lu.\n", curr_recno, input_string, tmp_index); */
      /*         fprintf(stderr,"%d:%s at %lu.\n", curr_recno, input_string, tmp_index); */
      free(input_string);

    } else {
      archCloseStrIdx(strhan);
      archFreeStrIdx(&strhan);
      error(A_ERR,"setup_insert","couldn't complete archAppendKey\n");
      free(outlist); free(input_string);
      return(ERROR);
    }
    /* "write out" the output record, with external pointers unresolved */

    if ( CSE_IS_FILE(curr_irec.core) ||
        CSE_IS_LINK(curr_irec.core) ||
        CSE_IS_DOC(curr_irec.core) ||
        CSE_IS_DIR(curr_irec.core) ) 
    {                           /* file or directory*/
      out_array[curr_recno].flags = (flags_t)curr_irec.core.flags;
      out_array[curr_recno].core.entry.size = curr_irec.core.size ;
      out_array[curr_recno].core.entry.date = curr_irec.core.date ;
      out_array[curr_recno].core.entry.rdate = curr_irec.core.rdate ;
      out_array[curr_recno].core.entry.perms = curr_irec.core.perms ;
      
      rec_numbers[curr_recno - extra_recs] = curr_recno;
    }else if (CSE_IS_KEY(curr_irec.core) )
    {
      out_array[curr_recno].flags = (flags_t)curr_irec.core.flags;        
      out_array[curr_recno].core.kwrd.weight = CSE_GET_WEIGHT(curr_irec.core);
      rec_numbers[curr_recno - extra_recs] = curr_recno;
    }else
    {
      out_array[curr_recno].flags = (flags_t)curr_irec.core.flags;        
      out_array[curr_recno].core.prnt_entry.strt_2 =  rec_numbers[curr_irec.core.parent_idx] ;
      out_array[curr_recno].strt_1 = 
      (index_t)(outlist[ rec_numbers[curr_irec.core.parent_idx] ]);
      rec_numbers[curr_recno - extra_recs] = curr_recno;
    }
    /* Increment the byte offset to next record */

    byte_offset +=  sizeof(parser_entry_t) + curr_irec.slen;

    /* Align it on the next 4 byte boundary. May be NON-PORTABLE*/
    if( byte_offset & 0x3 )
    byte_offset += (4 - (byte_offset & 0x3));

  }

  *out_rec_no = curr_recno;

  free(outlist);
  free(rec_numbers);
  /*
   * All done with the input, sort the outlist in increasing IP address and
   * increasing record number. This is to recognise when internal links are
   * to be made. See documentation
   */

  return(A_OK);    
}


