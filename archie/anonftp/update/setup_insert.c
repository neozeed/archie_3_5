/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <search.h>
#include <string.h>
#include "typedef.h"

#include "strings_index.h"

#include "site_file.h"
#include "header_def.h"

#include "host_db.h"
#include "insert_anonftp.h"
#include "error.h"
#include "archie_dbm.h"
#include "lang_anonftp.h"
#include "debug.h"
#include "files.h"


/*
 * setup_insert.c This file contains the routines for the first pass over
 * the input data for an insertion into the anonftp database.
 */

/* extern fprintf();    */

/*
 * count_extra_recs() Read the parser input file and construct the extra
 * number of records that will be added to reflect the parent-entries.
 */


status_t count_extra_recs(in_array, input_size, header_rec)
   char *in_array;		 /* Input parser "array" (mmaped file) */
   int input_size;
   header_t *header_rec;	 /* The input header record where no_recs and site_no_recs are found */
{

  parser_entry_t curr_irec;     /* Current input record */
  int curr_recno;               /* and its record number */


  int no_recs;
  int extra_recs = 3;      /* this counts for the top 3 extra entries
                            * inserted in setup_insert that should account
                            * for the hostname and the top / directory
                            */
  int byte_offset = 0;          /* Byte offset of current record */

  no_recs = header_rec->no_recs;

  ptr_check(in_array, char, "count_extra_recs", ERROR);


  /* Check to see if the header_rec->header_flags includes the site_no_recs
   * field. If it does then we don't need to recount the number of extra 
   * records. Otherwise we recount and assign the new number to 
   * header_rec.header_flags.
   */

  if(HDR_GET_SITE_NO_RECS(header_rec -> header_flags))
  {
    return(A_OK);
  }

  /* Go through the input one record at a time */


  for( curr_recno=0; byte_offset < input_size && curr_recno < no_recs; curr_recno++){

    memcpy(&curr_irec, &in_array[byte_offset], sizeof(parser_entry_t));

    if (CSE_IS_DIR(curr_irec.core))
    {
      extra_recs++;
    }
    
    byte_offset +=  sizeof(parser_entry_t) + curr_irec.slen;

    /* Align it on the next 4 byte boundary. May be NON-PORTABLE*/

    if( byte_offset & 0x3 )
    byte_offset += (4 - (byte_offset & 0x3));
  }

  HDR_SET_SITE_NO_RECS(header_rec -> header_flags);
  header_rec -> site_no_recs = no_recs + extra_recs;
  
  return(A_OK);    
}



   

/*
 * setup_insert() Read the parser input file and construct list of records
 * to be modified
 */


status_t setup_insert(strhan,in_array, input_size, out_array, out_rec_no, header_rec )
   struct arch_stridx_handle *strhan;
   char *in_array;		 /* Input parser "array" (mmaped file) */
   int input_size;
   full_site_entry_t *out_array; /* Output "array" (mmaped output file) */
   int *out_rec_no;
   header_t header_rec;
