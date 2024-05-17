#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <malloc.h>
#include <memory.h>
#include <unistd.h>
#include "protos.h"
#include "typedef.h"
#include "db_files.h"
#include "db_ops.h"
#include "files.h"
#include "host_db.h"
#include "error.h"
#include "lang_tools.h"
#include "archie_strings.h"
#include "master.h"
#include "archie_dbm.h"
#include "old_hostdb.h"
#include "utils.h"

static status_t new_old_dbname(db, new, old, ext)
  char *db;
  pathname_t new,old;
  char *ext;
{

  strcpy(old, db);
  strcpy(new, db);
  strcat(old, ".old");
  if ( ext != NULL )  {
    strcat(old, ext);
    strcat(new, ext);
  }
  return A_OK;
}

status_t rename_host_dbs(  hostbyaddr, hostdb, hostaux_db )
   file_info_t *hostbyaddr;
   file_info_t *hostdb;
   file_info_t *hostaux_db;
{

  pathname_t old_path, new_path;



  new_old_dbname(host_db_filename(DEFAULT_HOST_DB),new_path, old_path, ".pag");
  if ( access(old_path, R_OK|W_OK) != -1 ) {
    return ERROR;
  }
  
  new_old_dbname(host_db_filename(DEFAULT_HOST_DB),new_path, old_path, ".dir");
  if ( access(old_path, R_OK|W_OK) != -1 ) {
    return ERROR;
  }

  new_old_dbname(host_db_filename(DEFAULT_HOSTAUX_DB),new_path, old_path, ".pag");
  if ( access(old_path, R_OK|W_OK) != -1 ) {
    return ERROR;
  }  
  new_old_dbname(host_db_filename(DEFAULT_HOSTAUX_DB),new_path, old_path, ".dir");
  if ( access(old_path, R_OK|W_OK) != -1 ) {
    return ERROR;
  }

  new_old_dbname(host_db_filename(DEFAULT_HBYADDR_DB),new_path, old_path, ".pag");
  if ( access(old_path, R_OK|W_OK) != -1 ) {
    return ERROR;
  }  
  new_old_dbname(host_db_filename(DEFAULT_HBYADDR_DB),new_path, old_path, ".dir");
  if ( access(old_path, R_OK|W_OK) != -1 ) {
    return ERROR;
  }


  new_old_dbname(host_db_filename(DEFAULT_HOST_DB),new_path, old_path, ".pag");
  rename(new_path, old_path);
  
  new_old_dbname(host_db_filename(DEFAULT_HOST_DB),new_path, old_path, ".dir");
  rename(new_path, old_path);

  new_old_dbname(host_db_filename(DEFAULT_HOSTAUX_DB),new_path, old_path, ".pag");
  rename(new_path, old_path);
  
  new_old_dbname(host_db_filename(DEFAULT_HOSTAUX_DB),new_path, old_path, ".dir");
  rename(new_path, old_path);


  new_old_dbname(host_db_filename(DEFAULT_HBYADDR_DB),new_path, old_path, ".pag");
  rename(new_path, old_path);
  
  new_old_dbname(host_db_filename(DEFAULT_HBYADDR_DB),new_path, old_path, ".dir");
  rename(new_path, old_path);
  
  new_old_dbname(host_db_filename(DEFAULT_HOST_DB),new_path, old_path, NULL);
  strcpy(hostdb->filename, old_path);

  new_old_dbname(host_db_filename(DEFAULT_HOSTAUX_DB),new_path, old_path, NULL);
  strcpy(hostaux_db->filename, old_path);
  new_old_dbname(host_db_filename(DEFAULT_HBYADDR_DB),new_path, old_path, NULL);
  strcpy(hostbyaddr->filename, old_path);

  return A_OK;
}



/*
 * open_old_host_dbs: Open all the files in the host database.  mode is one of
 * the modes defined in open(2). Databases not to be opened are
 * (file_info_t *) NULL
 */


status_t open_old_host_dbs(hostbyaddr, host_db, hostaux_db, mode)
   file_info_t *hostbyaddr;
   file_info_t *host_db;
   file_info_t *hostaux_db;
   int mode;	  /* How to open the files */

