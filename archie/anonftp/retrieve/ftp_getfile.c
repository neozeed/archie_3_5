/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/errno.h>
#include <netdb.h>
#include <malloc.h>
#include <memory.h>
#include <varargs.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#ifdef AIX
#include <sys/select.h>
#endif
#ifdef SOLARIS
#include <sys/utsname.h>
#endif
#include <signal.h>
#include "protos.h"
#include "typedef.h"
#include "db_files.h"
#include "db_ops.h"
#include "ftp_getfile.h"
#include "ftp.h"
#include "header.h"
#include "host_db.h"
#include "protonet.h"
#include "error.h"
#include "archie_strings.h"
#include "files.h"
#include "lang_retrieve.h"
#include "master.h"
#include "archie_mail.h"
#include "times.h"
#include "protos.h"

/*
   ftp_getfile.c: routine to retrieve raw archie data via FTP

   

*/




file_info_t *curr_file;

#define	UC(x)	(int)(((int)(x))&0xff)

int verbose = 0;

int signal_set = 0;

char *prog;

int main(argc, argv)
   int argc;
   char *argv[];


{
   extern char *optarg;
#if 0
#ifdef __STDC__

   extern int getopt(int, char **, char *);

#else

   extern int getopt();
   extern int unlink();

#endif
#endif

   char **cmdline_ptr;
   int cmdline_args;

   pathname_t master_database_dir;
   pathname_t tmp_dir;

   pathname_t logfile;

   int logging = 0;

   file_info_t *header_file = create_finfo();
   file_info_t *defs_file = create_finfo();
   file_info_t *output_file = create_finfo();   

   header_t header_rec;
   retdefs_t *retdefs;

   int globbing = 1;

   int timeout = DEFAULT_TIMEOUT;
   int sleep_period;
   
   int pickup_lslR = 0;

   int compress_flag = 1;		/* Actively compress resulting retreived info == 1
					   Actively uncompress  == -1
					   Do nothing == 0
             Actively gzip == 2
             Actively gunzip == -2 */
   int option;

   cmdline_ptr = argv + 1;
   cmdline_args = argc - 1;

   curr_file = create_finfo();

   master_database_dir[0] = defs_file -> filename[0] = '\0';
   tmp_dir[0] = '\0';
   
   while((option = (int) getopt(argc, argv, "M:o:C:l:Uvni:L:T:gh:ZS:t:F")) != EOF){

      switch(option){

	 /* configuration filename */

	 case 'C':
	    strcpy(defs_file -> filename,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

    case 'F': /* Force retrieve .. not used here */
      cmdline_ptr++;
      cmdline_args++;
      break;
      
	 /* Master database directory name */

	 case 'M':
	    strcpy(master_database_dir,optarg);
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

    case 'S':
	    sleep_period = atoi(optarg); /* in seconds */
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* Actively uncompress retrieved information */

	 case 'U':
	    compress_flag = -1;
	    cmdline_ptr ++;
	    cmdline_args --;
	    break;

	 /* Get index file if it exists */

	 case 'Z':
	    pickup_lslR = 1;
	    cmdline_ptr ++;
	    cmdline_args --;
	    break;

	 /* turn globbing off */

	 case 'g':
	    globbing = 0;
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

	 default:

	    /* "Unknown option %c" */

	    error(A_ERR, argv[0], RETRIEVE_ANONFTP_001, option);
	    exit(ERROR);
	    break;
      }
   }

   if(logging){
      if(logfile[0] == '\0'){
	 if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

	    /*  "Can't open default log file" */

	    error(A_ERR, argv[0], RETRIEVE_ANONFTP_002);
	    exit(ERROR);
	 }
      }
      else{
	 if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

	    /* "Can't open log file %s" */

	    error(A_ERR, argv[0], RETRIEVE_ANONFTP_003, logfile);
	    exit(ERROR);
	 }
      }
   }

   if(header_file -> filename[0] == '\0'){

      /* "No input (header) file given" */

      error(A_ERR,argv[0], RETRIEVE_ANONFTP_004);
      exit(ERROR);
   }

   if(output_file -> filename[0] == '\0'){

      /* "No output file template specified" */

      error(A_ERR,argv[0], RETRIEVE_ANONFTP_005);
      exit(ERROR);
   }

   if(set_master_db_dir(master_database_dir) == (char *) NULL){

      /* "Error while trying to set master database directory" */

      error(A_ERR,argv[0], RETRIEVE_ANONFTP_006);
      exit(ERROR);
   }

   if(open_file(header_file, O_RDONLY) == ERROR){

      /* "Can't open input file %s" */

      error(A_ERR, argv[0], RETRIEVE_ANONFTP_007, header_file -> filename);
      exit(ERROR);
   }

   if(read_header(header_file -> fp_or_dbm.fp, &header_rec, (u32 *) NULL, 0, 0) == ERROR){

      /* "Can't read header of input file %s" */

      error(A_ERR, argv[0], RETRIEVE_ANONFTP_008, header_file -> filename);
      exit(ERROR);
   }

   if(close_file(header_file) == ERROR){

      /* "Can't close input file %s" */

      error(A_ERR, argv[0], RETRIEVE_ANONFTP_009, header_file -> filename);
   }

   sprintf(curr_file -> filename, "%s%s%s", output_file -> filename, SUFFIX_PARSE, TMP_SUFFIX);

   /* compose default configuration file if none given */

   if(defs_file -> filename[0] == '\0')
      sprintf(defs_file -> filename, "%s/%s/%s", get_archie_home(), DEFAULT_ETC_DIR, DEFAULT_RETDEFS);


   if((retdefs = (retdefs_t *) malloc(MAX_RETDEFS * sizeof(retdefs_t))) == (retdefs_t *) NULL){

      /* "Can't malloc() for retrieve defaults" */

      error(A_SYSERR,"ftp_getfile", RETRIEVE_ANONFTP_010);
      exit(ERROR);
   }

   if(read_retdefs(defs_file, retdefs) == ERROR){

      /* "Error reading configuration file %s" */

      error(A_ERR, argv[0], RETRIEVE_ANONFTP_011, defs_file -> filename);
      exit(ERROR);
   }

      
   if(do_retrieve(&header_rec, output_file, header_file, retdefs, compress_flag,timeout, globbing, pickup_lslR) == ERROR){

      /* "Error in retrieve from file %s" */

      error(A_ERR,argv[0], RETRIEVE_ANONFTP_012, header_file -> filename);
   }
   
   free(retdefs);
   
   if(unlink(header_file -> filename) == -1){

	 /* "Can't unlink header file %s" */

	 error(A_SYSERR,argv[0], RETRIEVE_ANONFTP_013, header_file -> filename);
	 exit(ERROR);
   }

   destroy_finfo(header_file);
   
   exit(A_OK);
   return(A_OK);
}


/* do_retrieve: main routine for performing retrieve via FTP */


status_t do_retrieve(header_rec, output_file, input_file, retdefs, compress_flag, timeout, globbing, pickup_lslR)
   header_t *header_rec;      /* input header record  */
   file_info_t *output_file;  /* output file pointer */
   file_info_t *input_file;   /* input file */
   retdefs_t *retdefs;	      /* configuration information */
   int compress_flag;	      /* compression status */
   int timeout;		      /* Timeout on retrieve */
   int globbing;	      /* Globbing on ? */
   int pickup_lslR;	      /* pickup the default index file */

