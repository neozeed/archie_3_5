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
 * db_check: generate stats about the archie catalogs
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
  pathname_t key;
  int key_flag = 0;
  
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

  while((option = (int) getopt(argc, argv, "H:d:h:M:L:lvp:k:")) != EOF){

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

    case 'k':
      strcpy(key,optarg);
      key_flag = 1;
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
      error(A_ERR,"db_check","Usage: [-p] [-d <debug level>] [-w <data directory>] <sitename>");
      exit(A_OK);
      break;
    }

  }

  /* set up logs */
   
  if(logging){  
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /*  "Can't open default log file" */

        error(A_ERR, "db_check","Can't open default log file" );
        exit(A_OK);
      }
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, "db_check", "Can't open log file %s", logfile);
        exit(A_OK);
      }
    }
  }


  /* Set up system directories and files */


  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR,"db_check",  "Error while trying to set master database directory" );
    exit(A_OK);
  }


  if(set_host_db_dir(host_database_dir) == (char *) NULL){
    /* "Error while trying to set host database directory" */
    error(A_ERR,"db_check","Error while trying to set host database directory");
    exit(A_OK);
  }

  if(open_host_dbs(hostbyaddr, hostdb, (file_info_t *) NULL, hostaux_db, O_RDONLY) != A_OK){

    /* "Error while trying to open host database" */

    error(A_ERR,"db_check","Error while trying to open host database" );
    exit(A_OK);
  }


  for(db = databases; *db != NULL && *db[0] != '\0'; db++ ) {
    int which = 0;
    
    if( !strcmp( *db , ANONFTP_DB_NAME )){
      which = FTP;
      if((dbdir = set_files_db_dir(files_database_dir)) == (char *) NULL){
        /* "Error while trying to set anonftp database directory" */
        error(A_ERR, "db_check", "Error while trying to set database directory");
        return(A_OK);
      }
    }else if( !strcmp( *db , WEBINDEX_DB_NAME )){
      which = WEB;
      if((dbdir = set_wfiles_db_dir(wfiles_database_dir)) == (char *) NULL){
        /* "Error while trying to set anonftp database directory" */
        error(A_ERR, "db_check","Error while trying to set database directory");
        return(A_OK);
      }
    }
    else {
        error(A_ERR, "db_check","Unknown Database: %s ",*db);
        return(A_OK);
    }

    if(set_start_db_dir(start_database_dir, *db) == (char *) NULL){

      /* "Error while trying to set master database directory" */

      error(A_ERR,"db_dump",  "Error while trying to set start database directory" );
      exit(A_OK);
    }
    
    if ( verbose )
    error(A_INFO, "db_check", "Processing database %s", *db);



    switch(which) {
    case WEB:
      func = get_wfiles_db_dir;
      break;
      
    case FTP:
      func = get_files_db_dir;
      break;
    }

    if ( key_flag ) {
      check_key(func(),key);
      continue;
    }

    if(hostname[0] != '\0'){
      hostdb_t hostdb_rec;
      pathname_t path;

      if ( get_dbm_entry(hostname,strlen(hostname)+1,&hostdb_rec, hostdb) == ERROR ){
        error(A_ERR, "db_check", "Can't get hostdb record ");
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

        error(A_ERR, "db_check", "Can't get list of files in directory %s", func());
        exit(ERROR);
      }
      
    }
    else{

      if(get_file_list(func(), &file_list, (char *) NULL, &count) == ERROR){

        /* "Can't get list of files in directory %s" */

        error(A_ERR, "db_check", "Can't get list of files in directory %s", get_files_db_dir());
        exit(ERROR);
      }
        
    }

    check_database(file_list,dbdir,which ); 


  }

  
  exit(A_OK);
  return(A_OK);

}





