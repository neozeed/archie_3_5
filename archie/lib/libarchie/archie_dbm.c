/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */



#include <stdio.h>
#include "typedef.h"
#include "error.h"
#include "lang_libarchie.h"

/*
 * get_dbm_entry: Get an entry from the given dbm file.
 */


status_t get_dbm_entry( key, keysize, entry, db )
   void *key;	     /* Pointer to the key */
   int keysize;	     /* Size in bytes of above key */
   void *entry;	     /* Pointer to storage to put the result */
   file_info_t *db;  /* pointer to open dbm database */
{
   datum data;
   datum search;

   if(key == (void *) NULL){

      /* "NULL key pointer given" */

      error(A_INTERR, "get_dbm_entry", GET_DBM_ENTRY_001);
      return(ERROR);
   }

   if(db -> fp_or_dbm.dbm == (DBM *) NULL){

      /* "Invalid or unopened dbm database given %s" */

      error(A_INTERR,"get_dbm_entry", GET_DBM_ENTRY_002, db-> filename);
      return(ERROR);
   }


   data.dptr = key;
   data.dsize = keysize;

   search = dbm_fetch(db -> fp_or_dbm.dbm, data);

   if(dbm_error(db -> fp_or_dbm.dbm)){

      /* "Error while reading dbm database %s" */

      error(A_INTERR,"get_dbm_entry", GET_DBM_ENTRY_003, db -> filename);
      return(ERROR);
   }

   /* not found */

   if(search.dptr == (char *) NULL)
      return(ERROR);

   memcpy(entry, search.dptr, search.dsize);

   return(A_OK);
}


/*
 * put_dbm_entry: put the entry ("data") into the given database ("db")
 * with the given key.
 */


status_t put_dbm_entry( key, keysize, data, datasize, db, replace)
   void *key;	  /* key that entry is to be placed under */
   int keysize;	  /* Size of above key */
   void *data;	  /* pointer to data */
   int datasize;  /* Size of data */
   file_info_t *db;  /* open database file information */
   int replace;	  /* Non-zero to over-write existing entry */
{
   datum search;
   datum entry;
   int mode,ret;

   if((key == (void *) NULL) || (data == (void *) NULL)){

      /* "NULL key or data pointer given" */

      error(A_INTERR, "put_dbm_entry", PUT_DBM_ENTRY_001);
      return(ERROR);
   }

   search.dptr = key;
   search.dsize = keysize;

   entry.dptr = data;
   entry.dsize = datasize;

   if(replace == TRUE)
      mode = DBM_REPLACE;
   else
      mode = DBM_INSERT;

   if((ret =dbm_store( db -> fp_or_dbm.dbm, search, entry, mode) )!= 0){

      /* "Unable to insert data into dbm database %s" */

      error(A_ERR,"put_dbm_entry", PUT_DBM_ENTRY_002, db -> filename);
      return(ERROR);
   }

   if(dbm_error(db -> fp_or_dbm.dbm)){

      /* "Error while writing dbm database %s" */

      error(A_INTERR,"put_dbm_entry", GET_DBM_ENTRY_003, db -> filename);
      return(ERROR);
   }

   return(A_OK);
}


status_t delete_dbm_entry(key, keysize, db)
   void *key;
   int keysize;
   file_info_t *db;
{
   datum delete_key;

   ptr_check(db, file_info_t, "delete_dbm_entry", ERROR);
   ptr_check(key, void, "delete_dbm_entry", ERROR);

   delete_key.dptr = (char *) key;
   delete_key.dsize = keysize;
   

   if(dbm_delete(db -> fp_or_dbm.dbm, delete_key) != 0)
      return(ERROR);

   return(A_OK);
}