{

#if 0
#ifdef __STDC__

  extern int strcasecmp(char *, char *);

#else

  extern int strcasecmp();
  extern int rename();

#endif
#endif 
  int sleepy_time = RETRY_DELAY;
  int retcode;
  char retr_list[MAX_ACCESS_COMM];
  char retr_string[MAX_ACCESS_COMM];

#ifdef SOLARIS
  struct utsname un;
#endif

  hostname_t hostname;
  ip_addr_t hostaddr, *hostaddr_ptr;

  char user[MAX_ACCESS_METHOD];
  char pass[MAX_ACCESS_METHOD];
  char acct[MAX_ACCESS_METHOD];
  char root_dir[MAX_ACCESS_METHOD];
  struct servent *serv;
  int finished;
  FILE *comm_in, *comm_out;
  int tries;
  int count = 0;
  retdefs_t *rt = (retdefs_t *) NULL;
  AR_DNS *dns;
  char **file_list;
  pathname_t home_dir;

  ptr_check(header_rec, header_t, "do_retrieve", ERROR);
  ptr_check(output_file, file_info_t, "do_retrieve", ERROR);
  ptr_check(retdefs, retdefs_t, "do_retrieve", ERROR);

  retr_list[0] = retr_string[0] = user[0] = pass[0] = acct[0] = root_dir[0] = '\0';


  /* Check what method is to be used to retrieve info */

  if(HDR_GET_ACCESS_COMMAND(header_rec -> header_flags)){

    strcpy(retr_string, header_rec -> access_command);

    if(str_decompose(retr_string,NET_DELIM_CHAR, retr_list,user,pass,acct,root_dir) == ERROR){

      /* "Unable to parse access command in header" */

      error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_001);
      return(ERROR);
    }
    sprintf(header_rec->access_command,"%s:%s:%s:%s",retr_list,user,pass,acct);
      
  }

  header_rec -> generated_by = RETRIEVE;
  HDR_SET_GENERATED_BY(header_rec -> header_flags);

  HDR_UNSET_PARSE_TIME(header_rec -> header_flags);
  HDR_UNSET_NO_RECS(header_rec -> header_flags);
  HDR_UNSET_CURRENT_STATUS(header_rec -> header_flags);   
  HDR_UNSET_HCOMMENT(header_rec -> header_flags);   

#ifndef SOLARIS
  if(gethostname(hostname, sizeof(hostname_t)) == -1){

    /* "get_archie_hostname() failed. Can't get local host name!" */

    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_002);
    return(ERROR);
  }
#else

  if(uname(&un) == -1){

    /* "get_archie_hostname() failed. Can't get local host name!" */

    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_002);
    return(ERROR);
  }

  strcpy(hostname, un.nodename);
   
#endif   

  if((dns = ar_open_dns_name(hostname, DNS_EXTERN_ONLY, (void *) NULL)) == (AR_DNS *) NULL){

    /* "Can't get hostname DNS record for %s" */

    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_003, hostname);
    return(ERROR);
  }

  if((hostaddr_ptr = get_dns_addr(dns)) == (ip_addr_t *) NULL){

    /* "Can't get host address for %s" */

    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_004 , hostname);
    return(ERROR);
  }


  ar_dns_close(dns);

  hostaddr = *hostaddr_ptr;
   

  if(user[0] == '\0' || pass[0] == '\0' || acct[0] == '\0'){

    for(count = 0, rt = &retdefs[0], finished = 0;
        (rt -> access_methods[0] != '\0') && !finished; count++){

      rt = &retdefs[count];

      if((strcasecmp(header_rec -> access_methods, rt -> access_methods) == 0)
         && (header_rec -> os_type == rt -> os_type))
      finished = 1;
    }

    if(!finished){

      /* "No default entry for %s in configuration file from %s"  */

      error_header(curr_file, output_file, count, header_rec,SUFFIX_PARSE,  DO_RETRIEVE_005, header_rec -> access_methods, hostname);
      return(ERROR);
    }

    if(user[0] == '\0')
    if(rt -> def_user[0] != '\0')
    strcpy(user, rt -> def_user);
    else
    strcpy(user, ARCHIE_USER);


    if(pass[0] == '\0')
    if(rt -> def_pass[0] != '\0')
    strcpy(pass, rt -> def_pass);
    else{
	    hostname_t tmp_hostname;
	 
	    sprintf(pass,"%s@%s", ARCHIE_USER, get_archie_hostname(tmp_hostname, sizeof(tmp_hostname)));
    }
  }

  count = 0;

  if((retr_list[0] == '\0') && (rt -> def_ftparg[0] == '\0')){

    /* "Neither access command nor default action provided for %s" */

    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_006, hostname);
    return(ERROR);
  }
      

  if((serv = getservbyname("ftp", "tcp")) == (struct servent *) NULL){

    /* "Can't find service 'ftp/tcp' in list of services" */

    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_007);
    return(ERROR);
  }

  for(tries = 0, finished = 0; (tries < MAX_RETRIEVE_RETRY) && !finished; tries++){

    switch(retcode = ftp_connect(header_rec -> primary_hostname, serv -> s_port, &comm_in, &comm_out, timeout)){

    case A_OK:
	    finished = 1;
	    break;

    case ERROR:                 /* Possible non-permenent error */
    case -CON_SOCKETFAILED:
    case -CON_UNREACHABLE:
    case -CON_NETUNREACHABLE:
    case -CON_TIMEOUT:

	    sleepy_time *= 2;
	    break;

    case -CON_HOST_UNKNOWN:

	    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, "Host unknown: %s", hostname);
	    finished = 2;
	    break;

    case -CON_REFUSED:
	    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, "Connection to %s refused", hostname);
	    finished = 2;
	    break;

    case FTP_LOST_CONN:
	    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, "Connection to %s lost", hostname);
	    finished = 2;
	    break;
            
    default:

	    /* "Error %d while trying to connect to %s" */

	    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_008, retcode, hostname);
	    finished = 2;
	    break;
	    
    }

    if(!finished){
      sleep(sleepy_time);
      finished = 0;
    }
    else
    break;
  }

  /*
   * Either MAX_RETRIEVE_RETRY has been exceeded or a fatal error has
   * occurred
   */

  if(finished == 1){

    switch(ftp_login(comm_in, comm_out, user, pass, acct, timeout)){

    case A_OK:
	    break;

    case FTP_NOT_LOGGED_IN:
    default:

	    /* "Can't login as %s" */

	    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_009, user);
	    return(ERROR);
	    break;
    }
  }
  else{
    char *erstr = get_conn_err(retcode);

    /* "Unable to connect: %s" */

    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_010, erstr);
    return(ERROR);
  }



  /* Novell Systems do not create the ls-lR file correctly...
     the root is not present in the listing.
     */
  if ( header_rec->os_type == NOVELL ) { /* Get the pwd */
    home_dir[0] = '\0';
    get_pwd(output_file, comm_in, comm_out,header_rec,hostname, hostaddr,rt,timeout,home_dir);

    strcat(header_rec -> access_command,":");
    strcat(header_rec -> access_command,home_dir);
  }

  /* Logged in ok, now figure out what to do */

  file_list = str_sep(retr_list,NET_SEPARATOR_CHAR);

  if((file_list == (char **) NULL) || (file_list[0] == '\0')){
    if(pickup_lslR){

      if((file_list = (char **) malloc(sizeof(char *) * 2)) == (char **) NULL){
        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, "Can't malloc for file list");
        return(ERROR);
      }

      file_list[0] = '\0';
      file_list[1] = '\0';

      check_for_lslRZ(comm_in, comm_out, file_list, header_rec, timeout, rt);      
    }
  }
  else{

    /* non-NULL file list. Check to see if files are atend */

    if(strcasecmp(file_list[0], ATEND_FILE) == 0)
    gather_atend(&file_list, input_file);
  }

  if(!file_list){
    /* "Can't determine action (no access_commands)" */

    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_026);
    return(ERROR);
  }

  if((file_list != (char **) NULL) && ((file_list[0] == (char *) NULL) || (strcasecmp(file_list[0], IGNORE_FILE) == 0)))
  get_list(output_file, comm_in, comm_out, header_rec, hostname, hostaddr, compress_flag, rt, timeout);
  else
  get_files(output_file, comm_in, comm_out, header_rec, hostname, hostaddr, compress_flag, rt, timeout, file_list, globbing);

  send_command(comm_in, comm_out, 1, timeout, "QUIT");

  destroy_finfo(curr_file);

