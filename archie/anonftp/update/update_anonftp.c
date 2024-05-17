/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#include <stdio.h>
#include <unistd.h>
#if !defined(AIX) && !defined(SOLARIS)
#include <vfork.h>
#endif
#include <sys/wait.h>
#include <time.h>
#include <stdlib.h>
#include "typedef.h"
#include "header.h"

#include "db_files.h"
#include "db_ops.h"
#include "webindexdb_ops.h"
#include "host_db.h"
#include "update_anonftp.h"
#include "error.h"
#include "archie_dbm.h"
#include "lang_anonftp.h"
#include "files.h"
#include "archie_strings.h"
#include "master.h"
#include "times.h"
#include "archie_mail.h"
#include "patrie.h"
#include "archstridx.h"


#ifdef __STDC__
   extern char *strrchr( char *, char );
#else
   extern char *strrchr();
#endif


status_t get_basename(path, file)
  char *path;
  char *file;
{
  char *t;

  if ( file == NULL ||  path == NULL ) 
    return ERROR;

  t = strrchr(path, '/');
  if (  t == NULL ) {
    strcpy(file,path);
    return A_OK;
  }

  if ( *(t+1) == '\0' ) {
    *t = '\0';
    t = strrchr(path, '/');
    if (  t == NULL ) {
      strcpy(file,path);
      return A_OK;
    }
  }

  t++;
  strcpy(file,t);

  return A_OK;

}


/*
 * update_anonftp.c: perform required updates on the anonftp database.
 * Performs the requried database locking

   argv, argc are used.


   Parameters:	  -i input filename. Mandatory.
		  -w <anonftp files database pathname>
		  -M <master database pathname>
		  -h <host database pathname>
		  -l write to log file (default)
		  -t temporary directory name
		  -I minimum size to be indexed
		  -L <log file>

 */


int verbose = 0;
pathname_t prog;


int main(argc, argv)
   int argc;
   char *argv[];

