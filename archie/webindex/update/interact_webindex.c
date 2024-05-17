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
#include "webindexdb_ops.h"
#include "host_db.h"
#include "error.h"
#include "files.h"
#include "archie_strings.h"
#include "archie_dbm.h"
#include "interact_webindex.h"
#include "core_entry.h"
#include "lang_webindex.h"
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

int verbose = 0;

char *prog;

status_t archQuery( );
status_t getResultFromStart( );
extern status_t host_table_find();
extern status_t update_start_dbs();
extern status_t close_start_dbs();
extern status_t open_start_dbs();
extern status_t get_index_start_dbs();
extern int archSearchSub();
extern status_t get_port();

status_t archQuery( strhan, qry, start_db)
   struct arch_stridx_handle *strhan;
   char *qry;
   file_info_t *start_db;
{
   /* File information */

   host_table_index_t inds[65536], hin;
   int indslen,j;
   unsigned long hits;

   hostname_t hnm;
   ip_addr_t ipaddr = (ip_addr_t)(0);

   int i;
   struct arch_search_state *what;
   int port;

   index_t start[10];
   hnm[0] = '\0';
   port = 0;

/*   if( !archSearchExact( strhan, qry, &start, &what) ){
	 error(A_ERR, "archQuery", "Could not perform exact search successfully.\n");
	 return(A_OK);
   }*/

   if( !archSearchSub( strhan, qry, 0, 10, &hits,  start, what) ){
	 error(A_ERR, "archQuery", "Could not perform exact search successfully.\n");
	 return(A_OK);
   }

   if( hits <= 0 ){
        fprintf(stderr,"String %s was not found in the database.\n",qry);
        return(A_OK);
   }
/*
   if( *what != 1 ){
        fprintf(stderr,"String %s was not found in the database.\n",qry);
        return(A_OK);
   }
*/
   for(j = 0 ; j < hits; j++ ){
     get_index_start_dbs(start_db, start[j], inds, &indslen);

     if( indslen <= 0 ){
/*        fprintf(stderr,"String %s is not in any site in the database.\n",qry);
        return(A_OK);*/
        continue;
     }

/*     fprintf(stderr,"total sites returned: %d\n",indslen); */
     for( i=0 ; i < indslen; i++ ){
        ipaddr = (ip_addr_t)(0);
        port = 0;
        hin = inds[i];
        if( host_table_find( &ipaddr, hnm, &port, &hin)==ERROR )
        {  error(A_ERR,"interact_webindex","could not find host in host-table. start/host dbase corrupt.\n");
           continue;
        }

        fprintf(stderr,"\n   Site:%s\n",inet_ntoa( ipaddr_to_inet(ipaddr)));

        if( getResultFromStart( start[j], strhan, (ip_addr_t)ipaddr, port )==ERROR )
        {  error(A_ERR,"interact_webindex","could not reconstruct the string from site file. site dbase corrupt.\n");
           continue;
        }
     }
   }
   return(A_OK);
}

