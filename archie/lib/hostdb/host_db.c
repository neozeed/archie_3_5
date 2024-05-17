/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <malloc.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "archie_dbm.h"
#include "typedef.h"
#include "db_files.h"
#include "domain.h"
#include "header.h"
#include "host_db.h"
#include "error.h"
#include "lang_hostdb.h"
#include "master.h"
#include "archie_strings.h"

#include "protos.h"

extern int errno;

static void create_db(filename)
  char *filename;
{
  pathname_t path;
	struct stat statbuf;
  DBM *db;
  int ret;

  strcpy(path, filename);
  strcat(path, ".db");
  ret = stat(path,&statbuf);

  if (ret && errno == ENOENT) {
    db = dbm_open(filename,O_RDWR|O_CREAT,DEFAULT_FILE_PERMS);
    if ( db != NULL ) {
      dbm_close(db);
    }
  }
}


/*
 * open_host_dbs: Open all the files in the host database.  mode is one of
 * the modes defined in open(2). Databases not to be opened are
 * (file_info_t *) NULL
 */


status_t open_host_dbs(hostbyaddr, host_db, domaindb, hostaux_db, mode)
   file_info_t *hostbyaddr;
   file_info_t *host_db;
   file_info_t *domaindb;
   file_info_t *hostaux_db;
   int mode;	  /* How to open the files */
{

  /*
   *                 HOST BY ADDR
   */
  
  /* Open hostbyaddr database */

  if(hostbyaddr != (file_info_t *) NULL){

    if(hostbyaddr -> filename[0] == '\0' )      
    strcpy(hostbyaddr -> filename, host_db_filename( DEFAULT_HBYADDR_DB ));

    create_db(hostbyaddr -> filename);
    
    if((hostbyaddr -> fp_or_dbm.dbm =
        dbm_open(hostbyaddr -> filename, mode ,
                 DEFAULT_FILE_PERMS)) == (DBM *) NULL){

      if (errno != ENOENT ||
          (hostbyaddr -> fp_or_dbm.dbm =
           dbm_open(hostbyaddr -> filename,
                    mode | O_CREAT, DEFAULT_FILE_PERMS)) == (DBM *) NULL) {
          
        /* "Can't open host address database %s" */

        error(A_SYSERR,"open_host_dbs", OPEN_HOST_DBS_001, hostbyaddr -> filename);
        return(ERROR);
      }
    }
  }

  /*
   *                    HOST DB
   */
    
  /* Open primary host database */

  if(host_db != (file_info_t *) NULL){

    if(host_db -> filename[0] == '\0' )
    strcpy(host_db -> filename, host_db_filename( DEFAULT_HOST_DB ));

    create_db(host_db->filename);
    
    if((host_db -> fp_or_dbm.dbm =
        dbm_open(host_db -> filename, mode,
                 DEFAULT_FILE_PERMS)) == (DBM *) NULL){

      if ( errno != ENOENT || 
          (host_db -> fp_or_dbm.dbm =
           dbm_open(host_db -> filename, mode | O_CREAT,
                    DEFAULT_FILE_PERMS)) == (DBM *) NULL ) {
        /* "Can't open primary host database %s" */

        error(A_SYSERR,"open_host_dbs", OPEN_HOST_DBS_002, host_db -> filename);
        return(ERROR);
      }
    }
  }

  /*
   *                    HOST AUX
   */
    
  if(hostaux_db != (file_info_t *) NULL){

    if(hostaux_db -> filename[0] == '\0' )
    strcpy(hostaux_db -> filename, host_db_filename( DEFAULT_HOSTAUX_DB ));

    create_db(hostaux_db -> filename);
    
    if((hostaux_db -> fp_or_dbm.dbm =
        dbm_open(hostaux_db -> filename,mode,
                 DEFAULT_FILE_PERMS)) == (DBM *) NULL){

      if ( errno != ENOENT ||
          (hostaux_db -> fp_or_dbm.dbm =
           dbm_open(hostaux_db -> filename, mode | O_CREAT,
                    DEFAULT_FILE_PERMS)) == (DBM *) NULL) {

        /* "Can't open auxiliary host database %s" */

        error(A_SYSERR,"open_host_dbs", OPEN_HOST_DBS_003, host_db -> filename);
        return(ERROR);
      }
    }
  }

  /*
   *                  DOMAIN DB
   */
    
  if(domaindb != (file_info_t *) NULL){

    if(domaindb -> filename[0] == '\0' )
    strcpy(domaindb -> filename, host_db_filename( DEFAULT_DOMAIN_DB ));

    create_db(domaindb->filename);
    
    if((domaindb -> fp_or_dbm.dbm =
        dbm_open(domaindb -> filename, mode,
                 DEFAULT_FILE_PERMS)) == (DBM *) NULL){

      if ( errno != ENOENT || 
          (domaindb -> fp_or_dbm.dbm =
           dbm_open(domaindb -> filename,  mode | O_CREAT, 
                    DEFAULT_FILE_PERMS)) == (DBM *) NULL){

        /* "Can't open domain database %s" */

        error(A_SYSERR,"open_host_dbs", OPEN_HOST_DBS_004, domaindb -> filename);
        return(ERROR);
      }
    }
  }

  return(A_OK);
}

