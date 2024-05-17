/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */



#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#if defined(AIX) || defined(SOLARIS)
#include <rpc/types.h>
#endif
#include <rpc/xdr.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef AIX
#include <sys/select.h>
#include <mp.h>
#endif

#include "protos.h"
#include "typedef.h"
#include "db_ops.h"
#include "host_db.h"
#include "parser_file.h"

#include "core_entry.h"
#include "site_file.h"

#include "start_db.h"

#include "patrie.h"
#include "archstridx.h"

#include "header.h"
#include "error.h"
#include "files.h"
#include "master.h"
#include "archie_dbm.h"
#include "archie_strings.h"
#include "net_anonftp.h"
#include "db_files.h"
#include "archie_xdr.h"
#include "lang_anonftp.h"

extern int errno;
/*
 * net_anonftp: this program is responsible for inter-archie transmission
 * of anonftp database files. It is not normally invoked manually. This
 * program can both transmit and receive data, which is transmitted in Sun
 * XDR format to allow inter-architecture communications

   argv, argc are used.


   Parameters:	  -I
		  -O
      -p <port number>
		  -w <anonftp files database pathname>
		  -M <master database pathname>
		  -h <host database pathname>
		  -l write to log file (default)
		  -L <log file>
		  -v verbose

 */

/* this has to be here so that the signal handler can see it */

file_info_t	*sitefile = (file_info_t *) NULL;

int verbose = 0;
int timeout = 900;

char *prog;

static int rec_number = 0;

static void sig_alarm(num)
  int num;
{
  static last_rec_number = 0;
  
  if  ( last_rec_number == rec_number ) {
    error(A_ERR,"net_anonftp","Timeout of %d seconds expired", timeout);
    exit(ERROR);
  }

  alarm(timeout);
  signal(SIGALRM,sig_alarm);
  last_rec_number = rec_number;
}


int main(argc, argv)
   int argc;
   char *argv[];

