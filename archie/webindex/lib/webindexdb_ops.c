/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include <string.h>
#include "typedef.h"
#include "db_ops.h"
#include "webindexdb_ops.h"
#include "db_files.h"
#include "master.h"
#include "error.h"
#include "lang_libwebindex.h"
#include "archie_dbm.h"
#include "files.h"

#include "protos.h"

/* Set up the directory name containing the files database */


char *set_wfiles_db_dir(wfiles_db_dir)
   char *wfiles_db_dir;

{
   static pathname_t wfiles_database_dir;
   static pathname_t data_dir;
   struct stat statbuf;

   if(wfiles_db_dir == (char *) NULL){

      strcpy(data_dir, wfiles_database_dir);
      return(data_dir);
   }

  if(wfiles_db_dir[0] != '\0')
     strcpy(wfiles_database_dir, wfiles_db_dir);
  else
     sprintf(wfiles_database_dir, "%s/%s%s", get_master_db_dir(), DEFAULT_WFILES_DB_DIR, DB_SUFFIX);


  if(stat(wfiles_database_dir, &statbuf) == -1){

     error(A_SYSERR,"set_wfiles_db_dir","Can't access dirctory %s", wfiles_database_dir);
     return((char *) NULL);
  }

  if(!S_ISDIR(statbuf.st_mode)){
     error(A_ERR,"set_host_db_dir","%s is not a directory", wfiles_database_dir);
     return((char *) NULL);
  }

  strcpy(data_dir, wfiles_database_dir);
  return(wfiles_database_dir);
}


char *get_wfiles_db_dir()

{
   return(set_wfiles_db_dir((char *) NULL));
}



char *wfiles_db_filename(filename, port)
  char *filename;
  int port;
{
  static pathname_t tmp_db_filename;

  if ( port > 0 )
    sprintf(tmp_db_filename,"%s/%s:%d",get_wfiles_db_dir(),filename,port);
  else
    sprintf(tmp_db_filename,"%s/%s",get_wfiles_db_dir(),filename);  

  return((char *) tmp_db_filename);
}


/*
 * Open all files database files
 */



status_t open_wfiles_db( host_hash_finfo, host_finfo, sel_finfo, sel_hash_finfo, mode)
   file_info_t *host_hash_finfo;
   file_info_t *host_finfo;
   file_info_t *sel_finfo;
   file_info_t *sel_hash_finfo;
   int mode;

{
   int tmp_fd;
   int testmode = R_OK | F_OK;

   if(host_hash_finfo != (file_info_t *) NULL){

      strcpy(host_hash_finfo -> filename, wfiles_db_filename( DEFAULT_HOST_HASH, (int)(0) ));

      host_hash_finfo -> fp_or_dbm.dbm =  dbm_open(host_hash_finfo -> filename,
					     (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT),
					     DEFAULT_FILE_PERMS);

      if((host_hash_finfo -> fp_or_dbm.dbm == (DBM *) NULL) || dbm_error(host_hash_finfo -> fp_or_dbm.dbm)){

         error(A_INTERR,"open_wfiles_db","error opening hashed host database %s", DEFAULT_HOST_HASH);
         return(ERROR);
      }
   }


   if(host_finfo != (file_info_t *) NULL){
      strcpy(host_finfo -> filename, wfiles_db_filename( DEFAULT_HOST_LIST,(int)(0) ));

      if(access(host_finfo -> filename, mode == O_RDONLY ? testmode : testmode | W_OK ) == -1){
         if(mode == O_RDONLY){
	    error(A_SYSERR, "open_wfiles_db","can't access %s", host_finfo -> filename);
	    return(ERROR);
	 }
      }
  
      if((tmp_fd = open(host_finfo -> filename, (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT), DEFAULT_FILE_PERMS)) == -1){

	       error(A_SYSERR, "open_wfiles_db","can't access %s", host_finfo -> filename);
	       return(ERROR);
      }

      if((host_finfo -> fp_or_dbm.fp = fdopen( tmp_fd, (mode == O_RDONLY ? "r" : "r+"))) == (FILE *) NULL){

         error(A_ERR,"open_wfiles_db","opening %s file", host_finfo -> filename);
         return(ERROR);
      }
   }


   if(sel_finfo != (file_info_t *) NULL){
      strcpy(sel_finfo -> filename, wfiles_db_filename( DEFAULT_SEL, (int)(0) ));

      if(access(sel_finfo -> filename, mode == O_RDONLY ? testmode : testmode | W_OK ) == -1){
         if(mode == O_RDONLY){
	    error(A_SYSERR, "open_wfiles_db","can't access %s", sel_finfo -> filename);
	    return(ERROR);
	 }
      }
  
      if((tmp_fd = open(sel_finfo -> filename, (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT), DEFAULT_FILE_PERMS)) == -1){

	       error(A_SYSERR, "open_wfiles_db","can't access %s", sel_finfo -> filename);
	       return(ERROR);
      }

      if((sel_finfo -> fp_or_dbm.fp = fdopen( tmp_fd, (mode == O_RDONLY ? "r" : "r+"))) == (FILE *) NULL){

         error(A_ERR,"open_wfiles_db","opening %s file", sel_finfo -> filename);
         return(ERROR);
      }
   }

   if(sel_hash_finfo != (file_info_t *) NULL){

      strcpy(sel_hash_finfo -> filename, wfiles_db_filename( DEFAULT_SEL_HASH, (int)(0) ));

      sel_hash_finfo -> fp_or_dbm.dbm =  dbm_open(sel_hash_finfo -> filename,
					     (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT),
					     DEFAULT_FILE_PERMS);

      if((sel_hash_finfo -> fp_or_dbm.dbm == (DBM *) NULL) || dbm_error(sel_hash_finfo -> fp_or_dbm.dbm)){

         error(A_INTERR,"open_wfiles_db","error opening hashed host database %s", DEFAULT_HOST_HASH);
         return(ERROR);
      }
   }


   return(A_OK);
}


status_t close_wfiles_db( host_hash_finfo, host_finfo, sel_finfo, sel_hash_finfo)
   file_info_t *host_hash_finfo;
   file_info_t *host_finfo;
   file_info_t *sel_finfo;
   file_info_t *sel_hash_finfo;
{

   if(host_finfo != (file_info_t *) NULL)
      close_file(host_finfo);

   if(sel_finfo != (file_info_t *) NULL)
      close_file(sel_finfo);

   if(host_hash_finfo != (file_info_t *) NULL)
      dbm_close(host_hash_finfo -> fp_or_dbm.dbm);

   if(sel_hash_finfo != (file_info_t *) NULL)
      dbm_close(sel_hash_finfo -> fp_or_dbm.dbm);

   return(A_OK);
}

