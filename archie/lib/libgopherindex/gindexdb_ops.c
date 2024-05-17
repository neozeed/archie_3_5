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
#include "gindexdb_ops.h"
#include "db_files.h"
#include "master.h"
#include "error.h"
#include "lang_libgopherindex.h"
#include "archie_dbm.h"
#include "files.h"

#include "protos.h"

/* Set up the directory name containing the files database */


char *set_gfiles_db_dir(gfiles_db_dir)
   char *gfiles_db_dir;

{
   static pathname_t gfiles_database_dir;
   static pathname_t data_dir;
   struct stat statbuf;

   if(gfiles_db_dir == (char *) NULL){

      strcpy(data_dir, gfiles_database_dir);
      return(data_dir);
   }

  if(gfiles_db_dir[0] != '\0')
     strcpy(gfiles_database_dir, gfiles_db_dir);
  else
     sprintf(gfiles_database_dir, "%s/%s%s", get_master_db_dir(), DEFAULT_GFILES_DB_DIR,DB_SUFFIX);


  if(stat(gfiles_database_dir, &statbuf) == -1){

     error(A_SYSERR,"set_gfiles_db_dir","Can't access dirctory %s", gfiles_database_dir);
     return((char *) NULL);
  }

  if(!S_ISDIR(statbuf.st_mode)){
     error(A_ERR,"set_host_db_dir","%s is not a directory", gfiles_database_dir);
     return((char *) NULL);
  }

  strcpy(data_dir, gfiles_database_dir);
  return(gfiles_database_dir);
}


char *get_gfiles_db_dir()
{
   return(set_gfiles_db_dir((char *) NULL));
}



char *gfiles_db_filename(filename)
   char *filename;

{
   static pathname_t tmp_db_filename;

   sprintf(tmp_db_filename,"%s/%s",get_gfiles_db_dir(),filename);

   return((char *) tmp_db_filename);
}


/*
 * Open all files database files
 */



status_t open_gfiles_db(strings_idx_finfo, strings_finfo, strings_hash_finfo, host_hash_finfo, host_finfo, sel_finfo, sel_hash_finfo, mode)
   file_info_t *strings_idx_finfo;
   file_info_t *strings_finfo;
   file_info_t *strings_hash_finfo;
   file_info_t *host_hash_finfo;
   file_info_t *host_finfo;
   file_info_t *sel_finfo;
   file_info_t *sel_hash_finfo;
   int mode;

