#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <memory.h>
#include <varargs.h>
#include "typedef.h"
#include "error.h"
#include "times.h"
#include "files.h"
#include "archie_inet.h"
#include "lang_libarchie.h"
#include "header_def.h"
#include "archie_mail.h"

#include "protos.h"


/*
 * init_header: initialize the header fields "set" field to zero
 */

void init_header()

{
   int i;

   for(i=0; header_fields[i].index != -1; i++ )
      header_fields[i].set = 0;

}


/*
 * read_header: Read a header record. The input FILE * is given, as well as
 * the header record to place the information into. "offset" is set to the
 * number of bytes in the header. "netcomm" is non-zero if the header is
 * being read off of a network connnection: it strips the '\r' from the end
 * of the line
 */


status_t read_header(ifp, header_rec, offset, netcomm, turn_off)
   FILE *ifp;		/* Input file pointer */
   header_t *header_rec;   /* header record for input information */
   u32 *offset;		/* Number of bytes of the header record */
   int netcomm;		/* non-zero of header is being read from a
			   remote network connection */
   int turn_off;	/* if non-zero, turn off buffering of
			   stdio library */

{
   
   int finished = FALSE;
   int header_index;
   a_header_field_t input_buf;
   a_header_field_t field_name_buf, arg_buf;
   char *tmp_ptr;

   init_header();

   memset(header_rec,'\0', sizeof(header_t));

   if(turn_off)
      setbuf(ifp, (char *) NULL);

   while(!finished){

      if(fgets(input_buf, MAX_ASCII_HEADER_LEN, ifp) == (char *) NULL){

	 if(feof(ifp)){

#if 0
	    /* "Premature end of file" */

	    error(A_ERR,"read_header", READ_HEADER_001);

	       /* "%s record not found" */
	    error(A_ERR,"read_header", READ_HEADER_002, (header_fields[0].set == FALSE) ? HEADER_BEGIN : HEADER_END);
#endif
	 return(ERROR);

	 }
	 return(ERROR);
	    
      }

      /* Do not look for comments in the header */

#if 0
      if((comment_ptr = strchr(input_buf, COMMENT_CHAR)) != (char *) NULL)
	 *comment_ptr = '\0';
#endif

      field_name_buf[0] = arg_buf[0] = '\0';

      if(sscanf(input_buf,"%s %s", field_name_buf, arg_buf) == -1)
	 continue;

     /*
      * Make sure that there is no dupicate HEADER_BEGIN or that we find
      * HEADER_END but no HEADER_BEGIN
      */

      if(header_fields[0].set == FALSE){ /* No header begin found */

	 if(strncasecmp(field_name_buf, HEADER_BEGIN, strlen(HEADER_BEGIN)) != 0){ 

	    /* This isn't header begin */

	    if(strncasecmp(field_name_buf, HEADER_END,strlen(HEADER_END)) == 0){

            /* Found end but no begin */

	       /* "%s found but no %s. Aborting" */

	       error(A_ERR,"read_header", READ_HEADER_003, HEADER_END, HEADER_BEGIN);
	       return(ERROR);
	    }

	 }
	 else{
	    header_fields[0].set = TRUE;
	    continue;
	 }
      }
      else{
         if(strncasecmp(field_name_buf, HEADER_BEGIN, strlen(HEADER_BEGIN)) == 0){

	    /* "Duplicate %s found. Ignored" */
	    
	    error(A_ERR,"read_header", READ_HEADER_004, HEADER_BEGIN);
	    continue;
	 }

	 if(strncasecmp(field_name_buf, HEADER_END, strlen(HEADER_END)) == 0){
	    finished = TRUE;
	    continue;
	 }
	    
      }

      if(arg_buf[0] == '\0')
         continue;

      if(netcomm && (tmp_ptr = strchr(arg_buf, '\r')) != (char *) NULL)
         *tmp_ptr = '\0';
	 
   
      for(header_index = 1;
          header_fields[header_index].index != -1;
	  header_index++){

	  if(strncasecmp(header_fields[header_index].field,
	     field_name_buf,
	     strlen(header_fields[header_index].field)) == 0){

	     if(header_fields[header_index].set == TRUE){

	       /* Field is already set */
	       /* "Duplicate %s found. Ignored" */

	       error(A_WARN,"read_header", READ_HEADER_004, header_fields[header_index].field);
	       continue;
	     }

	     switch(header_fields[header_index].index){

	        case GENERATED_BY_I:

	           header_fields[header_index].set = TRUE;
		   HDR_SET_GENERATED_BY(header_rec -> header_flags);
		   
		   if(strcasecmp(GEN_PROG_PARSER, arg_buf) == 0)
		      header_rec -> generated_by = PARSER;
		   else if(strcasecmp(GEN_PROG_RETRIEVE, arg_buf) == 0)
		      header_rec -> generated_by = RETRIEVE;
		   else if(strcasecmp(GEN_PROG_SERVER, arg_buf) == 0)
		      header_rec -> generated_by = SERVER;
		   else if(strcasecmp(GEN_PROG_ADMIN, arg_buf) == 0)
		      header_rec -> generated_by = ADMIN;
		   else if(strcasecmp(GEN_PROG_INSERT, arg_buf) == 0)
		      header_rec -> generated_by = INSERT;
	           else if(strcasecmp(GEN_PROG_CONTROL, arg_buf) == 0)
		      header_rec -> generated_by = CONTROL;
		   else{


		      /* "Invalid %s field: %s" */

		      error(A_INTERR, "read_header", READ_HEADER_005,header_fields[header_index].field, arg_buf);
		      HDR_UNSET_GENERATED_BY(header_rec -> header_flags);
		      header_fields[header_index].set = FALSE;
		   }
		break;
	  

	        case SOURCE_ARCHIE_HOSTNAME_I:

	           strcpy(header_rec -> source_archie_hostname, arg_buf);
	           header_fields[header_index].set = TRUE;
		   HDR_SET_SOURCE_ARCHIE_HOSTNAME(header_rec -> header_flags);

		break;

	        case PRIMARY_HOSTNAME_I:

		   strcpy(header_rec -> primary_hostname, arg_buf);
	           header_fields[header_index].set = TRUE;
		   HDR_SET_PRIMARY_HOSTNAME(header_rec -> header_flags);
		   
		break;

	        case PREFERRED_HOSTNAME_I:

		   strcpy(header_rec -> preferred_hostname, arg_buf);
		   header_fields[header_index].set = TRUE;
		   HDR_SET_PREFERRED_HOSTNAME(header_rec -> header_flags);

		break;

                case PRIMARY_IPADDR_I:

		   header_rec -> primary_ipaddr = inet_addr(arg_buf);
	           header_fields[header_index].set = TRUE;
		   HDR_SET_PRIMARY_IPADDR(header_rec -> header_flags);

		break;

                case ACCESS_METHODS_I:

	           strcpy(header_rec -> access_methods, arg_buf);
		   header_fields[header_index].set = TRUE;
		   HDR_SET_ACCESS_METHODS(header_rec -> header_flags);

		break;

                case ACCESS_COMMAND_I:

		   strcpy(header_rec -> access_command, arg_buf);
	           header_fields[header_index].set = TRUE;
		   HDR_SET_ACCESS_COMMAND(header_rec -> header_flags);

		break;

	        case OS_TYPE_I:
	           header_fields[header_index].set = TRUE;
		   HDR_SET_OS_TYPE(header_rec -> header_flags);
		   
		   if(strcasecmp(OS_TYPE_UNIX_BSD, arg_buf) == 0)
		      header_rec -> os_type = UNIX_BSD;
		   else if(strcasecmp(OS_TYPE_VMS_STD, arg_buf) == 0)
		      header_rec -> os_type = VMS_STD;
		   else if(strcasecmp(OS_TYPE_NOVELL, arg_buf) == 0)
		      header_rec -> os_type = NOVELL;
		   else{

		      /* "Invalid %s field: %s" */
		   
		      error(A_INTERR, "read_header", READ_HEADER_005,header_fields[header_index].field, arg_buf);
		      header_fields[header_index].set = FALSE;
		      HDR_UNSET_OS_TYPE(header_rec -> header_flags);
		   }
		break;

	        case TIMEZONE_I:
	           header_fields[header_index].set = TRUE;
		   HDR_SET_TIMEZONE(header_rec -> header_flags);
		   header_rec -> timezone = atoi(arg_buf);

		break;

	        case RETRIEVE_TIME_I:
	           header_fields[header_index].set = TRUE;
		   HDR_SET_RETRIEVE_TIME(header_rec -> header_flags);
		   header_rec -> retrieve_time = cvt_to_inttime(arg_buf,0);

		break;

	        case PARSE_TIME_I:
	           header_fields[header_index].set = TRUE;
		   header_rec -> parse_time = cvt_to_inttime(arg_buf, 0);
		   HDR_SET_PARSE_TIME(header_rec -> header_flags);
		break;

	        case UPDATE_TIME_I:
	           header_fields[header_index].set = TRUE;
		   HDR_SET_UPDATE_TIME(header_rec -> header_flags);
		   header_rec -> update_time = cvt_to_inttime(arg_buf,0);

		break;

	        case NO_RECS_I:
	           header_fields[header_index].set = TRUE;
		   HDR_SET_NO_RECS(header_rec -> header_flags);
		   header_rec -> no_recs = atoi(arg_buf);

		break;

	        case CURRENT_STATUS_I:
	           header_fields[header_index].set = TRUE;
		   HDR_SET_CURRENT_STATUS(header_rec -> header_flags);

		   if(strcasecmp(CURRENT_STATUS_ACTIVE, arg_buf) == 0)
		      header_rec -> current_status = ACTIVE;
		   else if(strcasecmp(CURRENT_STATUS_NOT_SUPPORTED, arg_buf) == 0)
      		      header_rec -> current_status = NOT_SUPPORTED;
		   else if(strcasecmp(CURRENT_STATUS_DEL_BY_ADMIN, arg_buf) == 0)
		      header_rec -> current_status = DEL_BY_ADMIN;
		   else if(strcasecmp(CURRENT_STATUS_DEL_BY_ARCHIE, arg_buf) == 0)
		      header_rec -> current_status = DEL_BY_ARCHIE;
		   else if(strcasecmp(CURRENT_STATUS_INACTIVE, arg_buf) == 0)
		      header_rec -> current_status = INACTIVE;
		   else if(strcasecmp(CURRENT_STATUS_DELETED, arg_buf) == 0)
		      header_rec -> current_status = DELETED;
		   else if(strcasecmp(CURRENT_STATUS_DISABLED, arg_buf) == 0)
		      header_rec -> current_status = DISABLED;
		   else{


		      /* "Invalid %s field: %s" */

		      error(A_INTERR, "read_header", READ_HEADER_005,header_fields[header_index].field, arg_buf);
		      header_fields[header_index].set = FALSE;
		      HDR_UNSET_CURRENT_STATUS(header_rec -> header_flags);
		   }
		break;

		case ACTION_STATUS_I:
	           header_fields[header_index].set = TRUE;
		   HDR_SET_ACTION_STATUS(header_rec -> header_flags);

		   if(strcasecmp(ACTION_STATUS_NEW, arg_buf) == 0)
		      header_rec -> action_status = NEW;
		   else if(strcasecmp(ACTION_STATUS_UPDATE, arg_buf) == 0)
      		      header_rec -> action_status = UPDATE;
		   else if(strcasecmp(ACTION_STATUS_DELETE, arg_buf) == 0)
		      header_rec -> action_status = DELETE;
		   else{

		      /* "Invalid %s field: %s" */

		      error(A_INTERR, "read_header", READ_HEADER_005,header_fields[header_index].field, arg_buf);
		      header_fields[header_index].set = FALSE;
		      HDR_UNSET_ACTION_STATUS(header_rec -> header_flags);
		   }
		break;

		case UPDATE_STATUS_I:
	           header_fields[header_index].set = TRUE;
		   HDR_SET_UPDATE_STATUS(header_rec -> header_flags);

		   if(strcasecmp(UPDATE_STATUS_SUCCEED, arg_buf) == 0)
		      header_rec -> update_status = SUCCEED;
		   else if(strcasecmp(UPDATE_STATUS_FAIL, arg_buf) == 0)
      		      header_rec -> update_status = FAIL;
		   else{

		      /* "Invalid %s field: %s" */

		      error(A_INTERR, "read_header", READ_HEADER_005,header_fields[header_index].field, arg_buf);
		      header_fields[header_index].set = FALSE;
		      HDR_UNSET_UPDATE_STATUS(header_rec -> header_flags);
		   }
		break;

		case FORMAT_I:
 	           header_fields[header_index].set = TRUE;
		   HDR_SET_FORMAT(header_rec -> header_flags);

		   if(strcasecmp(FORMAT_RAW, arg_buf) == 0)
   		      header_rec -> format = FRAW;
		   else if(strcasecmp(FORMAT_FXDR, arg_buf) == 0)
      		      header_rec -> format = FXDR;
		   else if(strcasecmp(FORMAT_COMPRESS_LZ, arg_buf) == 0)
      		      header_rec -> format = FCOMPRESS_LZ;
		   else if(strcasecmp(FORMAT_COMPRESS_GZIP, arg_buf) == 0)
      		      header_rec -> format = FCOMPRESS_GZIP;
		   else if(strcasecmp(FORMAT_XDR_COMPRESS_LZ, arg_buf) == 0)
      		      header_rec -> format = FXDR_COMPRESS_LZ;
		   else if(strcasecmp(FORMAT_XDR_GZIP, arg_buf) == 0)
      		      header_rec -> format = FXDR_GZIP;
		   else{

		      /* "Invalid %s field: %s" */

		      error(A_INTERR, "read_header", READ_HEADER_005,header_fields[header_index].field, arg_buf);
		      header_fields[header_index].set = FALSE;
		      HDR_UNSET_FORMAT(header_rec -> header_flags);
		   }
		   break;

		case PROSPERO_HOST_I:
 	           header_fields[header_index].set = TRUE;
		   HDR_SET_PROSPERO_HOST(header_rec -> header_flags);

		   if(strcasecmp(PROSPERO_YES, arg_buf) == 0)
   		      header_rec -> prospero_host = YES;
		   else if(strcasecmp(PROSPERO_NO, arg_buf) == 0)
   		      header_rec -> prospero_host = NO;
		   else{  

		      /* "Invalid %s field: %s" */

		      error(A_INTERR, "read_header", READ_HEADER_005,header_fields[header_index].field, arg_buf);
		      header_fields[header_index].set = FALSE;
		      HDR_UNSET_PROSPERO_HOST(header_rec -> header_flags);
		   }
		   break;

		case HCOMMENT_I:

		   /* This is a BUG */

		   if(arg_buf[0] != '\0'){
		      sscanf(input_buf,"%*s%*[ ]%[^\n]", arg_buf);

		      strcpy(header_rec -> comment, arg_buf);
		      header_fields[header_index].set = TRUE;
		      HDR_SET_HCOMMENT(header_rec -> header_flags);

		   }
		   break;   

                case DATA_NAME_I:

		   strcpy(header_rec -> data_name, arg_buf);
	           header_fields[header_index].set = TRUE;
		   HDR_SET_DATA_NAME(header_rec -> header_flags);
		   break;

		default:

		   /* "Unknown field in header: %s" */

		   error(A_INTERR,"read_header",READ_HEADER_006, field_name_buf );
		break;
		      
	     } /* switch */
	  }/* if */
       }/* for */
   }/* while */

   
   if(offset != (u32 *) NULL)
      *offset = (u32) ftell(ifp);

   return(A_OK);
}





