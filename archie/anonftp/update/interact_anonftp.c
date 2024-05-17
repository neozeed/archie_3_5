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
#include "domain.h"
#include "header.h"
#include "db_ops.h"
#include "host_db.h"
#include "error.h"
#include "files.h"
#include "archie_strings.h"
#include "archie_dbm.h"
#include "interact_anonftp.h"
#include "core_entry.h"
#include "lang_anonftp.h"
#include "debug.h"
#include "master.h"

#include "site_file.h"
#include "../startdb/start_db.h"
#include "../startdb/lang_startdb.h"
#include "patrie.h"
#include "archstridx.h"

#include "strings_index.h"
#include "protos.h"

#define MAX_STR  64

#define EXACT 0
#define SUB 1
#define REGEXP 2
#define MAX_HITS 100
#define MAX_MATCH 50

int verbose = 0;

char *prog;

status_t archQuery();
status_t getResultFromStart( );
extern status_t host_table_find();
extern status_t update_start_dbs();
extern status_t close_start_dbs();
extern status_t open_start_dbs(); 
extern status_t get_index_start_dbs();
extern int archSearchSub();
extern status_t get_port();

status_t archQuery( strhan, qry, type, case_sens, h, m, hpm, start_db)
   struct arch_stridx_handle *strhan;
   char *qry;
   int type;      /* type of search */
   int case_sens;   /* case_sens */
   int h;         /* maximum hits (all in all) */
   int m;         /* maximum match (different unique strings) */
   int hpm;       /* maximum hits per match */
   file_info_t *start_db;
{
  /* File information */

  host_table_index_t inds[65536], hin;
  int indslen,state,j,i,k,l,all = 0;
  unsigned long hits = 0;
  hostname_t hnm;
  ip_addr_t ipaddr = (ip_addr_t)(0);

  int port, site_hits;

  index_t *start;
  hnm[0] = '\0';
  port = 0;

  if(h<=0)    h= MAX_HITS;
  if(m<=0)    m= MAX_MATCH;
  switch( type ){

  case EXACT:

    start = (index_t *)malloc(sizeof(index_t));
    if( !archSearchExact( strhan, qry, &start[0], &state) ){
      error(A_ERR, "archQuery", "Could not perform exact search successfully.\n");
      return(A_OK);
    }
    if( state != 1 ){
      fprintf(stderr,"String %s was not found in the database.\n",qry);
      return(A_OK);
    }
    hits = 1;
    break;

  case SUB:

    start = (index_t *)malloc(sizeof(index_t)*m);
    if( !archSearchSub( strhan, qry, case_sens, m, &hits, start ) ){
      error(A_ERR, "archQuery", "Could not perform exact search successfully.\n");
      return(A_OK);
    }
    if( hits <= 0 ){
      fprintf(stderr,"String %s was not found in the database.\n",qry);
      return(A_OK);
    }
    break;

  default:
    error(A_ERR," archQuery", "Did not supply any search type for query");
    return(A_OK);
    break;
  }

  site_hits = 0;
  for(j=0,l=0 ; (j<hits) && (l<m) ; j++ ){
    get_index_start_dbs(start_db, start[j], &inds, &indslen);

    if( indslen <= 0 ){
      fprintf(stderr,"String %s is not in any site in the database.\n",qry);
      /*        return(A_OK);*/
      continue;
    }
    l++;    /* match (unique string) increased */

    /*  fprintf(stderr,"total sites returned: %d\n",indslen); */
    if(hpm<=0)  hpm=indslen;
    
    for( i=0,k=0 ; (i<indslen) && (k<hpm) ; i++ ){
      ipaddr = (ip_addr_t)(0);
      port = 0;
      hin = inds[i];
      if( host_table_find( &ipaddr, hnm, &port, &hin)==ERROR ) {
        error(A_ERR,"interact_anonftp","could not find host in host-table. start/host dbase corrupt.\n");
        continue;
      }
      k++;    /* hit per match has increased */

      /* fprintf(stderr,"\n   Site:%s\n",inet_ntoa( ipaddr_to_inet(ipaddr)));  */

      site_hits = hpm;
      if( getResultFromStart( start[j], strhan, (ip_addr_t)ipaddr, port, &site_hits )==ERROR ) {
        error(A_ERR,"interact_anonftp","could not reconstruct the string from site file. site dbase corrupt.\n");
        continue;
      }
      all+=site_hits;   /* hits (all in all) increased */
      if(all>=h){
        free(start);
        return(A_OK);
      }
    }
  }
  free(start);
  return(A_OK);
}

