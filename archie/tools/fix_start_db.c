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
#include "archstridx.h"
#include "start_db.h"
#include "protos.h"

extern int opterr;
extern char *optarg;

#define MAX_UTRIES 2
#define UPDATE_WAIT 30

extern int errno;

status_t check_database PROTO((char **, char *, int));

#define WEB 1
#define FTP 2

int verbose = 0;
char *prog;


/*
 * fix_start_db: generate stats about the archie catalogs
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
  pathname_t start_database_dir;

  
  char *database[2];
  char *dbdir;  
  char *databases_list[] = {
    "anonftp", "webindex", NULL
  };
  char **databases, **db;
  
  file_info_t *hostbyaddr = create_finfo();
  file_info_t *hostdb = create_finfo(); 
  file_info_t *hostaux_db = create_finfo();

  /* host databases */

  pathname_t logfile;
  hostname_t hostname,port;
  char * (*func)();
  
  int logging = 0;

  int count = 0;

  char **file_list = (char **) NULL;

  logfile[0] = hostname[0] = port[0] = '\0';

  databases = databases_list;
  
  start_database_dir[0] = files_database_dir[0] = host_database_dir[0] = master_database_dir[0] = wfiles_database_dir[0] = '\0';

  opterr = 0;                   /* turn off getopt() error messages */

  /* ignore argv[0] */

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;

  prog = argv[0];

  while((option = (int) getopt(argc, argv, "H:d:h:M:L:lvp:s:")) != EOF){

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

      /* change the name of the start_db dir */
    case 's':
     sprintf(start_database_dir, "%s/%s%s/%s%s",  get_master_db_dir(), 
            dbdir, DB_SUFFIX, DEFAULT_START_DB_DIR, DB_SUFFIX);
      strcpy(start_database_dir,optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
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
      error(A_ERR,"fix_start_db","Usage: [-p] [-d <debug level>] [-w <data directory>] <sitename>");
      exit(A_OK);
      break;
    }

  }

  /* set up logs */
   
  if(logging){  
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /*  "Can't open default log file" */

        error(A_ERR, "fix_start_db","Can't open default log file" );
        exit(A_OK);
      }
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, "fix_start_db", "Can't open log file %s", logfile);
        exit(A_OK);
      }
    }
  }


  /* Set up system directories and files */


  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR,"fix_start_db",  "Error while trying to set master database directory" );
    exit(A_OK);
  }


  if(set_host_db_dir(host_database_dir) == (char *) NULL){
    /* "Error while trying to set host database directory" */
    error(A_ERR,"fix_start_db","Error while trying to set host database directory");
    exit(A_OK);
  }

  if(open_host_dbs(hostbyaddr, hostdb, (file_info_t *) NULL, hostaux_db, O_RDONLY) != A_OK){

    /* "Error while trying to open host database" */

    error(A_ERR,"fix_start_db","Error while trying to open host database" );
    exit(A_OK);
  }


  for(db = databases; *db != NULL && *db[0] != '\0'; db++ ) {
    int which = 0;
    
    if( !strcmp( *db , ANONFTP_DB_NAME )){
      which = FTP;
      if((dbdir = set_files_db_dir(files_database_dir)) == (char *) NULL){
        /* "Error while trying to set anonftp database directory" */
        error(A_ERR, "fix_start_db", "Error while trying to set database directory");
        return(A_OK);
      }
    }else if( !strcmp( *db , WEBINDEX_DB_NAME )){
      which = WEB;
      if((dbdir = set_wfiles_db_dir(wfiles_database_dir)) == (char *) NULL){
        /* "Error while trying to set anonftp database directory" */
        error(A_ERR, "fix_start_db","Error while trying to set database directory");
        return(A_OK);
      }
    }
    else {
        error(A_ERR, "fix_start_db","Unknown Database: %s ",*db);
        return(A_OK);
    }

    if(set_start_db_dir(start_database_dir, *db) == (char *) NULL){

      /* "Error while trying to set master database directory" */

      error(A_ERR,"db_dump",  "Error while trying to set start database directory" );
      exit(A_OK);
    }
    
    if ( verbose )
    error(A_INFO, "fix_start_db", "Processing database %s", *db);



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
        error(A_ERR, "fix_start_db", "Can't get hostdb record ");
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

        error(A_ERR, "fix_start_db", "Can't get list of files in directory %s", func());
        exit(ERROR);
      }
      
    }
    else{

      if(get_file_list(func(), &file_list, (char *) NULL, &count) == ERROR){

        /* "Can't get list of files in directory %s" */

        error(A_ERR, "fix_start_db", "Can't get list of files in directory %s", get_files_db_dir());
        exit(ERROR);
      }
        
    }

    check_database(file_list,dbdir,which ); 


  }

  close_host_dbs(hostbyaddr, hostdb, (file_info_t *) NULL, hostaux_db);
  
  
  exit(A_OK);
  return(A_OK);

}





status_t check_entry(  startdb, hindex, kindex, file, recno )
  file_info_t *startdb;
  host_table_index_t hindex;
  index_t kindex;
  char *file;
  int recno;
{

  host_table_index_t index_list[MAX_LIST_SIZE];
  int i,length;
  
  if ( get_index_start_dbs(startdb, kindex, index_list, &length) == ERROR ) {
    error(A_ERR,"fix_start_db", "Site (%s) Record (%d): Cannot get list of sites for the given start",file,recno);
    return(ERROR);
  }

  for ( i = 0; i < length && index_list[i] != END_LIST; i++)  {

    if ( index_list[i]  == hindex )
    return A_OK;
  }

  if ( i == 0 && verbose ) {
    if ( length == 0 )
    error(A_INFO,"check_entry","Empty list for start %d",kindex);
    else
    error(A_INFO,"check_entry","No start list for start %d",kindex);
  }
  if ( update_start_dbs(startdb,kindex, hindex, ADD_SITE) == ERROR ) {
    error(A_ERR,"fix_start_db", "Site (%s) Record (%d): Cannot be fixed.",file,recno);
    return ERROR;
  }
  if ( verbose)
  error(A_INFO,"fix_start_db", "Site (%s) Record (%d):  Start (%d) Fixed !!",file,recno,kindex);
  return A_OK;
}


                     



