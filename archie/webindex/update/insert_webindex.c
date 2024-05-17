/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h>
#include <string.h>
#include <memory.h>
#include <fcntl.h>
#include "typedef.h"
#include "db_files.h"
#include "header.h"
#include "db_ops.h"
#include "webindexdb_ops.h"

#include "site_file.h"
#include "start_db.h"
#include "lang_startdb.h"
#include "patrie.h"
#include "archstridx.h"

#include "protos.h"
#include "host_db.h"
#include "insert_webindex.h"
#include "error.h"
#include "debug.h"
#include "lang.h"
#include "lang_webindex.h"
#include "files.h"
#include "master.h"
#include "archie_dbm.h"
#include "archie_strings.h"
#include "archie_mail.h"


extern int errno;

/*
   This function does a rename across partitions 
   The operation is performed by copying the file to the other 
   partition. To guarantee that the operation be done properly
   it is first copied to the other partition under a different
   name and then renamed.
   -Sandro Feb 8th 1995
*/

#define COPY_FILENAME "temp_COPY_file"
#define BUFF_SIZE 8192

#if 0
#ifdef __STDC__

   extern int rename(char *, char *);

#else

   extern int rename();

#endif
#endif


status_t insert_hstart_db();
status_t insert_compare_hstart_db( );
extern status_t count_extra_recs();
extern status_t host_table_find();
extern status_t host_table_add();
extern status_t update_start_dbs();
extern status_t close_start_dbs();
extern status_t open_start_dbs();
extern status_t inactivate_if_found();
extern status_t get_port();

/*  In order to allow an open file that will change in size to 
    either expand or shrink according to no_recs relative to what 
    currently is in the file */


status_t resize_output_file(output_finfo,no_recs)
   file_info_t *output_finfo;
   int no_recs;
{

#ifdef __STDC__

   extern int ftruncate(int, off_t);

#else

   extern int ftruncate();

#endif

   /* Check pointers */

   ptr_check(output_finfo, file_info_t, "resize_output_file", ERROR);

   /* Open the file */

   if(open_file(output_finfo,O_RDWR) == ERROR){

      /* "Can't open output file %s" */

      error(A_SYSERR,"resize_output_file", SETUP_OUTPUT_FILE_004, output_finfo -> filename);
      return(ERROR);
   }

   /*
    * We know how many records there are, so extend the new file to the
    * correct length
    */

   if((int) ftruncate(fileno(output_finfo -> fp_or_dbm.fp), (no_recs) * sizeof(full_site_entry_t)) != 0){

      /* "Can't ftruncate output file %s" */

      error(A_SYSERR,"resize_output_file", SETUP_OUTPUT_FILE_005, output_finfo -> filename);
      return(ERROR);
   }

   close_file(output_finfo);

   /*
    *  now map it into core /
    *  
    *    if(mmap_file(output_finfo, O_RDWR ) != A_OK){
    *  
    *  
    *       / "Can't mmap() output file %s" /
    *  
    *       error(A_INTERR, "resize_output_file", SETUP_OUTPUT_FILE_006,
    *       output_finfo -> filename); return(ERROR);
    *    }
    */

   return(A_OK);
}

static void unique( list, sz )
  index_t **list;
  int *sz;
{
  int i, bef, aft, rep;
  for( i=0, bef=0, aft=1, rep=0; aft<*sz ; i++ ){
    if( (*list)[bef] == (*list)[aft] ){
      aft++;
      rep = 1;
    }else if( (*list)[bef] < (*list)[aft] && rep == 1){
      rep = 0;
      bef++;
      (*list)[bef] = (*list)[aft];
    }else{
      bef++;
      aft++;
    }
  }
  *sz = bef+1;
}



/*
 * This is the main controlling routine for the insert_webindex program.
 * This program inserts information given in the input file into the archie
 * webindex database. It modifies the hostaux_db with the new information

   argv, argc are used.


   Parameters:	  -i <Input filename>  Mandatory
		  -w <webindex files database pathname>
		  -M <master database pathname>
		  -h <host database pathname>
		  -t <temporary directory pathname>
		  -l write to log file (default)
		  -L <log file>
		  

 */


int verbose = 0;  /* verbose */
char *prog;


int main(argc, argv)
   int argc;
   char *argv[];