/*
 * write_header: write the given header record to the file pointer. Offset
 * is ignored. netcomm is non-zero when writing to a network connection
 */

status_t write_header(ofp, header_rec, offset, netcomm, turn_off)
   FILE *ofp;			/* File pointer where output goes */
   header_t *header_rec;	/* Record to be written out	  */
   u32 *offset;			/* Ignored			  */
   int netcomm;			/* 0, not net command, non-zero otherwise */
   int turn_off;		/* Ignored */
{

   a_header_field_t output_buf;
   char tmp_string[8];

   if(netcomm)
      sprintf(tmp_string,"\r\n");
   else
      sprintf(tmp_string,"\n");

   /* Write header */

   fprintf(ofp,"%s%s", HEADER_BEGIN, tmp_string);

   if(HDR_GET_GENERATED_BY(header_rec -> header_flags)){
      sprintf(output_buf, "%s ", GENERATED_BY);

      if(header_rec -> generated_by == PARSER)
         strcat(output_buf, GEN_PROG_PARSER);
      else if(header_rec -> generated_by == RETRIEVE)
         strcat(output_buf, GEN_PROG_RETRIEVE);
      else if(header_rec -> generated_by == SERVER)
         strcat(output_buf, GEN_PROG_SERVER);
      else if(header_rec -> generated_by == ADMIN)
           strcat(output_buf, GEN_PROG_ADMIN);
      else if(header_rec -> generated_by == INSERT)
           strcat(output_buf, GEN_PROG_INSERT);
      else if(header_rec -> generated_by == CONTROL)
           strcat(output_buf, GEN_PROG_CONTROL);

      fprintf(ofp,"%s%s", output_buf,tmp_string);
   }

   if(HDR_GET_SOURCE_ARCHIE_HOSTNAME(header_rec -> header_flags))
      fprintf(ofp,"%s %s%s", SOURCE_ARCHIE_HOSTNAME, header_rec -> source_archie_hostname, tmp_string);

   if(HDR_GET_PRIMARY_HOSTNAME(header_rec -> header_flags))
      fprintf(ofp,"%s %s%s",PRIMARY_HOSTNAME,header_rec -> primary_hostname, tmp_string);

   if(HDR_GET_PREFERRED_HOSTNAME(header_rec -> header_flags))
      fprintf(ofp,"%s %s%s",PREFERRED_HOSTNAME, header_rec -> preferred_hostname, tmp_string);

   if(HDR_GET_PRIMARY_IPADDR(header_rec -> header_flags))
      fprintf(ofp,"%s %s%s", PRIMARY_IPADDR, inet_ntoa(ipaddr_to_inet(header_rec -> primary_ipaddr)), tmp_string);

   if(HDR_GET_ACCESS_METHODS(header_rec -> header_flags))
      fprintf(ofp,"%s %s%s", ACCESS_METHODS, header_rec -> access_methods, tmp_string);

   if(HDR_GET_ACCESS_COMMAND(header_rec -> header_flags))
      fprintf(ofp,"%s %s%s", ACCESS_COMMAND, header_rec -> access_command, tmp_string);

   if(HDR_GET_OS_TYPE(header_rec -> header_flags)){
      sprintf(output_buf, "%s ", OS_TYPE);

      if(header_rec -> os_type == UNIX_BSD)
         strcat(output_buf, OS_TYPE_UNIX_BSD);
      else if(header_rec -> os_type == VMS_STD)
         strcat(output_buf, OS_TYPE_VMS_STD);
      else if(header_rec -> os_type == NOVELL)
         strcat(output_buf, OS_TYPE_NOVELL);

      fprintf(ofp,"%s%s", output_buf, tmp_string);
   }

   if(HDR_GET_TIMEZONE(header_rec -> header_flags))
      fprintf(ofp,"%s %lu%s", TIMEZONE, header_rec -> timezone, tmp_string);

   if(HDR_GET_RETRIEVE_TIME(header_rec -> header_flags))
      fprintf(ofp,"%s %s%s", RETRIEVE_TIME, cvt_from_inttime(header_rec -> retrieve_time), tmp_string);
      
   if(HDR_GET_PARSE_TIME(header_rec -> header_flags))
      fprintf(ofp,"%s %s%s", PARSE_TIME, cvt_from_inttime(header_rec -> parse_time), tmp_string);

   if(HDR_GET_UPDATE_TIME(header_rec -> header_flags))
      fprintf(ofp,"%s %s%s", UPDATE_TIME, cvt_from_inttime(header_rec -> update_time), tmp_string);

   if(HDR_GET_NO_RECS(header_rec -> header_flags))
      fprintf(ofp,"%s %lu%s", NO_RECS, header_rec -> no_recs, tmp_string);

   if(HDR_GET_CURRENT_STATUS(header_rec -> header_flags)){
      sprintf(output_buf, "%s ", CURRENT_STATUS);

      if(header_rec -> current_status == ACTIVE)
         strcat(output_buf, CURRENT_STATUS_ACTIVE);
      else if(header_rec -> current_status == NOT_SUPPORTED)
         strcat(output_buf, CURRENT_STATUS_NOT_SUPPORTED);
      else if(header_rec -> current_status == DEL_BY_ADMIN)
           strcat(output_buf, CURRENT_STATUS_DEL_BY_ADMIN);
      else if(header_rec -> current_status == DEL_BY_ARCHIE)
           strcat(output_buf, CURRENT_STATUS_DEL_BY_ARCHIE);
      else if(header_rec -> current_status == INACTIVE)
           strcat(output_buf, CURRENT_STATUS_INACTIVE);
      else if(header_rec -> current_status == DELETED)
           strcat(output_buf, CURRENT_STATUS_DELETED);
      else if(header_rec -> current_status == DISABLED)
           strcat(output_buf, CURRENT_STATUS_DISABLED);

      fprintf(ofp,"%s%s", output_buf, tmp_string);
   }

   if(HDR_GET_ACTION_STATUS(header_rec -> header_flags)){
      sprintf(output_buf, "%s ", ACTION_STATUS);
   
      if(header_rec -> action_status == NEW)
         strcat(output_buf, ACTION_STATUS_NEW);
      else if(header_rec -> action_status == UPDATE)
         strcat(output_buf, ACTION_STATUS_UPDATE);
      else if(header_rec -> action_status == DELETE)
         strcat(output_buf, ACTION_STATUS_DELETE);

      fprintf(ofp,"%s%s", output_buf, tmp_string);
   }

   if(HDR_GET_UPDATE_STATUS(header_rec -> header_flags)){
      sprintf(output_buf, "%s ", UPDATE_STATUS);

      if(header_rec -> update_status == SUCCEED)
         strcat(output_buf, UPDATE_STATUS_SUCCEED);
      else if(header_rec -> update_status == FAIL)
           strcat(output_buf, UPDATE_STATUS_FAIL);

      fprintf(ofp,"%s%s", output_buf, tmp_string);
   }


   if(HDR_GET_FORMAT(header_rec -> header_flags)){
      sprintf(output_buf, "%s ", FORMAT);

      if(header_rec -> format == FRAW)
         strcat(output_buf, FORMAT_RAW);
      else if(header_rec -> format == FXDR)
         strcat(output_buf, FORMAT_FXDR);
      else if(header_rec -> format == FCOMPRESS_LZ)
         strcat(output_buf, FORMAT_COMPRESS_LZ);
      else if(header_rec -> format == FCOMPRESS_GZIP)
         strcat(output_buf, FORMAT_COMPRESS_GZIP);
      else if(header_rec -> format == FXDR_COMPRESS_LZ)
         strcat(output_buf, FORMAT_XDR_COMPRESS_LZ);
      else if(header_rec -> format == FXDR_GZIP)
         strcat(output_buf, FORMAT_XDR_GZIP);

      fprintf(ofp,"%s%s", output_buf, tmp_string);
   }

   if(HDR_GET_PROSPERO_HOST(header_rec -> header_flags)){
      sprintf(output_buf, "%s ", PROSPERO_HOST);

      if(header_rec -> prospero_host == YES)
         strcat(output_buf, PROSPERO_YES);
      else 
         strcat(output_buf, PROSPERO_NO);

      fprintf(ofp,"%s%s", output_buf, tmp_string);
   }

   if(HDR_GET_HCOMMENT(header_rec -> header_flags)){

      fprintf(ofp,"%s %s%s", HCOMMENT, header_rec -> comment, tmp_string);
   }

   if(HDR_GET_DATA_NAME(header_rec -> header_flags))
      fprintf(ofp,"%s %s%s", DATA_NAME, header_rec -> data_name, tmp_string);

   fprintf(ofp,"%s%s", HEADER_END, tmp_string);

   fflush(ofp);

   return(A_OK);
}

