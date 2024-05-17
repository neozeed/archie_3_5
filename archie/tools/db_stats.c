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

status_t stat_webindex PROTO((char **));
status_t stat_anonftp PROTO((char **));

#define WEB 1
#define FTP 2


int verbose = 0;
char *prog;

/*
 * db_stats: generate stats about the archie catalogs
 * 
 * 

   argv, argc are used.

   Parameters:	 
      -H <hostname>
      -p <port>
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


  while((option = (int) getopt(argc, argv, "H:d:h:M:L:lvp:")) != EOF){

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
      error(A_ERR,"db_stats","Usage: [-H <hostname>]  [-p <port>] [-M <master database pathname>] [-d <database>]	 [-h <host database pathname>] [-v verbose mode] [-l] [-L <log file>]");
      exit(A_OK);
      break;
    }

  }

  /* set up logs */
   
  if(logging){  
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /*  "Can't open default log file" */

        error(A_ERR, "db_stats","Can't open default log file" );
        exit(A_OK);
      }
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, "db_stats", "Can't open log file %s", logfile);
        exit(A_OK);
      }
    }
  }


  /* Set up system directories and files */


  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR,"db_stats",  "Error while trying to set master database directory" );
    exit(A_OK);
  }


  if(set_host_db_dir(host_database_dir) == (char *) NULL){
    /* "Error while trying to set host database directory" */
    error(A_ERR,"db_stats","Error while trying to set host database directory");
    exit(A_OK);
  }

  if(open_host_dbs(hostbyaddr, hostdb, (file_info_t *) NULL, hostaux_db, O_RDONLY) != A_OK){

    /* "Error while trying to open host database" */

    error(A_ERR,"db_stats","Error while trying to open host database" );
    exit(A_OK);
  }


  for(db = databases; *db != NULL && *db[0] != '\0'; db++ ) {
    int which = 0;
    
    if( !strcmp( *db , ANONFTP_DB_NAME )){
      which = FTP;
      if((dbdir = set_files_db_dir(files_database_dir)) == (char *) NULL){
        /* "Error while trying to set anonftp database directory" */
        error(A_ERR, "db_stats", "Error while trying to set database directory");
        return(A_OK);
      }
    }else if( !strcmp( *db , WEBINDEX_DB_NAME )){
      which = WEB;
      if((dbdir = set_wfiles_db_dir(wfiles_database_dir)) == (char *) NULL){
        /* "Error while trying to set anonftp database directory" */
        error(A_ERR, "db_stats","Error while trying to set database directory");
        return(A_OK);
      }
    }
    else {
        error(A_ERR, "db_stats","Unknown Database: %s ",*db);
        return(A_OK);
    }


    
    if ( verbose )
    error(A_INFO, "db_stats", "Processing database %s", *db);



    /* Open other files database files */
#if 0
    if(open_files_db(strings_idx, strings, (file_info_t *) NULL, O_RDWR) == ERROR){

      /* "Error while trying to open anonftp database" */

      error(A_ERR,"db_stats", "Error while trying to open anonftp database" );
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
        error(A_ERR, "db_stats", "Can't get hostdb record ");
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

        error(A_ERR, "db_stats", "Can't get list of files in directory %s", func());
        exit(ERROR);
      }
      
    }
    else{

      if(get_file_list(func(), &file_list, (char *) NULL, &count) == ERROR){

        /* "Can't get list of files in directory %s" */

        error(A_ERR, "db_stats", "Can't get list of files in directory %s", get_files_db_dir());
        exit(ERROR);
      }
        
    }

    switch ( which ) {
    case FTP:
      stat_anonftp(file_list);
      break;
    case WEB:
      stat_webindex(file_list);
      break;
    }
      
  }

  
  exit(A_OK);
  return(A_OK);

}


static status_t count_unique_keywords( unique_keys, which )
  int *unique_keys;
  int which;
{

  char buff[8*1024];
  int c;
  
  file_info_t *curr_finfo = create_finfo();

  if ( which == WEB ) 
    strcpy(curr_finfo -> filename, wfiles_db_filename("Stridx.Strings",-1));
  else
    strcpy(curr_finfo -> filename, files_db_filename("Stridx.Strings",-1));

  if(open_file(curr_finfo, O_RDONLY) == ERROR){

    /* "Ignoring %s" */
	 
    error(A_ERR, "check_indiv", "Ignoring %s" , curr_finfo->filename);
    return ERROR;
  }

  *unique_keys = 0;
  
  while (( c = fread( buff, sizeof(char), 8*1024, curr_finfo->fp_or_dbm.fp)) > 0) {
    int i;
    for ( i = 0; i < c; i++ ) {

      if ( buff[i] == '\n' ) {
        (*unique_keys)++;
      }
    }
  }

  
  close_file(curr_finfo);
  return A_OK;
}



