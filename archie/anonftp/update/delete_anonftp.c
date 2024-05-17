/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/* #include <strings.h> */
#include <sys/types.h>
#include "typedef.h"
#include "db_files.h"
#include "db_ops.h"
#include "host_db.h"
#include "error.h"
#include "files.h"
#include "archie_strings.h"
#include "archie_dbm.h"
#include "delete_anonftp.h"
#include "lang_anonftp.h"
#include "debug.h"
#include "master.h"

#include "../startdb/start_db.h"
#include "../startdb/lang_startdb.h"

extern int errno;
int verbose = 0;

char *prog;

#ifdef __STDC__
   extern status_t setup_delete( file_info_t *, file_info_t *,
                                 int, ip_addr_t, int );
#else
   extern status_t setup_delete();
   extern status_t  get_port();
#endif

/*
 * delete_anonftp: remove a site from the archie anonftp database. It
 * modifies the primary host database as well as the auxiliary database. It
 * performs no file locking, for exculsive file access

 NOTE: Return codes in this program are non-standard. Only database errors
       cause an exit code of ERROR, other errors exit with A_OK

   argv, argc are used.


   Parameters:	  -H <hostname> Mandatory.
      -p <port>
		  -w <anonftp files database pathname>
		  -M <master database pathname>
		  -h <host database pathname>
		  -v verbose
		  -l write to log file
		  -L <log file>
		  
*/


int main(argc, argv)
   int argc;
   char *argv[];