status_t getResultFromStart( start, strhan, ip, port, hits )
   index_t start;
   struct arch_stridx_handle *strhan;
   ip_addr_t ip;
   int port;
   int *hits;
{
  /* File information */
  char *res, *totstr, *tmpstr, t[MAX_STR];

  int l, act_size = 0;
  int cnt = 0;
  int limit = 0;
  index_t curr_strings;
  index_t curr_count;
  index_t curr_prnt=0;

  full_site_entry_t *curr_ent, *ent;
  full_site_entry_t *curr_endptr;
  full_site_entry_t *prnt_ent;

  file_info_t *curr_finfo = create_finfo();

  res = tmpstr = totstr = (char *)(0);
  limit = *hits;
  *hits = cnt;

  if(access(files_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port), F_OK) < 0){
    /* "File %s does not exist in the database. No need to look for starts in it" */
    error(A_WARN,"insert_anonftp", "File %s does not exist in the database. No need to look for starts in it.\n",
          files_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port));
    return(A_OK);

  }else{

    strcpy(curr_finfo -> filename, files_db_filename(inet_ntoa( ipaddr_to_inet(ip)) ,port));
    /* Open current file */
    if(open_file(curr_finfo, O_RDONLY) == ERROR){
      /* "Ignoring %s" */
      error(A_ERR, "interact_anonftp", "Ignoring %s", curr_finfo->filename);
      return(A_OK);
    }
   
    /* mmap it */
    if(mmap_file(curr_finfo, O_RDONLY) == ERROR){
      /* "Ignoring %s" */
      error(A_ERR, "interact_anonftp", "Ignoring %s", curr_finfo ->filename);
      return(A_OK);
    }
    act_size = curr_finfo -> size / sizeof(full_site_entry_t);
  }

  for(curr_ent = (full_site_entry_t *) curr_finfo -> ptr, curr_endptr = curr_ent + act_size, curr_count = 0;
      curr_ent < curr_endptr;
      curr_ent++, curr_count++){
    
    curr_strings = curr_ent -> strt_1;
    /* Check if string offset is valid */
    if(curr_strings >= (index_t)(0)){
      if ( CSE_IS_SUBPATH((*curr_ent))||
           CSE_IS_PORT((*curr_ent))||
           CSE_IS_NAME((*curr_ent)) ){
        curr_prnt = curr_count;
      }else{
        if( curr_strings == start ){
          sprintf(t,"%s \t size:%ld \t perms:%o", (char *)cvt_to_usertime(curr_ent -> core.entry.date,0), 
                  curr_ent -> core.entry.size,	curr_ent -> core.entry.perms);
          ent = curr_ent;
          tmpstr = (char *)0;
          while(( curr_prnt >= (index_t)(0) )&&( curr_strings >= (index_t)(0) )){
            if( !archGetString( strhan,  curr_strings, &res) )
            {                   /* "Site %s (%s) record %d has string index %d out of bounds" */
              error(A_ERR, "getResultFromStart", "Site %s (%s) record %d has string index %d out of bounds",
                    files_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port), curr_count, curr_strings );
              close_file(curr_finfo);
              free(curr_finfo);
              return(ERROR);
            }else{
              if( tmpstr == (char *)(0) ){
                l = (strlen(res)+1);
                tmpstr = (char *)malloc( sizeof(char)*l );
                totstr = (char *)malloc( sizeof(char)*l );
                sprintf(tmpstr,"%s",res);
                sprintf(totstr,"%s",res);
              }
              else{
                l = strlen(res) + strlen(totstr) + 2;
                if ( (tmpstr = (char *)realloc( tmpstr, sizeof(char)*(l) )) == (char *)NULL ){
                  error(A_SYSERR,"getResultFromStr","could not allocate memory.\n");
                  close_file(curr_finfo);
                  free(curr_finfo);
                  return(ERROR);
                }
                sprintf(tmpstr,"%s/%s",res,totstr);
                if ( (totstr = (char *)realloc( totstr, sizeof(char)*(l) )) == (char *)NULL ){
                  error(A_SYSERR,"getResultFromStr","could not allocate memory.\n");
                  close_file(curr_finfo);
                  free(curr_finfo);
                  return(ERROR);
                }
                strcpy(totstr ,tmpstr);
              }
              free( res );
              prnt_ent = (full_site_entry_t *) (curr_finfo -> ptr + curr_prnt*sizeof(full_site_entry_t));
              if(!(CSE_IS_SUBPATH((*prnt_ent))||
                   CSE_IS_PORT((*prnt_ent))||
                   CSE_IS_NAME((*prnt_ent))) ) {
                error(A_ERR, "getResultFromStart","Corruption in site file %s\n",
                      files_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port) );
                close_file(curr_finfo);
                free(curr_finfo);
                return(ERROR);
              }
              curr_prnt = prnt_ent->strt_1;
              if( curr_prnt >= (index_t)(0) ){
                ent = (full_site_entry_t *) (curr_finfo -> ptr + 
                                            prnt_ent->core.prnt_entry.strt_2*sizeof(full_site_entry_t));
                curr_strings = ent->strt_1;
              }
            }
          }
          fprintf(stderr,"   %s\n",totstr);

          free(tmpstr); 
          free(totstr);
          tmpstr = (char *)0;
          fprintf(stderr,"           %s\n",t);
          cnt++;          
        }
      }
      if( cnt >= limit ) break;
    }
  }

  *hits = cnt;
  close_file(curr_finfo);
  free(curr_finfo);
  return(A_OK);
}