{
#ifndef __STDC__

  extern int getopt();
  extern status_t  get_port();

#endif

  extern char *optarg;

  char **cmdline_ptr;
  int cmdline_args;

  int option;

  /* Directories */

  pathname_t files_database_dir;
  pathname_t start_database_dir;
  pathname_t master_database_dir;
  pathname_t host_database_dir;

  hostname_t io_host;

  hostdb_t hostdb_ent;
  hostdb_aux_t hostaux_ent;
  header_t header_rec;

  pathname_t tmp_string;
  pathname_t logfile;

  pathname_t compress_pgm;
   
  file_info_t *hostdb = create_finfo();
  file_info_t *hostaux_db = create_finfo();   

  file_info_t *start_db = create_finfo();
  file_info_t *domain_db = create_finfo();

  int output_mode = 1;

  format_t outhdr = FXDR;

  struct arch_stridx_handle *strhan;
  char *dbdir;

  int logging = 0;
  int port = 0;

  prog = argv[0];
  files_database_dir[0] = host_database_dir[0] = master_database_dir[0] = start_database_dir[0] = '\0';
  compress_pgm[0] = logfile[0] = '\0';

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;

  while((option = (int) getopt(argc, argv, "w:i:p:M:h:IO:lL:cvt:n:")) != EOF){

    switch(option){

      /* Input mode */

    case 'I':
	    output_mode = 0;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* Log filename */

    case 'L':
	    strcpy(logfile, optarg);
	    logging = 1;
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* port of the site */

    case 'p':
	    port = atoi(optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* Master database directory */

    case 'M':
	    strcpy(master_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* Output mode */

    case 'O':
	    strcpy(io_host,optarg);
	    output_mode = 1;
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* use compress(1) on outgoing data */

    case 'c':
	    strcpy(compress_pgm, COMPRESS_PGM);
	    outhdr = FXDR_COMPRESS_LZ;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* host database directory name */

    case 'h':
	    strcpy(host_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* Log process, default file */

    case 'l':
	    logging = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* Verbose mode */

    case 'v':
	    verbose = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

    case 'n':
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;
      
      /* anonftp database directory name  */

    case 'w':
	    strcpy(files_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* Timeout in seconds */
    case 't':
      timeout = atoi(optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;
      
    }
  }

  /* set up logs */
sleep(30);

  if(logging){  
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /*  "Can't open default log file" */

        error(A_ERR, "parse_anonftp", NET_ANONFTP_001);
        exit(ERROR);
      }
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, "parse_anonftp", NET_ANONFTP_002, logfile);
        exit(ERROR);
      }
    }
  }

  if(port==0){

    /* "No port number given" */

    /*    error(A_ERR,"net_anonftp","No port number given"); */
    if( get_port((char *)NULL, ANONFTP_DB_NAME, &port ) == ERROR ){
      error(A_ERR,"get_port","Could not get port for site");
      exit(A_OK);
    }
  }

  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR, "net_anonftp", NET_ANONFTP_003);
    exit(ERROR);
  }

  if((dbdir = set_files_db_dir(files_database_dir)) == (char *) NULL){

    /* "Error while trying to set anonftp database directory" */

    error(A_ERR, "net_anonftp", NET_ANONFTP_004);
    exit(ERROR);
  }

  if((set_start_db_dir(start_database_dir,DEFAULT_FILES_DB_DIR)) == (char *) NULL){

    /* "Error while trying to set start database directory" */

    error(A_ERR, "net_anonftp", "Error while trying to set start database directory\n");
    exit(A_OK);
  }

  if(set_host_db_dir(host_database_dir) == (char *) NULL){

    /* "Error while trying to set host database directory" */

    error(A_ERR, "net_anonftp", NET_ANONFTP_005);
    exit(ERROR);
  }

  /* Open other files database files */

  if ( output_mode ) {

    if ( open_start_dbs(start_db,domain_db,O_RDONLY ) == ERROR  && output_mode) {

      /* "Can't open start/host database" */

      error(A_ERR, "net_anonftp", "Can't open start/host database");
      exit(ERROR);
    }

    if ( !(strhan = archNewStrIdx()) ) {
      if ( output_mode ) {
        error( A_ERR, "net_anonftp", "Could create string handler" );
        exit(ERROR);
      }
    }
      
    if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_SEARCH  ) )
    {
      if ( output_mode ) {
        error( A_ERR, "net_anonftp", "Could not find strings_idx files" );
        exit(ERROR);
      }
    }

  }
  
  if(open_host_dbs((file_info_t *) NULL, hostdb, (file_info_t *) NULL, hostaux_db,O_RDWR) == ERROR){

    /* "Error while trying to open host database" */

    error(A_ERR, "net_anonftp", NET_ANONFTP_007);
    exit(ERROR);
  }

  /*fprintf(stderr,"sleeping zzz\n");
    sleep(15);
    */
  if(output_mode){


    signal(SIGPIPE, sig_handle);

    if(io_host[0] == '\0'){

      /* "No host specified for output mode" */

      error(A_ERR,"net_anonftp", NET_ANONFTP_008);
      exit(ERROR);
    }

    header_rec.header_flags = 0;
    HDR_HOSTDB_ALSO(header_rec.header_flags);
    HDR_HOSTAUX_ALSO(header_rec.header_flags);

    HDR_SET_UPDATE_STATUS(header_rec.header_flags);


    if(get_dbm_entry(make_lcase(io_host), strlen(io_host) + 1, &hostdb_ent, hostdb) == ERROR){

      /* "Can't find requested host %s in local primary host database" */

      error(A_ERR,"net_anonftp", NET_ANONFTP_009, io_host);
      header_rec.update_status = FAIL;
    }

    sprintf(tmp_string,"%s.%s.0",hostdb_ent.primary_hostname, ANONFTP_DB_NAME);

    if(get_dbm_entry(tmp_string, strlen(tmp_string) + 1, &hostaux_ent, hostaux_db) == ERROR){

      /* "Can't find requested host %s database %s in local auxiliary host database" */

      error(A_ERR,"net_anonftp", NET_ANONFTP_010, hostdb_ent.primary_hostname, ANONFTP_DB_NAME);
      header_rec.update_status = FAIL;
    }
    hostaux_ent.origin = NULL;    
    set_aux_origin(&hostaux_ent, ANONFTP_DB_NAME, 0);

    /* if the host is "ACTIVE" and the data file does not
       exist in the anonftp directory mark it as FAIL */

    if((hostaux_ent.current_status == ACTIVE)
       && (access(files_db_filename(inet_ntoa(ipaddr_to_inet(hostdb_ent.primary_ipaddr)),port), R_OK | F_OK) == -1))
    header_rec.update_status = FAIL;
    else
    header_rec.update_status = SUCCEED;


    make_header_hostdb_entry(&header_rec,&hostdb_ent,0);
    make_header_hostaux_entry(&header_rec,&hostaux_ent,0);   

    if((hostaux_ent.current_status == DEL_BY_ARCHIE) ||
       (hostaux_ent.current_status == DEL_BY_ADMIN) ||
       (hostaux_ent.current_status == DELETED)){

      header_rec.no_recs = 0;
      header_rec.site_no_recs = 0;
    }

    /* set the output header to whatever is the current method */

    header_rec.format = outhdr;
    HDR_SET_FORMAT(header_rec.header_flags);

    header_rec.generated_by = SERVER;
    HDR_SET_GENERATED_BY(header_rec.header_flags);

    /* set the header only to reflect the database name for this database */

    strcpy(header_rec.access_methods,ANONFTP_DB_NAME);

    if(send_anonftp_site( hostdb_ent.primary_ipaddr, &header_rec, &hostaux_ent, outhdr, compress_pgm, strhan) == ERROR){

      /* "Error sending anonftp site file" */

      error(A_ERR, "net_anonftp", NET_ANONFTP_012);
      exit(ERROR); 
    }

  }
  else{                         /* Input mode */

    signal(SIGHUP, sig_handle);


    if(get_anonftp_site(hostdb, hostaux_db ) == ERROR){

      /* "Error receiving anonftp site file" */

      error(A_ERR, "net_anonftp", NET_ANONFTP_013);
      exit(errno);
    }
  }

  if ( output_mode ) { 
    close_start_dbs(start_db,domain_db);
    archCloseStrIdx(strhan);
    archFreeStrIdx(&strhan);
  }

  exit(A_OK);   
  return(A_OK);
}