status_t check_database(file_list, dbdir, which)
   char **file_list;
  char *dbdir;
  int which;
{
  /* File information */

  char **curr_file;

  ip_addr_t ipaddr = 0;

  int tmp_val;
  int act_size;
  int flag;
  int mode;
  
  full_site_entry_t *curr_ent;

  int i,tries,finished;
  host_table_index_t hindex;
  index_t curr_strings;
  
  /*  struct arch_stridx_handle *strhan; */
  file_info_t *startdb = create_finfo();
  file_info_t *domaindb = create_finfo();
  file_info_t *curr_finfo = create_finfo();
  file_info_t *lock_file = create_finfo();

  ptr_check(file_list, char*, "check_indiv", ERROR);

  mode = O_RDWR;
  if (which == WEB )
  sprintf(lock_file -> filename,"%s/%s/%s", get_master_db_dir(), DEFAULT_LOCK_DIR, WEBINDEX_LOCKFILE);
  if ( which == FTP)
  sprintf(lock_file -> filename,"%s/%s/%s", get_master_db_dir(), DEFAULT_LOCK_DIR, ANONFTP_LOCKFILE);

  for(tries = 0, finished = 0; (tries < MAX_UTRIES) && !finished; tries++){


    if(access(lock_file -> filename, F_OK) == 0){

      /* lock file exists */

      /* "Waiting for lock file %s" */

      error(A_INFO, prog, "Waiting for lock file %s" , lock_file -> filename);
      sleep(UPDATE_WAIT);
    }
    else{

      if(errno != ENOENT){

        /* "Error while checking for lockfile %s" */

        error(A_SYSERR, prog, "Error while checking for lockfile %s" , lock_file -> filename);

        exit(ERROR);
      }
	    
      finished = 1;

      if(open_file(lock_file, O_WRONLY) == ERROR){

        /* "Can't open lock file %s" */

        error(A_ERR,prog, "Can't open lock file %s", lock_file -> filename);

        exit(ERROR);
      }

      /* "Update for %s (%s) at %s" */
	 
      fprintf(lock_file -> fp_or_dbm.fp, "fix_start_db is running with fix option");
      fflush(lock_file -> fp_or_dbm.fp);
    }
    

    if(!finished){

      /* "Giving up after %d tries to update %s" */

      error(A_ERR,prog, "Giving up after %d tries to run fix_start_db" , tries);

      exit(ERROR);
    }

    
  }
  
  if ( open_start_dbs(startdb, domaindb, mode) == ERROR ) {
    error(A_ERR, "fix_start_db", "Unable to open start_db");
    return ERROR;
  }

#if 0
  if ( !(strhan = archNewStrIdx()) ) {
    error( A_WARN, "fix_start_db", "Could not create string handle" );
    return(ERROR);
  }
  
  if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_SEARCH  ) ) {
    error( A_WARN, "fix_start_db", "Could not find strings_idx files aborting." );
    return(ERROR);
  }

#endif
  
  for(curr_file = file_list; *curr_file != (char *) NULL; curr_file++){
    pathname_t filename;
    char *port;
    hostname_t host;
    int port_no;

    if(sscanf(*curr_file, "%*d.%*d.%*d.%d:%*d", &tmp_val) != 1)
    continue;

    if ( strrcmp(*curr_file,".excerpt") == 0 )
    continue;

    if ( strrcmp(*curr_file,".idx") == 0 )
    continue;

    strcpy(filename,*curr_file);
    port = strchr(filename,':');
    if ( port == NULL )
    continue;
    *port++ = '\0';
    ipaddr = inet_addr(filename);

    host[0] = '\0';
    port_no = atoi(port);
    hindex = -1;
    if ( host_table_find(&ipaddr, host, &port_no, &hindex) == ERROR ) {
      error(A_ERR, "fix_start_db", "Unable to find host with IP address %s", filename);
      continue;
    }
    if ( which == WEB ) 
    strcpy(curr_finfo -> filename, wfiles_db_filename(filename,atoi(port)));
    else
    strcpy(curr_finfo -> filename, files_db_filename(filename,atoi(port)));
    /*    if ( verbose)
          error(A_INFO,"fix_start_db", "Processing file %s",*curr_file);
          */
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
    curr_ent = (full_site_entry_t *) curr_finfo -> ptr + 0;
    flag = 0;
    for ( i = 0; i < act_size;  i++, curr_ent++) { 


      
      if (! CSE_IS_SUBPATH((*curr_ent)) &&
          ! CSE_IS_PORT((*curr_ent)) &&
          ! CSE_IS_NAME((*curr_ent)) )  {

        curr_strings = curr_ent-> strt_1;
        if ( curr_strings > 0 )

        if ( check_entry( startdb, hindex, curr_strings, *curr_file, i) == ERROR ) {
          flag = 1;
        }
      }
    }
    if ( flag == 0 && verbose )
    error(A_INFO,"fix_start_db","Site (%s): The information is consitent",*curr_file);


    close_file(curr_finfo);

  }

  close_file(lock_file);
  unlink(lock_file->filename);
  destroy_finfo(lock_file);

  if ( close_start_dbs(startdb, domaindb) == ERROR ) {
    error(A_ERR, "fix_start_db", "Unable to close start_db");
    return ERROR;
  }
  

  return(A_OK);
}