/*   int no_records;			 * Number of input records */
{

  index_t *outlist;             /* Constructed outlist */
  parser_entry_t curr_irec;     /* Current input record */
  int curr_recno;               /* and its record number */
  char input_string[MAX_PATH_LEN];           /* Current input string */

  index_t tmp_index;
  index_t prev_prnt_dir;
  int curr_prnt_dir;

  int byte_offset = 0;          /* Byte offset of current record */
  
  int no_recs, web;

  int  *rec_numbers;
  /* Stores the original parent indexes mapped to 
     their new locations after the addition of the parent
     entries.*/

  int l, extra_recs = 0;

  no_recs = header_rec.site_no_recs;
  *out_rec_no = 0;
  if((outlist =  (index_t *)malloc( no_recs * sizeof(index_t))) == (index_t) 0){

    /* "Can't malloc() space for outlist" */

    error(A_SYSERR,"setup_insert", INSERT_ANONFTP_005);
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

  for( curr_recno=0; byte_offset < input_size && (curr_recno) < no_recs; curr_recno++){

    memcpy(&curr_irec, &in_array[byte_offset], sizeof(parser_entry_t));

    if( curr_irec.slen ){
      if( !(CSE_IS_SUBPATH(curr_irec.core) ||
            CSE_IS_PORT(curr_irec.core)  ||
            CSE_IS_NAME(curr_irec.core)) )
      {
        if ( !web && (curr_irec.core.parent_idx != prev_prnt_dir) )
        {
          extra_recs++;
          outlist[curr_recno] = curr_prnt_dir;

          CSE_SET_SUBPATH(out_array[curr_recno]);
          if( curr_irec.core.parent_idx >= 0 ) {
            out_array[curr_recno].core.prnt_entry.strt_2 = 
               rec_numbers[curr_irec.core.parent_idx] ;
            out_array[curr_recno].strt_1 = 
               (index_t)(outlist[ rec_numbers[curr_irec.core.parent_idx] ]);
            curr_prnt_dir = curr_recno;
            prev_prnt_dir = curr_irec.core.parent_idx;
            curr_recno++;
       
          }else
          {
            out_array[curr_recno].core.prnt_entry.strt_2 = 
               (index_t) (-1);
            out_array[curr_recno].strt_1 = (index_t)(-1);
            curr_prnt_dir = curr_recno;
            prev_prnt_dir = curr_irec.core.parent_idx;
            curr_recno++;

            /* This was written to insert the name of the site at the top
             * of the site file like it is done in webindex to relieve the
             * other programs from using other resources to find out the name
             * of the site. I am not sure if the source of my hostname is right.
             * I must check that.
             * 
             */
            if( HDR_GET_PREFERRED_HOSTNAME(header_rec.header_flags) ){
            
              l = strlen(header_rec.preferred_hostname);
/*              if((input_string = (char *) malloc( l + 1)) == (char *) NULL){
                / "Can't malloc space for input string" /
                error(A_SYSERR, "setup_insert", SETUP_INSERT_003);
                free(outlist);
                return(ERROR);
              }
*/
              input_string[0] = '\0';
              strncpy( input_string, header_rec.preferred_hostname,l);
              input_string[l] = '\0';
            
            }else if( HDR_GET_PRIMARY_HOSTNAME(header_rec.header_flags) ){
            
              l = strlen(header_rec.primary_hostname);
/*              if((input_string = (char *) malloc( l + 1)) == (char *) NULL){
                / "Can't malloc space for input string" /
                error(A_SYSERR, "setup_insert", SETUP_INSERT_003);
                free(outlist);
                return(ERROR);
              }
*/              
              input_string[0] = '\0';
              strncpy( input_string, header_rec.primary_hostname,l);
              input_string[l] = '\0';
            }
                
            if( (curr_recno==1) && (input_string[0]!='\0')){

              /* Inserting the entry representing the name as a list of the top
               * subpath "/"
               */
              extra_recs++;
              outlist[curr_recno] = 0;
              CSE_SET_DIR(out_array[curr_recno]);
              if( archAppendKey( strhan, input_string, &tmp_index ) ){
                out_array[curr_recno].strt_1 = tmp_index ;
/*                fprintf(stderr,"%d:%s at %lu.\n",curr_recno, input_string, tmp_index); */
/*                free(input_string);*/
              }else{
/*                archCloseStrIdx(&strhan);  */
                error(A_ERR,"setup_insert","couldn't complete archAppendKey\n");
                free(outlist);
/*                free(input_string);*/
                return(ERROR);
              }
              curr_recno++;

              /* Inserting the name as a parent entry with its parent pointers
               * defined.
               */

              extra_recs++;
              outlist[curr_recno] = 0;
              CSE_SET_NAME(out_array[curr_recno]);
              out_array[curr_recno].core.prnt_entry.strt_2 = (index_t) (curr_recno-1);
              out_array[curr_recno].strt_1 = (index_t)(curr_prnt_dir);
              prev_prnt_dir = curr_irec.core.parent_idx ;
              curr_prnt_dir = curr_recno;
              curr_recno++;

              /*            rec_numbers[0] = 1; */
            }
          }
        }
      }else{
        curr_prnt_dir = curr_recno;
        prev_prnt_dir = curr_irec.core.parent_idx;
        web = 1;
      }

      outlist[curr_recno] = curr_prnt_dir;

      input_string[0] = '\0';

      strncpy( input_string, (char *) &in_array[byte_offset] + sizeof(parser_entry_t),
              curr_irec.slen);

      input_string[curr_irec.slen] = '\0';
      if( archAppendKey( strhan, input_string, &tmp_index ) ){
        out_array[curr_recno].strt_1 = tmp_index ;
/*        fprintf(stderr,"%d:%s at %lu.\n",curr_recno, input_string, tmp_index); */
/*        free(input_string);*/
      } else {
/*        archCloseStrIdx(&strhan); */
        error(A_ERR,"setup_insert","couldn't complete archAppendKey\n");
        free(outlist);
/*        free(input_string);*/
        return(ERROR);
      }

      /* "write out" the output record, with external pointers unresolved */

      if ((CSE_IS_FILE(curr_irec.core) )||
          CSE_IS_LINK(curr_irec.core) ||
          CSE_IS_DOC(curr_irec.core) ||
          CSE_IS_DIR(curr_irec.core) ) 
      {                         /* file or directory*/
        out_array[curr_recno].flags = (flags_t)curr_irec.core.flags;
        out_array[curr_recno].core.entry.size = curr_irec.core.size ;
        out_array[curr_recno].core.entry.date = curr_irec.core.date ;
        out_array[curr_recno].core.entry.perms = curr_irec.core.perms ;
        rec_numbers[curr_recno - extra_recs] = curr_recno;
      }else if (CSE_IS_KEY(curr_irec.core) )
      {
        out_array[curr_recno].core.kwrd.weight = CSE_GET_WEIGHT(curr_irec.core);
      }
      /* Increment the byte offset to next record */

      byte_offset +=  sizeof(parser_entry_t) + curr_irec.slen;

      /* Align it on the next 4 byte boundary. May be NON-PORTABLE*/

      if( byte_offset & 0x3 )
      byte_offset += (4 - (byte_offset & 0x3));

    }
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