/*
 * close_host_dbs: close the host databases
 */


void close_host_dbs(hostbyaddr, host_db, domaindb, hostaux_db)
   file_info_t *hostbyaddr;
   file_info_t *host_db;
   file_info_t *domaindb;
   file_info_t *hostaux_db;
{
   if(hostbyaddr != (file_info_t *) NULL)
      if(hostbyaddr -> fp_or_dbm.dbm != (DBM *) NULL)
         dbm_close(hostbyaddr -> fp_or_dbm.dbm);

   if(host_db != (file_info_t *) NULL)
      if(host_db -> fp_or_dbm.dbm != (DBM *) NULL)
         dbm_close(host_db -> fp_or_dbm.dbm);

   if(domaindb != (file_info_t *) NULL)
      if(domaindb -> fp_or_dbm.dbm != (DBM *) NULL)
         dbm_close(domaindb -> fp_or_dbm.dbm);

   if(hostaux_db != (file_info_t *) NULL)
      if(hostaux_db -> fp_or_dbm.dbm != (DBM *) NULL)
         dbm_close(hostaux_db -> fp_or_dbm.dbm);

   return;
}


/*
 * set_host_db: Set up the name for the host database directory
 */

char *set_host_db_dir(host_db_dir)
   char *host_db_dir;	   /* Name of directory to be set */

{
   static pathname_t host_database_dir;
   char *data_dir;
   struct stat statbuf;

   if(host_db_dir == (char *) NULL){

      /* "Error trying to malloc for host database directory name" */

      if((data_dir = (char *) malloc(strlen(host_database_dir) + 1))
                    == (char *) NULL){

	/* "Error trying to malloc for host database directory name" */

	 error(A_SYSERR,"set_host_db_dir", SET_HOST_DB_DIR_001);
	 return((char *) NULL);
      }

      strcpy(data_dir, host_database_dir);
      return(data_dir);
   }

   if(host_db_dir[0] != '\0')
     strcpy(host_database_dir, host_db_dir);
   else
     sprintf(host_database_dir, "%s/%s%s",  get_master_db_dir(), DEFAULT_HOST_DB_DIR, DB_SUFFIX);

   if(stat(host_database_dir, &statbuf) == -1){

      /* "Can't access host database directory %s" */

      error(A_SYSERR,"set_host_db_dir", SET_HOST_DB_DIR_002, host_database_dir);
      return((char *) NULL);
  }

   if(!S_ISDIR(statbuf.st_mode)){

     /* "%s is not a directory" */

     error(A_ERR,"set_host_db_dir", SET_HOST_DB_DIR_003, host_database_dir);
     return((char *) NULL);
  }

  return(host_database_dir);

}


/*
 * get_host_db_dir: Get the host database directory name
 */

char *get_host_db_dir()

{
   return(set_host_db_dir((char *) NULL));
}


/*
 * host_db_filename: Prepend the name of the host database directory to the
 * given filename. Note that the result is held in a static area.
 */


char *host_db_filename(filename)
   char *filename;   /* Name to be converted */

{
   static pathname_t tmp_db_filename;
   char *ghdd = get_host_db_dir();  /* Need this to free memory allocated 
					  in set_host_db_dir() */


   sprintf(tmp_db_filename,"%s/%s",ghdd,filename);
   free(ghdd);		      /* Need to free */
   return(tmp_db_filename);
}

    
/*
 * get_host_error: assign the error message associated with the given code.
 * Codes are defined in host_db.h
 */



