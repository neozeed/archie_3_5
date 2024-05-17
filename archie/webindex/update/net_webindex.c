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
#include <sys/time.h>
#include <sys/resource.h>
#if defined(AIX) || defined(SOLARIS)
#include <rpc/types.h>
#endif
#include <rpc/xdr.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef AIX
#include <mp.h>
#endif
#include "protos.h"
#include "typedef.h"
#include "db_ops.h"
#include "host_db.h"
#include "parser_file.h"
#include "excerpt.h"
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
#include "net_webindex.h"
#include "db_files.h"
#include "archie_xdr.h"
#include "lang_webindex.h"

extern int errno;
char date_string[100];
static char *beg = "19700101010101";


static int rec_number = 0;

static void sig_alarm(num)
  int num;
{
  static last_rec_number = 0;
  
  if  ( last_rec_number == rec_number ) {
    error(A_ERR,"net_anonftp","Timeout of 900 seconds expired");
    exit(ERROR);
  }

  alarm(900);
  signal(SIGALRM,sig_alarm);
  last_rec_number = rec_number;
}



/*
 * net_webindex: this program is responsible for inter-archie transmission
 * of webindex database files. It is not normally invoked manually. This
 * program can both transmit and receive data, which is transmitted in Sun
 * XDR format to allow inter-architecture communications

   argv, argc are used.


   Parameters:	  -I
		  -O
      -p <port>
		  -w <webindex files database pathname>
		  -M <master database pathname>
		  -h <host database pathname>
		  -l write to log file (default)
		  -L <log file>
		  -v verbose

 */

/* this has to be here so that the signal handler can see it */


file_info_t	*sitefile = (file_info_t *) NULL;

int verbose = 0;

char *prog;

int main(argc, argv)
   int argc;
   char *argv[];
{
#if 0
#ifdef __STDC__

  extern int getopt(int, char **, char *);

#else

  extern int getopt();

#endif
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

  index_t index;
  int excerpt = 0;
  int suff_num = -1;
  
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
  char port_num[20];
  prog = argv[0];
  files_database_dir[0] = host_database_dir[0] = master_database_dir[0] = start_database_dir[0] = '\0';
  compress_pgm[0] = logfile[0] = '\0';

  strcpy(date_string,beg);
  
  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;

  while((option = (int) getopt(argc, argv, "w:i:p:M:h:IO:lL:cven:d:")) != EOF){

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

      /* port of the site */

    case 'p':
	    port = atoi(optarg);
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

    case 'd':
      strcpy(date_string,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;
      
    case 'e':
      excerpt = 1;
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

    case 'n':
      suff_num = atoi(optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* Verbose mode */

    case 'v':
	    verbose = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

      /* webindex database directory name  */

    case 'w':
	    strcpy(files_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

    }
  }

  /* set up logs */

  if(logging){  
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /*  "Can't open default log file" */

        error(A_ERR, "parse_webindex", NET_WEBINDEX_001);
        exit(ERROR);
      }
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, "parse_webindex", NET_WEBINDEX_002, logfile);
        exit(ERROR);
      }
    }
  }


  
#if 0
  if(port==0){

    /* "No port number given" */

    error(A_ERR,"net_webindex","No port number given");
    if( get_port((char *)NULL, WEBINDEX_DB_NAME, &port ) == ERROR ){
      error(A_ERR,"get_port","Could not get port for site");
      exit(A_OK);
    }
  }