{


#if 0 
#ifdef __STDC__

  extern int getopt(int, char **, char *);
  extern int ftruncate(int, off_t);

#else

  extern int getopt();
  extern int ftruncate();

#endif
#endif
   
  extern int opterr;            /* NOT USED */ 
  extern char *optarg;          /* option argument */



  char **cmdline_ptr;           /* command line arguments */
  int cmdline_args;             /* command line arg count */

  int option;                   /* Option character */
  
  int minidxsize = MIN_INDEX_SIZE;

  /* System directories */

  pathname_t master_database_dir;
  pathname_t start_database_dir;
  pathname_t files_database_dir;
  pathname_t host_database_dir;
  pathname_t tmp_dir;
  pathname_t file1, file2;

  pathname_t logfile;

  flags_t flags = 0;

  /* file handles */

  file_info_t *input_finfo = create_finfo();
  file_info_t *excerpt_finfo = create_finfo();
  file_info_t *hostbyaddr = create_finfo();
  file_info_t *hostdb = create_finfo();
  file_info_t *hostaux_db = create_finfo();
  file_info_t *output_finfo = create_finfo();
  file_info_t *curr_finfo = create_finfo();

  file_info_t *start_db = create_finfo();
  file_info_t *domain_db = create_finfo();

  int port = 0;
  int curr_size;
  int index_file = 0;
  int output_rec_no;

  header_t header_rec;          /* Input header record */

  host_status_t host_st;        /* Checking host status in database */

  u32 offset;                   /* Offset of start of data in input */

  
  struct arch_stridx_handle *strhan;
  char *dbdir;
  int first = 0;
  int ans = 0;
  int logging = FALSE;          /* write to log */

  prog = argv[0];
  input_finfo -> filename[0] = logfile[0] = '\0';
  excerpt_finfo->filename[0] = '\0';
  
  opterr = 0;

  /* ignore argv[0] */

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;

  master_database_dir[0] = files_database_dir[0] =  start_database_dir[0] =  host_database_dir[0] =  tmp_dir[0] = '\0';

  logfile[0] = '\0';

  /* Process command line options */

  while((option = getopt(argc, argv, "i:w:vt:h:M:sL:l")) != EOF){

    switch(option){

      /* Master directory name */

    case 'M':
	    strcpy(master_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* start/host database directory */

    case 'T':
	    strcpy(start_database_dir,optarg);
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

      /* host database directory name */

    case 'h':
	    strcpy(host_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* Input filename */

    case 'i':
	    strcpy(input_finfo -> filename,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* write to log */

    case 'l':
	    logging = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* Standalone mode... kept for backward compatibility */

    case 's':
	    cmdline_ptr++;
	    cmdline_args--;
	    break;


      /* temporary directory */

    case 't':
	    strcpy(tmp_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* Verbose */

    case 'v':
	    verbose = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* Minimum Index Size */

    case 'I':
      minidxsize = atoi(optarg);
      cmdline_ptr++;
      cmdline_args--;
      break;

      /* files database directory name */

    case 'w':
	    strcpy(files_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

    default:
	    error(A_ERR,"insert_webindex","Unknown option '%s'\nUsage: insert_webindex  -i <input> -w <webindex files database pathname>  -M <master database pathname> -T <start database pathname>  -h <host database pathname> -t <temporary directory pathname> -I <min index file size> -v -s -l -L <log file>", *cmdline_ptr);
	    exit(A_OK);

    }

  }
   

#ifdef SLEEP
  fprintf(stderr,"sleeping \n");
  sleep(30);
#endif
  
  /* Set up log files */

  if(logging){
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open default log file" */

        error(A_ERR, "insert_webindex", INSERT_WEBINDEX_020);
        exit(A_OK);
      }
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, "insert_webindex", INSERT_WEBINDEX_019, logfile);
        exit(A_OK);
      }
    }
  }


  /* Check that input file was given */

  if(input_finfo -> filename[0] == '\0'){

    /* "No site name or address given" */

    error(A_ERR,"insert_webindex", INSERT_WEBINDEX_024);
    exit(A_OK);
  }

  /* Open system files */

  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR, "insert_webindex", INSERT_WEBINDEX_022);
    exit(A_OK);
  }

#ifdef SLEEP
  fprintf(stderr,"sleeping.. 0\n");
  sleep(20);