status_t getResultFromStart( start, strhan, ip, port )
  index_t start;
  struct arch_stridx_handle *strhan;
  ip_addr_t ip;
  int port;
{
  /* File information */
  char *res, *totstr, *tmpstr, t[MAX_STR];

  int l, act_size = 0;

  index_t curr_strings;
  index_t curr_count, cp, cf;
  index_t curr_file=0;
  index_t curr_prnt=0;

  full_site_entry_t *curr_ent, *ent;
  full_site_entry_t *curr_endptr;
  full_site_entry_t *prnt_ent;

  file_info_t *curr_finfo = create_finfo();

  res = tmpstr = totstr = (char *)(0);

  if(access((char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port), F_OK) < 0){
    /* "File %s does not exist in the database. No need to look for starts in it" */
    error(A_WARN,"interact_webindex", "File %s does not exist in the database. No need to look for starts in it.\n",
          wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port));
    return(A_OK);

  }else{
    strcpy(curr_finfo -> filename, (char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port));
    /* Open current file */
    if(open_file(curr_finfo, O_RDONLY) == ERROR){
      /* "Ignoring %s" */
      error(A_ERR, "interact_webindex", "Ignoring %s", curr_finfo->filename);
      return(A_OK);
    }
   
    /* mmap it */
    if(mmap_file(curr_finfo, O_RDONLY) == ERROR){
      /* "Ignoring %s" */
      error(A_ERR, "interact_webindex", "Ignoring %s", curr_finfo ->filename);
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
      if ( CSE_IS_SUBPATH((*curr_ent)) ||
          CSE_IS_PORT((*curr_ent)) ||
          CSE_IS_NAME((*curr_ent)) ){
        curr_prnt = curr_count;
      }else{
        if( !CSE_IS_KEY((*curr_ent)) ){
          curr_file = curr_count;
        }
        cf = curr_file;
        cp = curr_prnt;
        if( curr_strings == start ){
          if( CSE_IS_KEY((*curr_ent)) )
          sprintf(t,"Weight:%f",(float)curr_ent -> core.kwrd.weight);
          else
          sprintf(t,"%s \t size:%ld \t perms:%o",
                  (char *)cvt_to_usertime(curr_ent -> core.entry.date,0),
                  curr_ent -> core.entry.size,	curr_ent -> core.entry.perms);
          tmpstr = (char *)0;
          if(( curr_file > curr_prnt) && (CSE_IS_KEY((*curr_ent))) ) {
            if( !archGetString( strhan,  curr_strings, &res) )
            {                   /* "Site %s (%s) record %d has string index %d out of bounds" */
              error(A_ERR, "getResultFromStart", "Site %s (%s) record %d has string index %d out of bounds",
                    wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port), curr_count, curr_strings );
              close_file(curr_finfo);
              free(curr_finfo);
              return(ERROR);
            }else{
              if( tmpstr == (char *)(0) ){
                l = (strlen(res)+1);
                tmpstr = (char *)malloc( sizeof(char)*l );
                /* totstr = (char *)malloc( sizeof(char)*l ); */
                sprintf(tmpstr,"%s",res);
                /* sprintf(totstr,"%s",res); */
                totstr = tmpstr;
              }
              else{
                l = strlen(res) + strlen(totstr) + 2;
                if ( (tmpstr = (char *)malloc( sizeof(char)*(l) )) == (char *)NULL )
                error(A_SYSERR,"getResultFromStr","could not allocate memory.\n");
                sprintf(tmpstr,"%s/%s",res,totstr);
                /* if ( (totstr = (char *)realloc( totstr, sizeof(char)*(l) )) == (char *)NULL )
                   error(A_SYSERR,"getResultFromStr","could not allocate memory.\n");
                   strcpy(totstr ,tmpstr); */
                free( totstr);                       
                totstr = tmpstr;
              }
            }
            if( curr_file >= 0 ){
              ent = (full_site_entry_t *) (curr_finfo -> ptr +
                                           curr_file*sizeof(full_site_entry_t));
              curr_strings = ent->strt_1;
            }
          }
          while(( curr_prnt >= (index_t)(0) )&&( curr_strings >= (index_t)(0) )){
            if( !archGetString( strhan,  curr_strings, &res) )
            {                   /* "Site %s (%s) record %d has string index %d out of bounds" */
              error(A_ERR, "getResultFromStart", "Site %s (%s) record %d has string index %d out of bounds",
                    wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port), curr_count, curr_strings );
              close_file(curr_finfo);
              free(curr_finfo);
              return(ERROR);
            }else{
              prnt_ent = (full_site_entry_t *) (curr_finfo -> ptr + curr_prnt*sizeof(full_site_entry_t));
              if( tmpstr == (char *)(0) ){
                l = (strlen(res)+1);
                tmpstr = (char *)malloc( sizeof(char)*l );
                /* totstr = (char *)malloc( sizeof(char)*l ); */
                sprintf(tmpstr,"%s",res);
                /* sprintf(totstr,"%s",res); */
                totstr = tmpstr;
              }
              else{
                l = strlen(res) + strlen(totstr) + 2;
                if ( (tmpstr = (char *)malloc(  sizeof(char)*(l) )) == (char *)NULL )
                error(A_SYSERR,"getResultFromStr","could not allocate memory.\n");
                if( CSE_IS_NAME((*prnt_ent)) )
                sprintf(tmpstr,"%s:%s",res,totstr);
                else
                sprintf(tmpstr,"%s/%s",res,totstr);
                /* if ( (totstr = (char *)realloc( totstr, sizeof(char)*(l) )) == (char *)NULL )
                   error(A_SYSERR,"getResultFromStr","could not allocate memory.\n");
                   strcpy(totstr ,tmpstr);*/
                free( totstr);
                totstr = tmpstr;
              }
              free( res );
              if( !(CSE_IS_SUBPATH((*prnt_ent)) ||
                    CSE_IS_PORT((*prnt_ent)) ||
                    CSE_IS_NAME((*prnt_ent)) ) ){
                error(A_ERR, "getResultFromStart","Corruption in site file %s\n",
                      wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port) );
                close_file(curr_finfo);
                free(curr_finfo);
                return(ERROR);
              }
              curr_prnt = prnt_ent->strt_1;
              if ( curr_prnt >= (index_t)(0) ){
                ent = (full_site_entry_t *) (curr_finfo -> ptr + 
                                             prnt_ent->core.prnt_entry.strt_2*sizeof(full_site_entry_t));
                curr_strings = ent->strt_1;
              }
            }
          }
          fprintf(stderr,"   %s\n",totstr);
          free(tmpstr);
          /* free(totstr); */
          tmpstr = (char *)0;
          curr_file = cf;
          curr_prnt = cp;
          fprintf(stderr,"       %s\n",t); 
        }
      }
    }
  }
  close_file(curr_finfo);
  free(curr_finfo);
  return(A_OK);
}