/*
 * get_anonftp_site: receive the site file from a remote process and write
 * it out
 */


status_t get_anonftp_site(hostdb, hostaux_db)
   file_info_t *hostdb;
   file_info_t *hostaux_db;

{

   header_t	header_rec;
   XDR		*xdrs;
   hostdb_t	hostdb_ent;
   hostdb_aux_t hostaux_ent;
   pathname_t	hostaux_name;
   pathname_t	tmp_filename;
   int	        finished, num_suff;
   format_t     origformat;
   int	        retcode;
   int	        p[2];
   FILE	        *fp;
   int	        statusp;
   time_t       start_time;
   time_t       end_time;
   struct stat  statbuf;

   ptr_check(hostdb, file_info_t, "get_anonftp_site", ERROR);
   ptr_check(hostaux_db, file_info_t, "get_anonftp_site", ERROR);

   hostaux_ent.origin = NULL;
   
   if(sitefile == (file_info_t *) NULL)
      sitefile = create_finfo();

   if(read_header(stdin, &header_rec, (u32 *) NULL,1,1) == ERROR){

      /* "Error reading header of remote site file" */
      
      error(A_ERR, "get_anonftp_site", GET_ANONFTP_SITE_001);
      return(ERROR);
   }

   if(header_rec.update_status == FAIL)
      return(A_OK);

   /* Try and randomize things a little */

   srand(time((time_t *) NULL));
   rand(); rand();
   
   for(finished = 0; !finished;){

      sprintf(sitefile -> filename,"%s/%s/%s-%s_%d%s%s", get_master_db_dir(), DEFAULT_TMP_DIR, header_rec.primary_hostname, header_rec.access_methods, (num_suff = rand() % 100), SUFFIX_UPDATE,TMP_SUFFIX);

      if(access(sitefile -> filename, R_OK | F_OK) == -1)
         finished = 1;
   }

   /* This may not be necessary */


   /* Check to see if the host already is in the local database */
      
   if(get_dbm_entry(header_rec.primary_hostname,
       strlen(header_rec.primary_hostname) + 1, &hostdb_ent, hostdb) == ERROR){

       header_rec.action_status = NEW;
       HDR_SET_ACTION_STATUS(header_rec.header_flags);
      
   }
   else{

     sprintf(hostaux_name,"%s.%s.0", header_rec.primary_hostname, header_rec.access_methods);

     if(get_dbm_entry(hostaux_name,
                      strlen(hostaux_name) + 1, &hostaux_ent, hostaux_db) == ERROR){
   
       header_rec.action_status = NEW;
       HDR_SET_ACTION_STATUS(header_rec.header_flags);
       hostaux_ent.origin = NULL;   
       set_aux_origin(&hostaux_ent,ANONFTP_DB_NAME, -1 );
     }
     else{

       header_rec.action_status = UPDATE;
       HDR_SET_ACTION_STATUS(header_rec.header_flags);
       hostaux_ent.origin = NULL;   
       set_aux_origin(&hostaux_ent,ANONFTP_DB_NAME, 0 );
     }
   }


   if(open_file(sitefile, O_WRONLY) == ERROR){

      /* "Error opening local site parser file %s" */

      error(A_ERR, "get_anonftp_site", GET_ANONFTP_SITE_002, sitefile -> filename);
      return(ERROR);
   }

   origformat = header_rec.format;

   header_rec.format = FRAW;
   HDR_SET_FORMAT(header_rec.header_flags);

   if(write_header( sitefile -> fp_or_dbm.fp, &header_rec, (u32 *) NULL, 0, 0) == ERROR){

      /* "Error writing header of local site parser file %s" */

      error(A_ERR, "get_anonftp_site", GET_ANONFTP_SITE_003, sitefile -> filename);
      return(ERROR);
   }

   /* if the record is a deletion, then just rename the file and finish */


   if((header_rec.current_status == DEL_BY_ARCHIE) ||
      (header_rec.current_status == DEL_BY_ADMIN) ||
      (header_rec.current_status == DELETED)){

      sprintf(tmp_filename,"%s/%s/%s-%s_%d%s", get_master_db_dir(), DEFAULT_TMP_DIR, header_rec.primary_hostname, header_rec.access_methods, num_suff, SUFFIX_UPDATE);

      if(rename(sitefile -> filename, tmp_filename) == -1){

        /* "Can't rename temporary file %s to %s" */

        error(A_SYSERR,"get_anonftp_site", GET_ANONFTP_SITE_006, sitefile -> filename, tmp_filename);
        return(ERROR);
      }

      exit(A_OK);
   }

   if(origformat == FXDR_COMPRESS_LZ) {

       if(verbose){

           /* "Input for %s in compressed format" */

           error(A_INFO, "get_anonftp_site", GET_ANONFTP_SITE_016, header_rec.primary_hostname);
       }

       if(pipe(p) == -1){

           /* "Can't open pipe to uncompression program" */

           error(A_ERR,"get_anonftp_site", GET_ANONFTP_SITE_008);
           return(ERROR);
       }

       if((retcode = fork()) == 0){

           if(dup2(p[1], 1) == -1){

               /* "Can't dup2 pipe to stdout" */

               error(A_ERR, "get_anonftp_site", GET_ANONFTP_SITE_009);
               exit(ERROR);
           }

           close(p[0]);

           execl(UNCOMPRESS_PGM, UNCOMPRESS_PGM, (char *) 0);

           /* "Can't execute uncompression program %s" */

           error(A_ERR, "get_anonftp_site", GET_ANONFTP_SITE_010, UNCOMPRESS_PGM);
           exit(ERROR);
       }
       else{
           if(retcode == -1){

               /* "Can't fork() for uncompression process" */

               error(A_ERR, "get_anonftp_file", GET_ANONFTP_SITE_011);
               return(ERROR);
           }
       }

       close(p[1]);

       if((fp = fdopen(p[0], "r")) == (FILE *) NULL){

           /* "Can't open incoming pipe" */

           error(A_ERR, "get_anonftp_site", GET_ANONFTP_SITE_012);
           return(ERROR);
       }


       if((xdrs = open_xdr_stream(fp, XDR_DECODE)) == (XDR *) NULL){

           /* "Can't open XDR stream for fp" */

           error(A_ERR, "get_anonftp_site", GET_ANONFTP_SITE_013);
           return(ERROR);
       }

   } else {                     /* Not compressed; only  XDR */

       if(verbose){
           /* "Input for %s not in compressed format" */
           error(A_INFO, "get_anonftp_site", GET_ANONFTP_SITE_017, header_rec.primary_hostname);
       }
#if 0
       /* This section of code is not required. - lucb */
       if(pipe(p) == -1){

           /* "Can't open pipe to cat program" */

           error(A_ERR,"get_anonftp_site", GET_ANONFTP_SITE_018);
           return(ERROR);
       }

       if((retcode = fork()) == 0){

           if(dup2(p[1], 1) == -1){

               /* "Can't dup2 pipe to stdout" */

               error(A_ERR, "get_anonftp_site", GET_ANONFTP_SITE_009);
               exit(ERROR);
           }

           close(p[0]);

           execl(CAT_PGM, CAT_PGM, (char *) 0);

           /* "Can't execute cat program %s" */

           error(A_ERR, "get_anonftp_site", GET_ANONFTP_SITE_019, CAT_PGM);
           exit(ERROR);
       } else {
           if(retcode == -1){

               /* "Can't fork() for cat process" */

               error(A_ERR, "get_anonftp_file", GET_ANONFTP_SITE_020);
               return(ERROR);
           }
       }

       close(p[1]);

       if((fp = fdopen(p[0], "r")) == (FILE *) NULL){

           /* "Can't open incoming pipe" */

           error(A_ERR, "get_anonftp_site", GET_ANONFTP_SITE_012);
           return(ERROR);
       }

#endif
      
       if((xdrs = open_xdr_stream(stdin, XDR_DECODE)) == (XDR *) NULL){

           /* "Can't open XDR stream for stdin" */

           error(A_ERR, "get_anonftp_site", GET_ANONFTP_SITE_004);
           return(ERROR);
       }
   }

   start_time = time((time_t *) NULL);

   if(copy_xdr_to_parser( xdrs, sitefile, header_rec.no_recs) == ERROR){
      
       /* "Error in translating XDR input. Deleting output file." */

       error(A_ERR, "get_anonftp_site", GET_ANONFTP_SITE_005);


       if(unlink(sitefile -> filename) == -1){

           /* "Can't unlink failed transfer file" */

           error(A_SYSERR, "get_anonftp_site", GET_ANONFTP_SITE_007);
           return(ERROR);
       }

       if ((origformat != FXDR) && (waitpid(-1, &statusp, WNOHANG) == -1)){

           error(A_ERR, "copy_xdr_to_parser", "Auxiliary program exited with %d", WEXITSTATUS(statusp));

           if(WTERMSIG(statusp))
           error(A_ERR, "copy_xdr_to_parser", "Auxiliary program exited with signal %d", WTERMSIG(statusp));
       }

       return(A_OK);
   }

   if((origformat != FXDR) && (waitpid(-1, &statusp, WNOHANG) == -1)){

       error(A_ERR, "copy_xdr_to_parser", "Auxiliary program exited with %d", WEXITSTATUS(statusp));

       if(WTERMSIG(statusp))
       error(A_ERR, "copy_xdr_to_parser", "Auxiliyar program exited with signal %d", WTERMSIG(statusp));
   }

   sprintf(tmp_filename,"%s/%s/%s-%s_%d%s", get_master_db_dir(), DEFAULT_TMP_DIR, header_rec.primary_hostname, header_rec.access_methods, num_suff, SUFFIX_UPDATE);

   if(verbose){
      double bps;

      if(fstat(fileno(sitefile -> fp_or_dbm.fp), &statbuf) == -1){

	 /* "Can't fstat() output file %s" */

	 error(A_SYSERR, "get_anonftp_site", GET_ANONFTP_SITE_014, sitefile -> filename);
      }

      /* "Tranferred %d bytes in %d seconds (%6.2f bytes/sec)" */

      bps = (double) statbuf.st_size / ((end_time = time((time_t *) NULL)) - start_time);

      error(A_INFO, "get_anonftp_site", GET_ANONFTP_SITE_015, statbuf.st_size, end_time - start_time, bps);
   }

   

   close_file(sitefile);

   if(rename(sitefile -> filename, tmp_filename) == -1){

      /* "Can't rename temporary file %s to %s" */

      error(A_SYSERR,"get_anonftp_site", GET_ANONFTP_SITE_006, sitefile -> filename, tmp_filename);
      return(ERROR);
   }

   close_xdr_stream(xdrs);   /* Wasn't xdr malloc'd? -lucb */
   destroy_finfo(sitefile);

   return(A_OK);
}