#if 0
  /* This is should work .. but generate core dumps on Solaris */
  if ( file_list ) {
    free_opts(file_list);
  }
#endif
  return(A_OK);
}
      
/*
 * read_retdefs: read the given configuration file and place infromation
 * into internal structures
 */
   
status_t read_retdefs(defs_file, retdefs)
   file_info_t *defs_file;    /* configuration file */
   retdefs_t *retdefs;	      /* returned information */

{
#if 0
#ifdef __STDC__

  extern int strcasecmp(char *, char *);

#else

  extern int strcasecmp();

#endif  
#endif 

  int count;
  char input_buf[BUFSIZ];
  retdefs_t *rt;
  char os[MAX_ACCESS_METHOD];
  char *chptr;
  pathname_t tmp_str;

  ptr_check(defs_file, file_info_t, "read_retdefs", ERROR);
  ptr_check(retdefs, retdefs_t, "read_retdefs", ERROR);


  if(open_file(defs_file, O_RDONLY) == ERROR){

    /* "Can't open configuration file %s" */

    error(A_ERR, "read_retdefs", READ_RETDEFS_001, defs_file -> filename);
    return(ERROR);
  }

  for(count = 0; fgets(input_buf, sizeof(input_buf), defs_file -> fp_or_dbm.fp) != (char *) NULL; count++){

    rt = &retdefs[count];

    memset(rt, '\0', sizeof(retdefs_t));

    if((chptr = strrchr(input_buf,'\n')) != (char *) NULL)
    *chptr = '\0';

    /* blank line */

    if(sscanf(input_buf, "%s", tmp_str) <= 0)
    continue;

    if(str_decompose(input_buf, NET_DELIM_CHAR, rt -> access_methods, os, rt -> bin_access,
                     rt -> def_compress_ext, rt -> def_user,
                     rt -> def_pass, rt -> def_acct, rt -> def_ftparg, rt -> def_ftpglob,
                     rt -> def_indexfile) == ERROR){

      /* "Error while parsing line %u in configuration file %s" */

      error(A_ERR, "read_retdefs", READ_RETDEFS_002, count, defs_file -> filename);
      return(ERROR);
    }

    if(rt -> access_methods[0] == '\0'){

      /* "Invalid empty access methods field in configuration file line %d" */

      error(A_ERR,"read_retdefs",READ_RETDEFS_003, count);
      return(ERROR);
    }

    if(rt -> bin_access[0] == '\0'){

      /* "Invalid empty binary access field line %d" */

      error(A_ERR,"read_retdefs", READ_RETDEFS_004, count);
      return(ERROR);
    }

    if(rt -> def_compress_ext[0] == '\0'){

      /* "Invalid empty default compress extension field line %d" */

      error(A_ERR,"read_retdefs", READ_RETDEFS_005, count);
      return(ERROR);
    }

    if(os[0] != '\0'){

      if(strcasecmp(OS_TYPE_UNIX_BSD, os) == 0)
	    rt -> os_type = UNIX_BSD;
      else if(strcasecmp(OS_TYPE_VMS_STD, os) == 0)
      rt -> os_type = VMS_STD;
      else if(strcasecmp(OS_TYPE_NOVELL, os) == 0)
      rt -> os_type = NOVELL;
      else{

        /* "Invalid OS field: %s" */

        error(A_ERR, "read_retdefs", READ_RETDEFS_006, os);
        return(ERROR);
      }
    }
    else{

      /* "Invalid empty OS field in configuration file line %d" */

      error(A_ERR,"read_retdefs", READ_RETDEFS_007, count);
      return(ERROR);
    }

  }


  retdefs[count].access_methods[0] = '\0';

  close_file(defs_file);

  return(A_OK);
}

/*
 * get_input: get the incoming data stream an write it to the output file
 */



status_t get_input(o_file, header_rec, compress_flag, data_socket, timeout)
   file_info_t *o_file;	 /* output file handle */
   header_t    *header_rec;	 /* input header record */
   int	       compress_flag;	 /* compress status */
   int	       data_socket;	 /* incoming data socket */
   int	       timeout;		 /* inactivity timeout */

