/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <memory.h>
#include "typedef.h"
#include "db_files.h"
#include "webindexdb_ops.h"
#include "host_db.h"
#include "error.h"
#include "files.h"
#include "archie_strings.h"
#include "archie_dbm.h"
#include "check_webindex.h"
#include "lang_webindex.h"
#include "debug.h"
#include "master.h"
#include "core_entry.h"

#include "site_file.h"
#include "start_db.h"

#include "patrie.h"
#include "archstridx.h"

#include "strings_index.h"
#include "protos.h"

int verbose = 0;

char *prog;

/*
 * check_webindex: perform consistency checking on webindex database
 * 
 * 

   argv, argc are used.

   Parameters:	  -w <webindex files database pathname>
		  -M <master database pathname>
		  -h <host database pathname>
		  -v verbose mode
		  -l write to log file
		  -L <log file>
		  
*/


int main(argc, argv)
   int argc;
   char *argv[];

{
#if 0
#ifdef __STDC__

/*  extern int getopt(int, char **, char *); */
  extern status_t get_port(access_comm_t, char *, int *);
/*  extern status_t get_input_file(hostname_t, char *, char *(*f)(), file_info_t *, hostdb_t *, hostdb_aux_t *, file_info_t *, file_info_t *, file_info_t *);     */
#else

  extern int getopt();
  extern status_t get_port();
  extern status_t get_input_file();  

#endif
#endif
  extern int opterr;
  extern char *optarg;

  char **cmdline_ptr;
  int cmdline_args;

  int option;

  /* Directory names */

  pathname_t master_database_dir;
  pathname_t wfiles_database_dir;
  pathname_t start_database_dir;
  pathname_t host_database_dir;
  char *dbdir;

  file_info_t *curr_finfo = create_finfo();

  file_info_t *hostbyaddr = create_finfo();
  file_info_t *hostdb = create_finfo();
  file_info_t *hostaux_db = create_finfo();

  /*startdb stuff*/
  file_info_t *start_db = create_finfo();
  file_info_t *domain_db = create_finfo();
  /*startdb stuff*/

  struct arch_stridx_handle *strhan;

  /* host databases */

  hostdb_t hostdb_rec;
  hostdb_aux_t hostaux_rec;

  pathname_t logfile;
  hostname_t hostname;

  int logging = 0;

  int port = 0;
  int count = 0;

  char **file_list = (char **) NULL;

  prog = argv[0];
  logfile[0] = hostname[0] = '\0';
  wfiles_database_dir[0] = start_database_dir[0] = host_database_dir[0] = master_database_dir[0] = '\0';
  opterr = 0;                   /* turn off getopt() error messages */

  /* ignore argv[0] */

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;
  wfiles_database_dir[0] = host_database_dir[0] = '\0';

  while((option = (int) getopt(argc, argv, "H:p:w:h:M:L:lv")) != EOF){

    switch(option){


      /* hostname to be checked */ 

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

      /* verbose mode */

    case 'v':
	    verbose = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* Get files database directory name, argument "w" */

    case 'w':
	    strcpy(wfiles_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;


    default:
	    error(A_ERR,"check_webindex","Usage: [-p] [-d <debug level>] [-w <data directory>] <sitename>");
	    exit(A_OK);
	    break;
    }

  }

  /* set up logs */
   
  if(logging){  
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /*  "Can't open default log file" */

        error(A_ERR, "check_webindex", CHECK_WEBINDEX_001);
        exit(A_OK);
      }
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, "check_webindex", CHECK_WEBINDEX_002, logfile);
        exit(A_OK);
      }
    }
  }


  /* Set up system directories and files */


  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR,"check_webindex", CHECK_WEBINDEX_003);
    exit(A_OK);
  }

  if(set_start_db_dir(start_database_dir,DEFAULT_WFILES_DB_DIR) == (char *) NULL){

    /* "Error while trying to set start database directory" */

    error(A_ERR, "check_webindex", "Error while trying to set start database directory\n");
    exit(A_OK);
  }

  if( (dbdir = set_wfiles_db_dir(wfiles_database_dir)) == (char *)NULL) {

    /* "Error while trying to set webindex database directory" */

    error(A_ERR, "check_webindex", INSERT_WEBINDEX_022);
    exit(A_OK);
  }

  if(set_host_db_dir(host_database_dir) == (char *) NULL){

    /* "Error while trying to set host database directory" */

    error(A_ERR,"check_webindex", CHECK_WEBINDEX_005);
    exit(A_OK);
  }

  /* Open other files database files */

  if ( open_start_dbs(start_db,domain_db,O_RDONLY ) == ERROR ) {

    /* "Can't open start/host database" */

    error(A_ERR, "inser_webindex", "Can't open start/host database");
    exit(A_OK);
  }

  if ( !(strhan = archNewStrIdx()) ) {
    error(A_WARN, "check_webindex","Could not create string handler");
    exit(A_OK);
  }

  if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_SEARCH ) )
  {
    error( A_WARN, "check_webindex", "Could not find strings_idx files, abort.\n" );
    exit(A_OK);
  }

  if(open_host_dbs(hostbyaddr, hostdb, (file_info_t *) NULL, hostaux_db, O_RDONLY) != A_OK){

    /* "Error while trying to open host database" */

    error(A_ERR,"check_webindex", CHECK_WEBINDEX_007);
    exit(A_OK);
  }

  if(hostname[0] != '\0'){
     index_t index;
     int i;

     find_hostaux_last(hostname, WEBINDEX_DB_NAME, &index, hostaux_db);

     if ( index == -1 ) {
       /* "Can't find host %s in webindex database" */

       error(A_ERR, "check_webindex", CHECK_WEBINDEX_008, hostname);
       exit(ERROR);
     }

     if((file_list = (char **) malloc( (2+(int)index) * sizeof(char *))) == (char **) NULL){

       /* "Can't malloc space for file list" */

       error(A_ERR, "check_webindex", CHECK_WEBINDEX_009);
       exit(ERROR);
     }

     for ( i = 0; i <= (int)index; i++ ) {
     
       if(get_input_file(hostname, WEBINDEX_DB_NAME, (index_t)i, wfiles_db_filename, curr_finfo, &hostdb_rec, &hostaux_rec, hostdb, hostaux_db, hostbyaddr) == ERROR){

         /* "Can't find host %s in webindex database" */

         error(A_ERR, "check_webindex", CHECK_WEBINDEX_008, hostname);
         exit(ERROR);
       }


       file_list[i] = strdup(inet_ntoa(ipaddr_to_inet(hostdb_rec.primary_ipaddr)));
     }
     count = (int)index+1;
     file_list[(int)index+1] = (char *) NULL;
#ifdef 0
    if(get_input_file(hostname, WEBINDEX_DB_NAME, wfiles_db_filename, curr_finfo, &hostdb_rec, &hostaux_rec, hostdb, hostaux_db, hostbyaddr) == ERROR){

      /* "Can't find host %s in webindex database" */

      error(A_ERR, "check_webindex", CHECK_WEBINDEX_008, hostname);
      exit(ERROR);
    }

    if((file_list = (char **) malloc( 2 * sizeof(char *))) == (char **) NULL){

      /* "Can't malloc space for file list" */

      error(A_ERR, "check_webindex", CHECK_WEBINDEX_009);
      exit(ERROR);
    }

    file_list[0] = strdup(inet_ntoa(ipaddr_to_inet(hostdb_rec.primary_ipaddr)));
    file_list[1] = (char *) NULL;
    count = 1;
#endif
  }
  else{

    if(get_file_list(get_wfiles_db_dir(), &file_list, (char *) NULL, &count) == ERROR){

      /* "Can't get list of files in directory %s" */

      error(A_ERR, "check_webindex", CHECK_WEBINDEX_010, get_wfiles_db_dir());
      exit(ERROR);
    }
  }

  if(port==0){

    /* "No port number given" */

    error(A_ERR,"check_webindex","No port number given");
    if( get_port((char *)NULL, WEBINDEX_DB_NAME, &port ) == ERROR ){
      error(A_ERR,"get_port","Could not get port for site");
      exit(A_OK);
    }
  }

  check_indiv(file_list, strhan, hostbyaddr, hostaux_db, port);
  close_start_dbs(start_db,domain_db);

  exit(A_OK);
  return(A_OK);

}

