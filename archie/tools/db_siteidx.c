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

status_t process_list PROTO((char **, int));

#define WEB 1
#define FTP 2


int verbose = 0;
char *prog;

/*
 * db_siteidx: generate siteidx about the archie catalogs
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
  pathname_t files_database_dir;
  pathname_t host_database_dir;
  pathname_t wfiles_database_dir;

  char *database[2];
  char *dbdir;  
  char *databases_list[] = {
    "anonftp", "webindex", NULL
  };
  char **databases, **db;

  int min_size = MIN_INDEX_SIZE;
  
/*  file_info_t *curr_finfo = create_finfo(); */

  file_info_t *hostbyaddr = create_finfo();
  file_info_t *hostdb = create_finfo(); 
  file_info_t *hostaux_db = create_finfo();

  /* host databases */

/*  hostdb_t hostdb_rec;
  hostdb_aux_t hostaux_rec; 
*/
  pathname_t logfile;
  hostname_t hostname,port;

  int logging = 0;

  int count = 0;
  char * (*func)();
  char **file_list = (char **) NULL;

  logfile[0] = hostname[0] = '\0';

  databases = databases_list;

  wfiles_database_dir[0] = files_database_dir[0] = host_database_dir[0] = master_database_dir[0] = '\0';

  opterr = 0;                   /* turn off getopt() error messages */

  /* ignore argv[0] */

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;


  while((option = (int) getopt(argc, argv, "H:d:h:M:L:lvp:I:")) != EOF){

    switch(option){


      /* hostname to be checked */ 

    case 'H':
      strcpy(hostname,optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;

      /* port number */
    case 'p':
      strcpy(port,optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;

    case 'I':
      min_size = atoi(optarg);
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
      error(A_ERR,"db_siteidx","Usage: [-p] [-d <debug level>] [-w <data directory>] <sitename>");
      exit(A_OK);
      break;
    }

  }

  /* set up logs */
   
  if(logging){  
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /*  "Can't open default log file" */

        error(A_ERR, "db_siteidx","Can't open default log file" );
        exit(A_OK);
      }
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, "db_siteidx", "Can't open log file %s", logfile);
        exit(A_OK);
      }
    }
  }


  /* Set up system directories and files */


  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR,"db_siteidx",  "Error while trying to set master database directory" );
    exit(A_OK);
  }


  if(set_host_db_dir(host_database_dir) == (char *) NULL){
    /* "Error while trying to set host database directory" */
    error(A_ERR,"db_siteidx","Error while trying to set host database directory");
    exit(A_OK);
  }

  if(open_host_dbs(hostbyaddr, hostdb, (file_info_t *) NULL, hostaux_db, O_RDONLY) != A_OK){

    /* "Error while trying to open host database" */

    error(A_ERR,"db_siteidx","Error while trying to open host database" );
    exit(A_OK);
  }


  for(db = databases; *db != NULL && *db[0] != '\0'; db++ ) {
    int which = 0;
    
    if( !strcmp( *db , ANONFTP_DB_NAME )){
      which = FTP;
      if((dbdir = set_files_db_dir(files_database_dir)) == (char *) NULL){
        /* "Error while trying to set anonftp database directory" */
        error(A_ERR, "db_siteidx", "Error while trying to set database directory");
        return(A_OK);
      }
    }else if( !strcmp( *db , WEBINDEX_DB_NAME )){
      which = WEB;
      if((dbdir = set_wfiles_db_dir(wfiles_database_dir)) == (char *) NULL){
        /* "Error while trying to set anonftp database directory" */
        error(A_ERR, "db_siteidx","Error while trying to set database directory");
        return(A_OK);
      }
    }
    else {
        error(A_ERR, "db_siteidx","Unknown Database: %s ",*db);
        return(A_OK);
    }


    
    if ( verbose )
    error(A_INFO, "db_siteidx", "Processing database %s", *db);



    /* Open other files database files */
#if 0
    if(open_files_db(strings_idx, strings, (file_info_t *) NULL, O_RDWR) == ERROR){

      /* "Error while trying to open anonftp database" */

      error(A_ERR,"db_siteidx", "Error while trying to open anonftp database" );
      exit(A_OK);
    }
#endif



    
    switch(which) {
    case WEB:
      func = get_wfiles_db_dir;
      break;
      
    case FTP:
      func = get_files_db_dir;
      break;
    }

    
    if(hostname[0] != '\0'){
      hostdb_t hostdb_rec;
      pathname_t path;

      if ( get_dbm_entry(hostname,strlen(hostname)+1,&hostdb_rec, hostdb) == ERROR ){
        error(A_ERR, "db_siteidx", "Can't get hostdb record ");
        return ERROR;
      }

      if ( port[0] == '\0' )  { /* No port given */
        sprintf(path,"%s",inet_ntoa(ipaddr_to_inet(hostdb_rec.primary_ipaddr)));
      }
      else {
        sprintf(path,"%s:%s",inet_ntoa(ipaddr_to_inet(hostdb_rec.primary_ipaddr)),port);
      }

      if(get_pre_file_list(func(), &file_list, path, &count) == ERROR){

        /* "Can't get list of files in directory %s" */

        error(A_ERR, "db_siteidx", "Can't get list of files in directory %s", func());
        exit(ERROR);
      }
      
    }
    else{

      if(get_file_list(func(), &file_list, (char *) NULL, &count) == ERROR){

        /* "Can't get list of files in directory %s" */

        error(A_ERR, "db_siteidx", "Can't get list of files in directory %s", get_files_db_dir());
        exit(ERROR);
      }
        
    }

    process_list(file_list, min_size);
    
  }

  
  exit(A_OK);
  return(A_OK);

}



status_t process_list(file_list,min_size)
  char **file_list;
  int min_size;
{
  /* File information */

  char **curr_file;

  ip_addr_t ipaddr = 0;

  int tmp_val;
  
  

  int act_size;
  
  file_info_t *curr_finfo = create_finfo();

  ptr_check(file_list, char*, "check_indiv", ERROR);


    
  for(curr_file = file_list; *curr_file != (char *) NULL; curr_file++){
    pathname_t filename, tmp;
    char *port;
    struct stat statbuf;
    
    if(sscanf(*curr_file, "%*d.%*d.%*d.%d:%*d", &tmp_val) != 1)
    continue;


    strcpy(filename,*curr_file);
    port = strchr(filename,':');
    if ( port == NULL )
    continue;
    *port++ = '\0';
    ipaddr = inet_addr(filename);

    strcpy(curr_finfo -> filename, files_db_filename(filename,atoi(port)));

    sprintf(tmp,"%s%s",curr_finfo->filename,SITE_INDEX_SUFFIX);
    if ( stat(tmp, &statbuf) != -1 )
      continue;

    /* Open current file */

    if(open_file(curr_finfo, O_RDONLY) == ERROR){

      /* "Ignoring %s" */
	 
      error(A_ERR, "check_indiv", "Ignoring %s" , *curr_file);
      continue;
    }

    /* mmap it */

    if(mmap_file(curr_finfo, O_RDONLY) == ERROR){

      /* "Ignoring %s" */
	 
      error(A_ERR, "check_indiv",  "Ignoring %s" , *curr_file);
      continue;
    }


    act_size = curr_finfo -> size / sizeof(full_site_entry_t);

    if ( curr_finfo->size >= min_size ) {
      if ( create_site_index(curr_finfo->filename,curr_finfo->ptr,act_size) == ERROR ) {
        error(A_ERR,"db_siteidx", "Unable to create site index file");
      }
    }
    close_file(curr_finfo);
  }
    
  return(A_OK);
}