#endif
   
  if((dbdir = (char *)set_wfiles_db_dir(files_database_dir)) == (char *) NULL){

    /* "Error while trying to set webindex database directory" */

    error(A_ERR, "insert_webindex", INSERT_WEBINDEX_022);
    exit(A_OK);
  }

  if((set_start_db_dir(start_database_dir, DEFAULT_WFILES_DB_DIR)) == (char *) NULL){

    /* "Error while trying to set start database directory" */

    error(A_ERR, "insert_webindex", "Error while trying to set start database directory\n");
    exit(A_OK);
  }

  if(set_host_db_dir(host_database_dir) == (char *) NULL){

    /* "Error while trying to set host database directory" */

    error(A_ERR, "insert_webindex", INSERT_WEBINDEX_023);
    exit(A_OK);
  }


  if(tmp_dir[0] == '\0')
  sprintf(tmp_dir,"%s/%s", get_master_db_dir(), DEFAULT_TMP_DIR);


  /* Open the input file and read its header */

  if(open_file(input_finfo, O_RDONLY) == ERROR){

    /* "Can't open input file %s" */

    error(A_SYSERR,"insert_webindex", INSERT_WEBINDEX_001, input_finfo -> filename);
    exit(A_OK);
  }

  if(read_header(input_finfo -> fp_or_dbm.fp, &header_rec, &offset, 0, 0) == ERROR){

    /* "Can't read header record of %s" */

    error(A_ERR,"insert_webindex", INSERT_WEBINDEX_002, input_finfo -> filename);
    exit(A_OK);
  }

  input_finfo -> offset = offset;

  if(mmap_file(input_finfo, O_RDONLY) == ERROR){

    /* "Can't mmap() input file %s" */

    error(A_SYSERR,"insert_webindex", INSERT_WEBINDEX_003, input_finfo -> filename);
    exit(A_OK);
  }

  if((int)(input_finfo -> size - offset) <= 0){

    /* "Input file %s contains no data after header" */

    error(A_ERR, "insert_webindex", INSERT_WEBINDEX_026, input_finfo -> filename);
    exit(A_OK);
  }

/*  fprintf(stderr,"records before recountig :no_recs=%li  site_no_recs=%li\n",header_rec.no_recs,header_rec.site_no_recs); */

  if( count_extra_recs( input_finfo->ptr, input_finfo->size, &header_rec) == ERROR){

    /* "Can't count extra records to be added to output file %s" */

    error(A_WARN, "insert_webindex","Can't count extra records to be added to output file %s", output_finfo -> filename);
    goto unlink_output;
  }