{
   extern ftruncate PROTO((int, off_t));

   int tmp_fd;
   int testmode = R_OK | F_OK;
   

   if(strings_idx_finfo != (file_info_t *) NULL){
      strcpy(strings_idx_finfo -> filename, gfiles_db_filename( DEFAULT_GSTRINGS_IDX ));

      if(access(strings_idx_finfo -> filename, mode == O_RDONLY ? testmode : testmode | W_OK ) == -1){
         if(mode == O_RDONLY){
	    error(A_SYSERR, "open_gfiles_db","can't access %s", strings_idx_finfo -> filename);
	    return(ERROR);
	 }
      }
  
      if((tmp_fd = open(strings_idx_finfo -> filename, (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT), DEFAULT_FILE_PERMS)) == -1){

	       error(A_SYSERR, "open_gfiles_db","can't access %s", strings_idx_finfo -> filename);
	       return(ERROR);
      }

      if((strings_idx_finfo -> fp_or_dbm.fp = fdopen( tmp_fd, (mode == O_RDONLY ? "r" : "r+"))) == (FILE *) NULL){

         error(A_ERR,"open_gfiles_db","opening %s file", strings_idx_finfo -> filename);
         return(ERROR);
      }
   }


   if(strings_finfo != (file_info_t *) NULL){
      int new_file = 0;

      strcpy(strings_finfo -> filename, gfiles_db_filename( DEFAULT_GSTRINGS_LIST ));


      if(access(strings_finfo -> filename,mode == O_RDONLY ? testmode : testmode | W_OK) == -1){
         if(mode == O_RDONLY){
	    error(A_SYSERR, "open_gfiles_db","can't access %s", strings_finfo -> filename);
	    return(ERROR);
	 }

	 else{
	    new_file = TRUE;
	 }

      }
  
      if((tmp_fd = open(strings_finfo -> filename, (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT), DEFAULT_FILE_PERMS)) == -1){

	       error(A_SYSERR, "open_gfiles_db","can't access %s", strings_finfo -> filename);
	       return(ERROR);
      }


      if((strings_finfo -> fp_or_dbm.fp = (FILE *) fdopen( tmp_fd, (mode == O_RDONLY ? "r" : "r+"))) == (FILE *) NULL){

         error(A_ERR,"open_gfiles_db","opening %s file",DEFAULT_STRINGS_LIST);
         return(ERROR);
      }

      if(new_file)

	 if(ftruncate(fileno(strings_finfo -> fp_or_dbm.fp), (off_t) INIT_STRING_SIZE) != 0){
	    error(A_SYSERR,"open_gfiles_db","extending strings file to default length");
	    return(ERROR);
	 }

   }

   if(strings_hash_finfo != (file_info_t *) NULL){

      strcpy(strings_hash_finfo -> filename, gfiles_db_filename( DEFAULT_GSTRINGS_HASH ));

      strings_hash_finfo -> fp_or_dbm.dbm =  dbm_open(strings_hash_finfo -> filename,
					     (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT),
					     DEFAULT_FILE_PERMS);

      if((strings_hash_finfo -> fp_or_dbm.dbm == (DBM *) NULL) || dbm_error(strings_hash_finfo -> fp_or_dbm.dbm)){

         error(A_INTERR,"open_gfiles_db","error opening hashed strings database %s", DEFAULT_STRINGS_HASH);
         return(ERROR);
      }
   }

   if(host_hash_finfo != (file_info_t *) NULL){

      strcpy(host_hash_finfo -> filename, gfiles_db_filename( DEFAULT_HOST_HASH ));

      host_hash_finfo -> fp_or_dbm.dbm =  dbm_open(host_hash_finfo -> filename,
					     (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT),
					     DEFAULT_FILE_PERMS);

      if((host_hash_finfo -> fp_or_dbm.dbm == (DBM *) NULL) || dbm_error(host_hash_finfo -> fp_or_dbm.dbm)){

         error(A_INTERR,"open_gfiles_db","error opening hashed host database %s", DEFAULT_HOST_HASH);
         return(ERROR);
      }
   }


   if(host_finfo != (file_info_t *) NULL){
      strcpy(host_finfo -> filename, gfiles_db_filename( DEFAULT_HOST_LIST ));

      if(access(host_finfo -> filename, mode == O_RDONLY ? testmode : testmode | W_OK ) == -1){
         if(mode == O_RDONLY){
	    error(A_SYSERR, "open_gfiles_db","can't access %s", host_finfo -> filename);
	    return(ERROR);
	 }
      }
  
      if((tmp_fd = open(host_finfo -> filename, (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT), DEFAULT_FILE_PERMS)) == -1){

	       error(A_SYSERR, "open_gfiles_db","can't access %s", host_finfo -> filename);
	       return(ERROR);
      }

      if((host_finfo -> fp_or_dbm.fp = fdopen( tmp_fd, (mode == O_RDONLY ? "r" : "r+"))) == (FILE *) NULL){

         error(A_ERR,"open_gfiles_db","opening %s file", host_finfo -> filename);
         return(ERROR);
      }
   }


   if(sel_finfo != (file_info_t *) NULL){
      strcpy(sel_finfo -> filename, gfiles_db_filename( DEFAULT_SEL ));

      if(access(sel_finfo -> filename, mode == O_RDONLY ? testmode : testmode | W_OK ) == -1){
         if(mode == O_RDONLY){
	    error(A_SYSERR, "open_gfiles_db","can't access %s", sel_finfo -> filename);
	    return(ERROR);
	 }
      }
  
      if((tmp_fd = open(sel_finfo -> filename, (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT), DEFAULT_FILE_PERMS)) == -1){

	       error(A_SYSERR, "open_gfiles_db","can't access %s", sel_finfo -> filename);
	       return(ERROR);
      }

      if((sel_finfo -> fp_or_dbm.fp = fdopen( tmp_fd, (mode == O_RDONLY ? "r" : "r+"))) == (FILE *) NULL){

         error(A_ERR,"open_gfiles_db","opening %s file", sel_finfo -> filename);
         return(ERROR);
      }
   }

   if(sel_hash_finfo != (file_info_t *) NULL){

      strcpy(sel_hash_finfo -> filename, gfiles_db_filename( DEFAULT_SEL_HASH ));

      sel_hash_finfo -> fp_or_dbm.dbm =  dbm_open(sel_hash_finfo -> filename,
					     (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT),
					     DEFAULT_FILE_PERMS);

      if((sel_hash_finfo -> fp_or_dbm.dbm == (DBM *) NULL) || dbm_error(sel_hash_finfo -> fp_or_dbm.dbm)){

         error(A_INTERR,"open_gfiles_db","error opening hashed host database %s", DEFAULT_HOST_HASH);
         return(ERROR);
      }
   }


   return(A_OK);
}


status_t close_gfiles_db(strings_idx_finfo, strings_finfo, strings_hash_finfo, host_hash_finfo, host_finfo, sel_finfo, sel_hash_finfo)
   file_info_t *strings_idx_finfo;
   file_info_t *strings_finfo;
   file_info_t *strings_hash_finfo;
   file_info_t *host_hash_finfo;
   file_info_t *host_finfo;
   file_info_t *sel_finfo;
   file_info_t *sel_hash_finfo;
{

   if(strings_idx_finfo != (file_info_t *) NULL)
      close_file(strings_idx_finfo);

   if(strings_finfo != (file_info_t *) NULL)
      close_file(strings_finfo);

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