char *get_host_error(status)
    host_status_t status;  /* error code */

{
   static char *outmessage;
   static pathname_t tmp_string;

   switch(status){

      case HOST_OK:

         /* "Host OK" */

	 outmessage = GET_HOST_ERROR_001;
	 break;

      case HOST_CNAME_MISMATCH:

	 /* "Preferred/primary names are not the same host" */

	 outmessage = GET_HOST_ERROR_002;
	 break;

      case HOST_CNAME_UNKNOWN:

	 /* "Preferred hostname (CNAME) cannot be resolved" */

	 outmessage = GET_HOST_ERROR_003;
	 break;

      case HOST_UNKNOWN:

	 /* "Host unknown" */

	 outmessage = GET_HOST_ERROR_004;
	 break;

      case HOST_ALREADY_EXISTS:

	 /*  "Host already stored in database" */

	 outmessage = GET_HOST_ERROR_005;
	 break;

      case HOST_PADDR_EXISTS:

	 /* "Primary IP address already exists in database" */

	 outmessage = GET_HOST_ERROR_006;
	 break;

      case HOST_UPDATE_FAIL:

	 /* "Update of primary host database failed" */

	 outmessage = GET_HOST_ERROR_007;
	 break;

      case HOST_ACCESS_UPDATE_FAIL:
         outmessage = "Update of database failed";
	 break;

      case HOST_STORED_OTHERWISE:

	 /* "Host stored in database under different name" */

	 outmessage = GET_HOST_ERROR_008;
	 break;

      case HOST_NAME_NOT_PRIMARY:

	 /* "Given name is not primary for host" */

	 outmessage = GET_HOST_ERROR_009;
	 break;

      case HOST_PADDR_MISMATCH:

	 /* "Address in database doesn't match external reference" */

	 outmessage = GET_HOST_ERROR_010;
	 break;

      case HOST_ACTIVE:

	 /* "Update attempted on active host entry" */
      
	 outmessage = GET_HOST_ERROR_011;
	 break;

      case HOST_DOESNT_EXIST:

	 /* "Host which should exist in database, doesn't" */

	 outmessage = GET_HOST_ERROR_013;
	 break;

      default:

	 /* "Unknown error" */

	 sprintf(tmp_string,"%s %x", GET_HOST_ERROR_012, status);
	 outmessage = tmp_string;
	 break;
   }

   return(outmessage);
}




/*
 * hostdb_cmp: compare to hostdb_t structures. Return A_OK if equal, ERROR
 * otherwise. If cmp_primary_hostname is non-zero then compare the
 * primary_name fields of the records
 */


status_t hostdb_cmp(rec1, rec2, cmp_primary_hostname)
   hostdb_t *rec1;	/* First entry */
   hostdb_t *rec2;	/* Second entry */
   int	    cmp_primary_hostname;      /* Compare primary hostnames if non-zero */

{

   char **ac1, **ac2, **tmp_ptr1, **tmp_ptr2;
   int count1, count2;

   ptr_check(rec1, hostdb_t, "hostdb_cmp", ERROR);
   ptr_check(rec2, hostdb_t, "hostdb_cmp", ERROR);   

   if(cmp_primary_hostname)
     if(strcmp(rec1 -> primary_hostname, rec2 -> primary_hostname) != 0)
        return(ERROR);

   if(rec1 -> primary_ipaddr != rec2 -> primary_ipaddr)
      return(ERROR);

   if(rec1 -> os_type != rec2 -> os_type)
      return(ERROR);

   if(rec1 -> timezone != rec2 -> timezone)
      return(ERROR);


   if((ac1 = str_sep(rec1 -> access_methods, NET_DELIM_CHAR)) == (char **) NULL)
      return(ERROR);

   if((ac2 = str_sep(rec2 -> access_methods, NET_DELIM_CHAR)) == (char **) NULL){
      free_opts(ac1);
      return(ERROR);
   }

   for(count1 = 0, tmp_ptr1 = ac1; *tmp_ptr1 != (char *) NULL;){
      count1++;
      tmp_ptr1++;
   }

   for(count2 = 0, tmp_ptr2 = ac2; *tmp_ptr2 != (char *) NULL;){
      count2++;
      tmp_ptr2++;
   }

   if(count1 != count2){
       free_opts(ac1);
       free_opts(ac2);
       return(ERROR);
   }

   if((count1 == 1) && (strcasecmp(ac1[0], ac2[0]) != 0))
      return(ERROR);

   qsort((char *) ac1, count1, sizeof(char **), cmp_strcase_ptr);
   qsort((char *) ac2, count2, sizeof(char **), cmp_strcase_ptr);
    
   for(tmp_ptr1 = ac1, tmp_ptr2 = ac2;
       (*tmp_ptr1 != (char *) NULL) && (*tmp_ptr2 != (char *) NULL);){

       if(strcasecmp(*tmp_ptr1, *tmp_ptr2) != 0){

	  free_opts(ac1);
	  free_opts(ac2);
	  return(ERROR);
       }

      tmp_ptr1++;
      tmp_ptr2++;
   }


   free_opts(ac1);
   free_opts(ac2);

   if(rec1 -> flags != rec2 -> flags)
      return(ERROR);

   return(A_OK);
}