/*
 * write_error_header: Write out an error header. The given file pointer is
 * rewound to the beginning of the file and the given header record written
 * out. This cannot be used on network connections (because one cannot
 * "rewind" the file)
 */
      

void write_error_header(curr_file, header_rec)
   file_info_t *curr_file;    /* file in which header is to be written */
   header_t *header_rec;      /* header to be written */

{
#ifdef AIX
#ifdef __STDC__

   extern time_t time(time_t *);

#else

   extern time_t time();
#endif
#endif   

   if(curr_file -> fp_or_dbm.fp == (FILE *) NULL){
      if(open_file(curr_file, O_WRONLY) == ERROR){

	 /* "Can't write error header for %s */

	 error(A_ERR,"write_error_header", WRITE_ERROR_HEADER_001, curr_file -> filename);
         return;
      }
   }
   else{
      ftruncate(fileno(curr_file -> fp_or_dbm.fp), (off_t) 0);
      rewind(curr_file -> fp_or_dbm.fp);
   }

   HDR_SET_HCOMMENT(header_rec -> header_flags);

   header_rec -> update_status = FAIL;
   HDR_SET_UPDATE_STATUS(header_rec -> header_flags);

   header_rec -> format = FRAW;
   HDR_SET_FORMAT(header_rec -> header_flags);

   header_rec -> retrieve_time = time((time_t *) NULL);
   HDR_SET_RETRIEVE_TIME(header_rec -> header_flags);

   if(write_header(curr_file -> fp_or_dbm.fp, header_rec, (u32 *) NULL, 0, 0) == ERROR){

      /* Can't write error header for %s" */

      error(A_ERR,"write_error_header", WRITE_ERROR_HEADER_001, curr_file -> filename);
      return;
   }

   close_file(curr_file);

   return;

}