/*  fprintf(stderr,"records after recountig :no_recs=%li  site_no_recs=%li\n",header_rec.no_recs,header_rec.site_no_recs);*/
   
  /*   header_rec.no_recs = final_no_recs; */

  if(setup_output_file(output_finfo, &header_rec, tmp_dir) == ERROR){

    /* "Can't set up output file %s" */

    error(A_WARN, "insert_webindex", INSERT_WEBINDEX_017, output_finfo -> filename);
    goto unlink_output;
  }

  /* Open other files database files */

  if ( open_start_dbs(start_db,domain_db,O_RDWR ) == ERROR ) {

    /* "Can't open start/host database" */

    error(A_ERR, "insert_webindex", "Can't open start/host database");
    goto unlink_output;
  }

  if ( !(strhan = archNewStrIdx()) ) {
    error( A_ERR,"insert_webindex","Could not create string handler");
    goto unlink_output;
  }
  
  if ( archStridxExists( dbdir, &ans) ){

    switch (ans){

    case 1:                     /* all the files exist */
      if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_APPEND ) ) {
        error( A_ERR, "insert_webindex", "All the Strings files exist but could not be open! Check you permissions. Exiting");
        exit(ERROR);
      }
      break;

    case -1:                    /* some files exist */
      error( A_ERR, "insert_webindex", "Some of the Strings files exist but not all of them. Couldn't open Strings files. Exiting");
      exit(ERROR);
      break;

    case 0:                     /* no strings files exist. */
      error( A_INFO, "insert_webindex", "Could not find Strings files, will create them." );
      first = 1;

      if ( !(strhan = archNewStrIdx()) ) {
        error( A_ERR, "insert_webindex", "Could not create a string handler");
        exit(A_OK);
      }
    
      if ( !archCreateStrIdx( strhan, dbdir, ARCH_INDEX_WORDS ) ) {
        error( A_ERR, "insert_webindex", "Could not create strings_idx files." );
        exit(A_OK);
      }
      break;
        
    }
  }else{
    error( A_ERR, "insert_webindex", "archStridxExists() failed. Something is terribly wrong! Report to archie-group@bunyip.com");
    exit(ERROR);
  }

  if(open_host_dbs(hostbyaddr, hostdb, (file_info_t *) NULL, hostaux_db, O_RDWR) != A_OK){

    /* "Can't open master database directory" */

    error(A_ERR, "insert_webindex", NOT_OPEN_MASTER_000);
    goto unlink_output;
  }

  /* "Doing hostname lookup" INSERT_WEBINDEX_006 */

  if(verbose)
  error(A_INFO,"insert_webindex","Inactivating the site if found in the database. ");

  if(inactivate_if_found(header_rec.primary_hostname, WEBINDEX_DB_NAME, header_rec.preferred_hostname, header_rec.access_command, hostaux_db) == ERROR){
    /* "Error while trying to inactivate %s in host database" */
    error(A_ERR,"insert_webindex", DELETE_WEBINDEX_009, header_rec.primary_hostname);
    goto unlink_output;
  }

  if((host_st = do_hostdb_update(hostbyaddr, hostdb, hostaux_db, &header_rec, WEBINDEX_DB_NAME, flags, 0)) != HOST_OK){
    if(host_st == HOST_IGNORE){
      close_file(input_finfo);
      error(A_INFO,"insert_webindex","Host ignored upon flag request.");
      goto unlink_output;
    }
    /* "Error updating host database: %s" */
    error(A_ERR,"insert_webindex", INSERT_WEBINDEX_007, get_host_error(host_st));
    goto unlink_output;
  }

  /* If the input record has a failure then we end here */

  if(header_rec.update_status == FAIL){
    if(unlink(output_finfo -> filename) == -1){
      /* "Can't unlink failed output file %s" */
      error(A_SYSERR, "insert_webindex", INSERT_WEBINDEX_025, output_finfo -> filename);
      exit(ERROR);
    }
    exit(A_OK);
  }

  /* "Reading input" */

  if(verbose)
  error(A_INFO,"insert_webindex", INSERT_WEBINDEX_009);

  if(setup_insert(strhan, input_finfo -> ptr,
                  (full_site_entry_t *) output_finfo -> ptr,
                  input_finfo->size,  &output_rec_no,
                  header_rec.site_no_recs ) == ERROR){

    error(A_ERR, "insert_webindex", INSERT_WEBINDEX_010);
    goto unlink_output;
  }

  if(activate_site(&header_rec, WEBINDEX_DB_NAME, hostaux_db) == ERROR){

    /* "Can't activate host database record for %s" */

    error(A_ERR,"insert_webindex", INSERT_WEBINDEX_016, header_rec.primary_hostname);
    exit(A_OK);
  }

  if( get_port( header_rec.access_command, WEBINDEX_DB_NAME, &port ) == ERROR ){
    error(A_ERR,"insert_webindex","Could not find the port for this site", header_rec.primary_hostname);
    exit(A_OK);
  }
  
  if(access((char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(header_rec.primary_ipaddr)),port), F_OK) < 0){

    /* "File %s does not exist in the database. No need to compare" */

    error(A_INFO,"insert_webindex", "File %s does not exist in the database. No need to compare",
          wfiles_db_filename(inet_ntoa( ipaddr_to_inet(header_rec.primary_ipaddr)),port));

    /* I need to have another processing here where I only insert the new_file
       into the host/start database */

    if(insert_hstart_db( (full_site_entry_t *)output_finfo -> ptr , 
                        start_db, domain_db, header_rec, port ) == ERROR )
    {
      error(A_ERR,"insert_webindex","Can't insert into start/host database site %s\n",
            (char *)(inet_ntoa( ipaddr_to_inet(header_rec.primary_ipaddr)),port));
      goto unlink_output;
    }
  }
  else
  {
    /*  This is where two of the site files are open. The newly created and the
     *  old site file. They are read, sorted according to their starts, compared
     *  and then the old strings are removed while the new strings are inserted
     *  into the starts/host database.
     */
   
   
    strcpy(curr_finfo -> filename, (char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(header_rec.primary_ipaddr)),port));

    /* Open current file */

    if(open_file(curr_finfo, O_RDONLY) == ERROR){
   
      /* "Ignoring %s" */
   	 
      error(A_ERR, "check_indiv", CHECK_INDIV_003, curr_finfo->filename);
      goto unlink_output;
    }
   
    /* mmap it */
   
    if(mmap_file(curr_finfo, O_RDONLY) == ERROR){
   
      /* "Ignoring %s" */
    
      error(A_ERR, "check_indiv", CHECK_INDIV_003, curr_finfo ->filename);
      goto unlink_output;
    }
   
    curr_size = curr_finfo -> size / sizeof(full_site_entry_t);
      
    if( insert_compare_hstart_db((full_site_entry_t *) output_finfo -> ptr,
                                 header_rec.site_no_recs,
                                 (full_site_entry_t *) curr_finfo -> ptr, 
                                 curr_size, start_db, header_rec, port ) == ERROR )
    {
      /* "Error in compare_site_files for site %s." */
   
      error(A_WARN,"insert_webindex","Error in compare_site_files for site %s\n." ,(char *)(inet_ntoa( ipaddr_to_inet(header_rec.primary_ipaddr))));
      goto unlink_output;
    }
  }


  /*
   *  Index File Creation
   */
  if ( output_rec_no * sizeof(full_site_entry_t) >= minidxsize ) {
    index_file = 1;
    if ( create_site_index(output_finfo->filename,output_finfo->ptr,output_rec_no) == ERROR ) {
      index_file = 0;
    }
  }
  
  close_file(output_finfo);
  resize_output_file( output_finfo, output_rec_no );
  
  if(rename(output_finfo -> filename, (char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(header_rec.primary_ipaddr)),port)) != 0){

    if ( errno != EXDEV ||
        archie_rename(output_finfo -> filename,
                    wfiles_db_filename(inet_ntoa( ipaddr_to_inet(header_rec.primary_ipaddr)),port)) != 0){
	    /* "Can't rename %s to %s. webindex database not changed." */
	    error(A_SYSERR,"insert_webindex", INSERT_WEBINDEX_013, output_finfo -> filename, wfiles_db_filename(inet_ntoa( ipaddr_to_inet(header_rec.primary_ipaddr)),port));
	    goto unlink_output;
    }
  }

  sprintf(file1,"%s%s",output_finfo->filename, SITE_INDEX_SUFFIX);
  sprintf(file2,"%s%s",wfiles_db_filename(inet_ntoa( ipaddr_to_inet(header_rec.primary_ipaddr)),port),SITE_INDEX_SUFFIX);
    
  if ( index_file ) {

    if(rename(file1, file2) != 0 ) {
      if ( errno != EXDEV || 
          archie_rename(file1, file2) != 0 ) {

        /* "Can't rename %s to %s. webindex database not changed." */
	    
        error(A_SYSERR,"insert_webindex", INSERT_WEBINDEX_013, file1, file2);
        goto unlink_output;
      }
    }
  }else{
    (void)unlink(file2);
  }

  sprintf(output_finfo->filename,"%s.excerpt",(char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(header_rec.primary_ipaddr)),port));

  strcpy(excerpt_finfo->filename,input_finfo->filename);
  {
    char *t;
    t = excerpt_finfo->filename + strlen(excerpt_finfo->filename)-8;

    /* Now t points to update_t */
    strcpy(t,"excerpt");


#if 0    
    if (rename(excerpt_finfo->filename,excerpt_out) != 0 ) {

      if ( errno != EXDEV ||
          archie_rename(excerpt_finfo -> filename,excerpt_out) != 0) {

        /* "Can't rename %s to %s. webindex database not changed." */
        error(A_SYSERR,"insert_webindex", INSERT_WEBINDEX_013, excerpt_finfo -> filename,excerpt_out);
        goto unlink_output;
      }
    }
    if ( verbose )
    error(A_INFO,"insert_webindex","renaming %s to %s \n",excerpt_finfo->filename,excerpt_out);
#endif

    if ( open_file(excerpt_finfo,O_RDONLY) == ERROR ) {
      /* "Can't open input file %s" */
      
      error(A_SYSERR,"insert_webindex", INSERT_WEBINDEX_001, excerpt_finfo -> filename);
      exit(A_OK);
    }
    

    if(read_header(excerpt_finfo -> fp_or_dbm.fp, &header_rec, &offset, 0, 0) == ERROR){

      /* "Can't read header record of %s" */

      error(A_ERR,"insert_webindex", INSERT_WEBINDEX_002, excerpt_finfo -> filename);
      exit(A_OK);
    }

    excerpt_finfo -> offset = offset;

    if(mmap_file(excerpt_finfo, O_RDONLY) == ERROR){

      /* "Can't mmap() input file %s" */

      error(A_SYSERR,"insert_webindex", INSERT_WEBINDEX_003, excerpt_finfo -> filename);
      exit(A_OK);
    }

    if((int)(excerpt_finfo -> size ) <= 0){

      /* "Input file %s contains no data after header" */

      error(A_ERR, "insert_webindex", INSERT_WEBINDEX_026, excerpt_finfo -> filename);
      exit(A_OK);
    }

    if ( open_file(output_finfo,O_WRONLY) == ERROR ) {
      /* "Can't open input file %s" */
      
      error(A_SYSERR,"insert_webindex", INSERT_WEBINDEX_001, output_finfo -> filename);
      exit(A_OK);
    }

    if ( fwrite(excerpt_finfo->ptr,sizeof(char),excerpt_finfo->size, output_finfo->fp_or_dbm.fp) != excerpt_finfo->size ){
      error(A_ERR,"insert_webindex","Error writing out the excerpt");
      exit(A_OK);
    }

    close_file(output_finfo);
    close_file(excerpt_finfo);
    unlink(excerpt_finfo->filename);
  }
  

  if ( first ){ 
    if ( ! archUpdateStrIdx(strhan)) {
      error( A_WARN, "insert_webindex", "Could not update strings_idx files.\n");
      archCloseStrIdx(strhan);
      archFreeStrIdx(&strhan);
      exit(A_OK);
    }
  } 
  
  close_file(input_finfo);
  close_start_dbs(start_db,domain_db);
  close_host_dbs(hostbyaddr, hostdb, (file_info_t *) NULL, hostaux_db);  
  archCloseStrIdx(strhan);
  archFreeStrIdx(&strhan);
  
  destroy_finfo(input_finfo);
  destroy_finfo(excerpt_finfo);
  destroy_finfo(hostbyaddr);
  destroy_finfo(hostdb);
  destroy_finfo(hostaux_db);
  destroy_finfo(output_finfo);
  destroy_finfo(curr_finfo);
  destroy_finfo(start_db);
  destroy_finfo(domain_db);

  return(A_OK);                 /* to make gcc happy */
  exit(A_OK);