/*
 * send_anonftp: Open the anonftp site file specified by siteaddr and
 * convert it to an XDR stream on stdout
 */


status_t send_anonftp_site(siteaddr, header_rec, hostaux_ent, outhdr, compress_pgm, strhan)
   ip_addr_t siteaddr;	      /* address of site to be opened */
   header_t *header_rec;      /* header record */
   hostdb_aux_t *hostaux_ent;
   format_t outhdr;	      /* outgoing format */
   char *compress_pgm;	      /* Compression program */
   struct arch_stridx_handle *strhan;
{
  XDR *xdrs;
  FILE *fp;
  int retcode;
  int p[2];
  int no_recs;
  int port;

  no_recs = header_rec -> no_recs;

  if( get_port( header_rec->access_command, ANONFTP_DB_NAME, &port ) == ERROR ){
    error(A_ERR,"insert_anonftp","Could not find the port for this site", header_rec->primary_hostname);
    exit(A_OK);
  }

  sitefile = create_finfo();

  /* Open apropriate site file */

  strcpy(sitefile -> filename, files_db_filename(inet_ntoa(ipaddr_to_inet(siteaddr)),port));

  if(open_file(sitefile, O_RDONLY) == ERROR){

    /* "Can't open anonftp database sitefile %s" */

    error(A_ERR,"send_anonftp_site", SEND_ANONFTP_SITE_001, sitefile -> filename);
    header_rec -> update_status = FAIL;
  }
  else{
    if(mmap_file(sitefile, O_RDONLY) == ERROR){

      /* "Can't mmap anonftp database sitefile %s" */

      error(A_ERR,"send_anonftp_site", SEND_ANONFTP_SITE_002, sitefile -> filename);
      header_rec -> update_status = FAIL;
    }
  }