status_t stat_webindex(file_list)
   char **file_list;
{
  /* File information */

  char **curr_file;

  ip_addr_t ipaddr = 0;

  int tmp_val;
  
  struct stat statbuf;
  
  full_site_entry_t *curr_ent;

  int total_sites;


  MINT *urls_total;
  MINT *indexed_urls_total;
  MINT *keys_total;
  MINT *tmps;

  long  excerpt_size_total = 0;
  long  site_size_total = 0;
  long  file_size = 0;
  long  index_size_total = 0;

  unsigned int act_size,doc_size;
  unsigned int urls_nums;
  unsigned int indexed_urls_nums;
  int keys_nums;
  int i;
  char *visited;
  int unique_keys;
  
  file_info_t *curr_finfo = create_finfo();

  ptr_check(file_list, char*, "check_indiv", ERROR);


  urls_total = itom(0);
  indexed_urls_total = itom(0);
  keys_total = itom(0);
  site_size_total = 0;
  excerpt_size_total = 0;
  
  index_size_total = 0;
  total_sites = 0;

  if ( verbose )  {
      printf("\n\n%-26s %7s %10s %10s %10s %10s\n\n","Site", "Port", "URLs",
             "Indexed", "Keywords", "Size");
  }
    
  for(curr_file = file_list, total_sites = 0; *curr_file != (char *) NULL; curr_file++){
    pathname_t filename,tmp;
    char *port;
    int flag;

    if ( strncmp(*curr_file, "Stridx.", 7) == 0 ) {
      pathname_t tmp;
      strcpy(tmp, wfiles_db_filename(*curr_file,-1));
      if ( stat(tmp,&statbuf) == -1 )
        continue;
      index_size_total += statbuf.st_size;
    }

    if ( strrcmp(*curr_file, ".idx") == 0 )
      continue;
    
    if(sscanf(*curr_file, "%*d.%*d.%*d.%d:%*d", &tmp_val) != 1)
    continue;

    if ( strrcmp(*curr_file,".excerpt") == 0 )
      continue;
    
    total_sites++;

    strcpy(filename,*curr_file);
    port = strchr(filename,':');
    if ( port == NULL )
    continue;
    *port++ = '\0';
    ipaddr = inet_addr(filename);

    strcpy(curr_finfo -> filename, wfiles_db_filename(filename,atoi(port)));
    

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


    site_size_total += curr_finfo -> size;

    sprintf(tmp,"%s.excerpt",curr_finfo->filename);
    if ( stat( tmp, &statbuf ) == -1 ) {
      error(A_ERR,"db_stats", "Unable to stat file %s\n",tmp);
    }
    else {
      excerpt_size_total += statbuf.st_size;
    }
    
    act_size = curr_finfo -> size / sizeof(full_site_entry_t);

    visited = (char*)malloc(sizeof(char)*act_size);
    
    if ( visited == NULL ) {
      error(A_ERR,"init_urldb","Unable to allocate memory for internal table");
      destroy_finfo(curr_finfo);
      return ERROR;
    }
    memset(visited,0,sizeof(char)*act_size);
  
    curr_ent = (full_site_entry_t *) curr_finfo -> ptr + (act_size-1);
    flag = 0;

    doc_size = urls_nums = indexed_urls_nums = keys_nums = 0;
    
    for ( i = act_size-1; i>=0; i--, curr_ent--) {
      int curr_index;
      full_site_entry_t *curr_ptr;
      
      if ( CSE_IS_KEY((*curr_ent)) )  { /* Skip over the keys */
        flag = 1;
        visited[i] = 1;
        keys_nums++;
        continue;
      }

      if ( visited[i] && flag == 0 ) {
        flag = 0;
        continue;
      }

      curr_index = i;

      curr_ptr = (full_site_entry_t *) curr_finfo -> ptr + (curr_index);
      if ( CSE_IS_DOC((*curr_ptr)) || flag == 0 ) {
        urls_nums++;
      }

      if ( CSE_IS_DOC((*curr_ptr)) )  {
        indexed_urls_nums++;
        doc_size += curr_ptr->core.entry.size;
        file_size += curr_ptr->core.entry.size;
      }
      
      while ( (curr_index >= 0) ) {
        curr_ptr = (full_site_entry_t *) curr_finfo -> ptr + (curr_index);
        if ( CSE_IS_SUBPATH((*curr_ptr))  || 
            CSE_IS_NAME((*curr_ptr)) ||
            CSE_IS_PORT((*curr_ptr)) ) {
          break;
        }
        curr_index--;
      }

      while ( curr_index > 0  ) {

        if ( curr_ptr->core.prnt_entry.strt_2 <0 ) {
          break;
        }
        
        if ( visited[curr_index] ) /* No need to follow the path again */
          break;
        
        visited[curr_index] = 1;
        visited[curr_ptr->core.prnt_entry.strt_2] = 1;
        curr_index = curr_ptr->strt_1;
        curr_ptr = (full_site_entry_t *)curr_finfo ->ptr + curr_index;

      }

      flag = 0;
    }
    free(visited);

    if(verbose){
      printf("%-26s %7s %10d %10d %10d %10u\n", filename, port, urls_nums,
             indexed_urls_nums, keys_nums, doc_size);
    }

    madd(keys_total, tmps = itom(keys_nums), keys_total);
    mfree(tmps);
    madd(urls_total, tmps = itom(urls_nums), urls_total);
    mfree(tmps);
    madd(indexed_urls_total, tmps = itom(indexed_urls_nums), indexed_urls_total);
    mfree(tmps);

      

    close_file(curr_finfo);

  }

  if ( count_unique_keywords( &unique_keys, WEB) == ERROR ) {
    error(A_ERR,"db_stats", "Cannot count the number of unique keys");
    exit(0);
  }
  
  printf("\n\nTotals:\n\n");
  printf("Number of sites: %d\n",total_sites);
  printf("Number of urls: "); mout(urls_total);
  printf("Number of indexed urls: ");  mout(indexed_urls_total);
  printf("Total doc sizes: %ld\n", file_size);
  printf("Unique keys: %d\n",unique_keys);

  printf("\n\nDatabase stats:\n\n");
  printf("Size of site files: %ld\n",site_size_total);
  printf("Size of excerpt files: %ld\n", excerpt_size_total);
  printf("Size of Index: %ld\n",  index_size_total);
  printf("Database size: %ld\n", site_size_total + excerpt_size_total + index_size_total);

  printf("Compression factor: %d%%\n",
         (int)((double)(site_size_total+ excerpt_size_total+ index_size_total)
               * 100.0 /(double)file_size));
  
  
  return(A_OK);
}