unlink_output:

  close_file(output_finfo);
  if ( index_file ) {
    pathname_t file1;

    sprintf(file1,"%s%s",output_finfo->filename, SITE_INDEX_SUFFIX);
    (void)unlink(file1);
  }
  close_file(input_finfo);
  close_host_dbs(hostbyaddr, hostdb, (file_info_t *) NULL, hostaux_db);
  close_start_dbs(start_db,domain_db);
  archCloseStrIdx(strhan);
  archFreeStrIdx(&strhan);

  unlink(output_finfo -> filename);

  destroy_finfo(input_finfo);
  destroy_finfo(excerpt_finfo);
  destroy_finfo(hostbyaddr);
  destroy_finfo(hostdb);
  destroy_finfo(hostaux_db);
  destroy_finfo(output_finfo);
  destroy_finfo(curr_finfo);
  destroy_finfo(start_db);
  destroy_finfo(domain_db);
  
  return(A_OK);                 /* to make gcc happy */
  exit(A_OK);
}

   
/*
 * setup_output_file: given the header record and a temporary directory,
 * create an output file and mmap it filling int the output_finfo
 * structure. We return ERROR on error.
 */

status_t setup_output_file(output_finfo, header_rec, tmp_dir)
   file_info_t *output_finfo;
   header_t *header_rec;   /* Header record to use */
   char *tmp_dir;	   /* pathname of temporary directory */
{

#ifdef __STDC__
   extern int ftruncate(int, off_t);
#else
   extern int ftruncate();
#endif

   char *str;

   /* Check pointers */

   ptr_check(output_finfo, file_info_t, "setup_output_file", ERROR);
   ptr_check(header_rec, header_t, "setup_output_file", ERROR);
   ptr_check(tmp_dir, char, "setup_output_file", ERROR);

   /* If the number of records is not greater than 0 then return */

   if((header_rec -> update_status == SUCCEED) && (header_rec -> no_recs <= 0)){

      /* "Number of records %d. Ignoring" */
      error(A_WARN,"setup_output_file", SETUP_OUTPUT_FILE_001, header_rec -> no_recs);
      return(ERROR);
   }

   if(header_rec -> update_status == FAIL)
      return(A_OK);

   /* Get a temporary name for the output file */
   str = tempnam(tmp_dir, DEFAULT_TMP_PREFIX);

   if(str){
     strcpy(output_finfo -> filename, str);
     free(str);
   }
   else{

     /* "Can't get temporary name for file %s." */

     error(A_INTERR,"preprocess_file", SETUP_OUTPUT_FILE_003, header_rec -> primary_hostname);
     return(ERROR);
   }

   /* Open the file */


   if(open_file(output_finfo,O_WRONLY) == ERROR){

      /* "Can't open output file %s" */

      error(A_SYSERR,"setup_output_file", SETUP_OUTPUT_FILE_004, output_finfo -> filename);
      return(ERROR);
   }

   /*
    * We know how many records there are, so extend the new file to the
    * correct length
    */
   

   if((int) ftruncate(fileno(output_finfo -> fp_or_dbm.fp), (header_rec -> site_no_recs ) * sizeof(full_site_entry_t)) != 0){

      /* "Can't ftruncate output file %s" */

      error(A_SYSERR,"setup_output_file", SETUP_OUTPUT_FILE_005, output_finfo -> filename);
      return(ERROR);
   }


   /* now map it into core */

   if(mmap_file(output_finfo, O_WRONLY) != A_OK){


      /* "Can't mmap() output file %s" */

      error(A_INTERR, "setup_output_file", SETUP_OUTPUT_FILE_006, output_finfo -> filename);
      return(ERROR);
   }

   return(A_OK);
}




   
/*
 * reopen_output_file: reopen an existing file that was mmaped and remap it.
 */


