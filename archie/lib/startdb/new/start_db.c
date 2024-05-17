/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */



#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <malloc.h>
#include "archie_dbm.h"
#include "typedef.h"
#include "db_files.h"
#include "domain.h"
#include "header.h"
#include "start_db.h"
#include "error.h"
#include "lang_startdb.h"
#include "master.h"
#include "archie_strings.h"

#include "protos.h"

char *start_db_filename();
char *host_db_filename();

/*
 * open_start_dbs: Open all the files in the host database.  mode is one of
 * the modes defined in open(2). Databases not to be opened are
 * (file_info_t *) NULL
 */

static index_list[65536];

status_t open_start_dbs(start_db, domaindb, mode)
file_info_t *start_db;
file_info_t *domaindb;
int mode;	  /* How to open the files */
{

   /* Open start database */

   if(start_db != (file_info_t *) NULL){

      if(start_db -> filename[0] == '\0' )
         strcpy(start_db -> filename, start_db_filename( DEFAULT_START_DB ));

      if((start_db -> fp_or_dbm.dbm = dbm_open(start_db -> filename, 
					     (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT),
					     DEFAULT_FILE_PERMS)) == (DBM *) NULL){

         /* "Can't open startdb host database %s" */

         error(A_SYSERR,"open_start_dbs", OPEN_START_DBS_001, start_db -> filename);
         return(ERROR);
      }
   }

   if(domaindb != (file_info_t *) NULL){

      if(domaindb -> filename[0] == '\0' )
        strcpy(domaindb -> filename, host_db_filename( DEFAULT_DOMAIN_DB ));

      if((domaindb -> fp_or_dbm.dbm = dbm_open(domaindb -> filename,
					       (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT),
					       DEFAULT_FILE_PERMS)) == (DBM *) NULL){

        /* "Can't open domain database %s" */

        error(A_SYSERR,"open_host_dbs", OPEN_START_DBS_002, domaindb -> filename);
        return(ERROR);
      }
   }

   if ( read_host_table() == ERROR ) {
     error(A_ERR,"open_host_dbs", "Can't read host table.\n");
     return ERROR;
   }

   if ( read_domain_table(domaindb) == ERROR ) {
     error(A_ERR,"open_host_dbs", "Can't read domain table.\n");
     return ERROR;
   }
   
   return(A_OK);
   
}



/*
 * close_start_dbs: close the host databases
 */


void close_start_dbs(start_db,domain_db)
file_info_t *start_db;
file_info_t *domain_db;
{

   if(start_db != (file_info_t *) NULL)
      if(start_db -> fp_or_dbm.dbm != (DBM *) NULL)
         dbm_close(start_db -> fp_or_dbm.dbm);

   if(domain_db != (file_info_t *) NULL)
      if(domain_db -> fp_or_dbm.dbm != (DBM *) NULL)
         dbm_close(domain_db -> fp_or_dbm.dbm);

   write_host_table();
   
   return;
}


/*
 * set_start_db_dir: Set up the name for the host database directory
 */

char *set_start_db_dir(start_db_dir, db_dir)
   char *start_db_dir;	   /* Name of directory to be set */
   char *db_dir;

{
   static pathname_t start_database_dir;
   char *data_dir;
   struct stat statbuf;

   if(start_db_dir == (char *) NULL){

      /* "Error trying to malloc for host database directory name" */

      if((data_dir = (char *) malloc(strlen(start_database_dir) + 1))
                    == (char *) NULL){

	/* "Error trying to malloc for host database directory name" */

	 error(A_SYSERR,"set_start_db_dir", SET_START_DB_DIR_001);
	 return((char *) NULL);
      }

      strcpy(data_dir, start_database_dir);
      return(data_dir);
   }

   if(start_db_dir[0] != '\0')
     strcpy(start_database_dir, start_db_dir);
   else
     sprintf(start_database_dir, "%s/%s%s/%s%s",  get_master_db_dir(), 
            db_dir, DB_SUFFIX, DEFAULT_START_DB_DIR, DB_SUFFIX);

   if(stat(start_database_dir, &statbuf) == -1){

      /* "Can't access host database directory %s" */

      error(A_SYSERR,"set_start_db_dir", SET_START_DB_DIR_002, start_database_dir);
      return((char *) NULL);
  }

   if(!S_ISDIR(statbuf.st_mode)){

     /* "%s is not a directory" */

     error(A_ERR,"set_start_db_dir", SET_START_DB_DIR_003, start_database_dir);
     return((char *) NULL);
  }

  return(start_database_dir);

}