  if(write_header(stdout, header_rec, (u32 *) NULL, 0,0) == ERROR){

    /* "Error while writing header of output file" */

    error(A_ERR, "send_anonftp_site", SEND_ANONFTP_SITE_011);
    return(ERROR);
  }

  if((hostaux_ent -> current_status == DEL_BY_ARCHIE) ||
     (hostaux_ent -> current_status == DEL_BY_ADMIN) ||
     (hostaux_ent -> current_status == DELETED) ||
     (header_rec -> update_status == FAIL)){

    return(A_OK);
  }

  if(outhdr != FXDR){

    if(verbose){

      /* "Output for %s in compressed format" */

      error(A_INFO, "send_anonftp_site", SEND_ANONFTP_SITE_012, header_rec -> primary_hostname);
    }

    if(pipe(p) == -1){

      /* "Can't open pipe to compression program" */

      error(A_ERR,"send_anonftp_site", SEND_ANONFTP_SITE_005);
      return(ERROR);
    }

    if((retcode = fork()) == 0){

      if(dup2(p[0], fileno(stdin)) == -1){

        /* "Can't dup2 pipe to stdin" */

        error(A_ERR, "send_anonftp_site", SEND_ANONFTP_SITE_006);
        exit(ERROR);
      }

      close(p[1]);

      execl(compress_pgm, compress_pgm, (char *) 0);

      /* "Can't execute compression program %s" */

      error(A_ERR, "send_anonftp_site", SEND_ANONFTP_SITE_007, compress_pgm);
      exit(ERROR);
    }
    else{
      if(retcode == -1){

        /* "Can't fork() for compression process" */

        error(A_ERR, "send_anonftp_file", SEND_ANONFTP_SITE_010);
        return(ERROR);
      }
    }

    close(p[0]);


    if((fp = fdopen(p[1], "w")) == (FILE *) NULL){

      /* "Can't open outgoing pipe" */

      error(A_ERR, "send_anonftp_site", SEND_ANONFTP_SITE_008);
      return(ERROR);
    }

    if((xdrs = open_xdr_stream(fp, XDR_ENCODE)) == (XDR *) NULL){

      /* "Can't open XDR stream for compression ftp" */

      error(A_ERR, "send_anonftp_site", SEND_ANONFTP_SITE_009);
      return(ERROR);
    }

  }
  else{

    /* found host db entry now open XDR stream */

    if((xdrs = open_xdr_stream(stdout, XDR_ENCODE)) == (XDR *) NULL){

      /* "Can't open XDR stream for stdout" */

      error(A_ERR, "send_anonftp_site", SEND_ANONFTP_SITE_003);
      return(ERROR);
    }
  }
  /* Convert site file into parser output format, then to XDR stream */

