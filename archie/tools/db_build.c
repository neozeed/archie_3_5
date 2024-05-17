/*
 * This file is copyright Bunyip Information Systems Inc., 1994. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#if !defined(AIX) && !defined(SOLARIS)
#include <vfork.h>
#endif
#include <malloc.h>
#include <search.h>
#include <memory.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include "typedef.h"
#include "db_files.h"
#include "host_db.h"
#include "header.h"
#include "error.h"
#include "archie_dbm.h"
#include "archie_strings.h"
#include "files.h"
#include "master.h"
#include "archie_mail.h"
#include "protos.h"
#include "db_ops.h"
#include "patrie.h"
#include "archstridx.h"
#include "webindexdb_ops.h"

#define MAX_NON_INDEXED_SIZE 1000000


int signal_set = 0;	      /* For timeout alarm */

int verbose = 0;	      /* verbose mode */

int send_mail = 0;	      /* send mail */
char *prog;
pathname_t temp_dir;

int main(argc, argv)
   int argc;
   char *argv[];

{
  extern int opterr;
  extern char *optarg;

#if 0
#ifdef __STDC__

  extern int getopt(int, char **, char *);

#else

  extern int getopt();

#endif
#endif
   
  char **cmdline_ptr;
  int cmdline_args;

  int option;
  pathname_t master_database_dir;
  pathname_t host_database_dir;
  pathname_t tmp_dir;
  pathname_t files_database_dir;
  pathname_t wfiles_database_dir;
  pathname_t logfile;
  

  char *dbdir;  
  struct arch_stridx_handle *strhan;
  
  char *database[2];
  
  char *databases_list[] = {
    "anonftp", "webindex", NULL
  };

  char **databases, **db;
  
  int force = 0;
  int logging = 0;
  long indsize, strsize;
  int kilobytes = -1;
  
  host_database_dir[0] = '\0';
  master_database_dir[0] = '\0';
  files_database_dir[0] = '\0';
  wfiles_database_dir[0] = '\0';
  tmp_dir[0] = logfile[0] = '\0';
  /* disable error messages of getopt */

  databases = databases_list;

  opterr = 0;

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;

  prog = argv[0];

  while((option = (int) getopt(argc, argv, "h:M:L:vld:ft:k:")) != EOF){

    switch(option){

      /* master database directory */

    case 'M':
	    strcpy(master_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* logfile name */

    case 'L':
      strcpy(logfile, optarg);
	    logging = 1;
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* host database directory */

    case 'h':
	    strcpy(host_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* log output, default file */

    case 'l':
	    logging = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* leave data in original format */

    case 'd':
      database[0] = (char*)malloc(strlen(optarg)+1);
      strcpy(database[0],optarg);
      database[1] = NULL;
      databases = database;
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

    case 't':
      strcpy(tmp_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* Max amount of memory in kilobytes */
      
    case 'k':
      kilobytes = atoi(optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;
      
      /* verbose mode */

    case 'v':
	    verbose = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

    case 'f':
      force = 1;
      cmdline_ptr++;
      cmdline_args--;
      break;
      
    }

  }


  /* set up logs */

  if(logging){
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR)
	    exit(ERROR);
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR)
	    exit(ERROR);
    }
  }

  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR,"arcontrol", "Error while trying to set master database directory");
    exit(ERROR);
  }



  if(set_host_db_dir(host_database_dir) == (char *) NULL){

    /* "Error while trying to set host database directory" */

    error(A_ERR,"arcontrol","Error while trying to set host database directory"); 
    exit(ERROR);
  }


  for(db = databases; *db != NULL && *db[0] != '\0'; db++ ) {

    if ( verbose )
      error(A_INFO, "db_build", "Processing database %s", *db);
    
    if( !strcmp( *db , ANONFTP_DB_NAME )){
      if((dbdir = set_files_db_dir(files_database_dir)) == (char *) NULL){
        /* "Error while trying to set anonftp database directory" */
        error(A_ERR, "db_build", "Error while trying to set database directory");
        return(A_OK);
      }
    }else if( !strcmp( *db , WEBINDEX_DB_NAME )){
      if((dbdir = set_wfiles_db_dir(wfiles_database_dir)) == (char *) NULL){
        /* "Error while trying to set anonftp database directory" */
        error(A_ERR, "db_build","Error while trying to set database directory");
        return(A_OK);
      }
    }

    if ( ! (strhan = archNewStrIdx()) ) {
      return ERROR;
    }
    
    if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_SEARCH ) ) {
      error( A_WARN, "db_build", "Could not find strings_idx files aborting.\n" );
      return(ERROR);
    }

    if ( !archGetStringsFileSize(strhan, &strsize) ) {
      error(A_WARN, "db_build", "Could not get the Strings file size");
      return ERROR;
    }
    
    if ( ! archGetIndexedSize(strhan, &indsize ) )  {
      error(A_WARN, "db_build", "Could not get the size of the index");
      return ERROR;
    }

    archCloseStrIdx(strhan);
    archFreeStrIdx(&strhan);

    if ( strsize - indsize > MAX_NON_INDEXED_SIZE  || force )  {

      if ( verbose ) {
        if (force)
          error(A_INFO,"db_build","Forcing rebuild");
        else
          error(A_INFO, "db_build", "Building database index");
      }

      if ( !(strhan = archNewStrIdx()) ) {
        return ERROR;
      }

      if ( tmp_dir[0] != '\0' ) {
        if ( !archSetTempDir(strhan,tmp_dir) ) {
          error( A_WARN, "db_build", "Unable to set tmp directory %s",tmp_dir);
        }
      }
      if ( kilobytes > 0 ) {
        if ( !archSetBuildMaxMem(strhan, kilobytes * 1024) ) {
          error( A_WARN, "db_build", "Could not set maximum memory to use (%dbytes).",kilobytes*1024);
          return(ERROR);
        }
      }

      if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_BUILD ) ) {
        error( A_WARN, "db_build", "Could not find strings_idx files aborting.\n" );
        return(A_ERR);
      }
        

      if ( ! archBuildStrIdx(strhan)) {
        error( A_WARN, "db_build", "Could not update strings files.");
        return(ERROR);
      }
      archCloseStrIdx(strhan);
      archFreeStrIdx(&strhan);
    }
    else {
      if ( verbose) 
          error(A_INFO, "db_build", "Not rebuilding the database index");
    }
    
  }

  



  if(logging)
  close_alog();

  exit(A_OK);
  return(A_OK);
}

