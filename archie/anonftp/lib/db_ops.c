/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include <string.h>
#include "typedef.h"
#include "db_ops.h"
#include "db_files.h"
#include "files.h"
#include "master.h"
#include "misc.h"
#include "error.h"
#include "lang_libanonftp.h"
#include "archie_dbm.h"

#include "protos.h"

/* Set up the directory name containing the files database */
extern char *prog;

char *set_files_db_dir(files_db_dir)
   char *files_db_dir;

{
   static pathname_t files_database_dir;
   char *data_dir;
   struct stat statbuf;

   if(files_db_dir == (char *) NULL){

      if((data_dir = (char *) malloc(strlen(files_database_dir) +1))
                    == (char *) NULL){
         error(A_SYSERR,"set_database_dir","trying to malloc for database directory name");
	 return((char *) NULL);
      }

      strcpy(data_dir, files_database_dir);
      return(data_dir);
   }

  if(files_db_dir[0] != '\0')
     strcpy(files_database_dir, files_db_dir);
  else
     sprintf(files_database_dir, "%s/%s%s", get_master_db_dir(), DEFAULT_FILES_DB_DIR,DB_SUFFIX);


  if(stat(files_database_dir, &statbuf) == -1){

     error(A_SYSERR,"set_files_db_dir","Can't access dirctory %s", files_database_dir);
     return((char *) NULL);
  }

  if(!S_ISDIR(statbuf.st_mode)){
     error(A_ERR,"set_host_db_dir","%s is not a directory", files_database_dir);
     return((char *) NULL);
  }

  return(files_database_dir);
}


char *get_files_db_dir()

{
   return(set_files_db_dir((char *) NULL));
}



char *files_db_filename(filename,port)
  char *filename;
  int port;
{
  static char tmp_db_filename[MAX_PATH_LEN];
  char *gfdd =  get_files_db_dir();    /* Need this to free memory allocated 
					  in set_files_db_dir() */

  if ( port > 0 )
   sprintf(tmp_db_filename, "%s/%s:%d", gfdd , filename , port);
  else
   sprintf(tmp_db_filename, "%s/%s", gfdd , filename );
  free(gfdd);
  return (char *)tmp_db_filename;
}


/*
 * Open all files database files
 */