{
#ifdef __STDC__

   extern int accept(int, struct sockaddr *, int *);
   extern int shutdown(int, int);
   extern time_t time(time_t *);

#else

   extern int accept();
   extern int shutdown();
   extern time_t time();

#endif


   struct sockaddr_in sockaddr;
   int retval = 0;
   int sockaddr_len;
   int status;
   pathname_t prog_name;
   int old_ds;
   fd_set readmask;
   struct timeval timeval_struct;
   int waitstat;
   int finished;
   hostname_t tmp_hostname;


   ptr_check(o_file, file_info_t, "get_input", ERROR);
   ptr_check(header_rec, header_t, "get_input", ERROR);

   if(timeout == 0)
      error(A_WARN, "get_input", "Timeout set to 0 seconds!");
   
   if(open_file(o_file, O_WRONLY) == ERROR){

      /* "Can't open output file %s" */

      error(A_ERR, "get_input", GET_INPUT_001, o_file -> filename);
      return(ERROR);
   }

   /* Accept the connection from the server */

   sockaddr_len = sizeof(struct sockaddr_in);

   old_ds = data_socket;

   FD_ZERO(&readmask);

   FD_SET(old_ds, &readmask);

   timeval_struct.tv_sec = (long) timeout;
   timeval_struct.tv_usec = 0L;

   if((finished = select(FD_SETSIZE, &readmask, (fd_set *) NULL, (fd_set *) NULL, &timeval_struct)) == 0){
      pathname_t tmp_str;

      /* "Timeout of %d minutes on site %s retrieve" */

      sprintf(tmp_str, GET_INPUT_012, timeout/60, header_rec -> primary_hostname);
      put_reply_string(tmp_str);
      error(A_ERR, "get_input", GET_INPUT_012,  timeout/60, header_rec -> primary_hostname);
      return(ERROR);
   }
   else{
      if(finished == -1){

	 /* "Error while in select() for data transfer" */

	 error(A_SYSERR, "get_input", GET_INPUT_015);
	 return(ERROR);
      }
   }
	 
   if((data_socket = accept(old_ds,  (struct sockaddr *) &sockaddr, &sockaddr_len)) == -1){

      /* "Can't accept() data connection from %s" */

      error(A_SYSERR,"get_input",GET_INPUT_002,  header_rec -> primary_hostname);
      return(ERROR);
   }

   close(old_ds);

   prog_name[0] = '\0';

   header_rec -> retrieve_time = time((time_t *) NULL);
   HDR_SET_RETRIEVE_TIME(header_rec -> header_flags);

   switch ( compress_flag ) {
   case 1: /* Actively compress */
      header_rec -> format = FCOMPRESS_LZ;
      HDR_SET_FORMAT(header_rec -> header_flags);

      strcpy(prog_name, COMPRESS_PGM);
      break;

    case -1:  /* Actively uncompress */
      header_rec -> format = FRAW;
      HDR_SET_FORMAT(header_rec -> header_flags);
      strcpy(prog_name, UNCOMPRESS_PGM);
      break;

    default:
    case 0: /* Leave it alone. Assume that format is set elsewhere */
      strcpy(prog_name, CAT_PGM);
      break;
      
   case 2: /* Actively gzip */
      header_rec -> format = FCOMPRESS_GZIP;
      HDR_SET_FORMAT(header_rec -> header_flags);
      if ( get_option_path( "COMPRESS", "GZIP", prog_name) == ERROR ) {
        return ERROR;
      }
      break;

    case -2:  /* Actively gunzip  */
      header_rec -> format = FRAW;
      HDR_SET_FORMAT(header_rec -> header_flags);
      if ( get_option_path( "UNCOMPRESS", "GZIP", prog_name) == ERROR ) {
        return ERROR;
      }
      break;
   }

   
   HDR_SET_UPDATE_STATUS(header_rec -> header_flags);

   header_rec -> update_status = SUCCEED;

   HDR_SET_SOURCE_ARCHIE_HOSTNAME(header_rec -> header_flags);
   strcpy(header_rec -> source_archie_hostname, get_archie_hostname(tmp_hostname, sizeof(tmp_hostname)));

   /* Write the header out to the output file */

   if(write_header(o_file -> fp_or_dbm.fp, header_rec, (u32 *) NULL, 0, 0)== ERROR){

      /* "Can't write header for incoming data to output file %s" */

      error(A_ERR, "get_input", GET_INPUT_004, o_file -> filename);
      return(ERROR);
   }

   /*
    * Set things up so that prog_name reads directly from the socket
    * and writes directly to the output file
    */

   if((retval = fork()) == 0){  /* child process */
      dup2(data_socket, 0);
      dup2(fileno(o_file -> fp_or_dbm.fp), 1);

      execlp(prog_name, prog_name, (char *) NULL);

      /* "Can't spawn process %s" */

      error(A_SYSERR,"get_input", GET_INPUT_005, prog_name);
      return(ERROR);
   }

   if(retval == -1){

      /* "Can't fork() for %s" */

      error(A_SYSERR,"get_input", GET_INPUT_006, prog_name);
      return(ERROR);
   }


   FD_ZERO(&readmask);

   FD_SET(data_socket, &readmask);

   /* First wait for half the timeout value */

#ifndef SOLARIS
   siginterrupt(SIGALRM, 1);
#endif

   signal(SIGALRM, sig_handle);

   alarm(timeout/2);

   if(((waitstat = wait(&status)) == -1) && (!signal_set)){

      /* "Error while in wait() for %s" */

      error(A_SYSERR,"get_input", GET_INPUT_007, prog_name);
      return(ERROR);
   }
   else if(waitstat > 0){

      if(WIFSIGNALED(status)){

	 /* "%s terminated abnormally with signal %u" */

	 error(A_ERR,"get_input", GET_INPUT_009, prog_name, WTERMSIG(status));
	 return(ERROR);
      }

      if(WIFEXITED(status) && WEXITSTATUS(status)){

	 /* "%s exited abnormally with status %u" */

	 error(A_ERR,"get_input", GET_INPUT_008, prog_name, WEXITSTATUS(status));

	 /* Kludge */

	 if(((header_rec -> format == FCOMPRESS_LZ) || (header_rec->format == FCOMPRESS_GZIP) )
      && (WEXITSTATUS(status) == 2)){

	    /* "Insufficient data to compress(1). Empty listings" */

	    put_request_string(GET_INPUT_011);
	    put_reply_string("");
	 }

	 close(data_socket);
	 close_file(o_file);

	 return(ERROR);
      }

      close(data_socket);
      close_file(o_file);

#ifndef SOLARIS
      siginterrupt(SIGALRM, 0);
#endif
      alarm(0);
      signal(SIGALRM, SIG_DFL);

      return(A_OK);
   }

#ifndef SOLARIS
   siginterrupt(SIGALRM, 0);
#endif

   alarm(0);

   signal(SIGALRM, SIG_DFL);

   timeval_struct.tv_sec = timeout/2;
   timeval_struct.tv_usec = 0;

   while(1){


      if((waitstat = waitpid(-1, &status, WNOHANG)) == -1){

	 /* "Error while in wait() for %s" */

	 error(A_SYSERR,"get_input", GET_INPUT_007, prog_name);
	 break;
      }
      else if(waitstat > 0)
	 goto testwait;

      /* Select on the socket for reading for the other half of the timeout */

      if((finished = select(FD_SETSIZE, &readmask, (fd_set *) NULL, (fd_set *) NULL, &timeval_struct)) >= 0){

	 if((waitstat = waitpid(-1, &status, WNOHANG)) == -1){

	    /* "Error while in wait() for %s" */

	    error(A_SYSERR,"get_input", GET_INPUT_007, prog_name);
	    break;
	 }
	 else if(waitstat > 0)
	    goto testwait;

	 if(finished == 0){
	    pathname_t tmp_str;

	    /* The timeout has expired */ 

	    /* "Timeout of %d minutes on site %s retrieve" */

	    sprintf(tmp_str, GET_INPUT_012, timeout/60, header_rec -> primary_hostname);
	    put_request_string(tmp_str);
	    put_reply_string("");
	    error(A_ERR, "get_input", GET_INPUT_012,  timeout/60, header_rec -> primary_hostname);
	    kill(retval, SIGHUP);
	    return(ERROR);

	 }
	 else{
	       sleep(timeout/2);

	       /* Check to see if the child has finished in the meantime */

	       if((waitstat = waitpid(-1, &status, WNOHANG)) == -1){

		  /* "Error while in wait() for %s" */

		  error(A_SYSERR,"get_input", GET_INPUT_007, prog_name);

		  break;
	       }
	       else if(waitstat > 0)
		  goto testwait;

	       /* Child not finished */

	       FD_ZERO(&readmask);
	       FD_SET(data_socket, &readmask);
	       timeval_struct.tv_sec = timeout/2;
	       timeval_struct.tv_usec = 0;
	       continue;
	 }

      }
      else{

	 /* "Error while in select() for %s" */

	 error(A_SYSERR, "get_input", GET_INPUT_014, prog_name);
      }

      if(wait(&status) == -1){

	 /* "Error while in wait() for %s" */

	 error(A_SYSERR,"get_input", GET_INPUT_007, prog_name);
	 break;
      }

testwait:

      if(WIFSIGNALED(status)){

	 /* "%s terminated abnormally with signal %u" */

	 error(A_ERR,"get_input", GET_INPUT_009, prog_name, WTERMSIG(status));
	 break;
      }

      if(WIFEXITED(status) && WEXITSTATUS(status)){

	 /* "%s exited abnormally with status %u" */

	 error(A_ERR,"get_input", GET_INPUT_008, prog_name, WEXITSTATUS(status));

	 /* Kludge */

	 if(((header_rec -> format == FCOMPRESS_LZ) || (header_rec->format == FCOMPRESS_GZIP) )
      && (WEXITSTATUS(status) == 2)){

	    /* "Insufficient data to compress(1). Empty listings" */

	    put_request_string(GET_INPUT_011);
	    put_reply_string("");
	 }
	 
	 return(ERROR);
      }

      break;
	 
   }

   close(data_socket);
   
   close_file(o_file);

   return(A_OK);
}