{
  extern int opterr;
  extern char *optarg;

  char **cmdline_ptr;
  int cmdline_args;

  int option;

  char *dbdir;
  pathname_t tmp_string;

  /* Directory names */

  pathname_t master_database_dir;
  pathname_t start_database_dir;
  pathname_t files_database_dir;
  pathname_t host_database_dir;

  /* File information */

  file_info_t *input_finfo = create_finfo();
  file_info_t *hostbyaddr = create_finfo();
  file_info_t *hostdb = create_finfo();
  file_info_t *hostaux_db = create_finfo();

  file_info_t *start_db = create_finfo();
  file_info_t *domain_db = create_finfo();

  /* host databases */

  header_t header_rec;          /* Input header record */

  hostdb_t hostdb_rec;
  hostdb_aux_t hostaux_rec;

  hostname_t hostname;
  pathname_t logfile;

  int logging = 0;

  int port = 0;
  int nofile = 0;

  prog = argv[0];

  input_finfo -> filename[0] = logfile[0] = '\0';

  opterr = 0;                   /* turn off getopt() error messages */

  /* ignore argv[0] */

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;

  master_database_dir[0] = start_database_dir[0] = files_database_dir[0] = host_database_dir[0] = '\0';

  hostname[0] = logfile[0] = '\0';

  while((option = (int) getopt(argc, argv, "w:h:H:p:t:M:L:lv")) != EOF){

    switch(option){


      /* hostname to be deleted */ 

    case 'H':
	    strcpy(hostname,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

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

      /* Host database directory name */

    case 'h':
	    strcpy(host_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* port of the site */

    case 'p':
	    port = atoi(optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;
      
      /* enable logging, default file */

    case 'l':
	    logging = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;


      /* debugging level */

    case 'v':
	    verbose = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* Get files database directory name, argument "w" */

    case 'w':
	    strcpy(files_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

    case 't':
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;
      
    default:
	    error(A_ERR,"delete_anonftp","Usage: -H <Hostname> [-p <Port>] [-L <log file>] [-M <master db>] [-l] [-v ] [-w <data directory>]");
      { int kk;
      for ( kk = 0 ; kk < argc; kk++ ) {
        fprintf(stderr,"%s\n",argv[kk]);
      }
      }
	    exit(A_OK);
	    break;
    }

  }

#ifdef SLEEP
  fprintf(stderr,"Sleeping...\n");
  sleep(20);
#endif
  
  /* set up logs */
   
  if(logging){  
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /*  "Can't open default log file" */

        error(A_ERR, "delete_anonftp", DELETE_ANONFTP_013);
        exit(A_OK);
      }
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, "delete_anonftp", DELETE_ANONFTP_014, logfile);
        exit(A_OK);
      }
    }
  }

  /* Check to see if the hostname was given. It is mandatory */

  if(hostname[0] == '\0'){

    /* "No site name or address given" */

    error(A_ERR,"delete_anonftp", DELETE_ANONFTP_001);
    exit(A_OK);
  }

  if(port==0){

    /* "No port number given" */

    error(A_INFO,"delete_anonftp","No port number given, assuming default one");
    if( get_port((char *)NULL, ANONFTP_DB_NAME, &port ) == ERROR ){
      error(A_ERR,"get_port","Could not get port for site");
      exit(A_OK);
    }
  }

  /* Set up system directories and files */

  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR,"delete_anonftp", DELETE_ANONFTP_002);
    exit(A_OK);
  }

  if((dbdir = set_files_db_dir(files_database_dir)) == (char *) NULL){

    /* "Error while trying to set anonftp database directory" */

    error(A_ERR,"delete_anonftp", DELETE_ANONFTP_003);
    exit(A_OK);
  }


  if((set_start_db_dir(start_database_dir,DEFAULT_FILES_DB_DIR)) == (char *) NULL){

    /* "Error while trying to set start database directory" */

    error(A_ERR, "insert_anonftp", "Error while trying to set start database directory\n");
    exit(A_OK);
  }

  if(set_host_db_dir(host_database_dir) == (char *) NULL){

    /* "Error while trying to set host database directory" */

    error(A_ERR,"delete_anonftp", DELETE_ANONFTP_004);
    exit(A_OK);
  }

  /* Open other files database files */

  if ( open_start_dbs(start_db,domain_db,O_RDWR ) == ERROR ) {

    /* "Can't open start/host database" */

    error(A_ERR, "delete_anonftp", "Can't open start/host database");
    exit(A_OK);
  }

  if(open_host_dbs(hostbyaddr, hostdb, (file_info_t *) NULL, hostaux_db, O_RDWR) != A_OK){

    /* "Error while trying to open host database" */

    error(A_ERR,"delete_anonftp", DELETE_ANONFTP_006);
    exit(A_OK);
  }


  /* get input file information */
  if(get_input_file(hostname, ANONFTP_DB_NAME, (index_t)0, files_db_filename, input_finfo, &hostdb_rec, &hostaux_rec, hostdb, hostaux_db, hostbyaddr) == ERROR){
/*
  if(get_input_file(hostname, ANONFTP_DB_NAME, files_db_filename, input_finfo, &hostdb_rec, &hostaux_rec, hostdb, hostaux_db, hostbyaddr) == ERROR){
*/
    /* "Can't determine input file for site %s" */

    error(A_ERR,"delete_anonftp", DELETE_ANONFTP_007, hostname);
    exit(A_OK);
  }
  else{
    /* Open it */

    if(open_file(input_finfo, O_RDWR) == ERROR){

      /* "Can't find input database file for %s" */

      error(A_WARN,"delete_anonftp", DELETE_ANONFTP_008, hostname);
      nofile = 1;
    }

  }

  /*   if(mmap_file(input_finfo, O_RDONLY) == ERROR){
       
       "Can't mmap() input file %s" 
       
       error(A_SYSERR,"insert_anonftp", INSERT_ANONFTP_003, input_finfo -> filename);
       exit(A_OK);
       }
       */

  if(delete_from_hostdb(hostdb_rec.primary_hostname, ANONFTP_DB_NAME, (index_t)0, hostaux_db) == ERROR){

    /* "Error while trying to inactivate %s in host database" */

    error(A_ERR,"delete_anonftp", DELETE_ANONFTP_009, hostdb_rec.primary_hostname);
    exit(A_OK);
  }

  if(nofile)
  exit(A_OK);

  sprintf(tmp_string,"%s.%s.0",hostdb_rec.primary_hostname, ANONFTP_DB_NAME);

  if(get_dbm_entry(tmp_string, strlen(tmp_string) + 1, &hostaux_rec, hostaux_db) == ERROR){

    /* "Can't find requested host %s database %s in local auxiliary host database" */

    error(A_ERR,"net_anonftp", NET_ANONFTP_010, hostdb_rec.primary_hostname, ANONFTP_DB_NAME);
    header_rec.update_status = FAIL;
  }
  
  if(setup_delete( input_finfo, start_db, (int)hostaux_rec.site_no_recs, hostdb_rec.primary_ipaddr, port) == ERROR ){

    error(A_ERR,"delete_anonftp","Failed in setup_delete.\n");
    if(close_file(input_finfo) == ERROR){

      /* "Can't close input file %s" */

      error(A_ERR, "delete_anonftp", DELETE_ANONFTP_012, input_finfo -> filename);
    }
    exit(A_OK);
    return(A_OK);       
  }
   
  /* Close data file */

  if(close_file(input_finfo) == ERROR){

    /* "Can't close input file %s" */

    error(A_ERR, "delete_anonftp", DELETE_ANONFTP_012, input_finfo -> filename);
  }

  close_start_dbs(start_db,domain_db);

  /* Remove data file from database */

  if(unlink(input_finfo -> filename) == -1){

    /* "Can't remove the site file %s" */

    error(A_SYSERR,"delete_anonftp", DELETE_ANONFTP_011, input_finfo -> filename);
    exit(A_OK);
  }

  strcat(input_finfo->filename,".idx");
  if(unlink(input_finfo -> filename) == -1){

    if ( errno != ENOENT ) {
      /* "Can't remove the site file %s" */

      error(A_SYSERR,"delete_anonftp", DELETE_ANONFTP_011, input_finfo -> filename);
      exit(A_OK);
    }
  }
  
  exit(A_OK);

  return(A_OK);

}