#endif
  
  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR, "net_webindex", NET_WEBINDEX_003);
    exit(ERROR);
  }

  if((dbdir = (char *)set_wfiles_db_dir(files_database_dir)) == (char *) NULL){

    /* "Error while trying to set webindex database directory" */

    error(A_ERR, "net_webindex", NET_WEBINDEX_004);
    exit(ERROR);
  }

  if((set_start_db_dir(start_database_dir,DEFAULT_WFILES_DB_DIR)) == (char *) NULL){

    /* "Error while trying to set start database directory" */

    error(A_ERR, "net_webindex", "Error while trying to set start database directory");
    exit(A_OK);
  }

  if(set_host_db_dir(host_database_dir) == (char *) NULL){

    /* "Error while trying to set host database directory" */

    error(A_ERR, "net_webindex", NET_WEBINDEX_005);
    exit(ERROR);
  }

  if ( output_mode ) {
    /* Open other files database files */

    if ( open_start_dbs(start_db,domain_db,O_RDONLY ) == ERROR ) {

      /* "Can't open start/host database" */

      error(A_ERR, "net_webindex", "Can't open start/host database");
      exit(A_OK);
    }

    if ( !(strhan = archNewStrIdx()) ) {
      error(A_WARN, "check_webindex","Could not create string handler");
      exit(A_OK);
    }

    if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_SEARCH ) )
    {
      error( A_WARN, "net_webindex", "Could not find strings_idx files, will create them.\n" );
      exit(A_OK);
    }

  }
  if(open_host_dbs((file_info_t *) NULL, hostdb, (file_info_t *) NULL, hostaux_db,O_RDWR) == ERROR){

    /* "Error while trying to open host database" */

    error(A_ERR, "net_webindex", NET_WEBINDEX_007);
    exit(ERROR);
  }


  if(output_mode){


    signal(SIGPIPE, sig_handle);

    if(io_host[0] == '\0'){
      /* "No host specified for output mode" */
      error(A_ERR,"net_webindex", NET_WEBINDEX_008);
      exit(ERROR);
    }

    header_rec.header_flags = 0;
    HDR_HOSTDB_ALSO(header_rec.header_flags);
    HDR_HOSTAUX_ALSO(header_rec.header_flags);

    HDR_SET_UPDATE_STATUS(header_rec.header_flags);

    /* if the host is "ACTIVE" and the data file does not
       exist in the webindex directory mark it as FAIL */

    if((hostaux_ent.current_status == ACTIVE)
       && (access((char*)wfiles_db_filename(inet_ntoa(ipaddr_to_inet(hostdb_ent.primary_ipaddr)),port), R_OK | F_OK) == -1))
    header_rec.update_status = FAIL;
    else
    header_rec.update_status = SUCCEED;

    if(get_dbm_entry(make_lcase(io_host), strlen(io_host) + 1, &hostdb_ent, hostdb) == ERROR){
      /* "Can't find requested host %s in local primary host database" */
      error(A_ERR,"net_webindex", NET_WEBINDEX_009, io_host);
      header_rec.update_status = FAIL;
    }

/*    sprintf(tmp_string,"%s.%s",hostdb_ent.primary_hostname, WEBINDEX_DB_NAME); */
    
/*    if(get_dbm_entry(tmp_string, strlen(tmp_string) + 1, &hostaux_ent, hostaux_db) == ERROR){ */

    sprintf(port_num,"%d",port);
     if ( get_hostaux_ent(hostdb_ent.primary_hostname, WEBINDEX_DB_NAME,&index,
                          NULL, port_num,
                          &hostaux_ent, hostaux_db) == ERROR){
      /* "Can't find requested host %s database %s in local auxiliary host database" */
      error(A_ERR,"net_webindex", NET_WEBINDEX_010, hostdb_ent.primary_hostname, WEBINDEX_DB_NAME);
      header_rec.update_status = FAIL;
    }

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
    strcpy(header_rec.access_methods,WEBINDEX_DB_NAME);
    if (excerpt )  {
      if(send_webindex_excerpt( hostdb_ent.primary_ipaddr, &header_rec, &hostaux_ent, outhdr, compress_pgm, strhan, cvt_to_inttime(date_string,0) ) == ERROR){
        /* "Error sending webindex site file" */
        error(A_ERR, "net_webindex", NET_WEBINDEX_012);
        exit(ERROR); 
      }
    }
    else {
      if(send_webindex_site( hostdb_ent.primary_ipaddr, &header_rec, &hostaux_ent, outhdr, compress_pgm, strhan, cvt_to_inttime(date_string,0) ) == ERROR){
        /* "Error sending webindex site file" */
        error(A_ERR, "net_webindex", NET_WEBINDEX_012);
        exit(ERROR); 
      }
    }

  }
  else{                         /* Input mode */
    signal(SIGHUP, sig_handle);
    if ( excerpt == 1 ) {
      if(get_webindex_excerpt(hostdb, hostaux_db,suff_num) == ERROR){
        /* "Error receiving webindex site file" */
        error(A_ERR, "net_webindex", NET_WEBINDEX_013);
        exit(errno);
      }
    }
    else {
      if(get_webindex_site(hostdb, hostaux_db,suff_num) == ERROR){
        /* "Error receiving webindex site file" */
        error(A_ERR, "net_webindex", NET_WEBINDEX_013);
        exit(errno);
      }
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
 * get_webindex_site: receive the site file from a remote process and write
 * it out
 */


status_t get_webindex_site(hostdb, hostaux_db,suff_num)
   file_info_t *hostdb;
   file_info_t *hostaux_db;
  int suff_num;
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
  
  ptr_check(hostdb, file_info_t, "get_webindex_site", ERROR);
  ptr_check(hostaux_db, file_info_t, "get_webindex_site", ERROR);

  if(sitefile == (file_info_t *) NULL)
  sitefile = create_finfo();

  if(read_header(stdin, &header_rec, (u32 *) NULL,1,1) == ERROR){

    /* "Error reading header of remote site file" */
      
    error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_001);
    return(ERROR);
  }

  if(header_rec.update_status == FAIL)
  return(A_OK);


  if ( suff_num == -1 ) {
    /* Try and randomize things a little */

    srand(time((time_t *) NULL));
    rand(); rand();
   
    for(finished = 0; !finished;){

      sprintf(sitefile -> filename,"%s/%s/%s-%s_%d%s%s", get_master_db_dir(), DEFAULT_TMP_DIR, header_rec.primary_hostname, header_rec.access_methods, (num_suff = rand() % 100), SUFFIX_PARTIAL_UPDATE,TMP_SUFFIX);

      if(access(sitefile -> filename, R_OK | F_OK) == -1)
      finished = 1;
    }
  }
  else {
      sprintf(sitefile -> filename,"%s/%s/%s-%s_%d%s%s", get_master_db_dir(), DEFAULT_TMP_DIR, header_rec.primary_hostname, header_rec.access_methods, suff_num, SUFFIX_PARTIAL_UPDATE,TMP_SUFFIX);
      num_suff = suff_num;
  }
  /* This may not be necessary */


  /* Check to see if the host already is in the local database */
      
  if(get_dbm_entry(header_rec.primary_hostname,
                   strlen(header_rec.primary_hostname) + 1, &hostdb_ent, hostdb) == ERROR){

    header_rec.action_status = NEW;
    HDR_SET_ACTION_STATUS(header_rec.header_flags);
      
  }
  else{
    index_t index = -1;

    if ( get_hostaux_ent(header_rec.primary_hostname,header_rec.access_methods,
                         &index,  header_rec.preferred_hostname,
                         header_rec.access_command,
                         &hostaux_ent, hostaux_db) == ERROR){
      
/*    if(get_dbm_entry(hostaux_name, strlen(hostaux_name) + 1,
                     &hostaux_ent, hostaux_db) == ERROR){
*/
      header_rec.action_status = NEW;
      HDR_SET_ACTION_STATUS(header_rec.header_flags);
      hostaux_ent.origin = NULL;   
      set_aux_origin(&hostaux_ent,WEBINDEX_DB_NAME, -1 );
      
    }
    else{

      header_rec.action_status = UPDATE;
      HDR_SET_ACTION_STATUS(header_rec.header_flags);
      hostaux_ent.origin = NULL;   
      set_aux_origin(&hostaux_ent,WEBINDEX_DB_NAME, index );
    }
    sprintf(hostaux_name,"%s.%s.%d", header_rec.primary_hostname, header_rec.access_methods,(int)index);
    
  }


  if(open_file(sitefile, O_WRONLY) == ERROR){

    /* "Error opening local site parser file %s" */

    error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_002, sitefile -> filename);
    return(ERROR);
  }

  origformat = header_rec.format;

  header_rec.format = FRAW;
  HDR_SET_FORMAT(header_rec.header_flags);

  if(write_header( sitefile -> fp_or_dbm.fp, &header_rec, (u32 *) NULL, 0, 0) == ERROR){

    /* "Error writing header of local site parser file %s" */

    error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_003, sitefile -> filename);
    return(ERROR);
  }

  /* if the record is a deletion, then just rename the file and finish */


  if((header_rec.current_status == DEL_BY_ARCHIE) ||
     (header_rec.current_status == DEL_BY_ADMIN) ||
     (header_rec.current_status == DELETED)){

    sprintf(tmp_filename,"%s/%s/%s-%s_%d%s", get_master_db_dir(), DEFAULT_TMP_DIR, header_rec.primary_hostname, header_rec.access_methods, num_suff, SUFFIX_PARTIAL_UPDATE);

    if(rename(sitefile -> filename, tmp_filename) == -1){

      /* "Can't rename temporary file %s to %s" */

      error(A_SYSERR,"get_webindex_site", GET_WEBINDEX_SITE_006, sitefile -> filename, tmp_filename);
      return(ERROR);
    }

    exit(A_OK);
  }

  if(origformat == FXDR_COMPRESS_LZ) {

    if(verbose){

      /* "Input for %s in compressed format" */

      error(A_INFO, "get_webindex_site", GET_WEBINDEX_SITE_016, header_rec.primary_hostname);
    }

    if(pipe(p) == -1){

      /* "Can't open pipe to uncompression program" */

      error(A_ERR,"get_webindex_site", GET_WEBINDEX_SITE_008);
      return(ERROR);
    }

    if((retcode = fork()) == 0){

      if(dup2(p[1], 1) == -1){

        /* "Can't dup2 pipe to stdout" */

        error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_009);
        exit(ERROR);
      }

      close(p[0]);

      execl(UNCOMPRESS_PGM, UNCOMPRESS_PGM, (char *) 0);

      /* "Can't execute uncompression program %s" */

      error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_010, UNCOMPRESS_PGM);
      exit(ERROR);
    }
    else{
      if(retcode == -1){

        /* "Can't fork() for uncompression process" */

        error(A_ERR, "get_webindex_file", GET_WEBINDEX_SITE_011);
        return(ERROR);
      }
    }

    close(p[1]);

    if((fp = fdopen(p[0], "r")) == (FILE *) NULL){

      /* "Can't open incoming pipe" */

      error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_012);
      return(ERROR);
    }


    if((xdrs = open_xdr_stream(fp, XDR_DECODE)) == (XDR *) NULL){

      /* "Can't open XDR stream for fp" */

      error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_013);
      return(ERROR);
    }

  } else {                      /* Not compressed; only  XDR */

    if(verbose){
      /* "Input for %s not in compressed format" */
      error(A_INFO, "get_webindex_site", GET_WEBINDEX_SITE_017, header_rec.primary_hostname);
    }
#if 0
    /* This section of code is not required. - lucb */
    if(pipe(p) == -1){

      /* "Can't open pipe to cat program" */

      error(A_ERR,"get_webindex_site", GET_WEBINDEX_SITE_018);
      return(ERROR);
    }

    if((retcode = fork()) == 0){

      if(dup2(p[1], 1) == -1){

        
        /* "Can't dup2 pipe to stdout" */

        error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_009);
        exit(ERROR);
      }

      close(p[0]);

      execl(CAT_PGM, CAT_PGM, (char *) 0);

      /* "Can't execute cat program %s" */

      error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_019, CAT_PGM);
      exit(ERROR);
    } else {
      if(retcode == -1){

        /* "Can't fork() for cat process" */

        error(A_ERR, "get_webindex_file", GET_WEBINDEX_SITE_020);
        return(ERROR);
      }
    }

    close(p[1]);

    if((fp = fdopen(p[0], "r")) == (FILE *) NULL){

      /* "Can't open incoming pipe" */

      error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_012);
      return(ERROR);
    }