{
  extern int opterr;
  extern char *optarg;

  extern int errno;
   
#ifndef __STDC__

  extern int getopt();
  extern time_t time();

#endif

  char **cmdline_ptr;
  int cmdline_args;

  int option;
  pathname_t master_database_dir;
  pathname_t host_database_dir;
  pathname_t files_database_dir;

  char minsize[256];

  pathname_t tmp_dir;

  pathname_t logfile;

  pathname_t process_pgm;
  index_t index;
  
  char **arglist;

  int i;

  int logging = 0;

  int minidxsize = 0;

  int tries, finished;
   
  file_info_t *input_file  = create_finfo();
  file_info_t *lock_file = create_finfo();
  file_info_t *hostdb = create_finfo();
  file_info_t *hostaux_db = create_finfo();

  header_t header_rec;

/*   hostdb_t hostdb_rec;*/
  hostdb_aux_t hostaux_ent;

  int status, result;

  char *dbdir;
  
  opterr = 0;
  
  if ( get_basename(argv[0],prog) == ERROR ) {
    strcpy(prog,"update_anonftp");
  }
  
  host_database_dir[0] = master_database_dir[0] = files_database_dir[0] = '\0';
  tmp_dir[0] = logfile[0] = '\0';

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;


  while((option = (int) getopt(argc, argv, "h:M:t:I:L:i:lw:vo:")) != EOF){

    switch(option){

      /* logging with specified file */

    case 'L':
      strcpy(logfile, optarg);
	    logging = 1;
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* master database directory name */

    case 'M':
	    strcpy(master_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;


      /* host database directory name */

    case 'h':
	    strcpy(host_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* input filename */

    case 'i':
      strcpy(input_file -> filename, optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* disregard */
    case 'o':
      break;
      
      /* logging, default file */

    case 'l':
	    logging = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* temporary directory name */

    case 't':
	    strcpy(tmp_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* Minimum Index file size */

    case 'I':
	    minidxsize = atoi(optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* verbose */

    case 'v':
	    verbose = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;
	 

      /* anonftp database directory name */

    case 'w':
	    strcpy(files_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;


    default:
      error(A_ERR,"update_anonftp","Unknown option '%s'\nUsage: update_anonftp	  -i <input filename. Mandatory>  -w <anonftp files database pathname>  -M <master database pathname>  -h <host database pathname>  -l <write to log file (default)>  -t <temporary directory name>  -I <minimum size to be indexed>  -L <log file>", *cmdline_ptr);
      exit(A_OK);
	    break;
    }

  }
  
#ifdef SLEEP  
  fprintf(stderr,"Sleeping\n");
  sleep(5);
#endif
  
  /* set up logs */
   
  if(logging){  
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /*  "Can't open default log file" */

        error(A_ERR, prog, UPDATE_ANONFTP_002);
        exit(ERROR);
      }
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, prog, UPDATE_ANONFTP_001, logfile);
        exit(ERROR);
      }
    }
  }


  if(input_file -> filename[0] == '\0'){

    /* "No input file given" */

    error(A_ERR,prog, UPDATE_ANONFTP_003);
    exit(ERROR);
  }


  /* Set up system directories and files */

  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR,prog, UPDATE_ANONFTP_002);
    exit(ERROR);
  }

  if((dbdir = set_files_db_dir(files_database_dir)) == (char *) NULL){

    /* "Error while trying to set anonftp database directory" */

    error(A_ERR,prog, UPDATE_ANONFTP_003);
    exit(ERROR);
  }


  if(set_host_db_dir(host_database_dir) == (char *) NULL){

    /* "Error while trying to set host database directory" */

    error(A_ERR,prog, UPDATE_ANONFTP_004);
    exit(ERROR);
  }

  if(open_host_dbs((file_info_t *) NULL, hostdb, (file_info_t *) NULL, hostaux_db, O_RDONLY) != A_OK){

    /* "Error while trying to open host database" */

    error(A_ERR,prog, DELETE_ANONFTP_006);
    exit(A_OK);
  }

  if ( strcmp(prog,"update_anonftp" ) == 0 ){
    sprintf(lock_file -> filename,"%s/%s/%s", get_master_db_dir(), DEFAULT_LOCK_DIR, ANONFTP_LOCKFILE);
  }else  if ( strcmp(prog,"update_webindex" ) == 0 ){
    sprintf(lock_file -> filename,"%s/%s/%s", get_master_db_dir(), DEFAULT_LOCK_DIR, WEBINDEX_LOCKFILE);
  }

  if(open_file(input_file, O_RDONLY) == ERROR){

    /* "Can't open given input file %s" */

    error(A_ERR, prog, UPDATE_ANONFTP_006, input_file -> filename);
    return(ERROR);
  }

  if(read_header(input_file -> fp_or_dbm.fp, &header_rec, (u32 *) NULL, 0, 0) == ERROR){

    /* "Can't read header on %s" */

    error(A_ERR,prog, UPDATE_ANONFTP_007, input_file -> filename);
    exit(ERROR);
  }

  close_file(input_file);

  if ( get_hostaux_ent(header_rec.primary_hostname, header_rec.access_methods,
                        &index, header_rec.preferred_hostname,
                        header_rec.access_command, &hostaux_ent, hostaux_db) != ERROR){

/*  if(get_hostaux_ent(header_rec.primary_hostname, header_rec.access_methods, &hostaux_ent, hostaux_db) != ERROR){
*/
    /* Site/database already exists in database */

    /* Check to see if it is older data than currently in database */

    if((header_rec.retrieve_time < hostaux_ent.retrieve_time)
       && !((header_rec.current_status == DEL_BY_ARCHIE) || (header_rec.current_status == DEL_BY_ADMIN))){

      /* "Input file %s has retrieve time older than current entry. Ignoring." */

      error(A_INFO, prog, UPDATE_ANONFTP_031, input_file -> filename);
      if(unlink(input_file -> filename) == -1){

        error(A_SYSERR, prog, "Can't unlink old input file %s", input_file -> filename);
      }
      exit(A_OK);
    }
  }

  for(tries = 0, finished = 0; (tries < MAX_UTRIES) && !finished; tries++){


    if(access(lock_file -> filename, F_OK) == 0){

      /* lock file exists */

      /* "Waiting for lock file %s" */

      error(A_INFO, prog, UPDATE_ANONFTP_028, lock_file -> filename);

      write_mail(MAIL_HOST_FAIL, UPDATE_ANONFTP_028, lock_file -> filename);

      sleep(UPDATE_WAIT);
    }
    else{

      if(errno != ENOENT){

        /* "Error while checking for lockfile %s" */

        error(A_SYSERR, prog, UPDATE_ANONFTP_008, lock_file -> filename);

        write_mail(MAIL_HOST_FAIL, UPDATE_ANONFTP_008, lock_file -> filename);

        exit(ERROR);
      }
	    
      finished = 1;

      if(open_file(lock_file, O_WRONLY) == ERROR){

        /* "Can't open lock file %s" */

        error(A_ERR,prog, UPDATE_ANONFTP_009, lock_file -> filename);

        write_mail(MAIL_HOST_FAIL, UPDATE_ANONFTP_009, lock_file -> filename);
        exit(ERROR);
      }

      /* "Update for %s (%s) at %s" */
	 
      fprintf(lock_file -> fp_or_dbm.fp, UPDATE_ANONFTP_010, header_rec.primary_hostname, input_file -> filename, cvt_from_inttime(time((time_t *) NULL)));
      fflush(lock_file -> fp_or_dbm.fp);
    }
  }

  if(!finished){

    /* "Giving up after %d tries to update %s" */

    error(A_ERR,prog, UPDATE_ANONFTP_011, tries, input_file -> filename);

    write_mail(MAIL_HOST_FAIL, UPDATE_ANONFTP_011, tries, input_file -> filename);
    exit(ERROR);
  }

  /* Check to see if the input file is older than information currently
     in database */

  if((arglist = (char **) malloc(MAX_NO_PARAMS * sizeof(char *))) == (char **) NULL){

    /* "Can't malloc space for argument list" */

    error(A_SYSERR, "cntl_function", UPDATE_ANONFTP_030);
    unlock_db(lock_file);
    exit(ERROR);
  }


  /* if the action status is NEW, then we don't need to delete this site */
   
  if(header_rec.action_status == NEW)
  goto insert_only;

  /* It has been decided to keep the old file around for processing to
   * compare the strings among the new and old files. The new file 
   * will replace the old file without us having to ermove the 
   * first file first.
   */

  /* Delete stuff deleted ! */

  /* database entry is to be deleted */

  if((header_rec.current_status == DEL_BY_ARCHIE) || (header_rec.current_status == DEL_BY_ADMIN)){
    hostdb_aux_t hostaux_rec;
    pathname_t tmp_name;
    index_t indx;
    file_info_t *hostaux_dbase = create_finfo();


    if(open_host_dbs((file_info_t *) NULL, (file_info_t *) NULL, (file_info_t *) NULL, hostaux_dbase, O_RDWR) != A_OK){

      /* "Error while trying to open host database" */

      error(A_ERR,prog, UPDATE_ANONFTP_025);
      write_mail(MAIL_HOST_FAIL, UPDATE_ANONFTP_025);
      unlock_db(lock_file);
      exit(A_OK);
    }

    /** starting actual deletion **/

    sprintf(process_pgm,"%s/%s/%s%s",get_archie_home(),DEFAULT_BIN_DIR, DELETE_PREFIX, header_rec.access_methods);

    /* set up argument list */

    i = 0;
    arglist[i++] = process_pgm;

    arglist[i++] = "-M";
    arglist[i++] = get_master_db_dir();

    arglist[i++] = "-w";
    if( !strcmp(header_rec.access_methods,DEFAULT_WFILES_DB_DIR) )
    arglist[i++] = get_wfiles_db_dir();
    else if( !strcmp(header_rec.access_methods,DEFAULT_FILES_DB_DIR) )
    arglist[i++] = get_files_db_dir();

    if ( tmp_dir[0] != '\0' ) {
      arglist[i++] = "-t";
      arglist[i++] = tmp_dir;
    }
    
    arglist[i++] = "-h";
    arglist[i++] = get_host_db_dir();

    arglist[i++] = "-H";
    arglist[i++] = header_rec.primary_hostname;

    if(logging){

      arglist[i++] = "-L";
      arglist[i++] = get_archie_logname();
    }

    if(verbose)
    arglist[i++] = "-v";

    arglist[i] = (char *) NULL;

    /* "Deleting %s" */

    if(verbose)
    error(A_INFO,prog,UPDATE_ANONFTP_012, header_rec.primary_hostname);

#ifndef AIX
    if((result = vfork()) == 0)
#else
    if((result = fork()) == 0)
#endif
    {
      /* Set logging on */

      execvp(process_pgm, arglist);

      /* "Can't execvp() delete program %s for %s database" */

      error(A_SYSERR,prog, UPDATE_ANONFTP_013 , tail(process_pgm), header_rec.access_methods);
      exit(ERROR);
    }


    if(result == -1){

      /* "Can't vfork() delete program %s" */

      error(A_SYSERR,prog, UPDATE_ANONFTP_014, tail(process_pgm));
      unlock_db(lock_file);
      exit(ERROR);
    }

    if(wait(&status) == -1){

      /* "Error while in wait() for delete program %s" */

      error(A_ERR,prog, UPDATE_ANONFTP_015, tail(process_pgm));
      unlock_db(lock_file);
      exit(ERROR);
    }

    if(WIFEXITED(status) && WEXITSTATUS(status)){

      /* "delete program %s exited with value %u" */

      error(A_ERR,prog, UPDATE_ANONFTP_016, tail(process_pgm), WEXITSTATUS(status));
      write_mail(MAIL_HOST_FAIL, UPDATE_ANONFTP_016, tail(process_pgm), WEXITSTATUS(status));
      exit(ERROR);
    }

    if(WIFSIGNALED(status)){

      /* "Delete program %s terminated abnormally with signal %u" */

      error(A_ERR,prog, UPDATE_ANONFTP_017, tail(process_pgm), WTERMSIG(status));
      write_mail(MAIL_HOST_FAIL, UPDATE_ANONFTP_017, tail(process_pgm), WTERMSIG(status));
      exit(ERROR);
    }

  
    /** ending actual deletion **/

    if ( get_hostaux_ent(header_rec.primary_hostname,header_rec.access_methods,
                         &indx, header_rec.preferred_hostname,
                         header_rec.access_command, &hostaux_rec, hostaux_dbase) == ERROR){

      /* "Can't get auxiliary database entry for site %s database %s for deletion" */
      
      error(A_INFO, prog, UPDATE_ANONFTP_026, header_rec.primary_hostname, header_rec.access_methods);
    }
    else{

      hostaux_rec.current_status = DELETED;
      sprintf(tmp_name,"%s.%s.%d", header_rec.primary_hostname,
              header_rec.access_methods, (int)indx);

      if(put_dbm_entry(tmp_name, strlen(tmp_name) + 1, &hostaux_rec, sizeof(hostdb_aux_t), hostaux_dbase, 1) == ERROR){

        /* "Can't write deletion record for %s to auxiliary host database" */

        error(A_ERR, prog, UPDATE_ANONFTP_027, tmp_name);
      }

      write_mail(MAIL_HOST_DELETE, "%s %s %d" , header_rec.primary_hostname,
                 header_rec.access_methods, (int)indx ) ;
    }

    unlock_db(lock_file);

    close_host_dbs((file_info_t *) NULL, (file_info_t *) NULL, (file_info_t *) NULL, hostaux_dbase);

    destroy_finfo(hostaux_dbase);

    unlink(input_file -> filename);
   
    exit(A_OK);
  }
  
insert_only:
  
  /* perform insertion */

  sprintf(process_pgm,"%s/%s/%s%s",get_archie_home(),DEFAULT_BIN_DIR, INSERT_PREFIX, header_rec.access_methods);

  /* set up argument list */

  i = 0;
  arglist[i++] = process_pgm;

  arglist[i++] = "-M";
  arglist[i++] = get_master_db_dir();

  arglist[i++] = "-w";
  if( !strcmp(header_rec.access_methods,DEFAULT_WFILES_DB_DIR) )
  arglist[i++] = get_wfiles_db_dir();
  else if( !strcmp(header_rec.access_methods,DEFAULT_FILES_DB_DIR) )
  arglist[i++] = get_files_db_dir();

  arglist[i++] = "-h";
  arglist[i++] = get_host_db_dir();

  arglist[i++] = "-i";
  arglist[i++] = input_file -> filename;

  if ( tmp_dir[0] != '\0' ) {
    arglist[i++] = "-t";
    arglist[i++] = tmp_dir;
  }
    
  if ( minidxsize != 0 ) {
    arglist[i++] = "-I";
    sprintf(minsize,"%d",minidxsize);
    arglist[i++] = minsize;
  }

  if(logging){

    arglist[i++] = "-L";
    arglist[i++] = get_archie_logname();
  }

  if(verbose)
  arglist[i++] = "-v";

  arglist[i] = (char *) NULL;

  /* "Inserting %s into %s with %s" */

  if(verbose)
  error(A_INFO,prog,UPDATE_ANONFTP_018, header_rec.primary_hostname, header_rec.access_methods, tail(process_pgm));

#ifndef AIX
  if((result = vfork()) == 0)
#else
  if((result = fork()) == 0)
#endif
  {
    /* Set logging on and make it not standalone */

    execvp(process_pgm, arglist);

    /* "Can't execvp() insert program %s for %s database" */
      
    error(A_SYSERR,UPDATE_ANONFTP_019, tail(process_pgm), header_rec.access_methods);
    unlock_db(lock_file);
    exit(ERROR);
  }


  if(result == -1){

    /* "Can't vfork() insert program %s" */

    error(A_SYSERR,prog, UPDATE_ANONFTP_020, tail(process_pgm));
    unlock_db(lock_file);
    write_mail(MAIL_HOST_FAIL, UPDATE_ANONFTP_020, tail(process_pgm));
    exit(ERROR);
  }

  if(wait(&status) == -1){

    /* "Error while in wait() for insert program %s" */

    error(A_ERR,prog, UPDATE_ANONFTP_021, tail(process_pgm));
    unlock_db(lock_file);
    write_mail(MAIL_HOST_FAIL, UPDATE_ANONFTP_021, tail(process_pgm));
    exit(ERROR);
  }

  if(WIFEXITED(status) && WEXITSTATUS(status)){

    /* "Insert program %s exited with value %u" */

    error(A_ERR,prog, UPDATE_ANONFTP_022, tail(process_pgm), WEXITSTATUS(status));
    write_mail(MAIL_HOST_FAIL, UPDATE_ANONFTP_022, tail(process_pgm), WEXITSTATUS(status));
    exit(ERROR);
  }

  if(WIFSIGNALED(status)){

    /* "Insert program %s terminated abnormally with signal %d" */
   
    error(A_ERR,prog, UPDATE_ANONFTP_023, tail(process_pgm), WTERMSIG(status));
    write_mail(MAIL_HOST_FAIL, UPDATE_ANONFTP_023, tail(process_pgm), WTERMSIG(status));
    exit(ERROR);
  }

  unlock_db(lock_file);

  if(unlink(input_file -> filename) == -1){

    /* "Can't unlink original input file %s" */

    error(A_SYSERR, prog, input_file -> filename);
  }

  /*  We must update the strings index files here */
  
  /*  Here we reset the fail-count to 0 since the insertion was successful 
      August 11 1995 */


  /*update_hostaux_rec_failcount_to_zero: */
  {
    hostdb_aux_t hostaux_rec;
    pathname_t tmp_name;
/*     file_info_t *hostaux_db = create_finfo();*/

    if(open_host_dbs((file_info_t *) NULL, (file_info_t *) NULL, (file_info_t *) NULL, hostaux_db, O_RDWR) != A_OK){

      /* "Error while trying to open host database" */

      error(A_ERR,prog, UPDATE_ANONFTP_025);
      write_mail(MAIL_HOST_FAIL, UPDATE_ANONFTP_025);
      exit(A_OK);
    }

    sprintf(tmp_name,"%s.%s.0", header_rec.primary_hostname, header_rec.access_methods);

    if(get_dbm_entry(tmp_name, strlen(tmp_name) + 1, &hostaux_rec, hostaux_db) == ERROR){

      /* "Can't get auxiliary database entry for site %s database %s for updating information" */
      
      error(A_INFO, prog, UPDATE_ANONFTP_033, header_rec.primary_hostname, header_rec.access_methods);
    }
    else{

      if (  hostaux_rec.fail_count > 0 ) {
        hostaux_rec.fail_count = 0 ; 
        if(put_dbm_entry(tmp_name, strlen(tmp_name) + 1, &hostaux_rec, sizeof(hostdb_aux_t), hostaux_db, 1) == ERROR){

          /* "Can't commit fail-count set to Zero for %s to auxiliary host database" */

          error(A_ERR, prog, UPDATE_ANONFTP_032, tmp_name);
        }
      }
    }
    close_host_dbs((file_info_t *) NULL, (file_info_t *) NULL, (file_info_t *) NULL, hostaux_db);

    destroy_finfo(hostaux_db);
  }

  exit(A_OK);
  return(A_OK);
}
