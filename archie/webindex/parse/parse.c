#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "typedef.h"
#include "header.h"
#include "db_files.h"
#include "db_ops.h"
#include "error.h"
#include "files.h"
#include "master.h"
#include "protos.h"
#include "sub_header.h"
#include "lang_parsers.h"
#include "archstridx.h"

#include "parse.h"

char *prog ;
int verbose = 0 ;

int local_timezone = 0;

int compress_flag = 1;
pathname_t compress_pgm;
pathname_t uncompress_pgm;

date_time_t retrieve_time;

extern int opterr;
extern char *optarg;


int output_header PROTO((FILE *, int, header_t*,  int));


static pathname_t tmp_dir;

void get_tmpfiles(root,file,cfile,ext)
pathname_t root,file,cfile;
char *ext;
{
  int finished;
  
  /* Generate a "random" file name */

  srand(time((time_t *) NULL));
  for(finished = 0; !finished;){
    sprintf(file,"%s/%s.%s_%d%s", tmp_dir, root,"tempWeb", rand() % 100, "_t");
    if ( ext != NULL )
      sprintf(cfile,"%s%s", file,ext);
    else
      strcpy(cfile,file);
    if(access(file, R_OK | F_OK) == -1 && access(cfile, R_OK | F_OK) == -1) 
      finished = 1;
  }

}