status_t reopen_output_file(output_finfo)
   file_info_t *output_finfo;
{
  /* Check pointers */

  ptr_check(output_finfo, file_info_t, "reopen_output_file", ERROR);

  /* Open the file */

  if(open_file(output_finfo,O_RDWR) == ERROR){

    /* "Can't open output file %s" */

    error(A_SYSERR,"reopen_output_file", SETUP_OUTPUT_FILE_004, output_finfo -> filename);
    return(ERROR);
  }


  /* now map it into core */

  if(mmap_file(output_finfo, O_RDWR ) != A_OK){


    /* "Can't mmap() output file %s" */

    error(A_INTERR, "reopen_output_file", SETUP_OUTPUT_FILE_006, output_finfo -> filename);
    return(ERROR);
  }

  return(A_OK);
}


int intcompare(i,j)
index_t *i, *j;
{
   return((int)(*i - *j));
}



/*  create_str_array: ceates an array of sorted starts from the array
 *  of records of type full_site_entry_t. sz is the size of the input array.
 */

status_t create_str_array( fp, sz, array, arsz )
   full_site_entry_t *fp;
   int sz;
   int *array;
   int *arsz;
{
  int no , i;

  i = 0;
  for ( no=0; no<sz ; no++ )
  {
    if (CSE_IS_DIR(fp[no]) ||
        CSE_IS_FILE(fp[no]) ||
        CSE_IS_LINK(fp[no]) ||
        CSE_IS_DOC(fp[no])||
        CSE_IS_KEY(fp[no]) )
    {
      if( i >= *arsz )
      {   error( A_ERR,"insert_webindex","failed in create_str_array");
          return(ERROR);
        }
          
      array[i] = fp[no].strt_1;
      i++;
    }
  }
  *arsz = i;
  qsort(array,*arsz,sizeof(int),intcompare);
  return (A_OK);
}