status_t check_entry( strhan, startdb, hindex, kindex, file, recno )
  struct arch_stridx_handle *strhan;
  file_info_t *startdb;
  host_table_index_t hindex;
  index_t kindex;
  char *file;
  int recno;
{

  char *word;
  unsigned long starts[2];
  unsigned long nhits;
  host_table_index_t index_list[MAX_LIST_SIZE];
  int i,length;
  
  if ( !archGetString(strhan, (long)kindex, &word ) ) {
    error(A_ERR,"db_check", "Site (%s) Record (%d): Cannot get word with index %ld",file, recno, kindex);
    return ERROR;
  }

  nhits = 0;
  if ( !archSearchExact(strhan, word, 1, 1, &nhits, starts ) ){
    error(A_ERR,"db_check", "Site (%s) Record (%d): Cannot find word %s in the index",file,recno, word);
    return ERROR;
  }

    
  if ( nhits != 1 ) {
    error(A_ERR,"db_check", "Site (%s) Record (%d): Cannot find word %s in the index, number of hits",file, recno, word, nhits);
    return ERROR;
  }


  if ( starts[0] != kindex ) {
    error(A_ERR,"db_check", "Site (%s) Record (%d): The index and record do not point to the same word %d != %d",file,recno,starts[0],kindex);
  }
  
  if ( get_index_start_dbs(startdb, starts[0], index_list, &length) == ERROR ) {
    error(A_ERR,"db_check", "Site (%s) Record (%d): Cannot get list of sites for the given start",file,recno);
    return(ERROR);
  }

  for ( i = 0; index_list[i] != END_LIST && i < length; i++)  {

    if ( index_list[i]  == hindex )
    return A_OK;
  }


  error(A_ERR,"db_check", "Site (%s) Record (%d): The list of starts do not contain a reference to this site, run fix_start_db ",file,recno);
  return ERROR;
  
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
  int mode = O_RDONLY;
  
  full_site_entry_t *curr_ent;

  int i;
  host_table_index_t hindex;
  index_t curr_strings;
  
  struct arch_stridx_handle *strhan;
  file_info_t *startdb = create_finfo();
  file_info_t *curr_finfo = create_finfo();

  ptr_check(file_list, char*, "check_indiv", ERROR);

  if ( open_start_dbs(startdb, NULL, mode) == ERROR ) {
    error(A_ERR, "db_check", "Unable to open start_db");
    return ERROR;
  }

  if ( !(strhan = archNewStrIdx()) ) {
    error( A_WARN, "db_check", "Could not create string handle" );
    return(ERROR);
  }
  
  if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_SEARCH  ) ) {
    error( A_WARN, "db_check", "Could not find strings_idx files aborting." );
    return(ERROR);
  }

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
      error(A_ERR, "db_check", "Unable to find host with IP address %s", filename);
      continue;
    }
    if ( which == WEB ) 
    strcpy(curr_finfo -> filename, wfiles_db_filename(filename,atoi(port)));
    else
    strcpy(curr_finfo -> filename, files_db_filename(filename,atoi(port)));
/*    if ( verbose)
      error(A_INFO,"db_check", "Processing file %s",*curr_file);
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

        if ( check_entry(strhan, startdb, hindex,curr_strings, *curr_file, i) == ERROR ) {
          flag = 1;
        }
     }
    }
    if ( flag == 0 && verbose )
      error(A_INFO,"db_check","Site (%s): The information is consitent",*curr_file);


    close_file(curr_finfo);

  }


  return(A_OK);
}





status_t find_entry( strhan, startdb, kindex, key)
  struct arch_stridx_handle *strhan;
  file_info_t *startdb;
  index_t kindex;
  char *key;
{

  char *word;
  unsigned long starts[2];
  unsigned long nhits;
  host_table_index_t index_list[MAX_LIST_SIZE];
  int i,length;
  
  if ( key == NULL ) {
  
    if ( !archGetString(strhan, (long)kindex, &word ) ) {
      error(A_ERR,"db_check", "Cannot get word with index %ld", kindex);
      return ERROR;
    }

    nhits = 0;
    if ( !archSearchExact(strhan, word, 1, 1, &nhits, starts ) ){
      error(A_ERR,"db_check", "Cannot find word %s in the index", word);
      return ERROR;
    }
  }
  else {
    nhits = 0;
    if ( !archSearchExact(strhan, key, 1, 1, &nhits, starts ) ){
      error(A_ERR,"db_check", "Cannot find word %s in the index", word);
      return ERROR;
    }
    if ( nhits != 1 ) {
      error(A_ERR,"db_check", "Cannot find word %s in the index, number of hits%d", word, nhits);
      return ERROR;
    }
    
    if ( !archGetString(strhan, (long)starts[0], &word ) ) {
      error(A_ERR,"db_check", "Cannot get word with index %ld", kindex);
      return ERROR;
    }


  }

    
  if ( nhits != 1 ) {
    error(A_ERR,"db_check", "Cannot find word %s in the index, number of hits %d", word, nhits);
    return ERROR;
  }


  if ( starts[0] != kindex ) {
    error(A_ERR,"db_check", "The index and record do not point to the same word %d != %d",starts[0],kindex);
  }
  
  if ( get_index_start_dbs(startdb, starts[0], index_list, &length) == ERROR ) {
    error(A_ERR,"db_check", "Cannot get list of sites for the given start");
    return(ERROR);
  }

  for ( i = 0; index_list[i] != END_LIST && i < length; i++)  {

    error(A_INFO,"db_check", "Word is in site %d",index_list[i]);
  }
  
  return A_OK;

}


status_t check_key(dbdir,key)
  char * dbdir;
  pathname_t key;
{
  /* File information */

  struct arch_stridx_handle *strhan;
  file_info_t *startdb = create_finfo();




  if ( open_start_dbs(startdb, NULL, O_RDONLY) == ERROR ) {
    error(A_ERR, "db_check", "Unable to open start_db");
    return ERROR;
  }

  if ( !(strhan = archNewStrIdx()) ) {
    error( A_WARN, "db_check", "Could not create string handle" );
    return(ERROR);
  }
  
  if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_SEARCH  ) ) {
    error( A_WARN, "db_check", "Could not find strings_idx files aborting." );
    return(ERROR);
  }


  find_entry( strhan, startdb, -1, key );
/*  archCloseStrIdx(); */

  return(A_OK);
}