int main(argc, argv)
   int argc;
   char *argv[];
{

  /* Directory pathnames */

  pathname_t master_database_dir;
  pathname_t host_database_dir;
  pathname_t start_database_dir;

   
  /* log filename */
   
  pathname_t logfile;

  char **cmdline_ptr;
  int cmdline_args;

  int option;

  /* Filter and parser programs */

  pathname_t outname;
  pathname_t excerptname;

  pathname_t outname2;
  pathname_t excerptname2;

  int port;
  int no_recs;
  
  int logging = 0;

  int ignore_header = 0;
  int finished;

  int erron = 0;

  file_info_t *input_file  = create_finfo();
  file_info_t *output_file = create_finfo();
  file_info_t *out_file = create_finfo();
  file_info_t *tmp_file = create_finfo();
  file_info_t *etmp_file = create_finfo();
  file_info_t *excerpt_file = create_finfo();
  
  char *str;
  header_t header_rec;

  opterr = 0;
  tmp_dir[0] = '\0';
   
  start_database_dir[0] = host_database_dir[0] = master_database_dir[0] = '\0';
  logfile[0] = '\0';

  /* ignore argv[0] */

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;
  
  prog = (char*)tail(argv[0]) ;
  while((option = (int) getopt(argc, argv, "h:M:t:L:i:o:lv")) != EOF){

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

#ifdef SLEEP
  fprintf(stderr,"Sleeping..\n");
  sleep(30);
#endif

  if(logging){  
    if(logfile[0] == '\0'){
      if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

        /*  "Can't open default log file" */

        error(A_ERR, "parse_webindex", PARSE_WEBINDEX_001);
        exit(ERROR);
      }
    }
    else{
      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

        /* "Can't open log file %s" */

        error(A_ERR, "parse_webindex", PARSE_WEBINDEX_002, logfile);
        exit(ERROR);
      }
    }
  }

  /* Check to see that input and output filename were given */

  if(input_file -> filename[0] == '\0'){

    /* "No input file given" */

    error(A_ERR,"parse_webindex", PARSE_WEBINDEX_003);
    exit(ERROR);
  }

  if(output_file -> filename[0] == '\0'){

    error(A_ERR,"parse_webindex", PARSE_WEBINDEX_004);
    exit(ERROR);
  }


  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory"*/

    error(A_ERR, "parse_webindex", PARSE_WEBINDEX_005);
    exit(ERROR);
  }

  if((set_start_db_dir(start_database_dir, DEFAULT_WFILES_DB_DIR)) == (char *) NULL){

    /* "Error while trying to set start database directory" */

    error(A_ERR, "parse_webindex", "Error while trying to set start database directory\n");
    exit(A_OK);
  }

  if(set_host_db_dir(host_database_dir) == (char *) NULL){

    /* "Error while trying to set host database directory" */

    error(A_ERR, "parse_webindex","Error while trying to set host database directory" );
    exit(A_OK);
  }

  if(tmp_dir[0] == '\0')
  sprintf(tmp_dir,"%s/%s",get_master_db_dir(), DEFAULT_TMP_DIR);


  if(open_file(input_file, O_RDONLY) == ERROR){

    /* "Can't open input file %s" */

    error(A_ERR, "parse_webindex", PARSE_WEBINDEX_006, input_file -> filename);
    exit(ERROR);
  }

  if(read_header(input_file -> fp_or_dbm.fp, &header_rec, (u32 *) NULL, 0, 1) == ERROR){

    /* "Error reading header of input file %s" */

    error(A_ERR, "parse_webindex", PARSE_WEBINDEX_007, input_file -> filename);
    goto onerr;
  }

  if(header_rec.format != FRAW){

    /* "Input file %s not in raw format. Cannot process"  */

    error(A_ERR,"parse_webindex", PARSE_WEBINDEX_008, input_file -> filename);
    goto onerr;
  }

  /* Generate a "random" file name */

  srand(time((time_t *) NULL));
   
  for(finished = 0; !finished;){
    int r;

    r = rand() % 100;
    sprintf(outname,"%s_%d%s%s", output_file -> filename, r, SUFFIX_UPDATE, TMP_SUFFIX);
    sprintf(excerptname,"%s_%d%s%s", output_file -> filename, r, SUFFIX_EXCERPT, TMP_SUFFIX);

    if(access(outname, R_OK | F_OK) == -1 &&
       access(excerptname, R_OK | F_OK ) == -1 )
    finished = 1;
  }

  
  str = get_tmp_filename(tmp_dir);

  if(str) {
    strcpy(tmp_file -> filename, str);
  }
  else{

    /* "Can't get temporary name for file %s" */
      
    error(A_INTERR,"parse_webindex", PARSE_WEBINDEX_009, input_file -> filename);
    goto onerr;
  }

  str = get_tmp_filename(tmp_dir);

  if(str)  {
    strcpy(etmp_file -> filename, str);
  }
  else{

    /* "Can't get temporary name for file %s" */
      
    error(A_INTERR,"parse_webindex", PARSE_WEBINDEX_009, input_file -> filename);
    goto onerr;
  }
  

  if(open_file(tmp_file, O_WRONLY) == ERROR){

    /* "Can't open temporary file %s" */

    error(A_ERR, "parse_webindex", PARSE_WEBINDEX_010, tmp_file -> filename);
    goto onerr;
  }


  if(open_file(etmp_file, O_WRONLY) == ERROR){

    /* "Can't open temporary file %s" */

    error(A_ERR, "parse_webindex", PARSE_WEBINDEX_010, excerpt_file -> filename);
    goto onerr;
  }

  strcpy(out_file->filename,outname);
  
  if(open_file(out_file, O_WRONLY) == ERROR){

    /* "Can't open temporary file %s" */

    error(A_ERR, "parse_webindex", PARSE_WEBINDEX_010, out_file -> filename);
    goto onerr;
  }

  output_header(etmp_file->fp_or_dbm.fp,ignore_header,&header_rec,0);
  
  get_port(header_rec.access_command, WEBINDEX_DB_NAME, &port);
  
  retrieve_time = header_rec.retrieve_time;
  
  if ( parse_file(input_file, tmp_file, etmp_file,
                  &no_recs,header_rec.primary_ipaddr,port ) == ERROR ) {
    
    HDR_SET_HCOMMENT(header_rec.header_flags);

    header_rec.format = FRAW;
    HDR_SET_FORMAT(header_rec.header_flags);
    if ( ! output_header(out_file->fp_or_dbm.fp,ignore_header,&header_rec,1)) {
      /* Error from output_header(): aborting" */

      error(A_ERR, "parse_webindex", PARSE_WEBINDEX_015);      
    }
  }
  else {

    header_rec.no_recs = no_recs;
    HDR_SET_NO_RECS(header_rec.header_flags);

    output_header(out_file->fp_or_dbm.fp, ignore_header, &header_rec, 0);

    close_file(tmp_file);
    if(open_file(tmp_file, O_RDONLY) == ERROR){

      /* "Can't open temporary file %s" */

      error(A_ERR, "parse_webindex", PARSE_WEBINDEX_010, tmp_file -> filename);
      goto onerr;
    }    

    if ( archie_fpcopy(tmp_file->fp_or_dbm.fp, out_file->fp_or_dbm.fp, -1) == ERROR ) {

    }

    
  }



  

   if(close_file(input_file) == ERROR){

      /* "Can't close input file %s" */

      error(A_ERR, "parse_webindex", PARSE_WEBINDEX_017, input_file -> filename);
   }

   if(close_file(tmp_file) == ERROR){

      /* "Can't close temporary file %s" */

      error(A_ERR, "parse_webindex", PARSE_WEBINDEX_018, tmp_file -> filename);
   }

   if(close_file(out_file) == ERROR){

      /* "Can't close temporary file %s" */

      error(A_ERR, "parse_webindex", PARSE_WEBINDEX_018, out_file -> filename);
   }  


  if(close_file(etmp_file) == ERROR){

      /* "Can't close temporary file %s" */

      error(A_ERR, "parse_webindex", PARSE_WEBINDEX_018, etmp_file -> filename);
   }

  goto jobok;
  
 onerr:
  erron = 1;
  
 jobok:
  
  srand(time((time_t *) NULL));
   
  for(finished = 0; !finished;){
    int r;

    r = rand() % 100;
    sprintf(outname2,"%s_%d%s", output_file -> filename, r, SUFFIX_UPDATE);
    sprintf(excerptname2,"%s_%d%s", output_file -> filename, r,SUFFIX_EXCERPT);

    if(access(outname2, R_OK | F_OK) == -1 &&
       access(excerptname2, R_OK | F_OK ) == -1 )
    finished = 1;
  }


  if ( archie_rename(outname, outname2) == -1 ) {
    /* "Can't rename output file %s" */

    error(A_SYSERR,"parse_webindex", PARSE_WEBINDEX_024, outname);
    erron = 1;
  }

  if ( archie_rename(etmp_file->filename,excerptname2) == -1 ) {
    /* "Can't rename output file %s" */

    error(A_SYSERR,"parse_webindex", PARSE_WEBINDEX_024, etmp_file->filename);
    erron = 1;
  }
    

   /* Don't unlink files if there is an error in parsing */

   if(!erron){
     if(unlink(input_file -> filename) == -1){

       /* "Can't unlink input file %s" */

       error(A_SYSERR,"parse_webindex", PARSE_WEBINDEX_025, input_file -> filename);
       erron = 1;
     }

     if(unlink(tmp_file -> filename) == -1){

       /* "Can't unlink temporary file %s" */

       error(A_SYSERR,"parse_webindex", PARSE_WEBINDEX_026, tmp_file -> filename);
       erron = 1;
     }
   }
   else{

      /* Rename the tmp file to something more useful */

      sprintf(outname2,"%s_%d%s", output_file -> filename, rand() % 100, SUFFIX_FILTERED);

      if(rename(tmp_file -> filename, outname2) == -1){

	 /* "Can't rename failed temporary file %s to %s" */

	 error(A_SYSERR,"parse_webindex", PARSE_WEBINDEX_028, tmp_file -> filename, outname2);
      }
   }

  destroy_finfo(input_file);
  destroy_finfo(output_file);
  destroy_finfo(out_file);
  destroy_finfo(tmp_file);
  destroy_finfo(etmp_file);
  destroy_finfo(excerpt_file);
  
               
   if(erron)
      exit(ERROR);

   exit(A_OK);
   return(A_OK);
}
  

  