{

  /* Open hostbyaddr database */

  if(hostbyaddr != (file_info_t *) NULL){

    if(hostbyaddr -> filename[0] == '\0' )
    return ERROR;

    if((hostbyaddr -> fp_or_dbm.dbm = dbm_open(hostbyaddr -> filename,
                                               (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT),
                                               DEFAULT_FILE_PERMS)) == (DBM *) NULL){


      /* "Can't open host address database %s" */

      error(A_SYSERR,"open_host_dbs", "Can't open host address database %s", hostbyaddr -> filename);
      return(ERROR);
    }
  }

  /* Open primary host database */

  if(host_db != (file_info_t *) NULL){

    if(host_db -> filename[0] == '\0' )
      return ERROR;

    if((host_db -> fp_or_dbm.dbm = dbm_open(host_db -> filename, 
                                            (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT),
                                            DEFAULT_FILE_PERMS)) == (DBM *) NULL){

      /* "Can't open primary host database %s" */

      error(A_SYSERR,"open_host_dbs", "Can't open primary host database %s", host_db -> filename);
      return(ERROR);
    }
  }

  if(hostaux_db != (file_info_t *) NULL){

    if(hostaux_db -> filename[0] == '\0' )
       return ERROR;

    if((hostaux_db -> fp_or_dbm.dbm = dbm_open(hostaux_db -> filename, 
                                               (mode == O_RDONLY ? O_RDONLY : mode | O_CREAT),
                                               DEFAULT_FILE_PERMS)) == (DBM *) NULL){


      /* "Can't open auxiliary host database %s" */

      error(A_SYSERR,"open_host_dbs", "Can't open auxiliary host database %s", host_db -> filename);

      return(ERROR);
    }
  }


  return(A_OK);
}



/*
 * close_old_host_dbs: close the host databases
 */


void close_old_host_dbs(hostbyaddr, host_db,  hostaux_db)
   file_info_t *hostbyaddr;
   file_info_t *host_db;
   file_info_t *hostaux_db;
{
   if(hostbyaddr != (file_info_t *) NULL)
      if(hostbyaddr -> fp_or_dbm.dbm != (DBM *) NULL)
         dbm_close(hostbyaddr -> fp_or_dbm.dbm);

   if(host_db != (file_info_t *) NULL)
      if(host_db -> fp_or_dbm.dbm != (DBM *) NULL)
         dbm_close(host_db -> fp_or_dbm.dbm);

   if(hostaux_db != (file_info_t *) NULL)
      if(hostaux_db -> fp_or_dbm.dbm != (DBM *) NULL)
         dbm_close(hostaux_db -> fp_or_dbm.dbm);

   return;
}




/*
 * sort_retrieve: sort the t_hold_t type based on retrieve time
 */


int sort_retrieve(a, b)
   t_hold_t *a, *b;

{
   if(a -> retrieve_time < b -> retrieve_time)
      return(-1);
   else if(a -> retrieve_time > b -> retrieve_time)
      return(1);
   else
      return(0);
}

static status_t find_in_aceess( access, db_name )
  access_methods_t access;
  char *db_name;
{

  char **av;
  int i;

  av = (char**) str_sep(access,':');
  if ( av == NULL )
    return ERROR;
 
  for (i  = 0 ; av[i] != NULL && av[i][0] != '\0'; i++ ) {
    if ( strcasecmp(av[i],db_name) == 0 ) {
      free_opts(av);
      return A_OK;
    }
  }

  free_opts(av);
  return ERROR;
  
}

/*
 * compose_tuples: generate the tuples in hostdb fulfilling the criteria
 * list by the databases, relation, fromdate and domains variables. The
 * list of tuples generated is returned and tuple count contains the number
 * of tuples generated
 */

   

tuple_t **old_compose_tuples( databases, relation, fromdate, domains, hostbyaddr, hostdb, domaindb, hostaux_db, tuple_count)
   char *databases;	      /* colon separated list of databases in which to look */
   char *relation;	      /* the before or after given "fromdate" */
   date_time_t fromdate;      /* cutoff date */
   char *domains;	      /* colon separated list of domains */
   file_info_t *hostbyaddr;   /* host address cache */
   file_info_t *hostdb;	      /* primary host database */
   file_info_t *domaindb;     /* domain database */
   file_info_t *hostaux_db;   /* auxiliary host database */
   int *tuple_count;	      /* number of tuples returned */

