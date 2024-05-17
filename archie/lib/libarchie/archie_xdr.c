/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#include <stdio.h>
#include <sys/types.h>

#if defined(AIX) || defined(SOLARIS)
#include <string.h>
#include <rpc/types.h>
#include <stdlib.h>
#endif

#include <rpc/xdr.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "typedef.h"
#include "db_ops.h"
#include "misc.h"
#include "error.h"
#include "lang_libarchie.h"


/*
 * This file contains routines for the archie XDR manipulation. Note that
 * much of the XDR routines themselves are in the files where they are used
 */


#if 0

/* read_xdr_input: read the XDR stream. Used by other XDR routines */


int read_xdr_input(fd, buf, len)
   void *fd;
   char *buf;
   int len;
{
   int bytes_read;

   if((bytes_read = read((int) fd, buf, len)) == -1){

      error(A_SYSERR,"read_xdr_input","reading input for xdr stream");
      return(-1);
   }

   return(bytes_read);
}

/* write_xdr_output: write an XDR stream. Used by other XDR routines */


int write_xdr_output( fd, buf, len )
   void *fd;
   char *buf;
   int len;

{
   int bytes_written;

   if((bytes_written = write((int) fd, buf, len)) == -1){

      error(A_SYSERR,"write_xdr_output","writing xdr stream");
      return(-1);
   }

   return(bytes_written);
}

#endif


/*
 * open_xdr_stream: the given file pointer <fp> is opened for <op>
 */



XDR *open_xdr_stream( fp, op)
   FILE *fp;
   enum xdr_op op;

{

   XDR *xdrs;

   if((xdrs = (XDR *) malloc(sizeof(XDR))) == (XDR *)NULL){

      /*  "Can't malloc space for xdr structure" */

      error(A_ERR, "open_xdr_stream", OPEN_XDR_STREAM_001);
      return((XDR *) NULL);
   }

   memset((char *) xdrs, '\0', sizeof(XDR));

   xdrstdio_create(xdrs, fp, op);

   return(xdrs);

}


/*
 * close_xdr_stream: close the XDR stream and free the associated space
 */


void close_xdr_stream(xdrs)
   XDR *xdrs;

{
   if(xdrs == (XDR *) NULL){

      /* "Can't close XDR stream" */

      error(A_ERR, "close_xdr_stream", CLOSE_XDR_STREAM_001);
      return;
   }

  xdr_destroy(xdrs);
  free(xdrs);
}

#if 0
bool_t xdr_header_t(XDR *xdrs, header_t *header_rec)

{
   char *tmp_string;

   if(! xdr_gen_prog_t(xdrs, &(header_rec -> generated_by)))
     return(FALSE);

   if(xdr_hostname_t(xdrs, &tmp_string) == FALSE)
      return(FALSE);

   strcpy(header_rec -> source_archie_host, tmp_string);

   xdr_free(xdr_string, tmp_string);

   if(xdr_hostname_t(xdrs, &tmp_string) == FALSE)   
      return(FALSE);

   strcpy(header_rec -> primary_hostname, tmp_string);

   if( xdrs -> x_op == XDR_DECODE)
      xdr_free(xdr_string, tmp_string);

   if(xdr_hostname_t(xdrs, &tmp_string) == FALSE)   
      return(FALSE);

   strcpy(header_rec -> preferred_hostname, tmp_string);

   if( xdrs -> x_op == XDR_DECODE)
      xdr_free(xdr_string, tmp_string);

   if(! xdr_ip_addr_t(xdrs, &(header_rec -> primary_ipaddr)))
     return(FALSE);

   if(! xdr_access_method_t(xdrs, &(header_rec -> access_method)))
     return(FALSE);

   if(xdr_access_comm_t(xdrs, &tmp_string) == FALSE)   
      return(FALSE);

   strcpy(header_rec -> access_command, tmp_string);

   if( xdrs -> x_op == XDR_DECODE)
      xdr_free(xdr_string, tmp_string);

   if(! xdr_os_type_t(xdrs, &(header_rec -> os_type)))
     return(FALSE);

   if(! xdr_timezone_t(xdrs, &(header_rec -> timezone)))
     return(FALSE);

   if(! xdr_date_time_t(xdrs, &(header_rec -> retrieve_time)))
     return(FALSE);

   if(! xdr_date_time_t(xdrs, &(header_rec -> parse_time)))
     return(FALSE);

   if(! xdr_date_time_t(xdrs, &(header_rec -> update_time)))
     return(FALSE);

   if(! xdr_index_t(xdrs, &(header_rec -> no_recs)))
     return(FALSE);
}

#endif