#endif
      
    if((xdrs = open_xdr_stream(stdin, XDR_DECODE)) == (XDR *) NULL){

      /* "Can't open XDR stream for stdin" */

      error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_004);
      return(ERROR);
    }
  }

  start_time = time((time_t *) NULL);

  if(copy_xdr_to_parser( xdrs, sitefile, header_rec.no_recs) == ERROR){
      
    /* "Error in translating XDR input. Deleting output file." */

    error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_005);


    if(unlink(sitefile -> filename) == -1){

      /* "Can't unlink failed transfer file" */

      error(A_SYSERR, "get_webindex_site", GET_WEBINDEX_SITE_007);
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

  sprintf(tmp_filename,"%s/%s/%s-%s_%d%s", get_master_db_dir(), DEFAULT_TMP_DIR, header_rec.primary_hostname, header_rec.access_methods, num_suff, SUFFIX_PARTIAL_UPDATE);

  if(verbose){
    double bps;

    if(fstat(fileno(sitefile -> fp_or_dbm.fp), &statbuf) == -1){

      /* "Can't fstat() output file %s" */

      error(A_SYSERR, "get_webindex_site", GET_WEBINDEX_SITE_014, sitefile -> filename);
    }

    /* "Tranferred %d bytes in %d seconds (%6.2f bytes/sec)" */

    bps = (double) statbuf.st_size / ((end_time = time((time_t *) NULL)) - start_time);

    error(A_INFO, "get_webindex_site", GET_WEBINDEX_SITE_015, statbuf.st_size, end_time - start_time, bps);
  }

   

  close_file(sitefile);

  if(rename(sitefile -> filename, tmp_filename) == -1){

    /* "Can't rename temporary file %s to %s" */

    error(A_SYSERR,"get_webindex_site", GET_WEBINDEX_SITE_006, sitefile -> filename, tmp_filename);
    return(ERROR);
  }

  close_xdr_stream(xdrs);       /* Wasn't xdr malloc'd? -lucb */
  destroy_finfo(sitefile);

  return(A_OK);
}