/*  insert_hstart_db: takes as argument an array of full_site_entry_t records
 *  and for each start in the file: it inserts the site into the host/start
 *  database. returns 0 on success.
 */

status_t insert_hstart_db( fp, start_db, domain_db, header_rec, port)
  full_site_entry_t *fp;       /* file pointer contains an array of records */
  file_info_t *start_db;
  file_info_t *domain_db;
  header_t    header_rec;
  int port;
{
  int no, sz;
  index_t in;
  ip_addr_t ip;
  host_table_index_t hin;


  sz = header_rec.site_no_recs;
  ip = header_rec.primary_ipaddr;

  if ( host_table_find( &ip, header_rec.primary_hostname, &port, &hin ) == ERROR )
  {                             /* "New site %s will be added to host/start table\n" */
    error(A_WARN,"insert_webindex","New site %s will be added to host/start table\n",
          (char *)(inet_ntoa( ipaddr_to_inet(ip))));
    if ( host_table_add( ip, header_rec.primary_hostname, port, &hin ) == ERROR )
    {
      /* "insert_hstart_db : Could not add host %s to host_table" */
      error(A_ERR,"insert_webindex","insert_hstart_db : Could not add host %s to host_table",
            (char *)(inet_ntoa( ipaddr_to_inet(ip))) );
      return ERROR;
    }
    /*fprintf(stderr, "host: %s (%s) hin=%d\n", (char *)(inet_ntoa( ipaddr_to_inet(ip))), header_rec.primary_hostname, hin);
     */
  }
 
  for ( no=0; no<sz ; no++ )
  {
    if (CSE_IS_DIR(fp[no]) ||
        CSE_IS_FILE(fp[no])||
        CSE_IS_LINK(fp[no])||
        CSE_IS_DOC(fp[no])||
        CSE_IS_KEY(fp[no]))
    {
      in = fp[no].strt_1;
      if ( update_start_dbs(start_db, in, hin, ADD_SITE ) == ERROR )
      {
        /* "Could not update start/host table with  %s ." */
        error(A_ERR,"insert_webindex","Could not update start/host table with  %s .\n",
              (char *)(inet_ntoa( ipaddr_to_inet(ip))) );
        return ERROR;
      }
    }
  }

  error(A_INFO,"insert_webindex","Successful update in start/host dbase with %s .",
        (char *)(inet_ntoa( ipaddr_to_inet(ip))) );
  return(A_OK);
}



/*  insert_compare_hstart_db: takes two arrays of full_site_entry_t as args
 *  , sorts those two arrays according to strings indexes and compares the
 *  two files inserting whatever in the first not the second in start/host
 *  dbase and deleting whatever in the second not the first in the dbase. 
 *  returns 0 on sucsses.
 */