int str_list_cmp(access1, access2)
  char *access1, *access2;
{

  char **ac1, **ac2;
  int count1, count2,min,i;

  if((ac1 = str_sep(access1, NET_DELIM_CHAR)) == (char **) NULL)
    return(ERROR);

  if((ac2 = str_sep(access2, NET_DELIM_CHAR)) == (char **) NULL){
    free_opts(ac1);
    return(ERROR);
  }

  for ( count1= 0; ac1[count1] != NULL ; )
    count1++;

  for ( count2= 0; ac2[count2] != NULL ; )
    count2++;

  min  = (count1 < count2) ? count1 : count2;
  for ( i = 0; i < min; i++ ) {
    if ( strcasecmp(ac1[i],ac2[i])  ) {
      free_opts(ac1);
      free_opts(ac2);
      return(ERROR);
    }
  }

  if ( min < count1  ) {
    for ( i = min; i < count1; i++ )  {
      if ( ac1[i][0] != '\0' ) {
        free_opts(ac1);
        free_opts(ac2);
        return(ERROR);
      }
    }
  }

  if ( min < count2  ) {
    for ( i = min; i < count2; i++ )  {
      if ( ac2[i][0] != '\0' ) {
        free_opts(ac1);
        free_opts(ac2);
        return(ERROR);
      }
    }
  }

  return (A_OK);
}

/*
 * hostaux_cmp: compare two auxiliary host database records.
 * cmp_access_methods is non-zero if the access_methods fields are to be
 * compared
 */


status_t hostaux_cmp(rec1, rec2, cmp_access_methods)
   hostdb_aux_t	  *rec1;
   hostdb_aux_t	  *rec2;
   int		  cmp_access_methods;	  /* Non-zero if access methods are to be compared */

{
   ptr_check(rec1, hostdb_aux_t, "hostaux_cmp", ERROR);
   ptr_check(rec2, hostdb_aux_t, "hostaux_cmp", ERROR);   

   if(cmp_access_methods != 0){

     if ( rec1->origin == NULL || rec2->origin == NULL ) 
       return(ERROR);
     
     if ( str_list_cmp(rec1->origin->access_methods,rec2->origin->access_methods) == ERROR )
       return ERROR;
     
     if ( str_list_cmp(rec1->access_command,rec2->access_command) == ERROR )
       return ERROR;
/*
     if(strcasecmp(rec1 -> origin -> access_methods, rec2 -> origin -> access_methods) != 0)
         return(ERROR);
   
   
      if(strcasecmp(rec1 -> access_command, rec2 -> access_command) != 0)
        return(ERROR);
*/
   }

   if(strcasecmp(rec1 -> source_archie_hostname, rec2 -> source_archie_hostname) != 0)
      return(ERROR);

   if(strcmp(rec1 -> preferred_hostname, rec2 -> preferred_hostname) != 0)
      return(ERROR);

   if(rec1 -> retrieve_time != rec2 -> retrieve_time)
      return(ERROR);
      
   if(rec1 -> parse_time != rec2 -> parse_time)
      return(ERROR);

   if(rec1 -> update_time != rec2 -> update_time)
      return(ERROR);

   if(rec1 -> no_recs != rec2 -> no_recs)
      return(ERROR);

   if(rec1 -> current_status != rec2 -> current_status)
      return(ERROR);

   if(rec1 -> fail_count != rec2 -> fail_count)
      return(ERROR);

   return(A_OK);

}