status_t get_webindex_excerpt(hostdb, hostaux_db, suff_num)
   file_info_t *hostdb;
   file_info_t *hostaux_db;
  int suff_num;
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
  int status;
  
  ptr_check(hostdb, file_info_t, "get_webindex_excerpt", ERROR);
  ptr_check(hostaux_db, file_info_t, "get_webindex_excerpt", ERROR);

  if(sitefile == (file_info_t *) NULL)
  sitefile = create_finfo();

  if(read_header(stdin, &header_rec, (u32 *) NULL,1,1) == ERROR){

    /* "Error reading header of remote site file" */
      
    error(A_ERR, "get_webindex_excerpt", GET_WEBINDEX_SITE_001);
    return(ERROR);
  }

  if(header_rec.update_status == FAIL)
  return(A_OK);

  if ( suff_num == -1 ) {
    /* Try and randomize things a little */

    srand(time((time_t *) NULL));
    rand(); rand();
   
    for(finished = 0; !finished;){

      sprintf(sitefile -> filename,"%s/%s/%s-%s_%d%s%s", get_master_db_dir(), DEFAULT_TMP_DIR, header_rec.primary_hostname, header_rec.access_methods, (num_suff = rand() % 100), SUFFIX_PARTIAL_EXCERPT,TMP_SUFFIX);

      if(access(sitefile -> filename, R_OK | F_OK) == -1)
      finished = 1;
    }
  }
  else {
      sprintf(sitefile -> filename,"%s/%s/%s-%s_%d%s%s", get_master_db_dir(), DEFAULT_TMP_DIR, header_rec.primary_hostname, header_rec.access_methods, num_suff, SUFFIX_PARTIAL_EXCERPT,TMP_SUFFIX);
      num_suff = suff_num;
  }


  
  if(open_file(sitefile, O_WRONLY) == ERROR){

    /* "Error opening local site parser file %s" */

    error(A_ERR, "get_webindex_excerpt", GET_WEBINDEX_SITE_002, sitefile -> filename);
    return(ERROR);
  }

  origformat = header_rec.format;

  header_rec.format = FRAW;
  HDR_SET_FORMAT(header_rec.header_flags);

  if(write_header(sitefile->fp_or_dbm.fp, &header_rec, (u32 *) NULL, 0,0) == ERROR){

    /* "Error while writing header of output file" */

    error(A_ERR, "get_webindex_excerpt", SEND_WEBINDEX_SITE_011);
    return(ERROR);
  }

  if ( header_rec.no_recs ==0 ) {
    return(A_OK);
  }
  
#if 0
  if(pipe(p) == -1){

    /* "Can't open pipe to uncompression program" */

    error(A_ERR,"get_webindex_site", GET_WEBINDEX_SITE_008);
    return(ERROR);
  }
#endif
  start_time = time((time_t *) NULL);
  
  if((retcode = fork()) == 0){

    if(dup2( fileno(sitefile->fp_or_dbm.fp),1) == -1){

      /* "Can't dup2 pipe to stdout" */

      error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_009);
      exit(ERROR);
    }

    if(origformat == FXDR_COMPRESS_LZ) {

      if(verbose){

        /* "Input for %s in compressed format" */

        error(A_INFO, "get_webindex_site", GET_WEBINDEX_SITE_016, header_rec.primary_hostname);
      }

      execl(UNCOMPRESS_PGM, UNCOMPRESS_PGM, (char *) 0);

      /* "Can't execute uncompression program %s" */

      error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_010, UNCOMPRESS_PGM);
      exit(ERROR);
    }

    if(verbose){
      /* "Input for %s not in compressed format" */
      error(A_INFO, "get_webindex_site", GET_WEBINDEX_SITE_017, header_rec.primary_hostname);
    }
    
    execl(CAT_PGM, CAT_PGM, (char *) 0);

    /* "Can't execute cat program %s" */
    
    error(A_ERR, "get_webindex_site", GET_WEBINDEX_SITE_019, CAT_PGM);
    exit(ERROR);
  }
  else{
    if(retcode == -1){

      /* "Can't fork() for uncompression process" */

      error(A_ERR, "get_webindex_file", GET_WEBINDEX_SITE_011);
      if(unlink(sitefile -> filename) == -1){

        /* "Can't unlink failed transfer file" */

        error(A_SYSERR, "get_webindex_excerpt", GET_WEBINDEX_SITE_007);
        return(ERROR);
      }
      return(ERROR);
    }
  }


  if (wait(&status)  == -1 ) {
    error(A_ERR,"net_webindex", "Error waiting");
    exit(0);
  }

  

  sprintf(tmp_filename,"%s/%s/%s-%s_%d%s", get_master_db_dir(), DEFAULT_TMP_DIR, header_rec.primary_hostname, header_rec.access_methods, num_suff, SUFFIX_PARTIAL_EXCERPT);

  
  if(verbose){
    double bps;

    if(fstat(fileno(sitefile -> fp_or_dbm.fp), &statbuf) == -1){

      /* "Can't fstat() output file %s" */

      error(A_SYSERR, "get_webindex_excerpt", GET_WEBINDEX_SITE_014, sitefile -> filename);
    }

    /* "Tranferred %d bytes in %d seconds (%6.2f bytes/sec)" */

    bps = (double) statbuf.st_size / ((end_time = time((time_t *) NULL)) - start_time);

    error(A_INFO, "get_webindex_excerpt", GET_WEBINDEX_SITE_015, statbuf.st_size, end_time - start_time, bps);
  }

   

  close_file(sitefile);

  if(rename(sitefile -> filename, tmp_filename) == -1){

    /* "Can't rename temporary file %s to %s" */

    error(A_SYSERR,"get_webindex_excerpt", GET_WEBINDEX_SITE_006, sitefile -> filename, tmp_filename);
    return(ERROR);
  }

  destroy_finfo(sitefile);

  return(A_OK);
}