status_t insert_compare_hstart_db( fp1, s1, fp2, s2, start_db, header_rec, port )
   full_site_entry_t *fp1;
   int s1;
   full_site_entry_t *fp2;
   int s2;
   file_info_t *start_db;
   header_t    header_rec;
   int port;
{
  int i, k, asz1, asz2;
  index_t *ar1, *ar2;
  ip_addr_t ip;
  host_table_index_t hin;


  ip = header_rec.primary_ipaddr;
  error(A_INFO,"insert_webindex","Starting insert_compare_hstart_db on %s\n",
        (char *)(inet_ntoa( ipaddr_to_inet(ip))) );

  ar1 = (index_t *)malloc(sizeof(index_t)*s1);
  ar2 = (index_t *)malloc(sizeof(index_t)*s2);

  k=0;
  /* This is the new file */
  for( i=0 ; i<s1; i++ )
  {
    if (CSE_IS_DIR(fp1[i]) ||
        CSE_IS_FILE(fp1[i])||
        CSE_IS_LINK(fp1[i])||
        CSE_IS_DOC(fp1[i])||
        CSE_IS_KEY(fp1[i]))
    {
      ar1[k++] = fp1[i].strt_1;
    }
  }
  asz1 = k;

  k=0;
  /* This is the old file */
  for( i=0 ; i<s2; i++ )
  {
    if (CSE_IS_DIR(fp2[i]) ||
        CSE_IS_FILE(fp2[i])||
        CSE_IS_LINK(fp2[i])||
        CSE_IS_DOC(fp2[i])||
        CSE_IS_KEY(fp2[i]))
    {
      ar2[k++] = fp2[i].strt_1;
    }
  }  
  asz2 = k;

  qsort( (index_t *) ar1, asz1, sizeof(index_t), intcompare);
  qsort( (index_t *) ar2, asz2, sizeof(index_t), intcompare);
  unique( &ar1, &asz1 );
  unique( &ar2, &asz2 );

  /* Must check if there is a site in the host database for this host.
     Error if there is none. */

  /* i will be the ar1 pointer and k will be the ar2 pointer */


  if ( host_table_find( &ip, header_rec.primary_hostname, &port, &hin ) == ERROR )
  {                             /* "Site %s should be in host/start table but is not. Corruption\n" */
    error(A_ERR,"insert_compare_hstart_db","Site %s should be in host/start table but is not. Corruption\n",
          (char *)(inet_ntoa( ipaddr_to_inet(ip))));
    return ERROR;
  }

  i = k = 0;
  while( (i<asz1) && (k<asz2) )
  {
    if( ar1[i] < ar2[k] )       /* new file has a new element */
    {
      /* add ar1[i] to the host/start db */
      if ( update_start_dbs(start_db, ar1[i], hin, ADD_SITE ) == ERROR )
      {                         /* "Could not update start/host table with  %s ." */
        error(A_ERR,"insert_compare_hstart_db","Could not update start/host table with  %s .\n",
              (char *)(inet_ntoa( ipaddr_to_inet(ip))) );
        return ERROR;
      }
      i++;
    }else if( ar1[i] > ar2[k] ) /* old file has an old element */
    {
      /* remove ar2[k] from the host/start db */
      if ( update_start_dbs(start_db, ar2[k], hin, DELETE_SITE ) == ERROR )
      {                         /* "Could not update start/host table with  %s ." */
        error(A_ERR,"insert_compare_hstart_db","Could not update start/host table with  %s .\n",
              (char *)(inet_ntoa( ipaddr_to_inet(ip))) );
        return ERROR;
      }
      k++;
    }else if( ar1[i] == ar2[k] ) /* both files have a shared element */
    {
      i++; k++;
    }
  }

  while( i<asz1 )
  {
    /* add ar1[i] to the host/start db */
    if ( update_start_dbs(start_db, ar1[i], hin, ADD_SITE ) == ERROR )
    {                           /* "Could not update start/host table with  %s ." */
      error(A_ERR,"insert_compare_hstart_db","Could not update start/host table with  %s .\n",
            (char *)(inet_ntoa( ipaddr_to_inet(ip))) );
      return ERROR;
    }
    i++;     
  }

  while( k<asz2 )
  {
    /* remove ar2[k] from the host/start db */
    if ( update_start_dbs(start_db, ar2[k], hin, DELETE_SITE ) == ERROR )
    {                           /* "Could not update start/host table with  %s ." */
      error(A_ERR,"insert_compare_hstart_db","Could not update start/host table with  %s .\n",
            (char *)(inet_ntoa( ipaddr_to_inet(ip))) );
      return ERROR;
    }
    k++;
  }
  error(A_INFO,"insert_webindex","Finished insert_compare_hstart_db on %s\n",
        (char *)(inet_ntoa( ipaddr_to_inet(ip))) );

  if( ar1!=NULL ){
    free( ar1 );
    ar1=NULL;
  }
  if( ar2!=NULL ){
    free( ar2 );
    ar2=NULL;
  }

  return( A_OK );
}


