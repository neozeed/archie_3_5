/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include "protos.h"
#include "typedef.h"
#include "db_files.h"
#include "header.h"
#include "error.h"
#include "master.h"
#include "retrieve_web.h"
#include "lang_retrieve.h"
#include "files.h"
#include "archie_dns.h"

int verbose = 0;

int signal_set = 0;

int mypid = 0;
int force = 0;

char curr_hostname[MAX_HOSTNAME_LEN];
int  curr_port;

int compress_flag = 1;		/* Actively compress resulting retreived info == 1
					   Actively uncompress  == -1
					   Do nothing == 0
             Actively gzip == 2
             Actively gunzip == -2 */

           
int level = 0;

int debug = 0;
char *prog = "retrieve_webindex";

int main(argc, argv)
   int argc;
   char *argv[];


{
   extern char *optarg;

/*   extern int getopt PROTO((int, char **, char *)); */

   char **cmdline_ptr;
   int cmdline_args;

   AR_DNS *hs;

   pathname_t master_database_dir;

   pathname_t logfile;
   pathname_t outname;
   hostname_t tmp_hostname;

   int logging = 0;

   file_info_t *header_file = create_finfo();
   file_info_t *output_file = create_finfo();   
   pathname_t host_database_dir;
   pathname_t tmp_dir;

   header_t header_rec;

   int timeout = DEFAULT_TIMEOUT;
   int sleep_period = DEFAULT_SLEEP_PERIOD;
   int num_retr = 100;
   int option;

   cmdline_ptr = argv + 1;
   cmdline_args = argc - 1;

   host_database_dir[0] = '\0';
   logfile[0] = outname[0] = master_database_dir[0] = '\0';
   tmp_hostname[0] = '\0';

   while((option = (int) getopt(argc, argv, "M:o:l:ZUvni:N:L:T:h:S:t:F")) != EOF){

      switch(option){

	 /* Master database directory name */

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

      case 'N':
        num_retr = atoi(optarg);
        cmdline_ptr += 2;
        cmdline_args -= 2;
        break;

	 /* log messages, given file */

	 case 'L':
            strcpy(logfile, optarg);
	    logging = 1;
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* timeout */

	 case 'T':
	    timeout = atoi(optarg) * 60; /* in minutes */
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* Actively uncompress retrieved information */

	 case 'U':
	    compress_flag = 0; /* The info is not compressed .. */
	    cmdline_ptr ++;
	    cmdline_args --;
	    break;


	 /* host db directory name. Ignored */

	 case 'h':
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* input file */

	 case 'i':
	    strcpy(header_file -> filename, optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* log messages, default file */

	 case 'l':
	    logging = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

	 /* leave data in original format */

	 case 'n':
	    compress_flag = 0;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

	 /* Output file */

	 case 'o':
	    strcpy(output_file -> filename,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* tmp directory */

    case 't':
	    strcpy(tmp_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* verbose mode */

	 case 'v':
	    verbose = 1;
	    cmdline_ptr ++;
	    cmdline_args --;
	    break;

	 case 'Z': 
	    cmdline_ptr ++;
	    cmdline_args --;
	    break;


    case 'S':
	    sleep_period = atoi(optarg); /* in seconds */
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;
      
	 default:

	    /* "Unknown option %c" */

	    error(A_ERR, argv[0], RETRIEVE_WEB_001, option);
	    exit(ERROR);
	    break;
      }
   }
#ifdef SLEEP
   fprintf(stderr,"Sleeping\n");
   sleep(30);
#endif
   
   if(logging){
      if(logfile[0] == '\0'){
        if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

          /*  "Can't open default log file" */

          error(A_ERR, argv[0], RETRIEVE_WEB_002);
          exit(ERROR);
        }
      }
      else{
        if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

          /* "Can't open log file %s" */

          error(A_ERR, argv[0], RETRIEVE_WEB_003, logfile);
          exit(ERROR);
        }
      }
    }
   
   mypid = getpid();
   
   if(header_file -> filename[0] == '\0'){
     
     /* "No input (header) file given" */
     
     error(A_ERR,argv[0], RETRIEVE_WEB_004);
     exit(ERROR);
   }
   
   if(output_file -> filename[0] == '\0'){
     
     /* "No output file template specified" */
     
     error(A_ERR,argv[0], RETRIEVE_WEB_005);
     exit(ERROR);
   }
   
   if(set_master_db_dir(master_database_dir) == (char *) NULL){
     
     /* "Error while trying to set master database directory" */
     
     error(A_ERR,argv[0], RETRIEVE_WEB_006);
     exit(ERROR);
   }
   
   
   if(set_host_db_dir(host_database_dir) == (char *) NULL){
     
     /* "Error while trying to set host database directory" */
     
     error(A_ERR,"update_gopherindex", "Error ");
     exit(ERROR);
   }
   
   if(open_file(header_file, O_RDONLY) == ERROR){
     
     /* "Can't open input file %s" */
     
     error(A_ERR, argv[0], RETRIEVE_WEB_007, header_file -> filename);
     exit(ERROR);
   }
   
   if(read_header(header_file -> fp_or_dbm.fp, &header_rec, (u32 *) NULL, 0, 0) == ERROR){
     
     /* "Can't read header of input file %s" */
     
     error(A_ERR, argv[0], RETRIEVE_WEB_008, header_file -> filename);
     exit(ERROR);
   }
   
   
   if(close_file(header_file) == ERROR){

      /* "Can't close input file %s" */

      error(A_ERR, argv[0], RETRIEVE_WEB_009, header_file -> filename);
   }

   strcpy(curr_hostname, header_rec.primary_hostname);

   if((hs = ar_open_dns_name(curr_hostname, DNS_EXTERN_ONLY, (file_info_t *) NULL)) == (AR_DNS *) NULL){

      error(A_ERR, argv[0], "Can't resolve hostname %s via DNS", curr_hostname);
      exit(ERROR);
   }

   if(cmp_dns_name(curr_hostname, hs) != 0){

      error(A_WARN, argv[0], "Given hostname %s is not primary for host: %s", curr_hostname, get_dns_primary_name(hs));

      strcpy(curr_hostname, get_dns_primary_name(hs));
   }

   ar_dns_close(hs);


   if(do_retrieve(&header_rec, output_file, timeout,sleep_period,num_retr) == ERROR){

      error(A_ERR, argv[0], RETRIEVE_WEB_012, header_file -> filename);

   }

   if(unlink(header_file -> filename) == -1){

	 /* "Can't unlink header file %s" */

     error(A_SYSERR,argv[0], RETRIEVE_WEB_013, header_file -> filename);
   }

   
   if(verbose)
      error(A_INFO, argv[0], "(%d) Done", mypid);

   destroy_finfo( header_file );
   destroy_finfo( output_file );   

   exit(A_OK);
   return(A_OK);
}