/*
 * sig_handle: set the global variable when a signal is caught
 */

#ifndef SOLARIS
void sig_handle(sig, code, scp, addr)
   int sig, code;
   struct sigcontext *scp;
   char *addr;
#else
void sig_handle(sig)
   int sig;
#endif
{

   if(sig == SIGALRM)
      signal_set = 1;
   else{

      /* "retrieve program terminated with signal %d" */

      error(A_ERR,"sig_handle",SIG_HANDLE_001, sig);
      exit(ERROR);
   }
}


status_t get_files(output_file, comm_in, comm_out, header_rec, hostname,hostaddr, compress_flag, rt, timeout, file_list, globbing)
   file_info_t *output_file;
   FILE *comm_in;
   FILE *comm_out;
   header_t *header_rec;
   hostname_t hostname;
   ip_addr_t hostaddr;
   int compress_flag;
   retdefs_t *rt;
   int timeout;
   char **file_list;
   int globbing;
{
  int count = 0;
  int retcode;
  pathname_t retr_file;
  pathname_t retr_dir;
  pathname_t home_dir;
  char **file_ltmp;
  int portint;
  int data_socket;
  u16 port;
  char *p, *h;
  pathname_t tmp_name;
  char type[MAX_ACCESS_METHOD];
  char **filel;
  char **fl_name;
  char *small_list[3];
  int orig_compress = compress_flag;
  int nodir;
  char **a;
  int i;
  
  retcode = send_command(comm_in, comm_out,0, timeout, "PWD");
   
  switch(retcode){
    char *pptr;
    char *qptr;

  case FTP_PATHNAME_NONRFC:
  case FTP_PATHNAME_CREATED:

    pptr = get_reply_string();

    if((pptr = strchr(pptr, '"')) == (char *) NULL){

	    /* "Can't get home directory name for site" */

	    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_011);
	    return(ERROR);
    }

    if((qptr = strrchr(pptr, '"')) == (char *) NULL){

	    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_011);
	    return(ERROR);
    }

    count = (qptr - 1) - pptr;

    strncpy(home_dir, pptr + 1, count);
    home_dir[count] = '\0';
    break;

    /* Try to fake it */

  case FTP_COMMAND_NOT_IMPL:
    strcpy(home_dir,"/");
    sprintf(header_rec -> comment, "Site %s does not implement PWD command", hostname);
    break;
	 

  default:

    /* "Unexpected return code" */

    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
    return(ERROR);
    break;
  }

  for(count = 0, file_ltmp = file_list ; *file_ltmp != (char *) NULL; file_ltmp++, count++){
    char *pptr;

    nodir = 0;

    compress_flag = orig_compress;

    sprintf(curr_file -> filename, "%s_%d%s%s", output_file -> filename, count, SUFFIX_PARSE, TMP_SUFFIX);

    retr_file[0] = retr_dir[0] = '\0';

    if(count != 0){
      retcode = send_command(comm_in, comm_out,0,timeout, "CWD %s", home_dir);

      switch(retcode){

	    case FTP_FILE_ACTION_OK:

        break;

	    default:

        /* "Unexpected return code" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
        return(ERROR);
        break;

      }
    }
    /* Get filename and the directory that its in to be retrieved */

    strcpy(retr_file, tail(*file_ltmp));

    strcpy(retr_dir, *file_ltmp);

    if((pptr = strrchr(retr_dir, '/')) == (char *) NULL)
    retr_dir[0] = '\0';
    else
    *pptr = '\0';
      
    if(retr_dir[0] != '\0'){

      char *curr_ptr;
      char dir_copy[MAX_ACCESS_METHOD];

      strcpy(dir_copy, retr_dir);

      curr_ptr = strtok(dir_copy,"/");


      /* Repeatedly do CWD commands until we're in the right directory */      

      while(!nodir && (curr_ptr != (char *) NULL)){

        retcode = send_command(comm_in, comm_out, 0, timeout, "CWD %s", curr_ptr);

        switch(retcode){

        case FTP_FILE_ACTION_OK:
          break;

        case FTP_LOST_CONN:

          /* "Lost connection" */

          error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_013);
          return(ERROR);
          break;

        case FTP_ACTION_NOT_TAKEN:

          /* "Can't change directory to %s" */

          error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_014, curr_ptr);
          nodir = 1;
          break;
	       

        default:
          error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
          return(ERROR);
          break;
        }

        curr_ptr = strtok((char *) NULL,"/");
      }

    }

    if(nodir)
    continue;

    HDR_UNSET_HCOMMENT(header_rec -> header_flags);   
    header_rec -> comment[0] = '\0';
   
    /* Support filename globbing if turned on  and the given
       filename contains globbing characters */

    if((strpbrk(retr_file, rt -> def_ftpglob) != (char *) NULL)
       && globbing){
      struct sockaddr_in sockaddr;
      int sockaddr_len;
      fd_set readmask;
      struct timeval timeval_struct;
      int old_ds;
      int curr_count = 0;
      int max_count = 0;
      char **tmp_list;

      int finished = 0;
      FILE *tmp_fp;

      if((tmp_list = (char **) malloc(DEFAULT_NO_FILES * sizeof(char *))) == (char **) NULL){

        /* "Can't malloc() space for file list" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_025);
        return(ERROR);
      }

      max_count = DEFAULT_NO_FILES;
      curr_count = 0;
	 
      /* Set up the data connection for NLST */

      if(get_new_port(&portint, &data_socket) == ERROR){

        /* "Unable to get local port for data transfer" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_015);
        return(ERROR);
      }

      port = portint;

      hostaddr = htonl(hostaddr);
      port = htons(port);

      h = (char *) &hostaddr;
      p = (char *) &port;

      retcode = send_command(comm_in, comm_out,0, timeout, "PORT %d,%d,%d,%d,%d,%d",
                             UC(h[0]),UC(h[1]),UC(h[2]),UC(h[3]),UC(p[0]),UC(p[1]));

      switch(retcode){

	    case FTP_COMMAND_OK:
        break;

	    case FTP_LOST_CONN:

        /* "Lost connection" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_013);
        return(ERROR);
        break;

        /*
         * In this case we should follow RFC 959 and accept the default data
         * port. Another version perhaps.
         */

	    case FTP_COMMAND_NOT_IMPL:

        /* "Command not implemented" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_016);
        return(ERROR);
        break;
		 
	    default:

        /* "Unexpected return code" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
        return(ERROR);
        break;
      }

      retcode = send_command(comm_in, comm_out, 0, timeout, "NLST %s", retr_file);

      switch(retcode){

	    case FTP_OPEN_DATACONN:
	    case FTP_DATACONN_OPEN:
        break;

	    case FTP_FILE_UNAVAILABLE:

        /* "Can't get file %s. Currently unavailable" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_019, retr_file);
        break;

	    case FTP_ACTION_NOT_TAKEN:

        /* "Can't get file %s. File does not exist" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_020, retr_file);
        continue;
        break;

	    default:

        /* "Unexpected return code" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
        return(ERROR);
        break;
      }

      /* Accept the connection from the server */

      sockaddr_len = sizeof(struct sockaddr_in);

      old_ds = data_socket;

      FD_ZERO(&readmask);

      FD_SET(old_ds, &readmask);

      timeval_struct.tv_sec = (long) timeout;
      timeval_struct.tv_usec = 0L;

      if((finished = select(FD_SETSIZE, &readmask, (fd_set *) NULL, (fd_set *) NULL, &timeval_struct)) == 0){
        pathname_t tmp_str;

        /* "Timeout of %d minutes on site %s retrieve" */

        sprintf(tmp_str, GET_INPUT_012, timeout/60, header_rec -> primary_hostname);
        put_reply_string(tmp_str);
        error(A_ERR, "get_files", GET_INPUT_012,  timeout/60, header_rec -> primary_hostname);
        return(ERROR);
      }
      else{
        if(finished == -1){

          /* "Error while in select() for data transfer" */

          error(A_SYSERR, "get_files", GET_INPUT_015);
          return(ERROR);
        }
      }
	       
      if((data_socket = accept(old_ds,  (struct sockaddr *) &sockaddr, &sockaddr_len)) == -1){

        /* "Can't accept() data connection from %s" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_020, retr_file);
        return(ERROR);
      }

      close(old_ds);

      if((tmp_fp = fdopen(data_socket, "r")) == (FILE *) NULL){

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_023);
        return(ERROR);
      }

      while(fgets(tmp_name, sizeof(tmp_name), tmp_fp) == tmp_name){

        *(strchr(tmp_name, '\r')) = '\0';

        if(verbose)
        error(A_INFO, "get_files", "File: %s", tmp_name);

        if(curr_count < max_count)
        tmp_list[curr_count] = strdup(tmp_name);
        else{
          char **mylist;

          if((mylist = (char **) realloc( tmp_list, (max_count + NO_FILES_INC) * sizeof(char *))) == (char **) NULL){

            /* "Can't realloc() space for file list" */

            error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_024);
            return(ERROR);
          }
          tmp_list = mylist;

          max_count += NO_FILES_INC;
          tmp_list[curr_count] = strdup(tmp_name);
        }

        curr_count++;
      }

      tmp_list[curr_count] = (char *) NULL;
      fclose(tmp_fp);
      filel = tmp_list;
	 
      if(get_reply(comm_in, comm_out, &retcode, 0, timeout) == ERROR){

        /* "Unable to perform transfer" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_021);
        return(ERROR);
      }


      switch(retcode){

	    case FTP_TRANSFER_COMPLETE:
        break;

	    default:

        /* "Unexpected return code" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
        return(ERROR);
        break;
      }



    }
    else{
      small_list[0] = retr_file;
      small_list[1] = (char *) NULL;
      filel = small_list;
    }

    for(fl_name = filel; *fl_name != (char *) NULL; fl_name++, count++){

      strcpy(retr_file, *fl_name);

      /* Set up the data connection */


      if(get_new_port(&portint, &data_socket) == ERROR){

        /* "Unable to get local port for data transfer" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_015);
        return(ERROR);
      }
      
      port = portint;

      hostaddr = htonl(hostaddr);
      port = htons(port);

      h = (char *) &hostaddr;
      p = (char *) &port;

      retcode = send_command(comm_in, comm_out,0, timeout, "PORT %d,%d,%d,%d,%d,%d",
                             UC(h[0]),UC(h[1]),UC(h[2]),UC(h[3]),UC(p[0]),UC(p[1]));

      switch(retcode){

	    case FTP_COMMAND_OK:
        break;

	    case FTP_LOST_CONN:

        /* "Lost connection" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_013);
        return(ERROR);
        break;

        /*
         * In this case we should follow RFC 959 and accept the default data
         * port. Another version perhaps.
         */

	    case FTP_COMMAND_NOT_IMPL:

        /* "Command not implemented" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_016);
        return(ERROR);
        break;
		 
	    default:

        /* "Unexpected return code" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
        return(ERROR);
        break;
      }
      

      header_rec -> format = FRAW;
      HDR_SET_FORMAT(header_rec -> header_flags);

      /* Non-default action to be taken. See if the retrieved file has the
         compression extension for that system */
      
      a = str_sep(rt->def_compress_ext,',');
      if ( a != NULL ) {
        for ( i = 0; a[i] != NULL; i++ ) {

          if(strcmp(retr_file + strlen(retr_file) - strlen(a[i]), a[i]) == 0){

            /* The file is compressed */
            if ( strcmp(a[i],".gz") == 0 ) {
              header_rec -> format = FCOMPRESS_GZIP;
              if ( compress_flag < 0 )
                orig_compress = compress_flag = -2;
            }

            if ( strcmp(a[i],".Z") == 0 ) {
              header_rec -> format = FCOMPRESS_LZ;
            }
            HDR_SET_FORMAT(header_rec -> header_flags);

            if(compress_flag >= 1){

              /* The file is compressed and we want it that way, so nothing
                 need be done to it */

              compress_flag = 0;
            }

            /* Have to change the transmission method to retrieve in the
               default binary access method for this system */

            if(strcasecmp(rt -> bin_access, "image") == 0)
            strcpy(type,"I");
            else if(strcasecmp(rt -> bin_access,"tenex") == 0)
            strcpy(type,"L 8");
            else{

              /* "Binary access method '%s' is not supported" */
              
              error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_018, rt -> bin_access);
              return(ERROR);
            }
	 

            retcode = send_command(comm_in, comm_out, 0, timeout, "TYPE %s", type);

            switch(retcode){

            case FTP_COMMAND_OK:
              break;

            case FTP_COMM_PARAM_NOT_IMPL:

              /* "Command not implemented" */

              error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_016);
              return(ERROR);

              break;

            case FTP_LOST_CONN:

              /* "Lost connection" */

              error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_013);
              return(ERROR);

              break;

            case FTP_COMMAND_NOT_IMPL:

              /* "Command not implemented" */

              error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_016);
              return(ERROR);
              break;
		     
            default:

              /* "Unexpected return code" */

              error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
              return(ERROR);
              break;
            }
            break;
          }
        }
      }
      if ( a == NULL || a[i] == NULL ) {
        if(compress_flag == -1)
          compress_flag = 0;
      }

      sprintf(header_rec -> data_name, "%s/%s", retr_dir, retr_file);
      HDR_SET_DATA_NAME(header_rec -> header_flags);

      retcode = send_command(comm_in, comm_out, 0, timeout, "RETR %s", retr_file);

      switch(retcode){

	    case FTP_OPEN_DATACONN:
        break;

	    case FTP_FILE_UNAVAILABLE:

        /* "Can't get file %s. Currently unavailable" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_019, retr_file);
        continue;
        break;

	    case FTP_ACTION_NOT_TAKEN:

        /* "Can't get file %s. File does not exist" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_020, retr_file);
        continue;
        break;

	    default:

        /* "Unexpected return code" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
        continue;
        break;
      }
      

      if(get_input(curr_file, header_rec, compress_flag, data_socket, timeout) == ERROR){

        /* "Unable to perform transfer" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_021);
        return(ERROR);
      }

      if(get_reply(comm_in, comm_out, &retcode, 0, timeout) == ERROR){

        /* "Unable to perform transfer" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_021);
        return(ERROR);
      }


      switch(retcode){

	    case FTP_TRANSFER_COMPLETE:
        break;

	    default:

        /* "Unexpected return code" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
        return(ERROR);
        break;
      }


      sprintf(tmp_name, "%s_%d%s", output_file -> filename, count, SUFFIX_PARSE);

      if(rename(curr_file -> filename, tmp_name) == -1){

        /* "Can't rename temporary file %s to %s" */

        error(A_SYSERR,"do_retrieve", DO_RETRIEVE_022, curr_file -> filename, tmp_name);
        return(ERROR);
      }
      
    }

    if(filel != small_list)
    free_opts(filel);
  }

  return(A_OK);
}