  if(copy_parser_to_xdr( xdrs, sitefile, no_recs, strhan) == ERROR){

    /* "Error translating site file to XDR stream" */

    error(A_ERR, "send_anonftp_site", SEND_ANONFTP_SITE_004);
    return(ERROR);
  }
   
  close_xdr_stream(xdrs);
  close_file(sitefile);

  destroy_finfo(sitefile);
   
  return(A_OK);
}

/*
 * copy_parser_to_xdr: take given sitefile, convert it to parser format and
 * then convert to XDR stream
 */


  
status_t copy_parser_to_xdr( xdrs, sitefile, no_recs, strhan)
   XDR *xdrs;		   /* active XDR stream */
   file_info_t *sitefile;  /* file control for site file */
   index_t no_recs;	   /* number of records in site file */
   struct arch_stridx_handle *strhan;
{
  parser_entry_t parser_rec;
  index_t *children_arr, *parents_arr;
  static full_site_entry_t *site_ent, *next_ent;
  /*   static char tmp_string[8096];    lucb - was pathname_t */
  char *tmp_string;
  int curr_recno;
  index_t *strings_arr;
  index_t  curr_parent = (index_t) (-1);
  int  *rec_numbers ;
  /* Stores the original parent indexes mapped to 
     their new locations after the addition of the parent
     entries.*/

  int num, i = 0;

  if ((rec_numbers = (int *)malloc(sizeof(int)*(no_recs*2)))== (int *) NULL){

    /* "Can't malloc space for integer" */

    error(A_SYSERR, "copy_parser_to_xdr", "Can't malloc space for integer array\n");
    return(ERROR);
  }


  ptr_check(xdrs, XDR, "copy_parser_to_xdr", ERROR);
  ptr_check(sitefile, file_info_t, "copy_parser_to_xdr", ERROR);   

  /* Map the strings file */
  if (( children_arr = (index_t *)malloc(sizeof(index_t)*(no_recs)))== (index_t *) NULL){

    /* "Can't malloc space for children array" */

    error(A_SYSERR, "copy_parser_to_xdr", "Can't malloc space for children array");
    return(ERROR);
  }

  if (( parents_arr = (index_t *)malloc(sizeof(index_t)*(no_recs)))== (index_t *) NULL){

    /* "Can't malloc space for parents array" */

    error(A_SYSERR, "copy_parser_to_xdr", "Can't malloc space for parents array");
    return(ERROR);
  }

  if (( strings_arr = (index_t *)malloc(sizeof(index_t)*(no_recs))) == (index_t *) NULL ){

    /* "Can't malloc space for char array" */

    error(A_SYSERR, "copy_parser_to_xdr", "Can't malloc space for char array\n");
    return(ERROR);
  }


  /* For each record in the site file */

  /* - We first take a first pass at all the records in which
   *  we create the following:
   *  1- the correct index mappings from the array of full_site_entry_t and
   *     to the array of  parser_entry_t. The latter is shorter since it does
   *     not contain the extra parent_entry records.
   *  2- recreate the correct child_idx pointers for the new parser records.
   */


  num = no_recs;
  i = 0;

  /* num will be increasing as potential parent_entries are
   * foreseen. Once I see a record that is a directory and not a file
   * num is increased.
   */

  for(curr_recno = 0; curr_recno < num; curr_recno++){

    /* Get the full site entry record */
    if( (curr_recno == 1)||(curr_recno == 2)) {
      rec_numbers[ curr_recno ] = i;      
      num++;
      continue;
    }
    site_ent = (full_site_entry_t *) sitefile -> ptr + curr_recno;
      
    /* Get the parser record */

    rec_numbers[ curr_recno ] = i;

    if(  !(CSE_IS_SUBPATH((*site_ent)) || 
           CSE_IS_NAME((*site_ent)) ||
           CSE_IS_PORT((*site_ent)) ) ) {

      /* rec_numbers takes care of the mapping from the indecies
       * of the site file (reflecting the bigger array) to the smaller
       * current array idecies to be sent.
       */

      parents_arr[i] = curr_parent;
      children_arr[i] = (index_t)(-1);
      strings_arr[i] = site_ent -> strt_1;

      i++;

      /* i only increases when an element is to be sent is being
       * prepeared. It is used for the mapping of indecies. 
       */

    }else
    {
	    num++;  

      /* - must increase it to take care of the extra 
       * records we add at insertion time for each parent entry..
       */

      /* rec_numbers takes care of the mapping from the indecies
       * from the site file (reflecting the bigger array) to the smaller
       * current array idecies to be sent.
       */

	    if( (index_t) site_ent -> core.prnt_entry.strt_2 >= 0 )
      {
        /* To get the original listing of this parent_entry. strt_2 points
         * to its original location in its parent's listing.
         */
		 
        curr_parent = rec_numbers[site_ent -> core.prnt_entry.strt_2] ;

        if( curr_recno+1 < num ) {

          /* This should take care of the case when there are no children for
           * this parent directory. If the next record is another parent directory
           * then there are no children for the current parent_entry.
           */
          next_ent = (full_site_entry_t *) sitefile -> ptr + curr_recno + 1;
          if( !(CSE_IS_SUBPATH((*next_ent)) || 
                CSE_IS_NAME((*next_ent)) ||
                CSE_IS_PORT((*next_ent)) ) )
          {	children_arr[ curr_parent ] = i;
            /* the first entry in this directory listing, i already is increased */
          }
        }
      }else 
      {
        curr_parent = (index_t) (-1);
        /*  This case only occurs in the top directory ./ */
      }
    }
  }

