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
/* #include "protos.h" */
#include "typedef.h"
#include "archie_dns.h"
#include "db_files.h"
#include "domain.h"
#include "header.h"
#include "db_ops.h"
#include "webindexdb_ops.h"
#include "host_db.h"
#include "error.h"
#include "files.h"
#include "archie_strings.h"
#include "archie_dbm.h"
#include "search.h"
#include "core_entry.h"
#include "debug.h"
#include "master.h"

#include "site_file.h"
#include "start_db.h"
#include "lang_startdb.h"
#include "patrie.h"
#include "archstridx.h"

#include "strings_index.h"
#include "get-info.h"

char *prog;
int verbose=1;

#ifndef SOLARIS
#ifdef __STDC__
#else
   extern status_t archQuery( );
   extern status_t archQueryMore( );
   extern status_t getResultFromStart( );
   extern status_t host_table_find();
   extern status_t update_start_dbs();
   extern status_t close_start_dbs();
   extern status_t open_start_dbs();
   extern status_t get_index_start_dbs();
   extern int archSearchSub();
   extern status_t get_port();
   extern int archWebBooleanQuery();
   extern int archWebBooleanQueryMore();
#endif
#endif

#define DEFAULT_QUERY_LOG_FILE "query.log"

/*
 *  interact: interacts with the databases through the CGI interface.
 *  
 */