status_t get_pwd( output_file, comm_in, comm_out, header_rec, hostname, hostaddr, rt, timeout, home_dir)
   file_info_t *output_file;
   FILE *comm_in;
   FILE *comm_out;
   header_t *header_rec;
   hostname_t hostname;
   ip_addr_t hostaddr;
   retdefs_t *rt;
   int timeout;
   pathname_t home_dir;

{

   int count = 0;
   int retcode = 0;

   retcode = send_command(comm_in, comm_out, 0, timeout, "PWD");

   
   switch(retcode){
      char *pptr;
      char *qptr;

      case FTP_PATHNAME_NONRFC:
      case FTP_PATHNAME_CREATED:

	 pptr = get_reply_string();

	 if((pptr = strchr(pptr, '"')) == (char *) NULL){

	    /* "Can't get home directory name for site" */

	    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_011);
	    return(ERROR);
	 }

	 if((qptr = strrchr(pptr, '"')) == (char *) NULL){

	    error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_011);
	    return(ERROR);
	 }

         count = (qptr - 1) - pptr;

	 strncpy(home_dir, pptr + 1, count);
	 home_dir[count] = '\0';
	 break;

      /* Try to fake it */

      case FTP_COMMAND_NOT_IMPL:
         strcpy(home_dir,"/");
         sprintf(header_rec -> comment, "Site %s does not implement PWD command", hostname);
	 break;
	 

      default:

	 /* "Unexpected return code" */

        error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
        return(ERROR);
	break;
   }

   return(A_OK);

}