status_t check_indiv(file_list, strhan, hostbyaddr, hostaux_db, port)
  char **file_list;
  struct arch_stridx_handle *strhan;
  file_info_t *hostbyaddr;
  file_info_t *hostaux_db;
  int port;
{
  /* File information */

  char **curr_file;
  char *result;

  ip_addr_t ipaddr = 0;
  hostbyaddr_t hba_ent;
  int act_size = 0;

  int tmp_val;

  hostdb_aux_t hostaux_ent;

  index_t curr_strings;
  index_t curr_count;

  full_site_entry_t *curr_ent;
  full_site_entry_t *curr_endptr;

  full_site_entry_t *tmp_ent;

  chklist_t *chklist;

  file_info_t *curr_finfo = create_finfo();

  ptr_check(file_list, char*, "check_indiv", ERROR);
  ptr_check(hostbyaddr, file_info_t, "check_indiv", ERROR);

  /* go through each file in file_list */

  for(curr_file = file_list; *curr_file != (char *) NULL; curr_file++){
    index_t index;
    int i;

    if(sscanf(*curr_file, "%*d.%*d.%*d.%d", &tmp_val) != 1)
    continue;

    ipaddr = inet_addr(*curr_file);

    if(get_dbm_entry(&ipaddr, sizeof(ip_addr_t), &hba_ent, hostbyaddr) == ERROR){

      /* "Can't get host address cache entry for %s" */

      error(A_INTERR, "check_indiv", CHECK_INDIV_001);
      continue;
    }

    find_hostaux_last(hba_ent.primary_hostname,WEBINDEX_DB_NAME, &index, hostaux_db);
    if ( index == -1 ) {
      /* "Ignoring %s" */
	 
      error(A_ERR, "check_indiv", CHECK_INDIV_003, *curr_file);
      continue;
    }
    
    for ( i = 0; i <= (int) index; i++ ) {
      
      if(get_hostaux_entry(hba_ent.primary_hostname, WEBINDEX_DB_NAME, (index_t)i,&hostaux_ent, hostaux_db) == ERROR){

        /* "Ignoring %s" */
	 
        error(A_ERR, "check_indiv", CHECK_INDIV_003, *curr_file);
        continue;
      }
#ifdef 0    
      if(get_hostaux_ent(hba_ent.primary_hostname, WEBINDEX_DB_NAME, &hostaux_ent, hostaux_db) == ERROR){

        /* "Ignoring %s" */
	 
        error(A_ERR, "check_indiv", CHECK_INDIV_003, *curr_file);
        continue;
      }
#endif

      if(verbose){

        /* "Checking site %s (%s)" */

        error(A_INFO,"check_indiv", CHECK_INDIV_002, hba_ent.primary_hostname, *curr_file);
      }

      strcpy(curr_finfo -> filename, wfiles_db_filename(*curr_file,port));

      /* Open current file */

      if(open_file(curr_finfo, O_RDONLY) == ERROR){

        /* "Ignoring %s" */
	 
        error(A_ERR, "check_indiv", CHECK_INDIV_003, *curr_file);
        continue;
      }

      /* mmap it */

      if(mmap_file(curr_finfo, O_RDONLY) == ERROR){

        /* "Ignoring %s" */
	 
        error(A_ERR, "check_indiv", CHECK_INDIV_003, *curr_file);
        continue;
      }

      act_size = curr_finfo -> size / sizeof(full_site_entry_t);

      if(hostaux_ent.site_no_recs != act_size){

        /* "Number of records in site file %s (%s) %d does not match auxiliary host database record %d" */

        error(A_WARN, "check_indiv", CHECK_INDIV_004, hba_ent.primary_hostname, *curr_file, hostaux_ent.no_recs, act_size);

      }

      if((chklist = (chklist_t *) malloc(act_size * sizeof(chklist_t))) == (chklist_t *) NULL){

        /* "Can't malloc space for checklist" */

        error(A_SYSERR, "check_indiv", CHECK_INDIV_005);
        return(ERROR);
      }
	  
      memset((char *)chklist, '\0', act_size * sizeof(chklist_t));

      /* Go through each entry in current site file */

      for(curr_ent = (full_site_entry_t *) curr_finfo -> ptr, curr_endptr = curr_ent + act_size, curr_count = 0;
          curr_ent < curr_endptr;
          curr_ent++){

        curr_strings = curr_ent -> strt_1;

        /* Check if string offset is valid */

        if(curr_strings < (index_t)(0)){
          /* "Site %s (%s) record %d has string index %d out of bounds" */
          error(A_ERR, "check_indiv", CHECK_INDIV_008, hba_ent.primary_hostname, *curr_file, curr_count, curr_strings);
        }else{
          fprintf(stdout,"\nrecno: %li\n",curr_count);
          fprintf(stdout,"flags: %d \n", curr_ent -> flags);
          fprintf(stdout,"string_idx: %li \n", curr_strings);
          if ( CSE_IS_DOC((*curr_ent)) ) 
            fprintf(stdout,"          excerpt recno: %d \n", curr_ent->core.entry.perms);

          if ( CSE_IS_SUBPATH((*curr_ent)) ||
              CSE_IS_PORT((*curr_ent)) ||
              CSE_IS_NAME((*curr_ent)) ) 
          {
            fprintf(stdout,"          original location: %li \n",curr_ent -> core.prnt_entry.strt_2);
            fprintf(stdout,"          parent: %li \n",curr_ent -> strt_1);

            if( curr_ent -> core.prnt_entry.strt_2 >= (index_t)(0)) 
            {   tmp_ent = ((full_site_entry_t *) curr_finfo -> ptr)+(curr_ent -> core.prnt_entry.strt_2);
                if( (tmp_ent->strt_1 < (index_t)(0) )|| !archGetString( strhan,  tmp_ent->strt_1, &result))
                {               /* "Site %s (%s) record %d has string index %d out of bounds" */
                  error(A_ERR, "check_indiv", CHECK_INDIV_008, hba_ent.primary_hostname, *curr_file, curr_count, curr_strings);
                }else
                {      fprintf(stdout,"          original:%s \n",(char *)(result) );
                       free(result);
                     }
              }
          }else
          {
            if( !CSE_IS_KEY((*curr_ent)) ){
              fprintf(stdout,"          size:  %li  date: %li \n", curr_ent -> core.entry.size, curr_ent -> core.entry.date);
                            fprintf(stdout,"          retrieve date: %li \n",  curr_ent -> core.entry.rdate);
              fprintf(stdout,"          perms: %o \n",(curr_ent -> core.entry.perms));
            }else{
              fprintf(stdout,"          Weight:  %f \n", curr_ent -> core.kwrd.weight);
            }
            if( (curr_strings < (index_t)(0) )|| !archGetString( strhan,  curr_strings, &result))
            {                   /* "Site %s (%s) record %d has string index %d out of bounds" */
              error(A_ERR, "check_indiv", CHECK_INDIV_008, hba_ent.primary_hostname, *curr_file, curr_count, curr_strings);
            }else
            fprintf(stdout,"          String = %s\n",(char *)result);
          }
          /* Check "next" links */
        }
        curr_count++;
      }
    }
    free(chklist);
    close_file(curr_finfo);
  }

  archCloseStrIdx(strhan);
  archFreeStrIdx(&strhan);
  return(A_OK);
}

