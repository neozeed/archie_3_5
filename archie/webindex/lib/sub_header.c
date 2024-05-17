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
#include "sub_header_def.h"
#include "archie_mail.h"
#include "lang_weblib.h"
#include "protos.h"


/*
 * init_sub_sub_header: initialize the sub_header fields "set" field to zero
 */
void init_sub_header()

{
   int i;

   for(i=0; sub_header_fields[i].index != -1; i++ )
      sub_header_fields[i].set = 0;

}


/*
 * read_sub_header: Read a sub_header record. The input FILE * is given, as well as
 * the sub_header record to place the information into. "offset" is set to the
 * number of bytes in the sub_header. "netcomm" is non-zero if the sub_header is
 * being read off of a network connnection: it strips the '\r' from the end
 * of the line
 */


status_t read_sub_header(ifp, sub_header_rec, offset, netcomm, turn_off)
   FILE *ifp;		/* Input file pointer */
   sub_header_t *sub_header_rec;   /* sub_header record for input information */
   u32 *offset;		/* Number of bytes of the sub_header record */
   int netcomm;		/* non-zero if sub_header is being read from a
			   remote network connection */
   int turn_off;	/* if non-zero, turn off buffering of
			   stdio library */

{
   
   int finished = FALSE;
   int sub_header_index;
   a_sub_header_field_t input_buf;
   a_sub_header_field_t field_name_buf, arg_buf;
   char *tmp_ptr;

   init_sub_header();

   memset(sub_header_rec,'\0', sizeof(sub_header_t));

   if(turn_off)
      setbuf(ifp, (char *) NULL);

   while(!finished){

      if(fgets(input_buf, MAX_ASCII_HEADER_LEN, ifp) == (char *) NULL){

	 if(feof(ifp)){

#if 0
	    /* "Premature end of file" */

	    error(A_ERR,"read_sub_header", READ_SUB_HEADER_001);

	       /* "%s record not found" */
	    error(A_ERR,"read_sub_header", READ_SUB_HEADER_002, (sub_header_fields[0].set == FALSE) ? SUB_HEADER_BEGIN : SUB_HEADER_END);
#endif
	 return(ERROR);

	 }
	 return(ERROR);
	    
      }

      /* Do not look for comments in the sub_header */

#if 0
      if((comment_ptr = strchr(input_buf, COMMENT_CHAR)) != (char *) NULL)
	 *comment_ptr = '\0';
#endif

      field_name_buf[0] = arg_buf[0] = '\0';

      if(sscanf(input_buf,"%s %s", field_name_buf, arg_buf) == -1)
	 continue;

     /*
      * Make sure that there is no dupicate SUB_HEADER_BEGIN or that we find
      * SUB_HEADER_END but no SUB_HEADER_BEGIN
      */

      if(sub_header_fields[0].set == FALSE){ /* No sub_header begin found */

	 if(strncasecmp(field_name_buf, SUB_HEADER_BEGIN, strlen(SUB_HEADER_BEGIN)) != 0){ 

	    /* This isn't sub_header begin */

	    if(strncasecmp(field_name_buf, SUB_HEADER_END,strlen(SUB_HEADER_END)) == 0){

            /* Found end but no begin */

	       /* "%s found but no %s. Aborting" */

	       error(A_ERR,"read_sub_header", READ_SUB_HEADER_003, SUB_HEADER_END, SUB_HEADER_BEGIN);
	       return(ERROR);
	    }

	 }
	 else{
	    sub_header_fields[0].set = TRUE;
	    continue;
	 }
      }
      else{
         if(strncasecmp(field_name_buf, SUB_HEADER_BEGIN, strlen(SUB_HEADER_BEGIN)) == 0){

	    /* "Duplicate %s found. Ignored" */
	    
	    error(A_ERR,"read_sub_header", READ_SUB_HEADER_004, SUB_HEADER_BEGIN);
	    continue;
	 }

	 if(strncasecmp(field_name_buf, SUB_HEADER_END, strlen(SUB_HEADER_END)) == 0){
	    finished = TRUE;
	    continue;
	 }
	    
      }

      if(arg_buf[0] == '\0')
         continue;

      if(netcomm && (tmp_ptr = strchr(arg_buf, '\r')) != (char *) NULL)
         *tmp_ptr = '\0';
	 
   
      for(sub_header_index = 1;
          sub_header_fields[sub_header_index].index != -1;
	  sub_header_index++){

	  if(strncasecmp(sub_header_fields[sub_header_index].field,
	     field_name_buf,
	     strlen(sub_header_fields[sub_header_index].field)) == 0){

	     if(sub_header_fields[sub_header_index].set == TRUE){

	       /* Field is already set */
	       /* "Duplicate %s found. Ignored" */

	       error(A_WARN,"read_sub_header", READ_SUB_HEADER_004, sub_header_fields[sub_header_index].field);
	       continue;
	     }

	     switch(sub_header_fields[sub_header_index].index){

       case LOCAL_URL_ENTRY_I:

         strcpy(sub_header_rec -> local_url, arg_buf);
         sub_header_fields[sub_header_index].set = TRUE;
         HDR_SET_LOCAL_URL_ENTRY(sub_header_rec -> sub_header_flags);
         break;

       case SIZE_ENTRY_I:

         sub_header_fields[sub_header_index].set = TRUE;
         HDR_SET_SIZE_ENTRY(sub_header_rec -> sub_header_flags);
         sub_header_rec -> size = atoi(arg_buf);
         break;

       case FSIZE_ENTRY_I:

         sub_header_fields[sub_header_index].set = TRUE;
         HDR_SET_FSIZE_ENTRY(sub_header_rec -> sub_header_flags);
         sub_header_rec -> fsize = atoi(arg_buf);
         break;

       case DATE_ENTRY_I:

         sub_header_fields[sub_header_index].set = TRUE;
         HDR_SET_DATE_ENTRY(sub_header_rec -> sub_header_flags);
         sub_header_rec -> date = cvt_to_inttime(arg_buf,0);

         break;
             
       case RECNO_ENTRY_I:

         sub_header_fields[sub_header_index].set = TRUE;
         HDR_SET_RECNO_ENTRY(sub_header_rec -> sub_header_flags);
         sub_header_rec -> recno = atoi(arg_buf);
         break;

       case PORT_ENTRY_I:

         sub_header_fields[sub_header_index].set = TRUE;
         HDR_SET_PORT_ENTRY(sub_header_rec -> sub_header_flags);
         sub_header_rec -> port = atoi(arg_buf);
         break;

       case SERVER_ENTRY_I:

         sub_header_fields[sub_header_index].set = TRUE;
         HDR_SET_SERVER_ENTRY(sub_header_rec -> sub_header_flags);
         strcpy(sub_header_rec -> server,arg_buf);
         break;
             
       case STATE_ENTRY_I:

         strcpy(sub_header_rec -> state, arg_buf);
         sub_header_fields[sub_header_index].set = TRUE;
         HDR_SET_STATE_ENTRY(sub_header_rec -> sub_header_flags);
         break;

       case TYPE_ENTRY_I:

         strcpy(sub_header_rec -> type, arg_buf);
         sub_header_fields[sub_header_index].set = TRUE;
         HDR_SET_TYPE_ENTRY(sub_header_rec -> sub_header_flags);
         break;

       case FORMAT_ENTRY_I:

         sub_header_fields[sub_header_index].set = TRUE;
         HDR_SET_FORMAT_ENTRY(sub_header_rec -> sub_header_flags);

         if(strcasecmp(FORMAT_RAW, arg_buf) == 0)
           sub_header_rec -> format = FRAW;
         else if(strcasecmp(FORMAT_FXDR, arg_buf) == 0)
           sub_header_rec -> format = FXDR;
         else if(strcasecmp(FORMAT_COMPRESS_LZ, arg_buf) == 0)
           sub_header_rec -> format = FCOMPRESS_LZ;
         else if(strcasecmp(FORMAT_COMPRESS_GZIP, arg_buf) == 0)
           sub_header_rec -> format = FCOMPRESS_GZIP;
         else if(strcasecmp(FORMAT_XDR_COMPRESS_LZ, arg_buf) == 0)
           sub_header_rec -> format = FXDR_COMPRESS_LZ;
         else if(strcasecmp(FORMAT_XDR_GZIP, arg_buf) == 0)
           sub_header_rec -> format = FXDR_GZIP;
         else{

           /* "Invalid %s field: %s" */

           error(A_INTERR, "read_sub_header", "Invalid %s field: %s",
                 sub_header_fields[sub_header_index].field, arg_buf);
           sub_header_fields[sub_header_index].set = FALSE;
           HDR_UNSET_FORMAT_ENTRY(sub_header_rec -> sub_header_flags);
         }
         break;
         
       default:

         /* "Unknown field in sub_header: %s" */

         error(A_INTERR,"read_sub_header",READ_SUB_HEADER_006, field_name_buf );
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
 * write_sub_header: write the given sub_header record to the file pointer. Offset
 * is ignored. netcomm is non-zero when writing to a network connection
 */

status_t write_sub_header(ofp, sub_header_rec, offset, netcomm, turn_off)
   FILE *ofp;			/* File pointer where output goes */
   sub_header_t *sub_header_rec;	/* Record to be written out	  */
   u32 *offset;			/* Ignored			  */
   int netcomm;			/* 0, not net command, non-zero otherwise */
   int turn_off;		/* Ignored */
{

/*   a_sub_header_field_t output_buf; */
   char tmp_string[8];

   if(netcomm)
      sprintf(tmp_string,"\r\n");
   else
      sprintf(tmp_string,"\n");

   /* Write sub_header */

   fprintf(ofp,"%s%s", SUB_HEADER_BEGIN, tmp_string);

   if(HDR_GET_LOCAL_URL_ENTRY(sub_header_rec -> sub_header_flags))
      fprintf(ofp,"%s %s%s", LOCAL_URL_ENTRY, sub_header_rec -> local_url, tmp_string);

   if(HDR_GET_STATE_ENTRY(sub_header_rec -> sub_header_flags))
      fprintf(ofp,"%s %s%s", STATE_ENTRY, sub_header_rec -> state, tmp_string);

   if(HDR_GET_SIZE_ENTRY(sub_header_rec -> sub_header_flags))
      fprintf(ofp,"%s %lu%s", SIZE_ENTRY, sub_header_rec -> size, tmp_string);

   if(HDR_GET_FSIZE_ENTRY(sub_header_rec -> sub_header_flags))
      fprintf(ofp,"%s %lu%s", FSIZE_ENTRY, sub_header_rec -> fsize, tmp_string);

   if(HDR_GET_DATE_ENTRY(sub_header_rec -> sub_header_flags))
      fprintf(ofp,"%s %s%s", DATE_ENTRY, cvt_from_inttime(sub_header_rec -> date), tmp_string);
   
   if(HDR_GET_RECNO_ENTRY(sub_header_rec -> sub_header_flags))
      fprintf(ofp,"%s %lu%s", RECNO_ENTRY, sub_header_rec -> recno, tmp_string);

   if(HDR_GET_PORT_ENTRY(sub_header_rec -> sub_header_flags))
      fprintf(ofp,"%s %lu%s", PORT_ENTRY, sub_header_rec -> port, tmp_string);

   if(HDR_GET_SERVER_ENTRY(sub_header_rec -> sub_header_flags))
      fprintf(ofp,"%s %s%s", SERVER_ENTRY, sub_header_rec -> server, tmp_string);
   if(HDR_GET_TYPE_ENTRY(sub_header_rec -> sub_header_flags))
      fprintf(ofp,"%s %s%s", TYPE_ENTRY, sub_header_rec -> type, tmp_string);

   if(HDR_GET_FORMAT_ENTRY(sub_header_rec -> sub_header_flags)) {
     pathname_t output_buf;
     sprintf(output_buf,"%s ",FORMAT);
     
     if(sub_header_rec -> format == FRAW)
     strcat(output_buf, FORMAT_RAW);
     else if(sub_header_rec -> format == FXDR)
     strcat(output_buf, FORMAT_FXDR);
     else if(sub_header_rec -> format == FCOMPRESS_LZ)
     strcat(output_buf, FORMAT_COMPRESS_LZ);
     else if(sub_header_rec -> format == FCOMPRESS_GZIP)
     strcat(output_buf, FORMAT_COMPRESS_GZIP);
     else if(sub_header_rec -> format == FXDR_COMPRESS_LZ)
     strcat(output_buf, FORMAT_XDR_COMPRESS_LZ);
     else if(sub_header_rec -> format == FXDR_GZIP)
     strcat(output_buf, FORMAT_XDR_GZIP);

    fprintf(ofp,"%s%s", output_buf, tmp_string);
   }       
   
   fprintf(ofp,"%s%s", SUB_HEADER_END, tmp_string);

   fflush(ofp);

   return(A_OK);
}