/*
 * Sendrevi_webindex: Open the webindex site file specified by siteaddr and
 * convert it to an XDR stream on stdout
 */


status_t send_webindex_site(siteaddr, header_rec, hostaux_ent, outhdr, compress_pgm, strhan, date)
   ip_addr_t siteaddr;	      /* address of site to be opened */
   header_t *header_rec;      /* header record */
   hostdb_aux_t *hostaux_ent;
   format_t outhdr;	      /* outgoing format */
   char *compress_pgm;	      /* Compression program */
   struct arch_stridx_handle *strhan;
   date_time_t date;
{
  XDR *xdrs;
  FILE *fp;
  int retcode;
  int p[2];
  int no_recs,recs_to_send;
  int port;
  

  if( get_port( header_rec->access_command, WEBINDEX_DB_NAME, &port ) == ERROR ){
    error(A_ERR,"insert_webindex","Could not find the port for this site", header_rec->primary_hostname);
    exit(A_OK);
  }

  sitefile = create_finfo();

  /* Open apropriate site file */

  strcpy(sitefile -> filename, (char *)wfiles_db_filename(inet_ntoa(ipaddr_to_inet(siteaddr)),port));

  if(open_file(sitefile, O_RDONLY) == ERROR){

    /* "Can't open webindex database sitefile %s" */

    error(A_ERR,"send_webindex_site", SEND_WEBINDEX_SITE_001, sitefile -> filename);
    header_rec -> update_status = FAIL;
  }
  else{
    if(mmap_file(sitefile, O_RDONLY) == ERROR){

      /* "Can't mmap webindex database sitefile %s" */

      error(A_ERR,"send_webindex_site", SEND_WEBINDEX_SITE_002, sitefile -> filename);
      header_rec -> update_status = FAIL;
    }
  }
  no_recs = header_rec -> no_recs;
  recs_to_send = no_recs_to_send( sitefile,date, header_rec->no_recs); 
  header_rec -> no_recs = recs_to_send;

  
  if(write_header(stdout, header_rec, (u32 *) NULL, 0,0) == ERROR){

    /* "Error while writing header of output file" */

    error(A_ERR, "send_webindex_site", SEND_WEBINDEX_SITE_011);
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

      error(A_INFO, "send_webindex_site", SEND_WEBINDEX_SITE_012, header_rec -> primary_hostname);
    }

    if(pipe(p) == -1){

      /* "Can't open pipe to compression program" */

      error(A_ERR,"send_webindex_site", SEND_WEBINDEX_SITE_005);
      return(ERROR);
    }

    if((retcode = fork()) == 0){

      if(dup2(p[0], fileno(stdin)) == -1){

        /* "Can't dup2 pipe to stdin" */

        error(A_ERR, "send_webindex_site", SEND_WEBINDEX_SITE_006);
        exit(ERROR);
      }

      close(p[1]);

      execl(compress_pgm, compress_pgm, (char *) 0);

      /* "Can't execute compression program %s" */

      error(A_ERR, "send_webindex_site", SEND_WEBINDEX_SITE_007, compress_pgm);
      exit(ERROR);
    }
    else{
      if(retcode == -1){

        /* "Can't fork() for compression process" */

        error(A_ERR, "send_webindex_file", SEND_WEBINDEX_SITE_010);
        return(ERROR);
      }
    }

    close(p[0]);


    if((fp = fdopen(p[1], "w")) == (FILE *) NULL){

      /* "Can't open outgoing pipe" */

      error(A_ERR, "send_webindex_site", SEND_WEBINDEX_SITE_008);
      return(ERROR);
    }

    if((xdrs = open_xdr_stream(fp, XDR_ENCODE)) == (XDR *) NULL){

      /* "Can't open XDR stream for compression ftp" */

      error(A_ERR, "send_webindex_site", SEND_WEBINDEX_SITE_009);
      return(ERROR);
    }

  }
  else{

    /* found host db entry now open XDR stream */

    if((xdrs = open_xdr_stream(stdout, XDR_ENCODE)) == (XDR *) NULL){

      /* "Can't open XDR stream for stdout" */

      error(A_ERR, "send_webindex_site", SEND_WEBINDEX_SITE_003);
      return(ERROR);
    }
  }
  /* Convert site file into parser output format, then to XDR stream */
  
  if(copy_parser_to_xdr( xdrs, sitefile, no_recs, date, strhan) == ERROR){

    /* "Error translating site file to XDR stream" */

    error(A_ERR, "send_webindex_site", SEND_WEBINDEX_SITE_004);
    return(ERROR);
  }
   
  close_xdr_stream(xdrs);
  close_file(sitefile);

  destroy_finfo(sitefile);
   
  return(A_OK);
}

