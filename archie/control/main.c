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
#include "hinfo.h"
#include "header.h"
#include "error.h"
#include "control.h"
#include "lang_control.h"
#include "archie_dbm.h"
#include "archie_strings.h"
#include "files.h"
#include "master.h"
#include "archie_mail.h"
#include "protos.h"
#include "db_ops.h"
#include "patrie.h"
#include "archstridx.h"


/* arcontrol: main controlling program for the archie system


   argv, argc are used.


   Parameters:	  -H <hostname> Mandatory.
		  -w <anonftp files database pathname>
		  -M <master database pathname>
		  -h <host database pathname>
		  -l (write to log file, default)
      -I <minimum size to index a site>
		  -L <log file>
		  -v (verbose)
		  -m (write mail)
		  -c (max count)
		  -T <timeout>  [passed to retrieval function]
		  -U (actively uncompress)   [passed to retrieval function]
		  -n (leave data in orginal form) [passed to retrieval function]
		  -Z (pickup ls-lR.Z or ls-lR files) [passed to retrieval function]
		  
*/


int signal_set = 0;	      /* For timeout alarm */

int verbose = 0;	      /* verbose mode */

int num_retr = 100;
int minidxsize = 0;
int force = 0;

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

  pathname_t logfile;

  char **file_list;
  char *suffix;

  int retr_timeout = DEFAULT_TIMEOUT;
  int retr_sleep_period = DEFAULT_SLEEP_PERIOD;
  
  int maxcount = 0;

  retlist_t *retlist;

  int function = NONE_MAN;

  int pickup_lslR = 0;

  int count = 0;

  int logging = 0;

  int post_process = 0;

  int compress_flag = 1;        /* Actively compress resulting retreived info == 1
                                   Actively uncompress  == -1
                                   Do nothing == 0 */

  host_database_dir[0] = '\0';
  master_database_dir[0] = '\0';

  tmp_dir[0] = logfile[0] = '\0';

  /* disable error messages of getopt */

  opterr = 0;

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;

  prog = argv[0];

  minidxsize = 0;

  while((option = (int) getopt(argc, argv, "h:M:t:I:rpuPL:T:m:vlUN:nZPS:F")) != EOF){

    switch(option){

      /* master database directory */

    case 'F':
      force = 1;
      cmdline_ptr++;
      cmdline_args++;
      break;
     
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

      /* post process not advertized for now */

    case 'E':
	    post_process = 1;
	    cmdline_ptr ++;
	    cmdline_args --;
	    break;

      /* retrieve timeout */

    case 'T':
	    retr_timeout = atoi(optarg) * 60;
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* Retrieve sleep period */
    case 'S':  
      retr_sleep_period = atoi(optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;
      

      /* Actively uncompress retrieved information */

    case 'U':
	    compress_flag = -1;
	    cmdline_ptr ++;
	    cmdline_args --;
	    break;

      /* pickup ls-lR.Z files */

    case 'Z':
	    pickup_lslR = 1;
	    cmdline_ptr ++;
	    cmdline_args --;
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

      /* maximum number of entries to process */

    case 'm':
	    maxcount = atoi(optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;


      /* leave data in original format */
    case 'N':
      num_retr = atoi(optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;
      
    case 'n':
	    compress_flag = 0;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* invoke in partial mode */

    case 'P':
	    function = PARTIAL_MAN;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* invoke in parse mode */
      
    case 'p':
	    function = PARSE_MAN;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* invoke in retrieve mode */

    case 'r':
	    function = RETRIEVE_MAN;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;
	    
      /* temporary directory name */

    case 't':
      strcpy(tmp_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* minimum index size */

    case 'I':
	    minidxsize = atoi(optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* invoke in update mode */

    case 'u':
	    function = UPDATE_MAN;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* verbose mode */

    case 'v':
	    verbose = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

    default:
      error(A_ERR,"arcontrol","Unknown option '%s'\nUsage: arcontrol -H<hostname> -w <anonftp files database pathname> -M <master database pathname> -h <host database pathname> -l -L <log file> -I <minimum size to create index file> -v -m -c -T <timeout> [passed to retrieval function] -U [passed to retrieval function] -S <sleep time> [Web retrieval] -n [passed to retrieval function] -Z [passed to retrieval function] ", *cmdline_ptr);
      exit(A_OK);
    }
  }

  if(function == NONE_MAN){
    error(A_ERR,"arcontrol","Option required 'r', 'p', 'P', or 'u'");
    exit(ERROR);
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

    error(A_ERR,"arcontrol", ARCONTROL_001);
    exit(ERROR);
  }



  if(set_host_db_dir(host_database_dir) == (char *) NULL){

    /* "Error while trying to set host database directory" */

    error(A_ERR,"arcontrol", ARCONTROL_002);
    exit(ERROR);
  }


  /* setup default tmp directory if not given */

  if(tmp_dir[0] == '\0')
  sprintf(tmp_dir,"%s/%s",get_master_db_dir(), DEFAULT_TMP_DIR);

  strcpy(temp_dir,tmp_dir);

  if((function == RETRIEVE_MAN) && (maxcount == 0))
  maxcount = DEFAULT_MAX_COUNT;


  if(post_process && (function != UPDATE_MAN))
  post_process = 0;

  /* determine the input function */

  switch(function){

  case UPDATE_MAN:
    suffix = SUFFIX_UPDATE;
    break;

  case PARSE_MAN:
    suffix = SUFFIX_PARSE;
    break;

  case RETRIEVE_MAN:
    suffix = SUFFIX_RETR;
    break;

  case PARTIAL_MAN:
    suffix = SUFFIX_PARTIAL_UPDATE;
    break;
    
  default:
    error(A_INTERR,"arcontrol", ARCONTROL_003);
    exit(ERROR);
    break;
  }

  if(get_file_list(tmp_dir, &file_list, suffix, &count) == ERROR){

    /* "Can't get list of files to process" */

    error(A_ERR,"arcontrol", ARCONTROL_004);
    exit(ERROR);
  }

  /* no files to process */

  if(count == 0){

    if(verbose){

      /* "No files found for processing" */

      error(A_INFO, "arcontrol", ARCONTROL_009);
    }
      
    exit(A_OK);
  }

  if((retlist = (retlist_t *) malloc(count * sizeof(retlist_t))) == (retlist_t *) NULL){

    /* "Can't allocate space for internal list" */

    error(A_SYSERR,"arcontrol", ARCONTROL_005);
    exit(ERROR);
  }

  if(preprocess_file(tmp_dir, file_list, retlist, maxcount, function) == ERROR){

    /* "Can't properly preprocess input files" */

    error(A_ERR,"arcontrol", ARCONTROL_006);
    exit(ERROR);
  }

  if(cntl_function(retlist,function, retr_timeout, retr_sleep_period, logging, compress_flag, pickup_lslR, post_process) == ERROR){

    /* "Error while trying to perform control actions" */

    error(A_ERR,"arcontrol", ARCONTROL_007);
    exit(ERROR);
  }

#if !defined(__STDC__) && !defined(AIX)

  if(free(retlist) == -1){

    /* "Error while trying to free internal list" */

    error(A_WARN, "arcontrol", ARCONTROL_008);
  }
#else

  free(retlist);

#endif

  if(logging)
  close_alog();

  exit(A_OK);
  return(A_OK);
}


/*
 * preprocess_file: perform actions necessary to make sure that input files
 * are correct format for actual processing
 */

   
status_t preprocess_file(tmp_dir, file_list, retlist, maxcount, function)
   char *tmp_dir;	/* tmp directory name */
   char **file_list;	/* list of input files */
   retlist_t *retlist;	/* internal list */
   int maxcount;	/* maximum number of files to be processed */
   int function;	/* function being performed */
{
#ifdef __STDC__

  extern time_t time(time_t *);

#else

  extern time_t time();
  extern int rename();

#endif

  char **ptr;
  pathname_t filen;
  header_t header_rec;
  u32 offset;
  char *str;
  int status, result;
  flags_t flags = 0, control_flags = 0;
  int count;
  struct stat statbuf;
  pathname_t stopfile;

  char *process_pgm = (char *) NULL;
  pathname_t process_name;
  
  file_info_t *tmp_file = create_finfo();
  file_info_t *input_file = create_finfo();

  file_info_t *hostbyaddr = create_finfo();
  file_info_t *hostdb = create_finfo();
  file_info_t *domaindb = create_finfo();
  file_info_t *hostaux_db = create_finfo();


  ptr_check(tmp_dir, char, "preprocess_file", ERROR);
  ptr_check(file_list, char*, "preprocess_file", ERROR);
  ptr_check(retlist, retlist_t, "preprocess_file", ERROR);

  if(open_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db, O_RDWR) != A_OK){
    /* "Error while trying to open host database" */

    error(A_ERR,"preprocess_file", PREPROCESS_FILE_003);
    return(ERROR);
  }

  /* if we're not retrieving and maxcount is zero then process
     as many files as we have */

  if((function != RETRIEVE_MAN) && (maxcount == 0))
  maxcount = INT_MAX;

  /* for all given files */

  sprintf(stopfile, "%s/%s/%s", get_archie_home(), DEFAULT_ETC_DIR, PROCESS_STOP_FILE);

  for(count = 0, ptr = file_list; (*ptr != (char *) NULL) && (count < maxcount) ; ptr++){

    if(access(stopfile, R_OK | F_OK) != -1){

      write_mail(MAIL_HOST_SUCCESS,"Execution terminated because of stopfile at data file %s", *ptr);
      exit(A_OK);
    }

    sprintf(filen, "%s/%s", tmp_dir, *ptr);
    strcpy(input_file -> filename, filen);

    if(open_file(input_file, O_RDONLY) == ERROR){

      /* "Can't open input file %s" */

      error(A_ERR,"preprocess_file",PREPROCESS_FILE_001, input_file -> filename);
      continue;
    }


    if(fstat(fileno(input_file -> fp_or_dbm.fp), &statbuf) == -1){

      /* "Can't fstat() file %s" */

      error(A_ERR, "preprocess_file", PREPROCESS_FILE_026, input_file -> filename);
      continue;
    }

    if(statbuf.st_size == 0){

      /* "File %s is empty. Ignoring" */

      error(A_ERR, "preprocess_file", PREPROCESS_FILE_027, input_file -> filename);

      close_file(input_file);
      unlink(input_file -> filename);
      continue;
    }
      

    if(read_header(input_file -> fp_or_dbm.fp, &header_rec, &offset, 0, 1) == ERROR){

      /* "Can't read header of %s" */

      error(A_ERR,"preprocess_file",PREPROCESS_FILE_002, input_file -> filename);

      continue;
    }

    /* don't retrieve header files which are delete records */

    if((function == RETRIEVE_MAN) &&
       ((header_rec.current_status == DEL_BY_ADMIN) || (header_rec.current_status == DEL_BY_ARCHIE))){
      pathname_t tmp_string;

      sprintf(tmp_string, "%s/%s-%s%s", tmp_dir, header_rec.primary_hostname, header_rec.access_methods, SUFFIX_UPDATE);

      if(rename(input_file -> filename, tmp_string) == -1){

        /* "Can't rename delete header file from %s to %s" */

        error(A_SYSERR, "preprocess_file", PREPROCESS_FILE_022, input_file -> filename, tmp_string);
      }

      close_file(input_file);
      continue;
    }


    str = get_tmp_filename(tmp_dir);

    if(str){
      strcpy(tmp_file -> filename, str);
      free(str);
    }
    else{

      /* "Can't get temporary name for file %s. Ignoring" */

      error(A_INTERR,"preprocess_file",PREPROCESS_FILE_007, input_file -> filename);
      continue;
    }

    if(open_file(tmp_file, O_WRONLY) == ERROR){

      /* "Can't open temp file %s" */

      error(A_ERR,"preprocess_file",PREPROCESS_FILE_008, tmp_file -> filename);
      continue;
    }
	 
    switch(header_rec.format){

    case FRAW:

	    process_pgm = CAT_PGM;

	    break;

    case FCOMPRESS_LZ:

	    process_pgm = UNCOMPRESS_PGM;

	    break;

    case FCOMPRESS_GZIP:

      if ( get_option_path( "UNCOMPRESS", "GZIP", process_name) == ERROR ) {
        return ERROR;
      }
      process_pgm = process_name;
	    break;

    default:

	    /* "Unknown format for %s. Ignoring" */

	    error(A_INTERR,"preprocess_files", PREPROCESS_FILE_009, input_file -> filename);
	    header_rec.update_status = FAIL;
	    break;
    }

    header_rec.format = FRAW;


    if(header_rec.update_status == FAIL){

      switch(function){

	    case PARSE_MAN:

        header_rec.parse_time = (date_time_t) time((time_t *) NULL);
        HDR_SET_PARSE_TIME(header_rec.header_flags);
        break;

	    case RETRIEVE_MAN:
        header_rec.retrieve_time = (date_time_t) time((time_t *) NULL);
        HDR_SET_RETRIEVE_TIME(header_rec.header_flags);
        break;

	    case UPDATE_MAN:
        {
          hostname_t hostptr;
          access_methods_t dbname;
          AR_DNS *dns_ent;
          hostdb_t orig_hostdb, new_hostdb;
          hostdb_aux_t orig_hostaux_db, new_hostaux_db;
          char newtmp[2*1024], oldtmp[2*1024];
          int j;
          index_t ind;

          ind=0;

          strcpy( hostptr, header_rec.primary_hostname );
          strcpy( dbname, header_rec.access_methods );

          if(get_dbm_entry(hostptr, strlen(hostptr) + 1,
                           &orig_hostdb, hostdb) == ERROR){

            /* "Can't find %s in primary host database" */

            error(A_ERR, "arcontrol", "Can't find %s in primary host database", hostptr);
            goto failure;
          }

          if((dns_ent = ar_open_dns_name(header_rec.preferred_hostname,
                                         DNS_EXTERN_ONLY, hostdb )) == (AR_DNS *) NULL){
            error(A_ERR,"arcontrol","unknown preferred name");
            goto failure;
          }else{

#ifdef SOLARIS
            if((cmp_dns_name(orig_hostdb.primary_hostname, dns_ent) == ERROR) &&
               (cmp_dns_name(header_rec.preferred_hostname, dns_ent) == ERROR)) 
#else
            if(cmp_dns_name(orig_hostdb.primary_hostname, dns_ent) == ERROR) 
#endif
            {
              /* preferred name %s doesn't map back to primary name %s but instead to %s */
              error(A_INTERR, "arcontrol","preferred name %s doesn't map back to primary name %s but instead to %s" , 
                    header_rec.preferred_hostname, 
                    orig_hostdb.primary_hostname,
                    get_dns_primary_name(dns_ent));

              /* make a new copy of the hostdb record */
              if(get_dbm_entry(hostptr, strlen(hostptr) + 1,
                               &new_hostdb , hostdb) == ERROR){

                /* "Can't find %s in primary host database" */

                error(A_ERR, "arcontrol","Can't find %s in primary host database" , hostptr);
                goto failure;
              }

              /* find the new primary name */

              strcpy(new_hostdb.primary_hostname, get_dns_primary_name(dns_ent));

              /* Check if this primary name is in the site already */
              if(get_dbm_entry(new_hostdb.primary_hostname,
                               strlen(new_hostdb.primary_hostname) + 1,
                               &new_hostdb , hostdb) != ERROR){
                error(A_ERR, "arcontrol", "The new primary name %s is also in the database, can't reintroduce it",
                      new_hostdb.primary_hostname);
                goto failure;
              }else if(put_dbm_entry(new_hostdb.primary_hostname,
                                     strlen(new_hostdb.primary_hostname) + 1,
                                     &new_hostdb, sizeof(hostdb_t) , hostdb, TRUE) == ERROR){
                /* "Can't find %s in primary host database" */
                error(A_ERR, "arcontrol", "Can't find %s in primary host database",
                      new_hostdb.primary_hostname);
                goto failure;
              }

              find_hostaux_last(orig_hostdb.primary_hostname,
                                dbname, &ind, hostaux_db);

              for( j=0; j<ind; j++){
                if ( get_hostaux_entry(orig_hostdb.primary_hostname, dbname,
                                       j, &orig_hostaux_db, hostaux_db) == ERROR ) { /* Last hostaux for the site */
                  /* no more! */
                }

                memcpy(  &new_hostaux_db , &orig_hostaux_db, sizeof(hostdb_aux_t) );
                orig_hostaux_db.current_status = DEL_BY_ARCHIE;

                sprintf(oldtmp, "%s.%s.%d", hostptr, dbname, (int)j);
                if(put_dbm_entry(oldtmp, strlen(oldtmp) + 1, &orig_hostaux_db,
                                 sizeof(hostdb_aux_t), hostaux_db, 1) == ERROR){
                  /* "Can't update new or modified auxilary host database entry" */
                  error(A_ERR, "arcontrol", "Can't update new or modified auxilary host database entry");
                  goto failure;
                }

                sprintf(newtmp,"%s.%s.%d",new_hostdb.primary_hostname, dbname, j );
                if(put_dbm_entry(newtmp, strlen(newtmp) + 1, &new_hostaux_db,
                                 sizeof(hostdb_aux_t), hostaux_db, 1) == ERROR){
                  /* "Can't update new or modified auxilary host database entry" */
                  error(A_ERR, "arcontrol","Can't update new or modified auxilary host database entry");
                  goto failure;
                }
              } /* for all the instances of hostaux for this dbname */
            }
            else {
              break;
            }
          }
          
        failure:
          if(write_hostdb_failure(hostdb, hostaux_db, &header_rec, flags, control_flags) == ERROR){

            /* "Can't write failure record to host databases" */

            error(A_INTERR,"preprocess_file", PREPROCESS_FILE_005);
          }

          write_mail(MAIL_HOST_FAIL,"%s %s %s see log files", header_rec.primary_hostname, header_rec.access_methods, header_rec.comment);

          close_file(input_file);
          unlink(input_file -> filename);

          close_file(tmp_file);
          unlink(tmp_file -> filename);
          continue;
          break;
        }
      case PARTIAL_MAN:
        break;
        
	    default:

        /* "Unknown control function %d. Aborting." */

        error(A_INTERR,"preprocess_file",PREPROCESS_FILE_010, function);
        break;
      }

      goto fail_handle;
    }


    if(write_header(tmp_file -> fp_or_dbm.fp, &header_rec, (u32 *) NULL, 0,0) == ERROR){

      /* "Failure writing header. Ignoring %s" */

      error(A_ERR,"preprocess_file", PREPROCESS_FILE_011, header_rec.primary_hostname);
      continue;
    }

    /* So that all processes don't start up too quickly */

    sleep(1);

    if((result = fork()) == 0){


      if(dup2(fileno(input_file -> fp_or_dbm.fp), 0) == -1){

        /* "Can't dup2() input file" */

        error(A_SYSERR, "preprocess_file", PREPROCESS_FILE_013);
        exit(ERROR);
      }

      if(dup2(fileno(tmp_file -> fp_or_dbm.fp), 1) == -1){

        /* "Can't dup2() output file" */

        error(A_SYSERR, "preprocess_file", PREPROCESS_FILE_014);
        exit(ERROR);
      }

      execl(process_pgm, process_pgm, (char *) NULL);

      /* "Can't execl() preprocess program %s" */

      error(A_SYSERR, PREPROCESS_FILE_012, process_pgm);
      exit(ERROR);
    }


    if(result == -1){

      /* "Can't vfork() preprocess program %s" */

      error(A_SYSERR,"preprocess_file", PREPROCESS_FILE_015, process_pgm);
      goto fail_handle;
    }


    if(wait(&status) == -1){

      /* "Error while in wait() for preprocess program %s" */

      error(A_ERR,"preprocess_file",PREPROCESS_FILE_017, process_pgm);
      goto fail_handle;
    }

    if(WIFEXITED(status) && WEXITSTATUS(status)){

      /* "Program %s exited with value %d" */

      error(A_ERR,"preprocess_file",PREPROCESS_FILE_017, process_pgm, WEXITSTATUS(status));
      goto fail_handle;
    }

    if(WIFSIGNALED(status)){

      /* "Preprocess program %s terminated abnormally with signal %d" */

      error(A_ERR,"preprocess_file", PREPROCESS_FILE_018, process_pgm, WTERMSIG(status));
      goto fail_handle;
	    
    }


  fail_handle:

    /* Using filenames as basis for naming. This is a stupid
       thing to do... has to be fixed */

    if(header_rec.update_status != FAIL)
    sprintf(filen,"%s%s", input_file -> filename, TMP_SUFFIX);
    else{
      int finished;

      /* Generate a "random" handle */

      srand(time((time_t *) NULL));

      for(finished = 0; !finished;){

        sprintf(filen,"%s/%s-%s_%d%s", tmp_dir, header_rec.primary_hostname, header_rec.access_methods, rand() % 100, function == PARSE_MAN ? SUFFIX_UPDATE : SUFFIX_PARSE);

        if(access(filen, R_OK | F_OK) == -1)
        finished = 1;
      }

      write_error_header(tmp_file, &header_rec);

    }

    if(close_file(input_file) == ERROR){

      /* "Can't close %s" */

      error(A_ERR,"preprocess_file", PREPROCESS_FILE_020, filen);
    }

    if((tmp_file -> fp_or_dbm.fp != (FILE *) NULL) && (close_file(tmp_file) == ERROR)){

      /* "Can't close %s" */

      error(A_ERR,"preprocess_file", PREPROCESS_FILE_020, tmp_file -> filename);
    }


    if(rename(tmp_file -> filename, filen) == -1){

      /* "Can't rename temporary file %s to %s" */

      error(A_SYSERR,"preprocess_file", PREPROCESS_FILE_021, tmp_file -> filename, *ptr);
    }
   

    if(unlink(input_file -> filename) == -1){

      /* "Can't unlink() original input data file %s" */

      error(A_SYSERR, "preprocess_file", PREPROCESS_FILE_019, input_file -> filename);
    }

    if(header_rec.update_status == FAIL)
    continue;

    strcpy(retlist[count].dbname, header_rec.access_methods);
    strcpy(retlist[count].input, filen);
    strcpy(retlist[count].primary_hostname, header_rec.primary_hostname);
    count++;
  }

  retlist[count].dbname[0] = '\0';
  destroy_finfo(input_file);
  destroy_finfo(tmp_file);

  close_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db);

  destroy_finfo(hostbyaddr);
  destroy_finfo(hostdb);
  destroy_finfo(domaindb);
  destroy_finfo(hostaux_db);

   
  return(A_OK);
}

/*
 * cntl_check_hostdb: verfy the host being processed
 */


host_status_t cntl_check_hostdb(hostbyaddr, hostdb, hostaux_db, header_rec, hdb_rec, flags, control_flags)
   file_info_t *hostbyaddr;   /* host address cache */
   file_info_t *hostdb;	      /* primary host database */
   file_info_t *hostaux_db;   /* auxiliary host database */
   header_t    *header_rec;   /* header record */
   hostdb_t    *hdb_rec;      /* returned primary host database record */
   flags_t     flags;
   flags_t     control_flags;

{
   hostdb_t hostdb_rec;
   hostbyaddr_t hostbyaddr_rec;
   host_status_t host_status = 0;


   ptr_check(hostbyaddr, file_info_t, "cntl_check_hostdb", HOST_PTR_NULL);
   ptr_check(hostdb, file_info_t, "cntl_check_hostdb", HOST_PTR_NULL);
   ptr_check(hostaux_db, file_info_t, "cntl_check_hostdb", HOST_PTR_NULL);   
   ptr_check(header_rec, header_t, "cntl_check_hostdb", HOST_PTR_NULL);
   ptr_check(hdb_rec, hostdb_t, "cntl_check_hostdb", HOST_PTR_NULL);
   

   make_hostdb_from_header(header_rec, &hostdb_rec, flags);

   if((host_status = check_new_hentry(hostbyaddr, hostdb, &hostdb_rec, hostaux_db, NULL, flags, control_flags) != HOST_OK)){

      if((host_status = check_old_hentry(hostbyaddr, hostdb, &hostdb_rec, hostaux_db, NULL, flags, control_flags)) != HOST_OK){

	 /* "Host %s: %s" */

	 error(A_ERR, "cntl_check_hostdb", CNTL_CHECK_HOSTDB_001, hostdb_rec.primary_hostname, get_host_error(host_status));
	 *hdb_rec = hostdb_rec;
	 return(host_status);
      }

      *hdb_rec = hostdb_rec;
   }
   else{

      /* Host is new */

      /* Just add the host, no auxiliary information */

      hostdb_rec.access_methods[0] = '\0';

      if(host_status != HOST_UNKNOWN){

	 if(put_dbm_entry(hostdb_rec.primary_hostname, strlen(hostdb_rec.primary_hostname) + 1, &hostdb_rec, sizeof(hostdb_t), hostdb, FALSE) != A_OK){

	    /* "Error trying to insert new host %s into primary database" */

	    error(A_ERR, "cntl_check_hostdb", CNTL_CHECK_HOSTDB_002, hostdb_rec.primary_hostname);
	    return(HOST_INS_FAIL);
	 }

	 hostbyaddr_rec.primary_ipaddr = hostdb_rec.primary_ipaddr;
	 strcpy(hostbyaddr_rec.primary_hostname, hostdb_rec.primary_hostname);

	 if(put_dbm_entry(&hostbyaddr_rec.primary_ipaddr, sizeof(ip_addr_t), &hostbyaddr_rec, sizeof(hostbyaddr_t), hostbyaddr, FALSE) == ERROR){

	    /* "Error trying to insert new host %s into host address cache" */

	    error(A_ERR, "cntl_check_hostdb", CNTL_CHECK_HOSTDB_003);
	    return(HOST_PADDR_FAIL);
	 }

	 write_mail(MAIL_HOST_ADD, "%s", hostdb_rec.primary_hostname);
      }
      else{

	 /* "Host %s cannot be resolved. Host unknown." */
	 
	 error(A_ERR, "cntl_check_hostdb", CNTL_CHECK_HOSTDB_004, header_rec -> primary_hostname);
         return(HOST_UNKNOWN);
      }
   }

   return(HOST_OK);
}

/*
 * retlist_cmp: compare process ids (pid's) in the retlist type
 */


int retlist_cmp(a, b)
   retlist_t *a;
   retlist_t *b;

{
   if(a -> pid > b -> pid)
      return(1);
   else if(a -> pid < b -> pid)
      return(-1);
   else
      return(0);
}
     
/*
 * cntl_function: perform the action for which the program was called
 */


status_t cntl_function(retlist, function, retr_timeout, retr_sleep_period, logging, compress_flag, pickup_lslR, post_process)
   retlist_t *retlist;
   int function;
   int retr_timeout, retr_sleep_period;
   int logging;
   int compress_flag;
   int pickup_lslR;
   int post_process;
{
  retlist_t *ptr;
  header_t header_rec;
  pathname_t process_pgm;
  pathname_t output_template;
  int result, status;
  file_info_t *input_file = create_finfo();
  pathname_t files_database_dir;
  pathname_t wfiles_database_dir;
  char **arglist, dbs[80]; /* need to think of what size to give it */
/*  struct arch_stridx_handle *strhan;*/
/*  char *dbdir;  */
/*  index_t d; */
/*  char s[1] = "0"; */
  char *tstr;
  int waitstate = CNTL_WAIT;
  int count;
  int i;
  pathname_t time_str;
  pathname_t sleep_str;
  pathname_t stopfile;
  char minsize[256];
  char num_retr_str[256];

  files_database_dir[0] = wfiles_database_dir[0] =  dbs[0] = '\0';

  ptr_check(retlist, retlist_t, "cntl_function", ERROR);

  sprintf(stopfile, "%s/%s/%s", get_archie_home(), DEFAULT_ETC_DIR, PROCESS_STOP_FILE);

  for(count = 0, ptr = retlist; ptr -> dbname[0] != '\0'; ptr++){

    /* Check to see if a file, ~archie/etc/update.stop exists,
       if so, exit without error */

    if(access(stopfile, R_OK | F_OK) != -1){

      write_mail(MAIL_HOST_SUCCESS,"Execution terminated because of stopfile at %s %s", ptr -> primary_hostname, ptr -> dbname);
      exit(A_OK);
    }

    strcpy(input_file -> filename, ptr -> input);

    /* Run the appropriate program on this information */
   
    switch(function){

    case UPDATE_MAN:

	    tstr = UPDATE_PREFIX;
	    waitstate = CNTL_WAIT;
	    break;

    case RETRIEVE_MAN:

	    tstr = RETRIEVE_PREFIX;
	    waitstate = CNTL_DONT_WAIT;
      minidxsize = 0;
	    break;

    case PARSE_MAN:
      
	    tstr = PARSE_PREFIX;
	    waitstate = CNTL_WAIT;
      minidxsize = 0;
	    break;

    case PARTIAL_MAN:
      tstr = PARTIAL_PREFIX;
      waitstate = CNTL_WAIT;
      minidxsize = 0;
      break;
      
    default:

	    /* "Unknown function type: %d" */

	    error(A_INTERR,"cntl_function", CNTL_FUNCTION_001, function);
	    break;
    }

	 
    sprintf(process_pgm,"%s/%s/%s%s",get_archie_home(),DEFAULT_BIN_DIR, tstr, ptr -> dbname);

    sprintf(output_template,"%s/%s-%s",  temp_dir, ptr -> primary_hostname, ptr -> dbname);

    if((arglist = (char **) malloc(MAX_NO_PARAMS * sizeof(char *))) == (char **) NULL){

      /* "Can't malloc space for argument list" */

      error(A_SYSERR, "cntl_function", CNTL_FUNCTION_020);
      return(ERROR);
    }

    /* set up argument list */

    i = 0;
    arglist[i++] = process_pgm;

    arglist[i++] = "-M";
    arglist[i++] = get_master_db_dir();

    arglist[i++] = "-h";
    arglist[i++] = get_host_db_dir();

    arglist[i++] = "-i";
    arglist[i++] = input_file -> filename;

    arglist[i++] = "-o";
    arglist[i++] = output_template;

    arglist[i++] = "-t";
    arglist[i++] = temp_dir;

    if (function == RETRIEVE_MAN && strcasecmp(ptr->dbname,"webindex") == 0 ) {
      arglist[i++] = "-N";
      sprintf(num_retr_str,"%d",num_retr);
      arglist[i++] = num_retr_str;
    }
    
    if( minidxsize != 0 ){
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

    if(function == RETRIEVE_MAN){

      sprintf(time_str, "%d", retr_timeout/60);

      arglist[i++] = "-T";
      arglist[i++] = time_str;

      sprintf(sleep_str, "%d", retr_sleep_period);

      arglist[i++] = "-S";
      arglist[i++] = sleep_str;

      if ( force ) {
        arglist[i++] = "-F";
      }
      
      if(compress_flag == -1)
      arglist[i++] = "-U";
      else if (compress_flag == 0)
      arglist[i++] = "-n";

      if(pickup_lslR == 1)
      arglist[i++] = "-Z";

    }

    arglist[i] = (char *) NULL;

    /* "Running %s on %s" */

    if(verbose){
      int j;
      char holds[1024];
      char thold[1024];

      error(A_INFO, "cntl_function", CNTL_FUNCTION_019, tail(process_pgm), tail(output_template));

      holds[0] = '\0';
	 
      for(j = 1; j < i; j++){

        sprintf(thold, " %s", arglist[j]);
        strcat(holds, thold);
      }
	 
      error(A_INFO, "cntl_function", "Calling %s %s", process_pgm, holds);
    }



    /* So you don't kill the machine */

    sleep(1);

    if((result = fork()) == 0){

      /* Set the master db directory to be the same as this process
         and turn logging on and turn off standalone */

      execv(process_pgm, arglist);

      /* "Can't execl() program %s for %s database" */

      error(A_SYSERR, "cntl_function", CNTL_FUNCTION_002, process_pgm, header_rec.access_methods);
      exit(ERROR);
    }

    if(arglist)
    free(arglist);

    if(result == -1){

      /* "Can't vfork() program %s" */

      error(A_SYSERR,"cntl_function", CNTL_FUNCTION_003, tail(process_pgm));
      return(ERROR);
    }


    if(waitstate == CNTL_WAIT){
      int fail = 0;

      if(wait(&status) == -1){
	 
        /* "Error while in wait() for program %s" */

        error(A_SYSERR,"cntl_function",CNTL_FUNCTION_004, tail(process_pgm));
        fail = 1;
      }

      if(WIFEXITED(status) && WEXITSTATUS(status)){

        /* "Program %s exited with value %u" */

        error(A_ERR,"cntl_function",CNTL_FUNCTION_005, tail(process_pgm), WEXITSTATUS(status));
        fail = 1;
      }


      if(WIFSIGNALED(status)){

        write_mail(MAIL_HOST_FAIL,"Program %s aborted with signal %d for %s %s", tail(process_pgm), WTERMSIG(status), ptr -> primary_hostname, ptr -> dbname);

        /* "Program %s terminated abnormally with signal %u" */
	 
        error(A_ERR,"cntl_function",CNTL_FUNCTION_006, tail(process_pgm), WTERMSIG(status));
        fail = 1;
      }
	 

      if(fail == 0){
        if(function == UPDATE_MAN){
          write_mail(MAIL_HOST_SUCCESS,"%s %s", ptr -> primary_hostname, ptr -> dbname);

          /*  Here we keep track of the different databases
           *  that have been updated
           */
          
          if( !strstr(dbs,ptr -> dbname) )
          strcat(dbs,ptr->dbname);
        }
      }
      else{

        if(function == UPDATE_MAN)
        write_mail(MAIL_HOST_FAIL,"%s %s update failed", ptr -> primary_hostname, ptr -> dbname);
        else if(function == PARSE_MAN)
        write_mail(MAIL_PARSE_FAIL,"%s %s parse failed", ptr -> primary_hostname, ptr -> dbname);
      }
    }
    else{
      ptr -> pid = result;
      strcpy(ptr -> proc_prog, tail(process_pgm));
      strcpy(ptr -> output, output_template);
      count++;
    }
  }


  if(waitstate == CNTL_DONT_WAIT){
    pid_t waitid;
    int i;

    /* sort by pid */

    qsort((char *) retlist, count, sizeof(retlist_t), retlist_cmp);

    for(i = count;i != 0; i--){
      retlist_t curr_ret_rec;

      waitid = wait(&status);

      /* wait for children to exit */

      if(waitid == -1){

        /* "wait() exited abnormally. Continuing" */

        error(A_SYSERR,"cntl_function", CNTL_FUNCTION_015);
        continue;
      }

      curr_ret_rec.pid = waitid;

      if((ptr = (retlist_t *) bsearch ((char *) &curr_ret_rec, (char *) retlist, count, sizeof (retlist_t), retlist_cmp)) == (retlist_t *) NULL){

        /* "Can't find child with pid %d in retlist table!" */

        error(A_INTERR,"cntl_function",CNTL_FUNCTION_016, waitid);
        continue;
      }

      if(WIFEXITED(status) && WEXITSTATUS(status)){

        /* "Program %s (%d) exited with value %d" */

        /* Note: mail will be written from retrieval program */

        error(A_ERR,"cntl_function", CNTL_FUNCTION_017, ptr -> proc_prog, ptr -> pid, WEXITSTATUS(status));

        continue;
      }

      if(WIFSIGNALED(status)){

        /* "Program %s (%d) terminated abnormally with signal %d" */

        error(A_ERR,"cntl_function",CNTL_FUNCTION_018, ptr -> proc_prog, ptr -> pid, WTERMSIG(status));
        continue;
      }

      ptr -> pid = 0;

      qsort((char *) retlist, count, sizeof(retlist_t), retlist_cmp);

    }
  }

  if(!post_process)
  return(A_OK);

  /* Do post processing */

  sprintf(process_pgm,"%s/%s/%s",get_archie_home(),DEFAULT_BIN_DIR, POST_PROCESS_PGM);

  /* If the post processing program does not exist then ignore rest */

  if((access(process_pgm, R_OK | X_OK | F_OK) != -1) && (function == UPDATE_MAN)){
    pid_t waitid;

    if((arglist = (char **) malloc(MAX_NO_PARAMS * sizeof(char *))) == (char **) NULL){

      /* "Can't malloc space for argument list" */

      error(A_SYSERR, "cntl_function", CNTL_FUNCTION_020);
      return(ERROR);
    }

    /* set up argument list */

    i = 0;
    arglist[i++] = process_pgm;

    arglist[i++] = "-M";
    arglist[i++] = get_master_db_dir();

    arglist[i++] = "-h";
    arglist[i++] = get_host_db_dir();

    if(logging){

      arglist[i++] = "-L";
      arglist[i++] = get_archie_logname();
    }

    if(verbose)
    arglist[i++] = "-v";

    arglist[i] = (char *) NULL;

    /* "Running %s on %s" */

    if(verbose){
      int j;
      char holds[1024];
      char thold[1024];

      error(A_INFO, "cntl_function", CNTL_FUNCTION_019, tail(process_pgm), tail(output_template));

      holds[0] = '\0';
	 
      for(j = 1; j < i; j++){

        sprintf(thold, " %s", arglist[j]);
        strcat(holds, thold);
      }
	 
      error(A_INFO, "cntl_function", "Calling %s %s", process_pgm, holds);
    }


    if((result = fork()) == 0){

      execv(process_pgm, arglist);

      /* "Can't execl() program %s for %s database" */

      error(A_SYSERR, "cntl_function", CNTL_FUNCTION_002, process_pgm, header_rec.access_methods);
      exit(ERROR);
    }

    if(arglist)
    free(arglist);

    if(result == -1){

      /* "Can't vfork() program %s" */

      error(A_SYSERR,"cntl_function", CNTL_FUNCTION_003, tail(process_pgm));
      return(ERROR);
    }

    waitid = wait(&status);

    /* wait for children to exit */

    if(waitid == -1){

      /* "wait() exited abnormally. Continuing" */

      error(A_SYSERR,"cntl_function", CNTL_FUNCTION_015);
    }

    if(WIFEXITED(status) && WEXITSTATUS(status)){

      /* "Program %s (%d) exited with value %d" */

      error(A_ERR,"cntl_function", CNTL_FUNCTION_017, process_pgm , waitid, WEXITSTATUS(status));

    }

    if(WIFSIGNALED(status)){

      /* "Program %s (%d) terminated abnormally with signal %d" */

      error(A_ERR,"cntl_function",CNTL_FUNCTION_018, process_pgm, waitid, WTERMSIG(status));
    }
  }

  return(A_OK);
}

/*
 * write_hostdb_failure: host an error entry into the primary and auxiliary
 * host databases
 */


      
status_t write_hostdb_failure(hostdb, hostaux_db, header_rec, flags, control_flags)
   file_info_t *hostdb;	      /* primary host database */
   file_info_t *hostaux_db;   /* auxiliary host database */
   header_t    *header_rec;   /* header record */
   flags_t     flags;
   flags_t     control_flags;
{
#if 0
#ifdef __STDC__

  extern int strcasecmp(char *, char *);
  extern time_t time(time_t *);

#else

  extern int strcasecmp();
  extern time_t time();

#endif
#endif

  hostdb_aux_t hostaux_rec;
  hostdb_t hostdb_rec;
  access_methods_t haux_name;
  pathname_t tmp_string;
  int finished;
  index_t index;


  ptr_check(hostdb, file_info_t, "write_hostdb_failure", ERROR);
  ptr_check(hostaux_db, file_info_t, "write_hostdb_failure", ERROR);
  ptr_check(header_rec, header_t, "write_hostdb_failure", ERROR);   
   



  memset(&hostaux_rec, '\0', sizeof(hostdb_aux_t));

  /* Don't really care if the record is there or not */


  if ( get_hostaux_ent(header_rec -> primary_hostname,
                       header_rec -> access_methods, &index,
                       header_rec -> preferred_hostname,
                       header_rec -> access_command,
                       &hostaux_rec, hostaux_db ) == ERROR ) {
    /*
       if(get_dbm_entry(haux_name, strlen(haux_name) + 1, &hostaux_rec, hostaux_db) == ERROR){
       */
    char **alist;
    char **aptr;

    if(get_dbm_entry(header_rec -> primary_hostname, strlen(header_rec -> primary_hostname) + 1, &hostdb_rec, hostdb) == ERROR){

#if 0
      /* "%s does not exist in database. Shouldn't happen" */

      error(A_INTERR,"write_hostdb_failure",WRITE_HOSTDB_FAILURE_001, header_rec -> primary_hostname);
#endif
      return(A_OK);
    }

    alist = str_sep(hostdb_rec.access_methods, NET_DELIM_CHAR);

    for(finished = 0, aptr = alist; *aptr != (char *) NULL;){

      if(strcasecmp(*aptr, header_rec -> access_methods) == 0){
        finished = 1;
        break;
      }

      aptr++;
    }

    if(!finished){
      *aptr = (char *) strdup(header_rec -> access_methods);

      sprintf(hostdb_rec.access_methods, "%s", *alist);

      aptr = alist + 1;

      while(*aptr != (char *) NULL){

        sprintf(tmp_string, ":%s",*aptr);
        strcat(hostdb_rec.access_methods, tmp_string);
        aptr++;

      }

      if(put_dbm_entry(hostdb_rec.primary_hostname, strlen(hostdb_rec.primary_hostname) + 1, &hostdb_rec, sizeof(hostdb_t), hostdb, TRUE) != A_OK){

        /* "Can't write failure to primary host database for %s" */

        error(A_ERR, "write_hostdb_failure", WRITE_HOSTDB_FAILURE_002, hostdb_rec.primary_hostname);
        return(ERROR);
      }
    }

    if(alist)
    free_opts(alist);
  }

  header_rec -> update_time = (date_time_t) time((time_t *) NULL);
  HDR_SET_UPDATE_TIME(header_rec -> header_flags);

  header_rec -> current_status = INACTIVE;
  HDR_SET_CURRENT_STATUS(header_rec -> header_flags);

  make_hostaux_from_header(header_rec, &hostaux_rec, flags);

  set_aux_origin(hostaux_rec, header_rec -> access_methods, -1);

  hostaux_rec.fail_count++;

  if(hostaux_rec.fail_count > MAX_FAIL)
  header_rec -> current_status = DEL_BY_ARCHIE;
   
  sprintf(haux_name,"%s.%s.%d", header_rec -> primary_hostname, header_rec -> access_methods,index);
  
  if(put_dbm_entry(haux_name, strlen(haux_name) + 1, &hostaux_rec, sizeof(hostdb_aux_t), hostaux_db, TRUE) == ERROR){

    /* "Can't update hostaux rec for %s" */

    error(A_INTERR,"write_hostdb_failure", WRITE_HOSTDB_FAILURE_003, haux_name);
    return(ERROR);
  }

  return(A_OK);
}