status_t open_files_db(strings_idx_finfo, strings_finfo, strings_hash_finfo, mode)
  file_info_t *strings_idx_finfo;
  file_info_t *strings_finfo;
  file_info_t *strings_hash_finfo;
  int mode;
{
  int tmp_fd;
  int testmode = R_OK | F_OK;
   
  if (strings_idx_finfo != (file_info_t *)NULL)
  {
    strcpy(strings_idx_finfo->filename, files_db_filename(DEFAULT_STRINGS_IDX, (int)(0)));
    if (access(strings_idx_finfo->filename, mode == O_RDONLY ? testmode : testmode | W_OK) == -1)
    {
      if (mode == O_RDONLY)
      {
        error(A_SYSERR, "open_files_db", "can't access %s", strings_idx_finfo->filename);
        return ERROR;
      }
#if 0
      else
      {
        error(A_SYSWARN, "open_files_db", "can't access %s.  Attempting to create new file",
              strings_idx_finfo->filename);
      }
#endif
    }
  
    if ((tmp_fd = open(strings_idx_finfo->filename, (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT),
                       DEFAULT_FILE_PERMS)) == -1)
    {
      error(A_SYSERR, "open_files_db", "can't access %s", strings_idx_finfo->filename);
      return ERROR;
    }

    if ((strings_idx_finfo->fp_or_dbm.fp = fdopen(tmp_fd, mode == O_RDONLY ? "r" : "r+"))
        == (FILE *)NULL)
    {
      error(A_ERR, "open_files_db", "opening %s file", DEFAULT_STRINGS_IDX);
      return ERROR;
    }
  }

  if (strings_finfo != (file_info_t *)NULL)
  {
    int new_file = 0;

    strcpy(strings_finfo->filename, files_db_filename(DEFAULT_STRINGS_LIST,(int)(0)));
    if (access(strings_finfo->filename, mode == O_RDONLY ? testmode : testmode | W_OK) == -1)
    {
      if (mode == O_RDONLY)
      {
        error(A_SYSERR, "open_files_db", "can't access %s", strings_finfo->filename);
        return ERROR;
      }
      else
      {
#if 0
        error(A_SYSWARN, "open_files_db", "can't access %s.  Attempting to create new file",
              strings_finfo->filename);
#endif
        new_file = TRUE;
      }
    }
  
    if ((tmp_fd = open(strings_finfo->filename, (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT),
                       DEFAULT_FILE_PERMS)) == -1)
    {
      error(A_SYSERR, "open_files_db", "can't access %s", strings_finfo->filename);
      return ERROR;
    }

    if ((strings_finfo->fp_or_dbm.fp = fdopen(tmp_fd, mode == O_RDONLY ? "r" : "r+"))
        == (FILE *)NULL)
    {
      error(A_SYSERR, "open_files_db", "opening %s file", DEFAULT_STRINGS_LIST);
      return ERROR;
    }

    if (new_file && ftruncate(fileno(strings_finfo->fp_or_dbm.fp), INIT_STRING_SIZE) != 0)
    {
	    error(A_SYSERR, "open_files_db", "extending strings file to default length");
	    return ERROR;
    }
  }

  if (strings_hash_finfo != (file_info_t *)NULL)
  {
    strcpy(strings_hash_finfo->filename, files_db_filename(DEFAULT_STRINGS_HASH,(int)(0)));
    strings_hash_finfo->fp_or_dbm.dbm =  dbm_open(strings_hash_finfo->filename,
                                                  (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT),
                                                  DEFAULT_FILE_PERMS);
    if ((strings_hash_finfo->fp_or_dbm.dbm == (DBM *)NULL) ||
        dbm_error(strings_hash_finfo->fp_or_dbm.dbm))
    {
      error(A_INTERR, "open_files_db", "error opening hashed strings database %s",
            DEFAULT_STRINGS_HASH);
      return ERROR;
    }
  }

  return A_OK;
}


status_t close_files_db(strings_idx_finfo, strings_finfo, strings_hash_finfo)
   file_info_t *strings_idx_finfo;
   file_info_t *strings_finfo;
   file_info_t *strings_hash_finfo;
{

   if(strings_idx_finfo != (file_info_t *) NULL)
      if(close_file(strings_idx_finfo)  == ERROR){

	 error(A_INTERR, "close_files_db", "Can't close strings idx file %s", strings_idx_finfo -> filename);
      }

   if(strings_finfo != (file_info_t *) NULL)
      if(close_file(strings_finfo) == ERROR){
      
	 error(A_INTERR, "close_files_db", "Can't close strings file %s", strings_finfo -> filename);
      }

   if(strings_hash_finfo != (file_info_t *) NULL)
      dbm_close(strings_hash_finfo -> fp_or_dbm.dbm);

   return(A_OK);
}

   

char *unix_perms_itoa(perms, directory, link)
   int perms;
   int directory;
   int link;

{
   static char result[ 16 ] ;

   int i;
   int tmp;

   if(directory)
      result[0] ='d';
   else if(link)
      result[0] ='l';
   else
      result[0] ='-';
      

   result[1] = '\0';

   for(i=8; i >= 0; i--){
      tmp = 0x1;
      if(perms & (tmp << i)){
	 switch(i){
	    case 8:
	    case 5:
	    case 2:
	       strcat(result,"r");
	       break;

	    case 7:
	    case 4:
	    case 1:
       	       strcat(result,"w");
	       break;
	    case 6:
	    case 3:
	    case 0:
       	       strcat(result,"x");
               break;
	 }
      }
      else
         strcat(result,"-");
   }

   return result ;

}