status_t send_webindex_excerpt(siteaddr, header_rec, hostaux_ent, outhdr, compress_pgm, strhan, date)
   ip_addr_t siteaddr;	      /* address of site to be opened */
   header_t *header_rec;      /* header record */
   hostdb_aux_t *hostaux_ent;
   format_t outhdr;	      /* outgoing format */
   char *compress_pgm;	      /* Compression program */
   struct arch_stridx_handle *strhan;
  date_time_t date;
{
  XDR *xdrs;
  FILE *fp;
  int retcode;
  int p[2];
  int no_recs,recs_to_send;
  int port;
  int status;
  file_info_t *excerptfile, *sitefile;
  no_recs = header_rec -> no_recs;

  if( get_port( header_rec->access_command, WEBINDEX_DB_NAME, &port ) == ERROR ){
    error(A_ERR,"insert_webindex","Could not find the port for this site", header_rec->primary_hostname);
    exit(A_OK);
  }

  excerptfile = create_finfo();
  sitefile = create_finfo();

  /* Open apropriate site file */

  sprintf(excerptfile -> filename, "%s%s",(char *)wfiles_db_filename(inet_ntoa(ipaddr_to_inet(siteaddr)),port),SUFFIX_EXCERPT);
  sprintf(sitefile -> filename, "%s",(char *)wfiles_db_filename(inet_ntoa(ipaddr_to_inet(siteaddr)),port));

  if(open_file(excerptfile, O_RDONLY) == ERROR){

    /* "Can't open webindex database excerptfile %s" */

    error(A_ERR,"send_webindex_excerpt", SEND_WEBINDEX_SITE_001, excerptfile -> filename);
    header_rec -> update_status = FAIL;
  }
  else{
    if(mmap_file(excerptfile, O_RDONLY) == ERROR){

      /* "Can't mmap webindex database excerptfile %s" */

      error(A_ERR,"send_webindex_excerpt", SEND_WEBINDEX_SITE_002, excerptfile -> filename);
      header_rec -> update_status = FAIL;
    }
  }


  if(open_file(sitefile, O_RDONLY) == ERROR){

    /* "Can't open webindex database sitefile %s" */

    error(A_ERR,"send_webindex_excerpt", SEND_WEBINDEX_SITE_001, sitefile -> filename);
    header_rec -> update_status = FAIL;
  }
  else{
    if(mmap_file(sitefile, O_RDONLY) == ERROR){

      /* "Can't mmap webindex database sitefile %s" */

      error(A_ERR,"send_webindex_excerpt", SEND_WEBINDEX_SITE_002, sitefile -> filename);
      header_rec -> update_status = FAIL;
    }
  }
  
  no_recs = header_rec -> no_recs;
  recs_to_send = no_excerpts_to_send( sitefile,date, header_rec->no_recs); 
  header_rec -> no_recs = recs_to_send;

  if(write_header(stdout, header_rec, (u32 *) NULL, 0,0) == ERROR){

    /* "Error while writing header of output file" */

    error(A_ERR, "send_webindex_excerpt", SEND_WEBINDEX_SITE_011);
    return(ERROR);
  }

  if ( recs_to_send == 0 ) {
    return(A_OK);
  }
  
  if((hostaux_ent -> current_status == DEL_BY_ARCHIE) ||
     (hostaux_ent -> current_status == DEL_BY_ADMIN) ||
     (hostaux_ent -> current_status == DELETED) ||
     (header_rec -> update_status == FAIL)){

    return(A_OK);
  }
  
#if 0
  if(pipe(p) == -1){

    /* "Can't open pipe to compression program" */

    error(A_ERR,"send_webindex_excerpt", SEND_WEBINDEX_SITE_005);
    return(ERROR);
  }

  if(dup2(p[0], fileno(excerptfile->fp_or_dbm.fp)) == -1){

    /* "Can't dup2 pipe to stdin" */

    error(A_ERR, "send_webindex_excerpt", SEND_WEBINDEX_SITE_006);
    exit(ERROR);
  }
#endif


  if ( outhdr == FXDR ) {
    
    if(verbose){
      /* "Output for %s not in compressed format" */
      error(A_INFO, "get_webindex_site", GET_WEBINDEX_SITE_017, header_rec->primary_hostname);
    }
    output_excerpt(sitefile, excerptfile, stdout, date, no_recs);

  }
  else {

    if(pipe(p) == -1){

      /* "Can't open pipe to compression program" */

      error(A_ERR,"send_webindex_site", SEND_WEBINDEX_SITE_005);
      return(ERROR);
    }

    if((retcode = fork()) == 0){

      if(dup2(p[0], fileno(stdin)) == -1){

        /* "Can't dup2 pipe to stdin" */

        error(A_ERR, "send_webindex_site", SEND_WEBINDEX_SITE_006);
        exit(ERROR);
      }

      close(p[1]);

      if(verbose){
          
        /* "Output for %s in compressed format" */

        error(A_INFO, "send_webindex_excerpt", SEND_WEBINDEX_SITE_012, header_rec -> primary_hostname);
      }

      execl(compress_pgm, compress_pgm, (char *) 0);

      /* "Can't execute compression program %s" */

      error(A_ERR, "send_webindex_excerpt", SEND_WEBINDEX_SITE_007, compress_pgm);
      exit(ERROR);
    }
    else{
      if(retcode == -1){

        /* "Can't fork() for compression process" */

        error(A_ERR, "send_webindex_excerpt", SEND_WEBINDEX_SITE_010);
        return(ERROR);
      }
    }

    close(p[0]);
  

#if 0
    if(dup2(fileno(stdout),p[1]) == - 1 )  {

      /* "Can't open outgoing pipe" */

      error(A_ERR, "send_webindex_excerpt", SEND_WEBINDEX_SITE_008);
      return(ERROR);
    }
#endif
    output_excerpt(sitefile, excerptfile, fdopen(p[1],"w"), date, no_recs);
  }




  close_file(sitefile);
  close_file(excerptfile);
  
  destroy_finfo(sitefile);
  destroy_finfo(excerptfile);
   
  return(A_OK);
}


/*
 * copy_parser_to_xdr: take given sitefile, convert it to parser format and
 * then convert to XDR stream
 */


  
status_t copy_parser_to_xdr( xdrs, sitefile, no_recs, date, strhan)
   XDR *xdrs;		   /* active XDR stream */
   file_info_t *sitefile;  /* file control for site file */
   index_t no_recs;	   /* number of records in site file */
   date_time_t date;
   struct arch_stridx_handle *strhan;
{
  parser_entry_t *parser_arr;
  static full_site_entry_t *site_ent, *next_ent;
  char *tmp_string;
  int curr_recno;
  char **strings_arr;
  index_t  curr_parent = (index_t) (-1);
  int  *rec_numbers ;
  int flag = 1;
  int excerpt_counter;
  
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
  /*
     if(mmap_file(strings, O_RDONLY) == ERROR){
     
     /  "Can't mmap strings file" /
     
     error(A_ERR, "copy_parser_to_xdr", COPY_PARSER_TO_XDR_001);
     return(ERROR);
     }
     */
  if (( parser_arr = (parser_entry_t *)malloc(sizeof(parser_entry_t)*(no_recs)))== (parser_entry_t *) NULL){

    /* "Can't malloc space for parser_entry_t" */

    error(A_SYSERR, "copy_parser_to_xdr", "Can't malloc space for parser_entry_t array");
    return(ERROR);
  }