/*
 * interact_webindex: perform consistency checking on webindex database
 * 
 * 

   argv, argc are used.

   Parameters:	  -w <webindex files database pathname>
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

   pathname_t logfile;
   hostname_t hostname;

   int logging = 0;

   prog = argv[0];

   logfile[0] = hostname[0] = '\0';

   files_database_dir[0] = start_database_dir[0] = host_database_dir[0]  = master_database_dir[0] = '\0';

   opterr = 0;  /* turn off getopt() error messages */

   /* ignore argv[0] */

   cmdline_ptr = argv + 1;
   cmdline_args = argc - 1;
   files_database_dir[0] = '\0';

   while((option = (int) getopt(argc, argv, "H:w:h:M:L:lv")) != EOF){

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

	 /* Get files database directory name, argument "w" */

	 case 'w':
	    strcpy(files_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;


	 default:
	    error(A_ERR,"interact_webindex","Usage: [-p] [-d <debug level>] [-w <data directory>] <sitename>");
	    exit(A_OK);
	    break;
      }

   }

   /* set up logs */
   
   if(logging){  
      if(logfile[0] == '\0'){
	 if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

	    /*  "Can't open default log file" */

	    error(A_ERR, "interact_webindex", "Can't open default log file.\n");
	    exit(A_OK);
	 }
      }
      else{
	 if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

	    /* "Can't open log file %s" */

	    error(A_ERR, "interact_webindex", "Can't open log file %s.\n", logfile);
	    exit(A_OK);
	 }
      }
   }


   /* Set up system directories and files */


   if(set_master_db_dir(master_database_dir) == (char *) NULL){

      /* "Error while trying to set master database directory" */

      error(A_ERR,"interact_webindex", "Error while trying to set master database directory.\n");
      exit(A_OK);
   }

   if((dbdir = (char *)set_wfiles_db_dir(files_database_dir)) == (char *) NULL){

      /* "Error while trying to set webindex database directory" */

      error(A_ERR,"interact_webindex", "Error while trying to set webindex database directory\n");
      exit(A_OK);
   }

   if(set_start_db_dir(start_database_dir,DEFAULT_WFILES_DB_DIR) == (char *) NULL){

      /* "Error while trying to set start database directory" */

      error(A_ERR, "interact_webindex", "Error while trying to set start database directory\n");
      exit(A_OK);
   }

   if(set_host_db_dir(host_database_dir) == (char *) NULL){

      /* "Error while trying to set host database directory" */

      error(A_ERR,"interact_webindex", "Error while trying to set host database directory\n");
      exit(A_OK);
   }

   /* Open other files database files */

   if ( open_start_dbs(start_db,domain_db,O_RDWR ) == ERROR ) {

      /* "Can't open start/host database" */

      error(A_ERR, "interact_webindex", "Can't open start/host database");
      exit(A_OK);
   }

   if ( !archOpenStrIdx( dbdir, ARCH_STRIDX_SEARCH, &strhan ) )
   {
      error( A_WARN, "interact_webindex", "Could not find strings_idx files, abort.\n" );
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

      if(qry[0] != '\0') 
      { if (archQuery( strhan, qry, start_db) == ERROR)
        {   error(A_ERR,"interact_webindex","archQuery failed, abort\n");
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

