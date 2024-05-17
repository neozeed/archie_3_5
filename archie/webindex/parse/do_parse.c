#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "typedef.h"
#include "files.h"
#include "web.h"
#include "sub_header.h"
#include "error.h"
#include "db_files.h"

#include "archstridx.h"
#include "parse.h"

extern int compress_flag;
extern pathname_t uncompress_pgm;
extern pathname_t compress_pgm;
extern int errno;

status_t  parse_rec(infp, outfp, excerptfp, shrec,offset, strhan)
FILE *infp,*outfp,*excerptfp;
sub_header_t *shrec;
long offset;
struct arch_stridx_handle *strhan;
{
  FILE *fp;
  int size = 0;
  urlname_t tmpname,ctmpname,system_cmd;
  char *filename;
  FILE *fpi;
  char *ext = NULL;

  pathname_t fileroot;

  sprintf(fileroot,"%s-%s",shrec->server,"webindex");
  
  if(HDR_GET_SIZE_ENTRY(shrec->sub_header_flags))
    size = shrec->fsize;

  if ( size == 0 )
    return A_OK;
  
  fseek(infp,offset,0);


  if ( HDR_GET_FORMAT_ENTRY(shrec->sub_header_flags)) {
    switch ( shrec->format ) {
    case FXDR:
    case FRAW:
      compress_flag = 0;
      ext = NULL;
      break;
    case FXDR_COMPRESS_LZ:
    case FCOMPRESS_LZ:
      compress_flag = 1;
      strcpy(compress_pgm, COMPRESS_PGM);
      strcpy(uncompress_pgm, UNCOMPRESS_PGM);
      ext = ".Z";
      break;
    case FXDR_GZIP:
    case FCOMPRESS_GZIP:
      compress_flag = 2;
      ext = ".gz";
      if ( get_option_path( "COMPRESS", "GZIP", compress_pgm) == ERROR )  {
        error(A_ERR, "parse_rec", "Unable to find COMPRESS GZIP in the options file");
      }
      if ( get_option_path( "UNCOMPRESS", "GZIP", uncompress_pgm) == ERROR )  {
        error(A_ERR, "parse_rec", "Unable to find UNCOMPRESS GZIP in the options file");
      }      
      break;
    }
  }
  else {
    compress_flag = 0;
  }

  get_tmpfiles(fileroot,tmpname,ctmpname,ext);
  
  if ( compress_flag )
    filename = ctmpname;
  else
    filename = tmpname;
  
  fp = fopen(filename,"w+");
  if ( fp == NULL ) {
    error(A_ERR,"parse_rec","Cannot open file %s, errno = %d",filename,errno );
    return(ERROR);
  }

  if ( archie_fpcopy(infp,fp,size) != A_OK ) {
    fclose(fp);
    unlink(filename);
    error(A_ERR,"parse_rec","Error copying data, errno = %d",errno );
    return ERROR;
  }

  fclose(fp);

  if ( compress_flag ) {
    /* Need to compress the file */
    if ( ctmpname[0] == '\0' ) {
      error(A_SYSERR,"parse_rec","Invalid compressed filename");
      return ERROR;
    }
    
    if ( compress_flag == 2 ) 
    sprintf(system_cmd,"%s -f -n %s",uncompress_pgm,ctmpname);
    if ( compress_flag == 1 )
    sprintf(system_cmd,"%s -f  %s",uncompress_pgm,ctmpname);
    
    if ( system(system_cmd) == -1 ) {
	    unlink(ctmpname);
      error(A_ERR,"parse_rec","Cannot run uncompress program %s, errno = %d",compress_pgm,errno );            
	    return ERROR;
    }
  }

  filename = tmpname;

    
  fpi = fopen(filename,"r");
  if ( fpi == NULL ) {
    error(A_ERR,"parse_rec","Cannot open file %s, errno = %d",filename,errno );
    return ERROR;
  }
  
  keyword_extract(fpi,outfp,strhan);  /* Extract keywords from the file */
  excerpt_extract(fpi,excerptfp);        /* Extract text portion from the file */
  
  fclose(fpi);
  unlink(tmpname); 
  return A_OK;
}



#if 0
int appendSubData (outfp,shrec,file,cfile,compress)
FILE *outfp;
sub_header_t *shrec;
pathname_t file,cfile;
int compress;
{
  FILE *fpi;
  struct stat st;
  pathname_t system_cmd;
  char *filename;
  
  if ( compress == 1 ) {
    /* Need to compress the file */
    if ( cfile[0] == '\0' ) {
      error(A_SYSERR,"appendSubData","Invalid compressed filename");
      return ERROR;
    }
    sprintf(system_cmd,"%s -f %s",compress_pgm,file);
    if ( system(system_cmd) == -1 ) {
      error(A_ERR,"appendSubData","Cannot run compress program %s, errno = %d",compress_pgm,errno );      
	    return ERROR;
    }
  }
  
  if ( compress )
    filename = cfile;
  else
    filename = file;

  if ( stat(filename,&st) == -1 ) {
    error(A_ERR,"appendSubData","Cannot stat file %s, errno = %d",filename,errno );
    return ERROR;
  }

  /* Add sub_header */
  if ( write_sub_header(outfp, shrec,0,0,0) == ERROR ) {
    error(A_ERR,"appendSubData","Error writing sub_header");
    return ERROR;
  }
  
  /* Add Data */
  fpi = fopen(filename,"r");
  if (fpi == NULL ) {
    error(A_ERR,"appendSubData","Cannot open file %s, errno = %d",filename,errno );
    return ERROR;
  }

  if ( archie_fpcopy(fpi,outfp,st.st_size) == ERROR ) {
    fclose(fpi);
    unlink(filename);
    error(A_ERR,"appendSubData","Unable to copy data");
    return ERROR;
  }
  
  fclose(fpi);
  unlink(filename);
  return A_OK;
}
#endif

status_t parse_file(input_file,output_file,ex_file, no_recs, ip,port)
file_info_t *input_file, *output_file, *ex_file;
int *no_recs;
ip_addr_t ip;
int port;
{
  FILE *infp = input_file->fp_or_dbm.fp;
  FILE *outfp = output_file->fp_or_dbm.fp;
  FILE *exfp = ex_file->fp_or_dbm.fp;
  
  stoplist_setup();

  if ( recurse(infp,outfp,exfp,  no_recs,ip,port) != A_OK ) {
    error(A_ERR,"recurse", "Error processing the list of URLs");
    return ERROR;
  }
  
  return A_OK;
  
}