/*
  If there are any directories left on the stack (i.e. there is a non-empty queue) then
  there is a directory that was listed as a file, but has no corresponding listing.
    
  Only print out warning messages.
*/



int
output_header(ofp, ignore_header, header_rec, parse_failed)
  FILE *ofp ;
  int ignore_header ;
  header_t *header_rec ;
  int parse_failed ;
{
  ptr_check(ofp, FILE, "output_header", 0) ;
  ptr_check(header_rec, header_t, "output_header", 0) ;

  if( ! ignore_header)
  {
    header_rec->generated_by = PARSER ;
    HDR_SET_GENERATED_BY(header_rec->header_flags) ;
#if 0
    header_rec->no_recs = elt_num() + 1 ; /* starts at 0 */
    HDR_SET_NO_RECS(header_rec->header_flags) ;
#endif
    header_rec->update_status = parse_failed ? FAIL : SUCCEED ;
    HDR_SET_UPDATE_STATUS(header_rec->header_flags) ;

    header_rec->parse_time = (date_time_t)time((time_t *)0) ;
    HDR_SET_PARSE_TIME(header_rec->header_flags) ;

    if(write_header(ofp, header_rec, (u32 *)0, 0, 0) != A_OK)
    {

      /* "Error from write_header()" */

      error(A_ERR, "parser_output", HANDLE_FILE_006);
      return 0 ;
    }
  }
  return 1 ;
}