/*
 * interact_anonftp: perform consistency checking on anonftp database
 * 
 * 

   argv, argc are used.

   Parameters:	  -w <anonftp files database pathname>
		  -M <master database pathname>
		  -v verbose mode
		  -l write to log file
		  -L <log file>
		  
*/


int main(argc, argv)
   int argc;
   char *argv[];

{

#ifdef __STDC__

  extern int getopt(int, char **, char *);
  extern status_t compile_domains( char *, domain_t *, file_info_t *, int *);

#else

  extern int getopt();
  extern status_t compile_domains();

#endif

   
  extern int opterr;
  extern char *optarg;

  char **cmdline_ptr;
  int cmdline_args;

  int option;

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
  char *dbdir;

  char qry[MAX_PATH_LEN];

  /* host databases */

  pathname_t logfile;
  hostname_t hostname;

  int logging = 0;
  int hits = 0;
  int mtch = 0;
  int hpm = 0;
  int type = EXACT;
  int case_sens = 0;

  prog = argv[0];

  logfile[0] = hostname[0] = '\0';

  host_database_dir[0] =  files_database_dir[0] = start_database_dir[0] = master_database_dir[0] = '\0';

  opterr = 0;                   /* turn off getopt() error messages */

  /* ignore argv[0] */

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;

  while((option = (int) getopt(argc, argv, "H:h:m:w:M")) != EOF){

    switch(option){

      /* Master directory */

    case 'M':
      strcpy(master_database_dir,optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;

      /* Type of search */

    case 't':
      type = atoi(optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* case sensetive */

    case 'c':
      case_sens = atoi(optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* Hits */

    case 'H':
      hits = atoi(optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;

      /* Hits per match */

    case 'h':
      hpm = atoi(optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;

      /* Match */

    case 'm':
      mtch = atoi(optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;

      /* Get files database directory name, argument "w" */

    case 'w':
      strcpy(files_database_dir,optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;


    default:
      error(A_ERR,"interact_anonftp","Usage: [-M master-database-dir] [-t <type-of-search>] [-c <case-sensetive>] [-H <total-hits>] [-h <hits-per-match>] [-m <strings-matched>] [-w <data-directory>]");
      exit(A_OK);
      break;
    }

  }

  /* set up logs */
   
  if(logging){  
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /*  "Can't open default log file" */

        error(A_ERR, "interact_anonftp", "Can't open default log file.\n");
        exit(A_OK);
      }
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, "interact_anonftp", "Can't open log file %s.\n", logfile);
        exit(A_OK);
      }
    }
  }


  /* Set up system directories and files */


  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR,"interact_anonftp", "Error while trying to set master database directory.\n");
    exit(A_OK);
  }

  if((dbdir = set_files_db_dir(files_database_dir)) == (char *) NULL){

    /* "Error while trying to set anonftp database directory" */

    error(A_ERR,"interact_anonftp", "Error while trying to set anonftp database directory\n");
    exit(A_OK);
  }

  if(set_host_db_dir(host_database_dir) == (char *) NULL){

    /* "Error while trying to set host database directory" */

    error(A_ERR,"interact_anonftp", "Error while trying to set host database directory\n");
    exit(A_OK);
  }


  if(set_start_db_dir(start_database_dir,DEFAULT_FILES_DB_DIR) == (char *) NULL){

    /* "Error while trying to set start database directory" */

    error(A_ERR, "interact_anonftp", "Error while trying to set start database directory\n");
    exit(A_OK);
  }

  /* Open other files database files */

  if ( open_start_dbs(start_db,domain_db,O_RDWR ) == ERROR ) {

    /* "Can't open start/host database" */

    error(A_ERR, prog, "Can't open start/host database");
    exit(A_OK);
  }

  if ( !archOpenStrIdx( dbdir, ARCH_STRIDX_SEARCH, &strhan ) ){
    error( A_WARN, "interact_anonftp", "Could not find strings files, aborting." );
    exit(A_OK);
  }

  while(1){
    char *p;
    fprintf(stderr,"\n#Give a string:");
    fgets(qry,MAX_PATH_LEN,stdin);
    p=qry;
    if(p!=(char *)NULL){
      for(; (*p!='\0')&&(*p!='\n'); p++);
      *p = '\0';
    }

    if(qry[0] != '\0') {
      if (archQuery( strhan, qry, type, hits, mtch, hpm, start_db) == ERROR){
        error(A_ERR,"interact_anonftp","archQuery failed, abort\n");
        exit(A_OK);
      }
    }else
    break;
  }

  close_start_dbs(start_db,domain_db);

  destroy_finfo(start_db);
  destroy_finfo(domain_db);
  exit(A_OK);
  return(A_OK);
}