int main(argc, argv)
int argc;
char *argv[];
{

#ifndef SOLARIS
#ifdef __STDC__

  extern status_t compile_domains( char *, domain_t *, file_info_t *, int *);
  extern int send_error_in_plain( char *);  
  extern char *dequoteString( const char  * );

#else

  extern int getopt();
  extern status_t compile_domains();
  extern int send_error_in_plain();
  extern char *dequoteString();

#endif
#endif

   
  extern int opterr;

  /* Directory names */

  pathname_t master_database_dir;
  pathname_t files_database_dir;
  pathname_t start_database_dir;
  pathname_t host_database_dir;

  /*startdb stuff*/
  file_info_t *start_db = create_finfo();
  file_info_t *domain_db = create_finfo();
  /*startdb stuff*/
  struct arch_stridx_handle *strhan;
  char *dbdir=NULL;
  int db = 0;

  pathname_t logfile;
  hostname_t hostname;

  entry *ents;
  domain_t d_list[MAX_NO_DOMAINS];
  int d_cnt = 0;


  int srch_type=0;
  int logging = 1; /* send all error messages to log file */
  bool_query_t qry;  /*used to be char *qry */
  char *han;
  int path_rel;
  int path_items;
  char **path_r; /* path restriction */
  int max_hits = 0;
  int ret_hits = 0;
  int match = 0;
  int hpm = 0;
  int type = EXACT;
  int booltype = 0;
  int case_sens = 0;
  int format = 0;
  start_return_t start_tbl;
  boolean_return_t more_tbl;

  char domains[MAX_PATH_LEN], *expath;
  query_result_t *result;
  index_t **strs;
  char *serv_url= NULL , *gif_url = NULL, *req = NULL;
  
  strcpy(domains,DOMAIN_ALL);
  expath = (char *)NULL;
  serv_url = (char *)NULL;
  start_tbl.stop = -1;
  start_tbl.string = -1;
  more_tbl.and_tpl = -1;
  path_r = NULL;
  path_rel = PATH_OR;
  path_items = 0;
  prog = argv[0];
  logfile[0] = hostname[0] = '\0';

  files_database_dir[0] = host_database_dir[0] = start_database_dir[0] = master_database_dir[0] = '\0';

  opterr = 0;                   /* turn off getopt() error messages */

  /* set up logs */
  if(logging){  
    if(logfile[0] == '\0'){
      sprintf(logfile, "%s/%s/%s", get_archie_home(), DEFAULT_LOG_DIR, DEFAULT_QUERY_LOG_FILE);
    }
    if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

      /* "Can't open log file %s" */

      error(A_ERR, prog, "Can't open log file %s.", logfile);
      exit(A_OK);
    }
  }

  if( !read_form_entries_post( &ents, &req)){
    error(A_ERR,"cgi-client","Could not get entries from form.");
    exit(A_OK);
  }else{
    error(A_INFO,"REQUEST:","(%s)",req);
  }
  free(req); req = NULL;
  
  if( !(srch_type = parse_entries(ents, &qry, &db, &type, &case_sens,
                                  &max_hits, &hpm, &match, &path_r, &expath,
                                  &path_rel, &path_items, domains, &serv_url,
                                  &gif_url, &han, &format, &booltype,
                                  &start_tbl, &more_tbl ))){
    error(A_ERR,prog,"Could not parse entries in form.");
    exit(A_OK);
  }

  if( strcmp( qry.string,"" )==0 ){
    send_error_in_plain("Your request is missing the query!");
    exit(A_OK);
  }
  if( serv_url == (char *)NULL ){
/*
 *  error(A_WARN,prog,"The HTML Page does not contain the location of the
 *                     archie gifs as hidden values.");
 *  send_error_in_plain("Your Search Page is missing the
 *                     url hidden value. Please fix your page."); return 0;
 */
  }
  if( serv_url ) {
    free(serv_url); serv_url = NULL;
  }
  if( gif_url ) {
    free(gif_url); gif_url = NULL;
  }
  
  /* Set up system directories and files */

  if(set_master_db_dir(master_database_dir) == (char *) NULL){
    /* "Error while trying to set master database directory" */
    send_error_in_plain("Error while trying to set master database directory.");
    error(A_ERR, prog, "Error while trying to set master database directory.");
    exit(A_OK);
  }

  if( db == I_WEBINDEX_DB){
    if((dbdir = (char *)set_wfiles_db_dir(files_database_dir)) == (char *) NULL){
      /* "Error while trying to set webindex database directory" */
      send_error_in_plain("Error while trying to set webindex database directory.");
      error(A_ERR, prog, "Error while trying to set webindex database directory");
      exit(A_OK);
    }
    if(set_start_db_dir(start_database_dir,DEFAULT_WFILES_DB_DIR) == (char *) NULL){
      /* "Error while trying to set start database directory" */
      send_error_in_plain("Error while trying to set start database directory.");
      error(A_ERR, prog, "Error while trying to set start database directory");
      exit(A_OK);
    }
  }else if( db == I_ANONFTP_DB){
    if((dbdir = (char *)set_files_db_dir(files_database_dir)) == (char *) NULL){
      /* "Error while trying to set anonftp database directory" */
      send_error_in_plain("Error while trying to set anonftp database directory.");
      error(A_ERR, prog, "Error while trying to set anonftp database directory");
      exit(A_OK);
    }
    if(set_start_db_dir(start_database_dir,DEFAULT_FILES_DB_DIR) == (char *) NULL){
      /* "Error while trying to set start database directory" */
      send_error_in_plain("Error while trying to set start database directory.");
      error(A_ERR, prog, "Error while trying to set start database directory");
      exit(A_OK);
    }
  }

  if( set_host_db_dir(host_database_dir) == (char *) NULL){
    /* "Error while trying to set host database directory" */
    send_error_in_plain("Error while trying to set host database directory.");
    error(A_ERR, prog, "Error while trying to set host database directory");
    exit(A_OK);
  }

  /* Open other files database files */

  if ( open_start_dbs(start_db,domain_db,O_RDONLY ) == ERROR ) {
    /* "Can't open start/host database" */
    send_error_in_plain("Error while trying to open start database directory.");
    error(A_ERR,prog,"Could not open start/host database.. exiting! Please inform the administrator of this site");
    exit(A_OK);
  }

  if ( !(strhan = archNewStrIdx()) ) {
    send_error_in_plain("Error while trying to get string handle.");
    error(A_ERR,prog,"Could not get string handle .. exiting! Please inform the administrator of this site");
    exit(A_OK);
  }
  
  if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_SEARCH ) ){
    send_error_in_plain("Error while trying to open strings files.");
    error(A_ERR,prog,"Could not open strings index files.. exiting! Please inform the administrator of this site");
    exit(A_OK);
  }

  if( srch_type == SEARCH_STRINGS_ONLY ){
    int html_format = 0;
    if (booltype == 0 ){
      strOnlyQuery(strhan, qry.string, type, case_sens, max_hits,
                   html_format );
      goto finish;
    }else{
      strOnlyBooleanQuery(strhan, qry, type, case_sens, max_hits,
                          0, &more_tbl, html_format );
      goto finish;
    }
  }

  if( compile_domains( domains, d_list, domain_db, &d_cnt) == ERROR ){
    error(A_ERR,"archQuery","Could not construct the domain list from domain_db");
    goto finish;
  }

  if( srch_type == SEARCH_MORE_STRINGS ){
    char errstr[MAX_STR], *deq_str;

    if (booltype == 0 ){
      if( han ) {
        deq_str = (char *)dequoteString(han);        
        free( han ); han = NULL;
      }else{
        sprintf(errstr,"Did not supply strhan string for more searches.");
        send_error_in_plain(errstr);
        error(A_ERR,prog,errstr);
        goto finish;
      }

      if (!archSetStateFromString(strhan,deq_str)){
        sprintf(errstr,"Could not convert the state string into strhan. String representing strhan = %s <P>",deq_str);
        free( deq_str );
        send_error_in_plain(errstr);
        error(A_ERR,prog,errstr);
        goto finish;
      }
      free( deq_str );
      if( archQueryMore( strhan, max_hits, match, hpm, path_r, expath,
                        path_rel, path_items, d_list, d_cnt, db, start_db,
                        0, &result, &strs, &ret_hits, &start_tbl,
                        (char *)NULL, 0, format) == ERROR){
        send_error_in_plain(" Could not complete archQueryMore()\n");
        error(A_ERR,prog," Could not complete archQueryMore()");
        goto finish;
      }
    }else{
      if( db == I_WEBINDEX_DB ){
        if( archWebBooleanQueryMore(strhan, qry, type, case_sens, max_hits,
                                    match, hpm, path_r, expath,
                                    path_rel, path_items, d_list, d_cnt, db,
                                    start_db, 0, &result, &strs, &ret_hits,
                                    &more_tbl, (char *)NULL, 0, format) == ERROR){
          send_error_in_plain(" Could not complete archWebBooleanQueryMore()\n");
          error(A_ERR,prog," Could not complete archWebBooleanQueryMore()");
          goto finish;
        }
      }else if( db == I_ANONFTP_DB ){
        if( archBooleanQueryMore(strhan, qry, type, case_sens, max_hits,
                                 match, hpm, path_r, expath,
                                 path_rel, path_items, d_list, d_cnt, db,
                                 start_db, 0, &result, &strs, &ret_hits,
                                 &more_tbl, (char *)NULL, 0, format) == ERROR){
          send_error_in_plain(" Could not complete archWebBooleanQueryMore()\n");
          error(A_ERR,prog," Could not complete archBooleanQueryMore()");
          goto finish;
        }
        /* put code for archBooleanQueryMore() here */
      }

    }
    send_result_in_plain(result, strs, ret_hits, start_tbl,
                         more_tbl, db, strhan, ents, format );
    goto finish;
  }

  if(qry.string != (char *)NULL && qry.string[0] != '\0') {
    if ( booltype ){
      memset(&(more_tbl),'\0',sizeof(boolean_return_t));
      if( db == I_ANONFTP_DB ){
        if (archBooleanQueryMore( strhan, qry, type, case_sens, max_hits,
                             match, hpm, path_r, expath, path_rel,
                             path_items, d_list, d_cnt, db, start_db,
                             0, &result, &strs, &ret_hits, &more_tbl,
                             (char *)NULL, 0, format) == ERROR){
          send_error_in_plain("archBooleanQuery failed, abort");
          error(A_ERR,prog,"archBooleanQuery failed, abort");
          goto finish;
        }
      }else if( db == I_WEBINDEX_DB ){
        if (archWebBooleanQueryMore( strhan, qry, type, case_sens, max_hits,
                                match, hpm, path_r, expath, path_rel,
                                path_items, d_list, d_cnt, db, start_db,
                                0, &result, &strs, &ret_hits, &more_tbl,
                                (char *)NULL, 0, format) == ERROR){
          send_error_in_plain("archWebBooleanQuery failed, abort");
          error(A_ERR,prog,"archWebBooleanQuery failed, abort");
          goto finish;
        }
      }

    }else{
      if (archQuery( strhan, qry.string, type, case_sens, max_hits, match,
                    hpm, path_r, expath, path_rel, path_items, d_list, d_cnt,
                    db, start_db, 0, &result, &strs, &ret_hits, &start_tbl,
                    (char *)NULL, 0, format) == ERROR){
        send_error_in_plain("archQuery failed, abort");
        error(A_ERR,prog,"archQuery failed, abort");
        goto finish;
      }
    }
    send_result_in_plain(result, strs, ret_hits, start_tbl,
                         more_tbl, db, strhan, ents, format );

  }

finish:
    
  free_entries( &ents );
  free_strings( &strs, max_hits );
  if ( qry.string != NULL ) free(qry.string);
  if ( qry.and_list != NULL ) free_list(&(qry.and_list),qry.orn); /* 222 MUST FREE .lowrds FOR EACH */ 
  if ( qry.not_list != NULL ) free_list(&(qry.not_list),qry.orn);
  if ( result != NULL ) free(result);
  
  archCloseStrIdx( strhan);
  archFreeStrIdx(&strhan);
  close_start_dbs(start_db,domain_db);
  destroy_finfo(start_db);
  destroy_finfo(domain_db);

  exit(A_OK);
  return(A_OK);

}