/*
 * get_start_db_dir: Get the host database directory name
 */

char *get_start_db_dir()

{
   return(set_start_db_dir((char *) NULL,(char *) NULL));
}


/*
 * start_db_filename: Prepend the name of the host database directory to the
 * given filename. Note that the result is held in a static area.
 */


char *start_db_filename(filename)
   char *filename;   /* Name to be converted */

{
   static pathname_t tmp_db_filename;
   char *ghdd = get_start_db_dir();  /* Need this to free memory allocated 
					  in set_start_db_dir() */


   sprintf(tmp_db_filename,"%s/%s",ghdd,filename);
   free(ghdd);		      /* Need to free */
   return(tmp_db_filename);
}

static int find_in_list(index)
host_table_index_t index;
{
  int i;

  for(i=0; index_list[i] != END_LIST; i++ ) {
    if ( index_list[i] == index ) {
      return i;
    }
  }
  return END_LIST;
}

static void insert_in_list(index)
host_table_index_t index;
{

  int i,j;
  
  for ( i = 0; index_list[i] != END_LIST; i++ ) {
    if ( host_table_cmp(index, index_list[i]) < 0 ) {
      for ( j = i; index_list[j] != END_LIST;  )
        j++;
      for ( j++; j > i; j-- )
        index_list[j] = index_list[j-1];
      index_list[i] = index;
      return ;
    }
  }

  index_list[i] = index;
  index_list[i+1] = END_LIST;

}


status_t update_start_dbs(start_db,start,index,what)
file_info_t *start_db;  
int start;
host_table_index_t index;
int what;
{
  int i,j;

  if ( what == ADD_SITE ) {
    if ( get_dbm_entry(&start,sizeof(int),index_list,start_db) == ERROR ) {
      index_list[0] = index;
      index_list[1] = END_LIST;
      if ( put_dbm_entry(&start,sizeof(int),index_list,(2)*sizeof(int),start_db,1) == ERROR ) {
        return ERROR;
      }
      return A_OK;
    }
    i = find_in_list(index);
    if ( i == END_LIST) { /* Not in list .. so find appropriate place */
      insert_in_list(index);

    }
    else { /* Already in it */
      return A_OK;
    }
  }

  if ( what == DELETE_SITE ) {
    if ( get_dbm_entry(&start,sizeof(int),index_list,start_db) == ERROR ) {
      return A_OK;
    }
    i = find_in_list(index);
    if (index_list[i] == END_LIST)
      return A_OK;
    
    for ( j = i+1; index_list[j] != END_LIST; j++,i++) 
      index_list[i] = index_list[j];

    index_list[i] = END_LIST;
  }

  for ( i = 0; index_list[i] != END_LIST;  )
    i++;

  if ( put_dbm_entry(&start,sizeof(int),index_list,(i+1)*sizeof(int),start_db,1) == ERROR ) {
    return ERROR;
  }
  return A_OK;
}

status_t get_index_start_dbs(start_db,start,index,length)
file_info_t *start_db;  
int start;
host_table_index_t *index;
int *length;
{
  int i;
  if ( get_dbm_entry(&start,sizeof(int),index_list,start_db) == ERROR ) {
      *length = 0;
      index = (host_table_index_t *)(0);
      return A_OK;
  }

  for ( i = 0; index_list[i] != END_LIST;  )
  {  index[i] = index_list[i];
     i++;
  }

  *length = i;

  return A_OK;
}

