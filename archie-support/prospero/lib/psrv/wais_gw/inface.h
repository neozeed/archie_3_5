/*****************************************************************************
* 	        (c) Copyright 1992 Wide Area Information Servers, Inc        *
*	   	  of California.   All rights reserved.   	             *
*									     *
*  This notice is intended as a precaution against inadvertent publication   *
*  and does not constitute an admission or acknowledgement that publication  *
*  has occurred or constitute a waiver of confidentiality.		     *
*									     *
*  Wide Area Information Server software is the proprietary and              *
*  confidential property of Wide Area Information Servers, Inc.              *
*****************************************************************************/

#ifndef interface_h
#define interface_h

#include "wprot.h"		/* For WAISSearchResponse */
#define Z39_50_SERVICE "210"
#define MAX_MESSAGE_LENGTH 100000
#define MAX_SERVER_LENGTH 1000

WAISSearchResponse* waisQuery (char* server_name, 
			       char* service, 
			       char* database, 
			       char* keywords);

any* waisRetrieve(char* server_name, 
		  char* service, 
		  char* database, 
		  any*  DocumentID,
		  char* DocumentType,
		  long  DocumentLength);
extern any *un_urlascii(char *docid);
#endif 

/*---------------------------------------------------------------------------*/
