/*
 * This file is copyright Bunyip Information Systems Inc., 1993, 1994. This
 * file may not be reproduced, copied or transmitted by any means
 * mechanical or electronic without the express written consent of Bunyip
 * Information Systems Inc.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <memory.h>
#include <mp.h>
#include "typedef.h"
#include "db_files.h"
#include "db_ops.h"
#include "webindexdb_ops.h"
#include "host_db.h"
#include "start_db.h"
#include "error.h"
#include "files.h"
#include "archie_strings.h"
#include "archie_dbm.h"
#include "debug.h"
#include "master.h"
#include "site_file.h"
#include "strings_index.h"
#include "protos.h"

extern int madd proto_((MINT *a, MINT *b, MINT *c));
extern int mout proto_((MINT *a));
extern int opterr;
extern char *optarg;




int verbose = 0;
char *prog;

/*
 * db_reorder: generate stats about the archie catalogs
 * 
 * 

   argv, argc are used.

   Parameters:	 
		  -M <master database pathname>
      -d <database>
		  -h <host database pathname>
		  -v verbose mode
		  -l write to log file
		  -L <log file>
		  
*/


int main(argc, argv)
   int argc;
   char *argv[];

{

  char **cmdline_ptr;
  int cmdline_args;
  int option;

  /* Directory names */

  pathname_t master_database_dir;
  pathname_t start_database_dir;
  pathname_t files_database_dir;
  pathname_t wfiles_database_dir;
  pathname_t host_database_dir;

  char *database[2];
  char *dbdir;  
  char *databases_list[] = {
    "anonftp", "webindex", NULL
  };
  char **databases, **db;
  
/*  file_info_t *curr_finfo = create_finfo(); */

  file_info_t *hostbyaddr = create_finfo();
  file_info_t *hostdb = create_finfo();
  file_info_t *domaindb = create_finfo(); 
  file_info_t *hostaux_db = create_finfo();
  file_info_t *new_startdb = create_finfo();
  file_info_t *old_startdb = create_finfo();
  file_info_t *lock_file = create_finfo();
  
  /* host databases */

/*  hostdb_t hostdb_rec;
  hostdb_aux_t hostaux_rec; 
*/
  pathname_t logfile;

  int logging = 0;


  logfile[0] = '\0';

  databases = databases_list;

  host_database_dir[0] = files_database_dir[0] = master_database_dir[0] = '\0';
  wfiles_database_dir[0] = 0;
  
  opterr = 0;                   /* turn off getopt() error messages */

  /* ignore argv[0] */

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;


  while((option = (int) getopt(argc, argv, "d:h:M:L:lv")) != EOF){

    switch(option){


      /* Log filename */

    case 'L':
      strcpy(logfile, optarg);
      logging = 1;
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;

      /* Master directory */

    case 'M':
      strcpy(master_database_dir,optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;


      /* enable logging, default file */

    case 'l':
      logging = 1;
      cmdline_ptr++;
      cmdline_args--;
      break;

      /* verbose mode */

    case 'v':
      verbose = 1;
      cmdline_ptr++;
      cmdline_args--;
      break;

    case 'h':
      strcpy(host_database_dir,optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;

      /* Get files database directory name, argument "w" */

    case 'd':
      database[0] = (char*)malloc(strlen(optarg)+1);
      strcpy(database[0],optarg);
      database[1] = NULL;
      databases = database;
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

    default:
      error(A_ERR,"db_reorder","Usage: Usage: [-M <master database pathname>] [-d <database>]	 [-h <host database pathname>] [-v verbose mode] [-l] [-L <log file>]");
      exit(A_OK);
      break;
    }

  }

  /* set up logs */
   
  if(logging){  
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /*  "Can't open default log file" */

        error(A_ERR, "db_reorder","Can't open default log file" );
        exit(A_OK);
      }
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, "db_reorder", "Can't open log file %s", logfile);
        exit(A_OK);
      }
    }
  }


  /* Set up system directories and files */


  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR,"db_reorder",  "Error while trying to set master database directory" );
    exit(A_OK);
  }


  if(set_host_db_dir(host_database_dir) == (char *) NULL){
    /* "Error while trying to set host database directory" */
    error(A_ERR,"db_reorder","Error while trying to set host database directory");
    exit(A_OK);
  }

  if(open_host_dbs(hostbyaddr, hostdb,  domaindb, hostaux_db, O_RDONLY) != A_OK){

    /* "Error while trying to open host database" */

    error(A_ERR,"db_reorder","Error while trying to open host database" );
    exit(A_OK);
  }

  if ( read_domain_table(domaindb ) == ERROR ) {
    error(A_ERR,"db_reorder","Error while trying to open domain table" );
    exit(A_OK);
  }
  
  for(db = databases; *db != NULL && *db[0] != '\0'; db++ ) {
    int which = 0;
    
    if( !strcmp( *db , ANONFTP_DB_NAME )){
      which = 1;
      if((dbdir = set_files_db_dir(files_database_dir)) == (char *) NULL){
        /* "Error while trying to set anonftp database directory" */
        error(A_ERR, "db_reorder", "Error while trying to set database directory");
        return(A_OK);
      }
      
      sprintf(lock_file -> filename,"%s/%s/%s", get_master_db_dir(), DEFAULT_LOCK_DIR, ANONFTP_LOCKFILE);
      
    }else if( !strcmp( *db , WEBINDEX_DB_NAME )){
      which = 2;
      if((dbdir = set_wfiles_db_dir(wfiles_database_dir)) == (char *) NULL){
        /* "Error while trying to set anonftp database directory" */
        error(A_ERR, "db_reorder","Error while trying to set database directory");
        return(A_OK);
      }
      
      sprintf(lock_file -> filename,"%s/%s/%s", get_master_db_dir(), DEFAULT_LOCK_DIR, WEBINDEX_LOCKFILE);

    }
    else {
        error(A_ERR, "db_reorder","Unknown Database: %s ",*db);
        continue;
    }

    if(access(lock_file -> filename, F_OK) == 0){

      /* lock file exists */

      /* "Waiting for lock file %s" */

      error(A_INFO, "db_reorder", "An update process (arcontrol -u) is underway, db_reorder cannot run at the same time");
      continue;
    }

    if ( verbose )
    error(A_INFO, "db_reorder", "Processing database %s", *db);
    

    /* Open other files database files */
#if 0
    if(open_files_db(strings_idx, strings, (file_info_t *) NULL, O_RDWR) == ERROR){

      /* "Error while trying to open anonftp database" */

      error(A_ERR,"db_reorder", "Error while trying to open anonftp database" );
      exit(A_OK);
    }
#endif

    start_database_dir[0] = '\0';
    if(set_start_db_dir(start_database_dir, *db) == (char *) NULL){

      /* "Error while trying to set master database directory" */

      error(A_ERR,"db_reorder",  "Error while trying to set start database directory" );
      exit(A_OK);
    }


    
    if ( open_start_dbs(old_startdb,NULL,O_RDWR) == ERROR ) {
      
      error(A_ERR,"db_reorder",  "Error while trying to open start database " );
      continue;
    }

    strcpy(new_startdb->filename,start_db_filename("start-db-temp"));
    
    if ( open_start_dbs(new_startdb,NULL,O_RDWR) == ERROR ) {
      
      error(A_ERR,"db_reorder",  "Error while trying to open start database " );
      continue;
    }

    host_table_priority();


    if ( reorder_start_dbs(old_startdb, new_startdb) == ERROR ){
      error(A_ERR,"db_reorder",  "Error while reordering the start database " );
    }
    
    if ( close_start_dbs(old_startdb,NULL) == ERROR ) {
      error(A_ERR,"db_reorder",  "Error while trying to close start database " );
    }

    if ( close_start_dbs(new_startdb,NULL) == ERROR ) {
      error(A_ERR,"db_reorder",  "Error while trying to close start database " );
    }


    { /* Rename the temp database to be the official one .. */
      pathname_t temp1,temp2;

      sprintf(temp1,"%s.dir",old_startdb->filename);
      sprintf(temp2,"%s.dir",new_startdb->filename);
      rename(temp2,temp1);
      sprintf(temp1,"%s.pag",old_startdb->filename);
      sprintf(temp2,"%s.pag",new_startdb->filename);
      rename(temp2,temp1);
    }
  }

  exit(A_OK);
  return(A_OK);

}