/*
 * delete_from_hostdb: inactivate the record for the given hostname in the
 * auxiliary database
 */


status_t delete_from_hostdb(hostname, dbname, index, hostaux_db)
   char *hostname;
   char *dbname;
  index_t index;
   file_info_t *hostaux_db;

{
#ifdef __STDC__

   extern time_t time(time_t *);

#else

   extern time_t time();

#endif

   pathname_t aux_name;
   hostdb_aux_t hostaux_rec;

   ptr_check(hostaux_db, file_info_t, "delete_from_hostdb", ERROR);

   if((hostname == (char *) NULL) || (hostname[0] == '\0')){

      /* "Invalid hostname parameter" */

      error(A_ERR, "update_hostaux", DELETE_FROM_HOSTDB_001);
      return(HOST_ERROR);
   }

   sprintf(aux_name, "%s.%s.%d", hostname, dbname,(int)index);

   if(get_dbm_entry(aux_name, strlen(aux_name) + 1, &hostaux_rec, hostaux_db) == ERROR){

      /* "Can't find host %s with database %s" */

      error(A_ERR, "delete_from_hostdb", DELETE_FROM_HOSTDB_002, hostname, dbname);
      return(ERROR);
   }

   hostaux_rec.update_time = time((time_t *) NULL);
   hostaux_rec.no_recs = 0;
   hostaux_rec.current_status = INACTIVE;

   if(put_dbm_entry(aux_name, strlen(aux_name) + 1, &hostaux_rec, sizeof(hostdb_aux_t), hostaux_db, TRUE) == ERROR){

      /* "Can't update entry for host %s database %s primary host database" */

      error(A_ERR, "delete_from_hostdb", DELETE_FROM_HOSTDB_003, hostname, dbname);
      return(ERROR);
   }
	    
   return(A_OK);
}


/*
 * inactivate_if_found: inactivate the record for the given hostname in the
 * auxiliary database
 */


status_t inactivate_if_found(hostname, dbname, preferred_hostname, access_command, hostaux_db)
  hostname_t hostname;
  char *dbname;
  hostname_t preferred_hostname;
  access_comm_t access_command;
  file_info_t *hostaux_db;
{
#ifdef __STDC__

  extern time_t time(time_t *);

#else

  extern time_t time();

#endif
  index_t index;
  pathname_t aux_name;
  hostdb_aux_t hostaux_rec;

  ptr_check(hostaux_db, file_info_t, "inactivate_if_found", ERROR);

  hostaux_rec.origin = NULL;
  
  if((hostname == (char *) NULL) || (hostname[0] == '\0')){

    /* "Invalid hostname parameter" */

    error(A_ERR, "update_hostaux", DELETE_FROM_HOSTDB_001);
    return(HOST_ERROR);
  }
  /*     
     sprintf(aux_name, "%s.%s", hostname, dbname);
     
     if(get_dbm_entry(aux_name, strlen(aux_name) + 1, &hostaux_rec, hostaux_db) == ERROR){
     */

   
  if ( get_hostaux_ent(hostname, dbname, &index, preferred_hostname,
                       access_command, &hostaux_rec, hostaux_db) == ERROR){

    /* "Can't find host %s with database %s" */

    error(A_INFO, "inactivate_if_found", DELETE_FROM_HOSTDB_002, hostname, dbname);
    return(A_OK);
  }

  set_aux_origin(&hostaux_rec, dbname, index);
  
  hostaux_rec.update_time = time((time_t *) NULL);
  hostaux_rec.no_recs = 0;
  hostaux_rec.current_status = INACTIVE;

  sprintf(aux_name, "%s.%s.%d", hostname, dbname, (int)index);  
  if(put_dbm_entry(aux_name, strlen(aux_name) + 1, &hostaux_rec, sizeof(hostdb_aux_t), hostaux_db, TRUE) == ERROR){

    /* "Can't update entry for host %s database %s primary host database" */

    error(A_ERR, "inactivate_if_found", DELETE_FROM_HOSTDB_003, hostname, dbname);
    free(hostaux_rec.origin);
    return(ERROR);
  }
  
  free(hostaux_rec.origin);	    
  return(A_OK);
}