  for(i = 0, curr_recno = 0; curr_recno < num; curr_recno++){


    if( (curr_recno == 1)||(curr_recno == 2)) {
      continue;
    }
    
    site_ent = (full_site_entry_t *) sitefile -> ptr + curr_recno;
      
    /* Get the parser record */

    if(  !(CSE_IS_SUBPATH((*site_ent)) || 
           CSE_IS_NAME((*site_ent)) ||
           CSE_IS_PORT((*site_ent)) ) ) {

      /* rec_numbers takes care of the mapping from the indecies
       * of the site file (reflecting the bigger array) to the smaller
       * current array idecies to be sent.
       */

      parser_rec.core.size = site_ent -> core.entry.size;
      parser_rec.core.date = site_ent -> core.entry.date;
      parser_rec.core.perms = site_ent -> core.entry.perms;
      parser_rec.core.parent_idx = parents_arr[i];
      parser_rec.core.child_idx = children_arr[i];
      parser_rec.core.flags = site_ent -> flags;
    
      /* Get the string associated with parser record */

      if( !archGetString( strhan,  strings_arr[i], &tmp_string) ){
        error(A_ERR,"copy_parser_to_xdr","Could not find the string using archGetString\n");
        return( ERROR );
      }
      
      parser_rec.slen = strlen(tmp_string);
      
      if( xdr_parser_entry_t(xdrs,&parser_rec) != TRUE){
        
        /* "Conversion to XDR stream from raw format failed" */
        
        error(A_INTERR,"copy_parser_to_xdr", COPY_PARSER_TO_XDR_002);
        return(ERROR);
      }
      
      /*
       * This might have to be changed to xdr_array but since xdr_char
       * unpacks characters into 4 bytes each the space requirements would
       * be too expensive. We'll try with this for now
       */
      
      if( xdr_opaque( xdrs, tmp_string, parser_rec.slen) != TRUE){
        
        /* "Conversion of string to XDR opaque type from raw format failed" */
        
        error(A_INTERR,"copy_parser_to_xdr", COPY_PARSER_TO_XDR_003);
        return(ERROR);
      }

      i++;
      free(tmp_string);

    }
    /* when it is not a file do nothing */
  }
  if( strings_arr != NULL ) { free(strings_arr); strings_arr = NULL; }
  if( parents_arr != NULL ) { free(parents_arr); parents_arr = NULL; }
  if( children_arr != NULL ) { free(children_arr); children_arr = NULL; }
  if( rec_numbers != NULL ) { free(rec_numbers); rec_numbers = NULL; }  
  return(A_OK);
}


/*
 * copy_xdr_to_parser: copy incoming XDR stream (in parser format) to raw
 * parser format
 */


status_t copy_xdr_to_parser( xdrs, sitefile, no_recs)
   XDR *xdrs;		   /* active XDR stream */
   file_info_t *sitefile;  /* file to be written to */
   index_t no_recs;	   /* number of records in site file */