status_t stat_anonftp(file_list)
  char **file_list;
{
  /* File information */

  char **curr_file;

  ip_addr_t ipaddr = 0;

  int tmp_val;
  
  struct stat statbuf;
  
  full_site_entry_t *curr_ent;

  int total_sites;

  int unique_files;
  
  MINT *files_total;
  MINT *dirs_total;
  MINT *tmps;

  long files_nums = 0;
  long dirs_nums = 0;
  long index_size_total = 0;
  int act_size;
  long site_size_total = 0;
  int i;
  
  file_info_t *curr_finfo = create_finfo();

  ptr_check(file_list, char*, "check_indiv", ERROR);


  files_total = itom(0);
  dirs_total = itom(0);
  
  total_sites = 0;

  if ( verbose )  {
      printf("\n\n%-26s %10s %10s %12s \n\n","Site", "Files", "Dirs",
             "Files+Dirs");
  }
    
  for(curr_file = file_list, total_sites = 0; *curr_file != (char *) NULL; curr_file++){
    pathname_t filename,tmp;
    char *port;
    int flag;

    if ( strncmp(*curr_file, "Stridx.", 7) == 0 ) {
      pathname_t tmp;
      strcpy(tmp, files_db_filename(*curr_file,-1));
      if ( stat(tmp,&statbuf) == -1 )
        continue;
      index_size_total += statbuf.st_size;
    }

    if ( strrcmp(*curr_file, ".idx") == 0 )
      continue;
    
    if(sscanf(*curr_file, "%*d.%*d.%*d.%d:%*d", &tmp_val) != 1)
    continue;

    total_sites++;

    strcpy(filename,*curr_file);
    port = strchr(filename,':');
    if ( port == NULL )
    continue;
    *port++ = '\0';
    ipaddr = inet_addr(filename);

    strcpy(curr_finfo -> filename, files_db_filename(filename,atoi(port)));
    

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


    site_size_total += curr_finfo -> size;

    act_size = curr_finfo -> size / sizeof(full_site_entry_t);

  
    curr_ent = (full_site_entry_t *) curr_finfo -> ptr + 0;
    flag = 0;

    files_nums = dirs_nums = 0;
    
    for ( i = 0; i < act_size; i++, curr_ent++) {

      if ( CSE_IS_FILE((*curr_ent)) ) {
        files_nums++;
      }

      if ( CSE_IS_DIR((*curr_ent)) ) {
        dirs_nums++;
      }
      
    }

    madd(files_total, tmps = itom(files_nums), files_total);
    mfree(tmps);
    madd(dirs_total, tmps = itom(dirs_nums), dirs_total);
    mfree(tmps);

    if(verbose){
      printf("%-26s %10ld %10ld %12ld\n", filename, files_nums, dirs_nums,
             files_nums+dirs_nums);
    }

    close_file(curr_finfo);

  }

  if ( count_unique_keywords( &unique_files, FTP) == ERROR ) {
    error(A_ERR,"db_stats", "Cannot count the number of unique keys");
    exit(0);
  }
  
  printf("\n\nTotals:\n\n");
  printf("Number of sites: %d\n",total_sites);
  printf("Number of files: "); mout(files_total);
  printf("Number of dirs: ");  mout(dirs_total);
  printf("Unique files+dirs: %d\n",unique_files);

  printf("\n\nDatabase stats:\n\n");
  printf("Size of site files: %ld\n",site_size_total);
  printf("Size of Index: %ld\n",  index_size_total);
  printf("Database size: %ld\n\n", site_size_total + index_size_total);
  
  return(A_OK);
}




