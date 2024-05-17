/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 *
 * $Id: parse_anonftp.c,v 2.0 1996/08/20 22:42:27 archrlse Exp $
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#if !defined(AIX) && !defined(SOLARIS)
#include <vfork.h>
#endif
#include <sys/wait.h>
#include <time.h>
#include "typedef.h"
#include "header.h"
#include "db_files.h"
#include "error.h"
#include "files.h"
#include "lang_anonftp.h"
#include "master.h"
#include "protos.h"

/*
 * parse_anonftp: handles the parsing of an anonftp listing, spawning the
 * appropriate parser

   argv, argc are used.


   Parameters:	  
		  -i <input filename> Mandatory
		  -o <output filename> Mandatory
		  -t <tmp directory>
		  -f <filter program>
		  -M <master database pathname>
		  -h <host database pathname>
		  -l write to log file (default)
		  -L <log file>
*/

int verbose = 0;
char *prog;

int main(argc, argv)
   int argc;
   char *argv[];

{
#if 0
#ifdef __STDC__

   extern int getopt(int, char **, char *);
   extern time_t time(time_t *);

#else

   extern int getopt();
   extern time_t time();

#endif
#endif
   
   extern int opterr;
   extern char *optarg;

   char **cmdline_ptr;
   int cmdline_args;

   int option;

   /* Directory pathnames */

   pathname_t master_database_dir;
   pathname_t host_database_dir;
   pathname_t tmp_dir;
   
   /* log filename */
   
   pathname_t logfile;


   /* Filter and parser programs */

   pathname_t filter_pgm;
   pathname_t process_pgm;

   pathname_t outname;
   pathname_t outname2;

   char **arglist;


   int logging = 0;

   int finished;

   int erron = 0;

   file_info_t *input_file  = create_finfo();
   file_info_t *output_file = create_finfo();
   file_info_t *tmp_file = create_finfo();

   char *str;
   header_t header_rec;

   int status, result;

   char retr_list[MAX_ACCESS_COMM];
   char retr_string[MAX_ACCESS_COMM];
   char user[MAX_ACCESS_METHOD];
   char pass[MAX_ACCESS_METHOD];
   char acct[MAX_ACCESS_METHOD];
   char root_dir[MAX_ACCESS_METHOD];

   opterr = 0;
   tmp_dir[0] = '\0';
   
   host_database_dir[0] = master_database_dir[0] = filter_pgm[0] = '\0';
   logfile[0] = '\0';

   /* ignore argv[0] */

   cmdline_ptr = argv + 1;
   cmdline_args = argc - 1;

   while((option = (int) getopt(argc, argv, "h:M:t:L:i:o:f:lv")) != EOF){

      switch(option){

	 /* Log filename */

	 case 'L':
            strcpy(logfile, optarg);
	    logging = 1;
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* master database directory */

	 case 'M':
	    strcpy(master_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* name of filter program */

	 case 'f':
            strcpy(filter_pgm, optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* host database directory */

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

	 /* log output, default file */

	 case 'l':
	    logging = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

	 /* output filename */

	 case 'o':
            strcpy(output_file -> filename, optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* tmp directory */

	 case 't':
	    strcpy(tmp_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* verbose */

	 case 'v':
	    verbose = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;
      }

   }


   if(logging){  
      if(logfile[0] == '\0'){
	 if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

	    /*  "Can't open default log file" */

	    error(A_ERR, "parse_anonftp", PARSE_ANONFTP_001);
	    exit(ERROR);
	 }
      }
      else{
	 if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

	    /* "Can't open log file %s" */

	    error(A_ERR, "parse_anonftp", PARSE_ANONFTP_002, logfile);
	    exit(ERROR);
	 }
      }
   }

   /* Check to see that input and output filename were given */

   if(input_file -> filename[0] == '\0'){

      /* "No input file given" */

      error(A_ERR,"parse_anonftp", PARSE_ANONFTP_003);
      exit(ERROR);
   }

   if(output_file -> filename[0] == '\0'){

      error(A_ERR,"parse_anonftp", PARSE_ANONFTP_004);
      exit(ERROR);
   }


   if(set_master_db_dir(master_database_dir) == (char *) NULL){

      /* "Error while trying to set master database directory"*/

      error(A_ERR, "parse_anonftp", PARSE_ANONFTP_005);
      exit(ERROR);
   }


   if(tmp_dir[0] == '\0')
      sprintf(tmp_dir,"%s/%s",get_master_db_dir(), DEFAULT_TMP_DIR);


   if(open_file(input_file, O_RDONLY) == ERROR){

      /* "Can't open input file %s" */

      error(A_ERR, "parse_anonftp", PARSE_ANONFTP_006, input_file -> filename);
      exit(ERROR);
   }

   if(read_header(input_file -> fp_or_dbm.fp, &header_rec, (u32 *) NULL, 0, 1) == ERROR){

      /* "Error reading header of input file %s" */

      error(A_ERR, "parse_anonftp", PARSE_ANONFTP_007, input_file -> filename);
      goto onerr;
   }

   if(header_rec.format != FRAW){

      /* "Input file %s not in raw format. Cannot process"  */

      error(A_ERR,"parse_anonftp", PARSE_ANONFTP_008, input_file -> filename);
      goto onerr;
   }

   /* Generate a "random" file name */

   srand(time((time_t *) NULL));
   
   for(finished = 0; !finished;){

      sprintf(outname,"%s_%d%s%s", output_file -> filename, rand() % 100, SUFFIX_UPDATE, TMP_SUFFIX);

      if(access(outname, R_OK | F_OK) == -1)
         finished = 1;
   }

   str = get_tmp_filename(tmp_dir);

   if(str) 
      strcpy(tmp_file -> filename, str);
   else{

      /* "Can't get temporary name for file %s" */
      
      error(A_INTERR,"parse_anonftp", PARSE_ANONFTP_009, input_file -> filename);
      goto onerr;
   }

   if(open_file(tmp_file, O_WRONLY) == ERROR){

      /* "Can't open temporary file %s" */

      error(A_ERR, "parse_anonftp", PARSE_ANONFTP_010, tmp_file -> filename);
      goto onerr;
   }

   if ( header_rec.os_type == NOVELL ) {
       retr_list[0] = retr_string[0] = user[0] = pass[0] = acct[0] = root_dir[0] = '\0';
 

       /* Check what method is to be used to retrieve info */

       if(HDR_GET_ACCESS_COMMAND(header_rec.header_flags)){

	  strcpy(retr_string, header_rec.access_command);

	  if(str_decompose(retr_string,NET_DELIM_CHAR,retr_list,user,pass,acct,root_dir) == ERROR){

	     /* "Unable to parse access command in header" */

	     do_error_header(tmp_file, output_file,0, &header_rec, SUFFIX_PARSE, PARSE_ANONFTP_029);
	     return(ERROR);
	  }
	  sprintf(retr_string,"%s:%s:%s:%s:%s",retr_list,user,pass,acct,root_dir);
	 
       }
   }

   if(write_header(tmp_file -> fp_or_dbm.fp, &header_rec, (u32 *) NULL, 0, 0) == ERROR){

      /* "Error while writing header of output file" */

      error(A_ERR, "parse_anonftp", PARSE_ANONFTP_011);
      goto onerr;
   }

   switch(header_rec.os_type){

      case UNIX_BSD:
	 if(filter_pgm[0] == '\0')
	    sprintf(filter_pgm,"%s/%s/%s%s_%s", get_archie_home(), DEFAULT_BIN_DIR, FILTER_PREFIX, header_rec.access_methods, OS_TYPE_UNIX_BSD);
	 break;

      case VMS_STD:
	 if(filter_pgm[0] == '\0')
	    sprintf(filter_pgm,"%s/%s/%s%s_%s", get_archie_home(), DEFAULT_BIN_DIR, FILTER_PREFIX, header_rec.access_methods, OS_TYPE_VMS_STD);
	 break;

      case NOVELL:

	 if(filter_pgm[0] == '\0')
	    sprintf(filter_pgm,"%s/%s/%s%s_%s", get_archie_home(), DEFAULT_BIN_DIR, FILTER_PREFIX, header_rec.access_methods, OS_TYPE_NOVELL);
	 break;
   }


   /* first fork the filter program */

#ifndef AIX
   if((result = vfork()) == 0){
#else
   if((result = fork()) == 0){
#endif

      /* Make the input file the stdin of filter and tmp_file the stdout */

      dup2(fileno(input_file -> fp_or_dbm.fp), 0);
      dup2(fileno(tmp_file -> fp_or_dbm.fp), 1);

      execlp(filter_pgm, filter_pgm, (char *) NULL);

      /* if we've gotten here then the execlp didn't work */

      /* "Can't execlp() filter program %s" */

      error(A_SYSERR, "parse_anonftp", PARSE_ANONFTP_012, filter_pgm);
   }

   if(result == -1){

      /* "Can't vfork() filter program %s" */

      error(A_SYSERR,"parse_anonftp", PARSE_ANONFTP_013, filter_pgm);
      goto onerr;
   }

   if(wait(&status) == -1){

      /* "Error while in wait() for filter program %s" */

      error(A_SYSERR,"parse_anonftp", PARSE_ANONFTP_014, filter_pgm);
      goto onerr;
   }


   if(WIFEXITED(status) && WEXITSTATUS(status)){

      /* "Filter program %s exited abnormally with exit code %d" */

      error(A_ERR,"parse_anonftp", PARSE_ANONFTP_015, filter_pgm, WEXITSTATUS(status));
      goto onerr;
   }

   if(WIFSIGNALED(status)){

      /* "Filter program %s terminated abnormally with signal %d" */

      error(A_ERR,"parse_anonftp", PARSE_ANONFTP_016, filter_pgm, WTERMSIG(status));
      goto onerr;
   }


   if(close_file(input_file) == ERROR){

      /* "Can't close input file %s" */

      error(A_ERR, "parse_anonftp", PARSE_ANONFTP_017, input_file -> filename);
   }

   if(close_file(tmp_file) == ERROR){

      /* "Can't close temporary file %s" */

      error(A_ERR, "parse_anonftp", PARSE_ANONFTP_018, tmp_file -> filename);
   }

   if((arglist = (char **) malloc(MAX_NO_PARAMS * sizeof(char *))) == (char **) NULL){

      /* "Can't malloc space for argument list" */

      error(A_SYSERR, "parse_anonftp", PARSE_ANONFTP_027);
      exit(ERROR);
   }

   switch(header_rec.os_type){

      case UNIX_BSD:

	 sprintf(process_pgm,"%s/%s/%s%s_%s", get_archie_home(), DEFAULT_BIN_DIR, PARSE_PREFIX, header_rec.access_methods, OS_TYPE_UNIX_BSD);
	 break;

      case VMS_STD:
	 sprintf(process_pgm,"%s/%s/%s%s_%s", get_archie_home(), DEFAULT_BIN_DIR, PARSE_PREFIX, header_rec.access_methods, OS_TYPE_VMS_STD);
	 break;

      case NOVELL:
	 sprintf(process_pgm,"%s/%s/%s%s_%s", get_archie_home(), DEFAULT_BIN_DIR, PARSE_PREFIX, header_rec.access_methods, OS_TYPE_NOVELL);
	 break;
   }

   /* launch the parsing process */


   if((result = fork()) == 0){

      switch(header_rec.os_type){
	 char *logname;
	 int i;

	 case UNIX_BSD:

	    i = 0;
	    arglist[i++] = process_pgm;

	    arglist[i++] = "-r";
	    arglist[i++] = ".:";

	    arglist[i++] = "-p";
	    arglist[i++] = ".";

	    arglist[i++] = "-i";
    	    arglist[i++] = tmp_file -> filename;

	    arglist[i++] = "-o";
	    arglist[i++] = outname;

	    logname = get_archie_logname();

	    if(logname[0]){
	       
	       arglist[i++] = "-L";
	       arglist[i++] = logname;
	    }

	    if(verbose)
	       arglist[i++] = "-v";

	    arglist[i] = (char *) NULL;

	    if(verbose){
	       int j;
	       pathname_t holds;
	       pathname_t thold;

	       holds[0] = '\0';
	       
	       for(j = 0; j < i; j++){

		  sprintf(thold, " %s", arglist[j]);
		  strcat(holds, thold);
	       }
	       
	       error(A_INFO, "parse_anonftp", "Calling %s %s", process_pgm, holds);
	    }

	    execv(process_pgm, arglist);
	    break;

	 case VMS_STD:

	    i = 0;
	    arglist[i++] = process_pgm;

	    arglist[i++] = "-i";
    	    arglist[i++] = tmp_file -> filename;

	    arglist[i++] = "-o";
	    arglist[i++] = outname;

	    logname = get_archie_logname();

	    if(logname[0]){
	       
	       arglist[i++] = "-L";
	       arglist[i++] = logname;
	    }

	    if(verbose)
	       arglist[i++] = "-v";

	    arglist[i] = (char *) NULL;

	    if(verbose){
	       int j;
	       pathname_t holds;
	       pathname_t thold;

	       holds[0] = '\0';
	       
	       for(j = 0; j < i; j++){

		  sprintf(thold, " %s", arglist[j]);
		  strcat(holds, thold);
	       }
	       
	       error(A_INFO, "parse_anonftp", "Calling %s %s", process_pgm, holds);
	    }

	    execv(process_pgm, arglist);
	    break;

	 case NOVELL:

	    i = 0;
	    arglist[i++] = process_pgm;

	    arglist[i++] = "-i";
    	    arglist[i++] = tmp_file -> filename;

	    arglist[i++] = "-o";
	    arglist[i++] = outname;

/*	    arglist[i++] = "-r";
	    arglist[i++] = ".:";
*/

	    arglist[i++] = "-r";
	    arglist[i++] = root_dir;
	    strcat(arglist[i-1],":");
	    logname = get_archie_logname();

	    if(logname[0]){
	       
	       arglist[i++] = "-L";
	       arglist[i++] = logname;
	    }

	    if(verbose)
	       arglist[i++] = "-v";

	    arglist[i] = (char *) NULL;

	    if(verbose){
	       int j;
	       pathname_t holds;
	       pathname_t thold;

	       holds[0] = '\0';
	       
	       for(j = 0; j < i; j++){

		  sprintf(thold, " %s", arglist[j]);
		  strcat(holds, thold);
	       }
	       
	       error(A_INFO, "parse_anonftp", "Calling %s %s", process_pgm, holds);
	    }

	    execv(process_pgm, arglist);
	    break;

      }
   

      /* "Can't execv() parse program %s" */

      error(A_SYSERR, "parse_anonftp", PARSE_ANONFTP_019, process_pgm);
      goto onerr;
   }


   free(arglist);

   if(result == -1){

      /* "Can't vfork() parse program %s" */

      error(A_SYSERR,"parse_anonftp", PARSE_ANONFTP_020, process_pgm);
      goto onerr;
   }

   if(wait(&status) == -1){

      /* "Error while in wait() for parse program %s" */

      error(A_ERR,"parse_anonftp", PARSE_ANONFTP_021, process_pgm);
      goto onerr;
   }

   if(WIFEXITED(status) && WEXITSTATUS(status)){

      /* "Parse program %s exited abnormally with code %d" */

      error(A_ERR,"parse_anonftp", PARSE_ANONFTP_022, process_pgm, WEXITSTATUS(status));
      goto onerr;
   }

   if(WIFSIGNALED(status)){

      /* "Parse program %s terminated abnormally with signal %d" */

      error(A_ERR,"parse_anonftp", PARSE_ANONFTP_023, process_pgm, WTERMSIG(status));
      goto onerr;
   }

   goto jumpos;

onerr:

   erron = 1;

jumpos:


   for(finished = 0; !finished;){

      sprintf(outname2,"%s_%d%s", output_file -> filename, rand() % 100, SUFFIX_UPDATE);

      if(access(outname2, R_OK | F_OK) == -1)
         finished = 1;
   }

   if(rename(outname, outname2) == -1){

      /* "Can't rename output file %s" */

      error(A_SYSERR,"parse_anonftp", PARSE_ANONFTP_024, outname);
      erron = 1;
   }


   /* Don't unlink files if there is an error in parsing */

   if(!erron){
      if(unlink(input_file -> filename) == -1){

	 /* "Can't unlink input file %s" */

	 error(A_SYSERR,"parse_anonftp", PARSE_ANONFTP_025, input_file -> filename);
	 erron = 1;
      }

      if(unlink(tmp_file -> filename) == -1){

	 /* "Can't unlink temporary file %s" */

	 error(A_SYSERR,"parse_anonftp", PARSE_ANONFTP_026, tmp_file -> filename);
	 erron = 1;
      }
   }
   else{

      /* Rename the tmp file to something more useful */

      sprintf(outname2,"%s_%d%s", output_file -> filename, rand() % 100, SUFFIX_FILTERED);

      if(rename(tmp_file -> filename, outname2) == -1){

	 /* "Can't rename failed temporary file %s to %s" */

	 error(A_SYSERR,"parse_anonftp", PARSE_ANONFTP_028, tmp_file -> filename, outname2);
      }
   }

   if(erron)
      exit(ERROR);

   exit(A_OK);
   return(A_OK);
}