{

   int curr_recno;
   parser_entry_t parser_rec;
   static char string_buf[8096];    /* lucb - was pathname_t [256] */
   int s_length;
   fd_set fds;
   
   ptr_check(xdrs, XDR, "copy_xdr_to_parser", ERROR);
   ptr_check(sitefile, file_info_t, "copy_xdr_to_parser", ERROR);     

   

   /* for each record in the incoming stream */

   signal(SIGALRM,sig_alarm);
   alarm(timeout);   

   for(curr_recno = 0; curr_recno < no_recs; curr_recno++){
#if 0
     struct  timeval timeval_struct;
     int finished;
     
     FD_ZERO(&fds);
     FD_SET(0,&fds);
     timeval_struct.tv_sec = (long) timeout;
     timeval_struct.tv_usec = 0L;

     /* convert from XDR to parser format */
     if ( (finished=select(1 , &fds,NULL,NULL,&timeval_struct)) >= 0 ) {
       if ( finished == 0 ) {
         error(A_ERR,"net_anonftp", "Timeout of %d secs while receiving data",timeout);
         return ERROR;
       }
     }
     else {
       error(A_ERR,"net_anonftp", "Select failed with errno %d",errno);
       return ERROR;
     }
#endif
     memset(&parser_rec,0,sizeof(parser_rec));
     rec_number = curr_recno;
     if(xdr_parser_entry_t(xdrs,&parser_rec) != TRUE){

       /* "Conversion to raw format from XDR format failed" */
       /* debug("copy_xdr_to_parser", "Failed on record %d", curr_recno); */
       error(A_WARN,"copy_xdr_to_parser", COPY_XDR_TO_PARSER_001);
       return(ERROR);
     }

     /* LIttle hack in the case of anonymous ftp .. */
     if ( parser_rec.core.flags > 3 )
        parser_rec.core.flags &= 0x03;

     /* write out the record */

     if(fwrite((char *) &parser_rec, sizeof(parser_entry_t), 1, sitefile -> fp_or_dbm.fp) == 0){

       /* "Can't write out parser record %d" */

       error(A_SYSERR,"copy_xdr_to_parser", COPY_XDR_TO_PARSER_002, curr_recno );
       return(ERROR);
     }

     if (parser_rec.slen >= 8096) {
       error(A_SYSERR, "copy_xdr_to_parser",
             "Internal buffer overflow (%d > 8096)", parser_rec.slen);
       return(ERROR);
     }
     if(xdr_opaque(xdrs, string_buf, parser_rec.slen) != TRUE){

       /* "Conversion of string to parser from XDR format failed for record %d" */

       error(A_WARN,"copy_xdr_to_parser", COPY_XDR_TO_PARSER_003, curr_recno);
       return(ERROR);
     }


     /*
      * length of incoming string has been padded up to the nearest word
      * boundary (4 bytes, 32 bits)
      */

     s_length = parser_rec.slen;

     if( s_length & 0x3 )
     s_length += (4 - (s_length & 0x3));


     /*   printf("string=%s recno=%d \n",string_buf,curr_recno); */

      
     if(fwrite(string_buf, s_length, 1, sitefile -> fp_or_dbm.fp) == 0){

       /* "Can't write out parser string %s record %d" */

       error(A_SYSERR,"copy_xdr_to_parser", COPY_XDR_TO_PARSER_004, string_buf, curr_recno);
       return(ERROR);
     }
     memset(string_buf,0,s_length);
   }

   return(A_OK);
}
      


/*
 * Note that many of the xdr_<type> calls for the "basic" (simple) elements
 * are macros not procedure calls defined in typedef.h
 */


/*
 * xdr_core_site_entry_t: convert a core_site_entry_t record to/from XDR
 * stream format
 */

bool_t xdr_core_site_entry_t( xdrs, core_rec )
   XDR *xdrs;
   core_site_entry_t *core_rec;

{
#if 0

   int r1,r2,r3,r4,r5,r6;

   ptr_check(xdrs, XDR, "xdr_core_site_entry_t", FALSE);
   ptr_check(core_rec, core_site_entry_t, "xdr_core_site_entry_t", FALSE);   

   /* debugging */

   r1 = xdr_file_size_t(xdrs, &(core_rec -> size));
   r2 = xdr_date_time_t(xdrs, &(core_rec -> date));
   r3 = xdr_index_t(xdrs, &(core_rec -> parent_idx));
   r4 = xdr_index_t(xdrs, &(core_rec -> child_idx));
   r5 = xdr_perms_t(xdrs, &(core_rec -> perms));
   r6 = xdr_flags_t(xdrs, &(core_rec -> flags));

   return (r1 && r2 && r3 && r4 && r5 && r6);
   
#else

   ptr_check(xdrs, XDR, "xdr_core_site_entry_t", FALSE);
   ptr_check(core_rec, core_site_entry_t, "xdr_core_site_entry_t", FALSE);   


   return( xdr_file_size_t(xdrs, &(core_rec -> size)) &&
           xdr_date_time_t(xdrs, &(core_rec -> date)) &&
	   xdr_index_t(xdrs, &(core_rec -> parent_idx)) &&
   	   xdr_index_t(xdrs, &(core_rec -> child_idx)) &&
	   xdr_perms_t(xdrs, &(core_rec -> perms)) &&
	   xdr_flags_t(xdrs, &(core_rec -> flags))
	   );
#endif
}



/*
 * xdr_parser_entry_t: convert parser record to/from XDR stream
 */


bool_t xdr_parser_entry_t( xdrs, parser_rec)
   XDR *xdrs;
   parser_entry_t *parser_rec;

{

   ptr_check(xdrs, XDR, "xdr_parser_entry_t", FALSE);
   ptr_check(parser_rec, parser_entry_t, "xdr_parser_entry_t", FALSE);   

   return(xdr_core_site_entry_t(xdrs, &(parser_rec -> core)) &&
          xdr_strlen_t(xdrs, &(parser_rec -> slen)));
}

/* Signal handler */


void sig_handle(sig)
   int sig;
{

   if(sig == SIGPIPE){

      /* "Broken pipe: remote data transfer process existed prematurely" */

      error(A_ERR, "sig_handle", NET_ANONFTP_014);
      exit(A_OK);
   }

   if(sig == SIGHUP){

      if(sitefile -> fp_or_dbm.fp != (FILE *) NULL)
         close_file(sitefile);

      if(sitefile -> filename[0] != '\0')
	 unlink(sitefile -> filename);
   }

   exit(A_OK);
}