/* do_error_header: write a error header file containing information as to failure */


void do_error_header( va_alist )
   va_dcl

/*   file_info_t *curr_file;
   file_info_t *output_file;
   int count;
   header_t *header_rec;
   char *suffix;
   char *format;
   arglist
*/   

{
   extern int vsprintf();

   file_info_t *curr_file;  /* the output file */
   file_info_t *output_file;  /* the output file */
   int count;		      /* */
   header_t *header_rec;      /* the header record to be written */
   char *suffix;
   pathname_t tmp_name;
   va_list al;
   char *format;

   va_start( al );

   curr_file = va_arg(al, file_info_t *);
   output_file = va_arg(al, file_info_t *);
   count = va_arg(al, int);
   header_rec = va_arg(al, header_t *);
   suffix = va_arg(al, char *);
   format = va_arg(al, char *);

   ptr_check(curr_file, file_info_t, "do_error_header",);
   ptr_check(output_file, file_info_t, "do_error_header",);
   ptr_check(header_rec, header_t, "do_error_header",);


   header_rec -> comment[0] = '\0';
   HDR_SET_HCOMMENT(header_rec -> header_flags);

   vsprintf(header_rec -> comment, format, al);

   write_mail(MAIL_RETR_FAIL, "%s %s %s", header_rec -> primary_hostname, header_rec -> access_methods, header_rec -> comment);

   write_error_header(curr_file, header_rec);

   sprintf(tmp_name, "%s_%d%s", output_file -> filename, count, suffix);

   va_end( al );

   if(rename(curr_file -> filename, tmp_name) == -1){

      error(A_ERR,"do_retrieve", "Can't rename temporary file %s to %s", curr_file -> filename, tmp_name);
      return;
   }

   return;
}