  num = no_recs;

  if (( strings_arr = (char **)malloc(sizeof(char *)*(no_recs))) == (char **) NULL ){

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

  /* This is different from anonftp */
  i = 0;

  /* num will be increasing as potential parent_entries are
   * foreseen. Once I see a record that is a directory and not a file
   * num is increased.
   */

  excerpt_counter = 0;

  for(curr_recno = 0; curr_recno < num; curr_recno++){

    /* Get the full site entry record */

    site_ent = (full_site_entry_t *) sitefile -> ptr + curr_recno;
      
    /**sizeof(full_site_entry_t);*/

    /* Get the parser record */

    if(  !(CSE_IS_SUBPATH((*site_ent)) || 
           CSE_IS_NAME((*site_ent)) ||
           CSE_IS_PORT((*site_ent)) ) ) {

      rec_numbers[ curr_recno ] = i;

      /* rec_numbers takes care of the mapping from the indecies
       * of the site file (reflecting the bigger array) to the smaller
       * current array idecies to be sent.
       */

      if( CSE_IS_KEY((*site_ent)) ){
        if ( flag == 0) {
          rec_numbers[curr_recno] = -1;
          continue;
        }
        CSE_STORE_WEIGHT( parser_arr[i].core , site_ent->core.kwrd.weight );
      }else{
        parser_arr[i].core.size = site_ent -> core.entry.size;
        parser_arr[i].core.date = site_ent -> core.entry.date;
        parser_arr[i].core.rdate = site_ent -> core.entry.rdate;
        if ( CSE_IS_DOC((*site_ent)) ) {
          if ( date < site_ent->core.entry.rdate ) {
            flag = 1;
            parser_arr[i].core.perms = excerpt_counter++;
          }
          else {
            flag = 0;
            parser_arr[i].core.perms = 0;
          }
        }
        else {
          flag = 1;
        }
/*              
        parser_arr[i].core.perms = site_ent -> core.entry.perms;
*/
      }
      
      parser_arr[i].core.parent_idx = curr_parent;
      parser_arr[i].core.child_idx = (index_t)(-1);
      parser_arr[i].core.flags = site_ent -> flags;
      /* Get the string associated with parser record */
    
      if( !archGetString( strhan,  site_ent -> strt_1, &tmp_string) ){
        error(A_ERR,"copy_parser_to_xdr","Could not find the string using archGetString\n");
        return( ERROR );
      }

      parser_arr[i].slen = strlen(tmp_string);

      if (( strings_arr[i] = (char *)malloc(sizeof(char)*(parser_arr[i].slen)+1)) == (char *)NULL) {

        /* "Can't malloc space for char array" */

        error(A_SYSERR, "net_webindex", "Can't malloc space for char array");
        return(ERROR);
      }

      strcpy(strings_arr[i], tmp_string);

      i++;

      /* i only increases when an element is to be sent is being
       * prepeared. It is used for the mapping of indecies. 
       */

    }else
    {
      flag = 1;
	    /*  num++;   */

      /* - must increase it to take care of the extra 
       * records we add at insertion time for each parent entry..
       */

      rec_numbers[ curr_recno ] = i;
      parser_arr[i].slen = 0;
      
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
        parser_arr[i].core.size = 0;
        parser_arr[i].core.date = 0;
        parser_arr[i].core.rdate = 0;
        parser_arr[i].core.perms = 0;
        parser_arr[i].core.parent_idx = curr_parent;
        parser_arr[i].core.child_idx = (index_t)(-1);
        parser_arr[i].core.flags = site_ent -> flags;

        tmp_string = strings_arr[rec_numbers[site_ent -> core.prnt_entry.strt_2]];
        parser_arr[i].slen = strlen(tmp_string);
        if (( strings_arr[i] = (char *)malloc(sizeof(char)*(parser_arr[i].slen+1))) == (char *)NULL) {

          /* "Can't malloc space for char array" */

          error(A_SYSERR, "net_webindex", "Can't malloc space for char array");
          return(ERROR);
        }

        strcpy(strings_arr[i], tmp_string);

        i++;
        
        curr_parent = rec_numbers[site_ent -> core.prnt_entry.strt_2] ;

        if( curr_recno+1 < num ) {

          /* This should take care of the case when there are no children for
           * this parent directory. If the next record is another parent directory
           * then there are no children for the current parent_entry.
           */
          next_ent = (full_site_entry_t *) sitefile -> ptr + curr_recno+1;
          if( !(CSE_IS_SUBPATH((*next_ent)) || 
                CSE_IS_NAME((*next_ent)) ||
                CSE_IS_PORT((*next_ent)) ) )
          {	parser_arr[ curr_parent ].core.child_idx = i;
            /* the first entry in this directory listing, i already is increased */
          }
        }
      }else 
      {
        num++;
        curr_parent = (index_t) (0);
        /*  This case only occurs in the top directory ./ */
      }
    }
  }

/*  for(curr_recno = 0; curr_recno < no_recs; curr_recno++){ */
  for(curr_recno = 0; curr_recno < i; curr_recno++){ 

    /*
       fprintf(stderr,"%d: %s\t| %d",curr_recno,strings_arr[curr_recno],parser_arr[curr_recno].core.flags); 
       error(A_INFO,"c_p_t_xdr","%d: %s\t| %d",curr_recno,strings_arr[curr_recno],parser_arr[curr_recno].core.flags);
       */
    
    if( xdr_parser_entry_t(xdrs,&parser_arr[curr_recno]) != TRUE){
    
      /* "Conversion to XDR stream from raw format failed" */
    
      error(A_INTERR,"copy_parser_to_xdr", COPY_PARSER_TO_XDR_002);
      return(ERROR);
    }
    
    /*
     * This might have to be changed to xdr_array but since xdr_char
     * unpacks characters into 4 bytes each the space requirements would
     * be too expensive. We'll try with this for now
     */
  
    if( xdr_opaque( xdrs, strings_arr[curr_recno], parser_arr[curr_recno].slen) != TRUE){

	    /* "Conversion of string to XDR opaque type from raw format failed" */

      error(A_INTERR,"copy_parser_to_xdr", COPY_PARSER_TO_XDR_003);
      return(ERROR);
    }
       
  }
  free_net_strings(&strings_arr);
  if( parser_arr != NULL ) { free(parser_arr); parser_arr = NULL; }
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
  static char string_buf[8096]; /* lucb - was pathname_t [256] */
  int s_length;