status_t get_list(output_file, comm_in, comm_out, header_rec, hostname,hostaddr, compress_flag, rt, timeout)
   file_info_t *output_file;
   FILE *comm_in;
   FILE *comm_out;
   header_t *header_rec;
   hostname_t hostname;
   ip_addr_t hostaddr;
   int compress_flag;
   retdefs_t *rt;
   int timeout;
{
   int portint;
   int data_socket;
   u16 port;
   char *p, *h;
   int count = 0;
   int retcode = 0;
   pathname_t tmp_name;
   

   /* Set up the data connection */

   if(get_new_port(&portint, &data_socket) == ERROR){

      /* "Unable to get local port for data transfer" */

      error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_015);
      return(ERROR);
   }


   port = portint;

   hostaddr = htonl(hostaddr);
   port = htons(port); 


   h = (char *) &hostaddr;
   p = (char *) &port;

     
   retcode = send_command(comm_in, comm_out,0, timeout, "PORT %d,%d,%d,%d,%d,%d",
			  UC(h[0]),UC(h[1]),UC(h[2]),UC(h[3]),UC(p[0]),UC(p[1]));

   switch(retcode){

      case FTP_COMMAND_OK:
	 break;

      case FTP_LOST_CONN:

      /* "Lost connection" */

      error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_013);
      return(ERROR);
      break;

      /*
       * In this case we should follow RFC 959 and accept the default data
       * port. Another version perhaps.
       */

      case FTP_COMMAND_NOT_IMPL:

	 /* "Command not implemented" */

	 error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_016);
	 return(ERROR);
	 break;
	   
      default:

	 /* "Unexpected return code" */

	 error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
	 return(ERROR);
	 break;
   }


   header_rec -> format = FRAW;
   HDR_SET_FORMAT(header_rec -> header_flags);


   /* LIST command retrieves in uncompressed ASCII. If uncompress flag
      is set, then turn it off */

   if(compress_flag <= -1)
      compress_flag = 0;
  

   /* Port command was accepted, issue the LIST command */

   retcode = send_command(comm_in, comm_out, 0, timeout, "LIST %s", rt -> def_ftparg);

   switch(retcode){

      case FTP_DATACONN_OPEN:
      case FTP_OPEN_DATACONN:
	 break;

      case FTP_CANT_DATACONN:
      case FTP_ABORT_DATACONN:
      case FTP_LOCAL_ERROR:
      case FTP_FILE_UNAVAILABLE:

	 /* "Error in making data connection" */

	 error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_017);
	 return(ERROR);
	 break;

      case FTP_LOST_CONN:

	 /* "Lost connection" */

	 error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_013);
	 return(ERROR);
	 break;

      case FTP_COMMAND_NOT_IMPL:

	 /* "Command not implemented" */

	 error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_016);
	 break;
	    
      default:

	 /* "Unexpected return code" */

	 error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
	 return(ERROR);
	 break;
   }


   if(get_input(curr_file, header_rec, compress_flag, data_socket, timeout) == ERROR){

      /* "Unable to perform transfer" */

      error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_021);
      return(ERROR);
   }

   if(get_reply(comm_in, comm_out, &retcode, 0, timeout) == ERROR){

      /* "Unable to perform transfer" */

      error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_021);
      return(ERROR);
   }


   switch(retcode){

      case FTP_TRANSFER_COMPLETE:
	 break;

      default:

	 /* "Unexpected return code" */

	 error_header(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_012);
	 return(ERROR);
	 break;
   }

   sprintf(tmp_name, "%s_%d%s", output_file -> filename, count, SUFFIX_PARSE);

   if(rename(curr_file -> filename, tmp_name) == -1){

      /* "Can't rename temporary file %s to %s" */

      error(A_SYSERR,"do_retrieve", DO_RETRIEVE_022, curr_file -> filename, tmp_name);
      return(ERROR);
   }

   return(A_OK);
}



/* Check to see if ls-lR.Z file exists. If so, then see if modification time
   is more recent that when we last looked */