/* do_problem_header: write a error header file containing information as to failure */


void do_problem_header( va_alist )
   va_dcl

/*   file_info_t *curr_file;
   file_info_t *output_file;
   int count;
   header_t *header_rec;
   char *suffix;
   char *format;
   arglist
*/   

{
   extern int vsprintf();

   file_info_t *curr_file;  /* the output file */
   file_info_t *output_file;  /* the output file */
   int count;		      /* */
   header_t *header_rec;      /* the header record to be written */
   char *suffix;
   pathname_t tmp_name;
   va_list al;
   char *format;

   va_start( al );

   curr_file = va_arg(al, file_info_t *);
   output_file = va_arg(al, file_info_t *);
   count = va_arg(al, int);
   header_rec = va_arg(al, header_t *);
   suffix = va_arg(al, char *);
   format = va_arg(al, char *);

   ptr_check(curr_file, file_info_t, "do_error_header",);
   ptr_check(output_file, file_info_t, "do_error_header",);
   ptr_check(header_rec, header_t, "do_error_header",);


   header_rec -> comment[0] = '\0';
   HDR_SET_HCOMMENT(header_rec -> header_flags);

   vsprintf(header_rec -> comment, format, al);

   write_mail(MAIL_RETR_FAIL, "%s %s %s", header_rec -> primary_hostname, header_rec -> access_methods, header_rec -> comment);

   write_error_header(curr_file, header_rec);


   va_end( al );


   return;
}

