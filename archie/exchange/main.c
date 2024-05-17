/*
 * This file is copyright Bunyip Information Systems Inc., 1994. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */



#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "typedef.h"
#include "db_files.h"
#include "db_ops.h"
#include "host_db.h"
#include "listd.h"
#include "error.h"
#include "lang_exchange.h"
#include "files.h"
#include "master.h"

/*
 * arserver: In server mode, this process is invoked by inetd and thus
 * performs all operations on stdin and stdout. Other modes are client and
 * retrieval manager


   argv, argc are used.


   Parameters:	  -C <config file>
		  -L <log file>
		  -M <master database pathname>
		  -S (sleep)
      -e (expand domains)
		  -f <force hosts>
		  -h <host database pathname>
		  -j (just list)
		  -l (write to default log file)
		  -F <server>
		  -T <timeout>
		  

 */

/* verbose mode */

char *prog;
int verbose = 0;


int main(argc, argv)
   int argc;
   char *argv[];

{
  extern char *optarg;
#if 0
#ifdef __STDC__

  extern int strcasecmp(char *, char *);
  extern int getopt(int, char **, char *);
  extern int sleep(int);

#else

  extern int strcasecmp();
  extern int getopt();
  extern int sleep();


#endif      
#endif
  char **cmdline_ptr;
  int cmdline_args;

  int option;
  pathname_t master_database_dir;
  pathname_t host_database_dir;

  pathname_t databases;

  pathname_t logfile;
  hostname_t fromhost;

  pathname_t force_hosts;

  pathname_t tmp_dir;


  udb_config_t	config_info[MAX_NO_SOURCE_ARCHIES];

  /* server mode by default */

  int server_mode = 1;
  int ret_manager = 0;

  file_info_t *hostbyaddr = create_finfo();
  file_info_t *hostdb = create_finfo();
  file_info_t *domaindb = create_finfo();
  file_info_t *hostaux_db = create_finfo();

  file_info_t  *config_file = create_finfo();

  int sleep_switch = 0;

  int expand_domains = 1;

  int justlist = FALSE;

  int logging = 0;

  int timeout = DEFAULT_TIMEOUT;

  int compress_trans = 0;

  prog = argv[0];
  host_database_dir[0] = '\0';
  master_database_dir[0] = '\0';
  logfile[0] = '\0';

  force_hosts[0] = '\0';
  fromhost[0] = '\0';

  databases[0] = '\0';

  tmp_dir[0] = '\0';
   
  if(strcmp(tail(argv[0]), RET_MANAGER_NAME) == 0){
    ret_manager = 1;
    server_mode = 0;
  }

  if(strcmp(tail(argv[0]), CLIENT_NAME) == 0){
    server_mode = 0;
  }

  /* disable error messages of getopt */

  /*   opterr = 0; */

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;

  /* BUG: this has to be changed */


  setbuf(stdin, (char *) NULL);
  setbuf(stdout, (char *) NULL);




  while((option = (int) getopt(argc, argv, "h:M:f:SjvlL:C:F:T:t:ecud:")) != EOF){

    switch(option){

      /* configuration file */

    case 'C':
      strcpy(config_file -> filename, optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;

      /* from host */

    case 'F':
      strcpy(fromhost, optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;
	    

      /* log file name */

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

      /* Sleep for 20 seconds after startup. Debugging */

    case 'S':
      sleep_switch = 1;
      cmdline_ptr++;
      cmdline_args--;
      break;


      /* master database directory name */

    case 'T':
      timeout = atoi(optarg) * 60;
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;


      /* compress transmission */

    case 'c':
      compress_trans = 1;
      cmdline_ptr++;
      cmdline_args--;
      break;

      /* database list */

    case 'd':
      strcpy(databases, optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;

      /* expand domains */

    case 'e':
      expand_domains = 0;
      cmdline_ptr++;
      cmdline_args--;
      break;

      /* force hosts */

    case 'f':
      strcpy(force_hosts, optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;

      /* host database directory name */

    case 'h':
      strcpy(host_database_dir,optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;
	 
      /* run retrieval manager but only list the sites to be retrieved */

    case 'j':
      justlist = TRUE;
      cmdline_ptr++;
      cmdline_args--;
      break;
	    
      /* logging. Default file */

    case 'l':
      logging = 1;
      cmdline_ptr++;
      cmdline_args--;
      break;

    case 't':
      strcpy(tmp_dir, optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;

      /* force uncompression on transmission */

    case 'u':
      compress_trans = -1;
      cmdline_ptr++;
      cmdline_args--;
      break;

      /* verbose */

    case 'v':
      verbose = 1;
      cmdline_ptr++;
      cmdline_args--;
      break;

    default:
      error(A_ERR,"arexchange","Unknown option '%s'\nUsage: arexchange   Parameters:	  -C <config file>  -L <log file>  -M <master database pathname>  -S (sleep)  -e (expand domains)	  -f <force hosts>  -h <host database pathname>  -j (just list)  -l (write to default log file)  -F <server>  -T <timeout>", *cmdline_ptr);
      exit(A_OK);
    }

  }

  /*
   * For debugging. If you want to attach the debugger to the server after
   * it has started up this gives you the time to do it
   */

  if(server_mode && sleep_switch)
  sleep(20);

  if(server_mode)
  logging = 1;

  if(logging){
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open default log file" */
        error(A_ERR, "arserver", ARSERVER_001);
        exit(ERROR);
      }

    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, "arserver", ARSERVER_002, logfile);
        exit(ERROR);
      }

    }
  }

  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR,"arserver", ARSERVER_003);
    exit(ERROR);
  }


  if(set_host_db_dir(host_database_dir) == (char *) NULL){

    /* "Error while trying to set host database directory" */

    error(A_ERR,"arserver", ARSERVER_004);
    exit(ERROR);
  }



  if(open_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db, O_RDONLY) != A_OK){

    /* "Error while trying to open host database" */

    error(A_ERR,"arserver", ARSERVER_005);
    exit(ERROR);
  }


  if(tmp_dir[0] == '\0')
  sprintf(tmp_dir, "%s/%s", get_master_db_dir(), DEFAULT_TMP_DIR);

  if(!ret_manager){

    if(config_file -> filename[0] == '\0'){
      sprintf(config_file -> filename, "%s/%s/%s", get_archie_home(), DEFAULT_ETC_DIR, DEFAULT_ARUPDATE_CONFIG);
    }
  }
  else{
   
    if(config_file -> filename[0] == '\0'){
      sprintf(config_file -> filename, "%s/%s/%s", get_archie_home(), DEFAULT_ETC_DIR, DEFAULT_RETRIEVE_CONFIG);
    }
  }


  if(read_arupdate_config(config_file, config_info) == ERROR){

    /* "Error reading configuration file %s" */

    error(A_ERR, "arserver", ARSERVER_006, config_file);
    exit(ERROR);

  }


  if(server_mode){


    if(check_authorization(config_info) == ERROR){


      write_net_command(C_AUTH_ERR, (void *) NULL, stdout);

      /* "Authorization error" */

      error(A_ERR, "arserver", ARSERVER_007);
      exit(ERROR);
    }

    if(do_server(hostbyaddr,hostdb,hostaux_db,domaindb, config_info, timeout, compress_trans) == ERROR){

      /* "Error while performing server operations" */

      error(A_ERR, "arserver", ARSERVER_008);
      exit(ERROR);
    }

  }
  else{

    if(do_client(hostbyaddr,hostdb,hostaux_db,domaindb,config_info, force_hosts, databases, ret_manager, justlist, fromhost, timeout, expand_domains, compress_trans) == ERROR){

      /* "Error while performing client operations" */

      error(A_ERR, "arserver", ARSERVER_009);
      exit(ERROR);
    }

    if(!justlist && (force_hosts[0] == '\0')){
      if(write_arupdate_config(config_file, config_info, tmp_dir) == ERROR){

        /* "Error while rewriting configuration file %s" */

        error(A_ERR, "arserver", ARSERVER_010, config_file);
        exit(ERROR);
      }
    }
  }

  if(logging)
  close_alog();


  exit(A_OK);
  return(A_OK);
}