int check_for_lslRZ(comm_in, comm_out, file_list, header_rec, timeout, rt)
   FILE *comm_in;
   FILE *comm_out;
   char **file_list;
   header_t *header_rec;
   int timeout;
   retdefs_t *rt;
{
  pathname_t tmp_str;
  pathname_t ft;
  date_time_t last_time;
  pathname_t currf;
  pathname_t currd;
  int retcode;

  char **a;
  int i;


  a = str_sep(rt->def_compress_ext,',');

  if ( a == NULL ) 
  return 0;

  for ( i = 0; a[i] != NULL; i++ )  {
    /*  sprintf(tmp_str, "%s%s", rt -> def_indexfile, rt -> def_compress_ext); */
    sprintf(tmp_str, "%s%s", rt -> def_indexfile, a[i]);
    strcpy(currf, tmp_str);
    currd[0] = '\0';

    if((retcode = send_command(comm_in, comm_out, 0, timeout, "MDTM %s", currf)) != FTP_FILE_STATUS){

      if(retcode == FTP_COMMAND_NOT_IMPL){
        if(file_list){
          free_opts(file_list);
          file_list[0] = (char *) NULL;
        }
        return 0;
      }

      strcpy(currf, tmp_str);

      if(send_command(comm_in, comm_out, 0, timeout, "MDTM %s", currf) != FTP_FILE_STATUS){

        /* No such file or directory */

        /* Change into the "pub" directory */

        strcpy(currd, "/PUB/"); 

        /* Have to check for both "PUB" and "pub" */

        if((send_command(comm_in, comm_out, 0, timeout, "CWD %s", currd) == FTP_FILE_ACTION_OK)
           || (send_command(comm_in, comm_out, 0, timeout, "CWD %s", make_lcase(currd)) == FTP_FILE_ACTION_OK)){

          sprintf(tmp_str, "%s%s", rt -> def_indexfile, a[i]);
          strcpy(currf, tmp_str);

          if(send_command(comm_in, comm_out, 0, timeout, "MDTM %s", currf) != FTP_FILE_STATUS){


            sprintf(tmp_str, "%s%s", rt -> def_indexfile, a[i]);
            strcpy(currf, tmp_str);

            if(send_command(comm_in, comm_out, 0, timeout, "MDTM %s", currf) != FTP_FILE_STATUS){

              send_command(comm_in, comm_out, 0, timeout, "CDUP");
              if(file_list){
/*                free_opts(file_list); */
                file_list[0] = (char *) NULL;
              }
              continue; /*return 0; */
            }
          }
        }
        else
        continue; /*return 0; */
      }
    }
    break;
  }
  if ( a[i] == NULL ) {
    return 0;
  }
  strcpy(tmp_str, get_reply_string());

  sscanf(tmp_str, "%*d %s", ft);

  last_time = cvt_to_inttime(ft, 0);

  if(last_time > header_rec -> retrieve_time){
    char *tmp;
    /* Next check the size */

    if(send_command(comm_in, comm_out, 0, timeout, "SIZE %s", currf) == FTP_FILE_STATUS){

      strcpy(tmp_str, get_reply_string());

      sscanf(tmp_str, "%*d %s", ft);

      if(atoi(ft) < MIN_LSLR_SIZE){

        /* "WARNING: This listing file '%s%s' may be a soft link" */

        sprintf(header_rec -> comment, CHECK_FOR_LSLRZ_001, currd, currf);
        HDR_SET_HCOMMENT(header_rec -> header_flags);
        write_mail(MAIL_RETR_FAIL, "%s %s %s", header_rec -> primary_hostname, header_rec -> access_methods, header_rec -> comment);
      }
    }

    sprintf(tmp_str, "%s%s", currd, currf);
    tmp = strdup(tmp_str);
    file_list[0] = tmp; /*strdup(tmp_str);*/
    file_list[1] = (char *) NULL;

  }
  else{
    pathname_t timehold;

    strcpy(timehold, cvt_to_usertime(last_time, 1));

    /* "WARNING: file '%s%s' has date of %s, last retrieve done %s. Not taken" */
    if ( last_time != 0 ) {
      sprintf(header_rec -> comment, CHECK_FOR_LSLRZ_002, currd, currf, timehold, cvt_to_usertime(header_rec -> retrieve_time, 1));
      HDR_SET_HCOMMENT(header_rec -> header_flags);
      write_mail(MAIL_RETR_FAIL, "%s %s %s", header_rec -> primary_hostname, header_rec -> access_methods, header_rec -> comment);
    }
    if(file_list){
/*      free_opts(file_list); */
      file_list[0] = (char *) NULL;
    }
  }

  if(currd[0])
  send_command(comm_in, comm_out, 0, timeout, "CDUP");

  return 0;
}


status_t gather_atend(file_list, input_file)
   char ***file_list;
   file_info_t *input_file;
{
   char **local_list;
   header_t discard_header;
   pathname_t inbuf;
   int curr_count;
   int max_count;

   max_count = MAX_DEF_FILES;

   if((local_list = (char **) malloc(max_count * sizeof(char *))) == (char **) NULL){

      error(A_ERR, "gather_atend", "Can't malloc space for default file list");
      return(ERROR);
   }

   curr_count = 0;

   if(open_file(input_file, O_RDONLY) == ERROR){

      error(A_ERR, "gather_atend", "Can't open input file %s", input_file -> filename);
      return(ERROR);
   }
   
   if(read_header(input_file -> fp_or_dbm.fp, &discard_header, (u32 *) NULL, 0, 0) == ERROR){

      error(A_ERR, "gather_atend", "Can't read header of input file %s", input_file -> filename);
      return(ERROR);
   }
   
   /* Have read and thown away the header */

   while(fgets(inbuf, sizeof(inbuf), input_file -> fp_or_dbm.fp) == inbuf){
      char *tstr;

      if((tstr = strrchr(inbuf, '\n')) != (char *) NULL)
	 *tstr = '\0';

      local_list[curr_count++] = strdup(inbuf);

      if(curr_count == max_count){

	 max_count += DEF_INCR;

	 if((local_list = (char **) realloc(local_list, max_count * sizeof(char *))) == (char **) NULL){

	    error(A_ERR, "gather_atend", "Can't realloc space for file list");
	    return(ERROR);
	 }
      }
   }

   local_list[curr_count] = (char *) NULL;

   *file_list = local_list;

   close_file(input_file);

   return(A_OK);
}
   

void error_header( va_alist )
   va_dcl

/*   file_info_t *curr_file;
   file_info_t *output_file;
   int count;
   header_t *header_rec;
   char *format;
   char *suffix;
   arglist
*/   
{
   pathname_t tmp_name;
   file_info_t *c_file;  /* the output file */
   file_info_t *output_file;  /* the output file */
   int count;		      /* */
   header_t *header_rec;      /* the header record to be written */
   char *suffix;
   char holdstr[1024];
   va_list al;

   va_start( al );

   c_file = va_arg(al, file_info_t *);
   output_file = va_arg(al, file_info_t *);
   count = va_arg(al, int);
   header_rec = va_arg(al, header_t *);
   suffix = va_arg(al, char *);

   sprintf(tmp_name, "%s (%s) (%s)", va_arg(al, char *), get_request_string(), get_reply_string());

   vsprintf(holdstr, tmp_name, al);

   do_error_header(c_file, output_file, count, header_rec, suffix, holdstr);

   va_end( al );

}