  ptr_check(xdrs, XDR, "copy_xdr_to_parser", ERROR);
  ptr_check(sitefile, file_info_t, "copy_xdr_to_parser", ERROR);     

  /* for each record in the incoming stream */

  alarm(900);
  for(curr_recno = 0; curr_recno < no_recs; curr_recno++){

    /* convert from XDR to parser format */

    rec_number = curr_recno;

    if(xdr_parser_entry_t(xdrs,&parser_rec) != TRUE){

      /* "Conversion to raw format from XDR format failed" */
      /* debug("copy_xdr_to_parser", "Failed on record %d", curr_recno); */
      error(A_WARN,"copy_xdr_to_parser", COPY_XDR_TO_PARSER_001);
      return(ERROR);
    }

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

  int r1,r2,r3,r4,r5,r6,r7;

  ptr_check(xdrs, XDR, "xdr_core_site_entry_t", FALSE);
  ptr_check(core_rec, core_site_entry_t, "xdr_core_site_entry_t", FALSE);   

  /* debugging */

  r1 = xdr_file_size_t(xdrs, &(core_rec -> size));
  r2 = xdr_date_time_t(xdrs, &(core_rec -> date));
  r7 = xdr_date_time_t(xdrs, &(core_rec -> rdate));
  r3 = xdr_index_t(xdrs, &(core_rec -> parent_idx));
  r4 = xdr_index_t(xdrs, &(core_rec -> child_idx));
  r5 = xdr_perms_t(xdrs, &(core_rec -> perms));
  r6 = xdr_flags_t(xdrs, &(core_rec -> flags));

  return (r1 && r2 && r3 && r4 && r5 && r6 && r7);
   
#else

  ptr_check(xdrs, XDR, "xdr_core_site_entry_t", FALSE);
  ptr_check(core_rec, core_site_entry_t, "xdr_core_site_entry_t", FALSE);   


  return( xdr_file_size_t(xdrs, &(core_rec -> size)) &&
         xdr_date_time_t(xdrs, &(core_rec -> date)) &&
         xdr_date_time_t(xdrs, &(core_rec -> rdate)) &&         
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

      error(A_ERR, "sig_handle", NET_WEBINDEX_014);
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

void free_net_strings( strs )
  char ***strs;
{
  int i;
   
  if ( *strs == NULL )
  return;
   
  for (i=0; (*strs)[i] != NULL; i++ ) {
    free((*strs)[i]);
  }
 
  free(*strs);
  *strs = NULL;
}




int no_recs_to_send( sitefile, date, no_recs )
  file_info_t *sitefile;
  date_time_t date;
  int no_recs;
{

  int curr_recno;
  int counter = 0;
  int flag = 0;
  full_site_entry_t *site_ent;

  /* It starts at 1 .. cause we do not want to count the
     first empty rec...
   */
  for(curr_recno = 1; curr_recno <= no_recs; curr_recno++){

    /* Get the full site entry record */

    site_ent = (full_site_entry_t *) sitefile -> ptr + curr_recno;
      
    /* Get the parser record */

    if ( CSE_IS_DOC((*site_ent)) ) {
      if ( site_ent->core.entry.rdate <= date ) { /* Do not send keys */
        flag = 1;
      }
      else {
        flag = 0;
      }
      counter++;
    }
    else {
      if ( CSE_IS_KEY((*site_ent)) ) {
        if ( flag == 0 ) {
          counter++;
        }
      }
      else {
        counter++;
        flag = 0;
      }
    }
  }
  return counter;
}


int no_excerpts_to_send( sitefile, date, no_recs )
  file_info_t *sitefile;
  date_time_t date;
  int no_recs;
{

  int curr_recno;
  int counter = 0;
  int flag = 0;
  full_site_entry_t *site_ent;
  
  for(curr_recno = 0; curr_recno < no_recs; curr_recno++){

    /* Get the full site entry record */

    site_ent = (full_site_entry_t *) sitefile -> ptr + curr_recno;
      
    /* Get the parser record */

    if ( CSE_IS_DOC((*site_ent)) ) {
      if ( site_ent->core.entry.rdate >= date ) {
        counter++;
      }
    }
  }
  return counter;
}





void output_excerpt(sitefile, excerptfile, outfile, date, no_recs)
  file_info_t *sitefile;
  file_info_t *excerptfile;
  FILE *outfile;
  date_time_t date;
  int no_recs;
{


  int curr_recno;
  int flag = 0;
  full_site_entry_t *site_ent;
  excerpt_t *excerpt_ent;
  
  for(curr_recno = 0; curr_recno < no_recs; curr_recno++){

    /* Get the full site entry record */

    site_ent = (full_site_entry_t *) sitefile -> ptr + curr_recno;
      
    /* Get the parser record */

    if ( CSE_IS_DOC((*site_ent)) ) {
      if ( site_ent->core.entry.rdate > date ) {
        excerpt_ent = (excerpt_t*)excerptfile->ptr+site_ent->core.entry.perms;
        fwrite(excerpt_ent, sizeof(excerpt_t), 1, outfile);
      }
    }
  }

}