{
  datum ip_search, ip_data;
  tuple_t **tuple_list;
  t_hold_t *th_hold;
  old_hostdb_t hostdb_entry;
  old_hostdb_aux_t hostaux_entry;
  hostbyaddr_t hbaddr;
  int tuple_max;
  database_name_t  database_list[MAX_NO_DATABASES];
  int count;
  domain_t domain_list[MAX_NO_DOMAINS];
  int domain_count;
  pathname_t tmp_string;
  int mycount;

  int all_databases = 0;
  int tuple_idx = 0;
  int database_count;

  ptr_check(databases, char, "compose_tuples", (tuple_t **) NULL);
  ptr_check(relation, char, "compose_tuples", (tuple_t **) NULL);
  ptr_check(domains, char, "compose_tuples", (tuple_t **) NULL);
  ptr_check(tuple_count, int, "compose_tuples", (tuple_t **) NULL);

  ptr_check(hostbyaddr, file_info_t, "compose_tuples", (tuple_t **) NULL);
  ptr_check(hostdb, file_info_t, "compose_tuples", (tuple_t **) NULL); 
  ptr_check(domaindb, file_info_t, "compose_tuples", (tuple_t **) NULL); 
  ptr_check(hostaux_db, file_info_t, "compose_tuples", (tuple_t **) NULL);

  tuple_idx = 0;

  tmp_string[0] = '\0';
   
  if(compile_domains(domains, domain_list, domaindb, &domain_count) == ERROR){

    /* "Can't compile domain list %s" */

    error(A_ERR,"compose_tuples", "Can't compile domain list %s", domains );
    return((tuple_t **) NULL);
  }

  if(compile_database_list(databases, database_list, &database_count) == ERROR){

    /* "Can't compile database list %s" */

    error(A_ERR,"compose_tuples", "Can't compile database list %s", databases);
    return((tuple_t **) NULL);
  }

  if(strcmp(database_list[0],DATABASES_ALL) == 0)
  all_databases = 1;

  tuple_max = DEFAULT_TUPLE_LIST_SIZE;

  if((th_hold = (t_hold_t *) malloc(tuple_max * sizeof(t_hold_t))) == (t_hold_t *) NULL){

    /* "Can't malloc space for tuple hold list" */

    error(A_SYSERR,"compose_tuples", "Can't malloc space for tuple hold list");
    return((tuple_t **) NULL);
  }

  ip_search = dbm_firstkey(hostbyaddr -> fp_or_dbm.dbm);

  if(ip_search.dptr == (char *) NULL){

    /* "Can't find first entry in hostbyaddr database" */

    error(A_INTERR,"compose_tuples", "Can't find first entry in hostbyaddr database");
    return((tuple_t **) NULL);
  }

  do{

    ip_data = dbm_fetch(hostbyaddr -> fp_or_dbm.dbm, ip_search);

    memcpy(&hbaddr, ip_data.dptr, sizeof(hostbyaddr_t));

    if(get_dbm_entry(hbaddr.primary_hostname,strlen(hbaddr.primary_hostname) + 1, &hostdb_entry, hostdb) == ERROR){

      /* "Located %s in hostbyaddr database. Can't find in primary host database" */

      error(A_ERR,"compose_tuples", "Located %s in hostbyaddr database. Can't find in primary host database", hbaddr.primary_hostname);
      ip_search = dbm_nextkey(hostbyaddr -> fp_or_dbm.dbm);

      if(dbm_error(hostdb -> fp_or_dbm.dbm)){

        /* "Can't find next entry in hostbyaddr database" */

        error(A_INTERR,"compose_tuples", "Can't find next entry in hostbyaddr database" );
        return((tuple_t **) NULL);
      }

      continue;
    }
      
    if(find_in_domains(hostdb_entry.primary_hostname, domain_list, domain_count) == 0){
      ip_search = dbm_nextkey(hostbyaddr -> fp_or_dbm.dbm);

      if(dbm_error(hostdb -> fp_or_dbm.dbm)){

        /* "Can't find next entry in hostbyaddr database" */

        error(A_INTERR,"compose_tuples", "Can't find next entry in hostbyaddr database" );
        return((tuple_t **) NULL);
      }

      continue;
    }

    if(all_databases)
    compile_database_list(hostdb_entry.access_methods, database_list, &database_count);

    for(count = 0; database_list[count][0] != '\0'; count++){

      if ( find_in_aceess( hostdb_entry.access_methods, database_list[count]) == ERROR )
      continue;
      
      sprintf(tmp_string, "%s.%s", hostdb_entry.primary_hostname, database_list[count]);

      if(get_dbm_entry(tmp_string, strlen(tmp_string) + 1, &hostaux_entry, hostaux_db) == ERROR)
      continue;

      if((hostaux_entry.current_status == DISABLED)
         || (hostaux_entry.current_status == NOT_SUPPORTED))
      continue;
      /* construct the tuple */

      if(tuple_idx == tuple_max){
        t_hold_t *thold;

        /* Allocate more space */

        tuple_max += DEFAULT_TUPLE_LIST_SIZE;
	 
        if((thold = (t_hold_t *) realloc(th_hold, tuple_max * sizeof(t_hold_t))) == (t_hold_t *) NULL){

          /* "Can't realloc space for tuple hold list" */

          error(A_INTERR,"compose_tuples", "Can't realloc space for tuple hold list" );
          return((tuple_t **) NULL);
        }

        th_hold = thold;

      }

      strcpy(th_hold[tuple_idx].source_archie_hostname, hostaux_entry.source_archie_hostname);
      th_hold[tuple_idx].retrieve_time = hostaux_entry.retrieve_time;
      strcpy(th_hold[tuple_idx].primary_hostname, hostdb_entry.primary_hostname);
      strcpy(th_hold[tuple_idx].preferred_hostname, hostdb_entry.preferred_hostname);
      th_hold[tuple_idx].primary_ipaddr = hostdb_entry.primary_ipaddr;
      strcpy(th_hold[tuple_idx].database_name, database_list[count]);
      th_hold[tuple_idx].flags = hostaux_entry.flags;
          
      tuple_idx++;
    }
		
    ip_search = dbm_nextkey(hostbyaddr -> fp_or_dbm.dbm);
      
    if(dbm_error(hostdb -> fp_or_dbm.dbm)){

      /* "Can't find next entry in hostbyaddr database" */

      error(A_ERR,"compose_tuples", "Can't find next entry in hostbyaddr database" );
      return((tuple_t **) NULL);
    }

  }  while(ip_search.dptr != (char *) NULL);


  qsort(th_hold, tuple_idx, sizeof(t_hold_t), sort_retrieve);

  if((tuple_list = (tuple_t **) malloc(tuple_max * sizeof(tuple_t *))) == (tuple_t **) NULL){

    /* "Can't malloc space for tuple list" */

    error(A_SYSERR,"compose_tuples", "Can't malloc space for tuple list" );
    return((tuple_t **) NULL);
  }

  for(count = 0, mycount = 0; count < tuple_idx; count++){
      
    if(((relation[0] == '>') && ((int) fromdate < (int) th_hold[count].retrieve_time))
       || ((relation[0] == '<') && ((int) fromdate > (int) th_hold[count].retrieve_time))
       || HADB_IS_FORCE_UPDATE(th_hold[count].flags)){

      if((tuple_list[mycount] = (tuple_t *) malloc(MAX_TUPLE_SIZE)) == (tuple_t *) NULL){


        error(A_SYSERR,"compose_tuples", "Cannot allocate memory");
        return((tuple_t **) NULL);
      }

      sprintf((char *) tuple_list[mycount],
              "%s:%s:%s:%s:%s:%s", th_hold[count].source_archie_hostname,
              cvt_from_inttime(th_hold[count].retrieve_time),
              th_hold[count].primary_hostname,
              th_hold[count].preferred_hostname,
              inet_ntoa(ipaddr_to_inet(th_hold[count].primary_ipaddr)),
              th_hold[count].database_name);
      
      mycount++;

    }
  }

  *tuple_count = mycount;

  if(th_hold)
  free(th_hold);

  return(tuple_list);
}
