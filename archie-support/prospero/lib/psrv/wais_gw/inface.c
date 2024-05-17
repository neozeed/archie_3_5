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

#define INPROSPERO

#include "zutil.h"
#include "zprot.h"
#include "wprot.h"
#include "wmessage.h"
#include "inface.h"
#include "sockets.h"
#include "waislog.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef INPROSPERO
#include <pfs.h>
#include <perrno.h>
#include "buffalloc.h"
#endif
#include <stdlib.h>           /* For malloc and free */

/* from the Prospero library.  Also prototyped in <pfs.h> */
extern const char *unixerrstr(void); 

static char Err_Connect[]="WAIS gateway unable to connect to: %s(%s)";

#define CHARS_PER_PAGE 10000 /* number of chars retrieved in each request */
#define RETURN(val)	{retval = val; goto cleanup; }

#define WAIS_verbose FALSE
#define TIMEOUT_WAIS_READ 20

#ifdef COMMENT
main()
{
  char* server = "server.wais.com";
  char* service = "210";
  char* database = "smithsonian-pictures";
  char* query = "gorilla";
  WAISSearchResponse* response;
  WAISDocumentHeader* header;
  any* text;

  response = waisQuery(server, service, database, query);
  printf("%s\n", 
	p_err_string;
  );
  if (response != NULL) {
    header = response->DocHeaders[1];
    text = 
      waisRetrieve(server, service, database, 
		   header->DocumentID, 
		   "TEXT",   /* JFIF, THNL */
		   header->DocumentLength);
    printf("%s", text->bytes);
    printf("%s\n", p_err_string);
  }
}
#endif

static void showDiags(diagnosticRecord **d);

static long interpret_message(char* request_message,
			      long request_length, /* length of the buffer */
			      char* response_message,
			      long response_buffer_length,
			      FILE* connection,
			      boolean verbose);

static long transport_message(FILE* connection,
			      char* request_message,
			      long request_length,
			      char* response_message,
			      long response_buffer_length);

static long initConnection(char* inBuffer, 
			   char* outBuffer, 
			   long bufferSize, 
			   FILE* connection, 
			   char* userInfo);

static char* generate_search_apdu(char* buff,
				  long* buff_len,
				  char* seed_words,
				  char* database_name,
				  DocObj** docobjs,
				  long maxDocsRetrieved);

static char* generate_retrieval_apdu(char* buff,
				     long* buff_len,
				     any* docID,
				     long chunk_type,
				     long start,
				     long end,
				     char* type,
				     char* database_name);

/* return NULL pointer in event of an error */

WAISSearchResponse* waisQuery (char* server_name, 
			       char* service, 
			       char* database, 
			       char* keywords)
{
/*FIX:Mitra:leaks, malloc,p_err_string */
  char userInfo[500];
  char hostname[80];
  char domainname[80];
  WAISMSGBUFF request_msg = NULL; /* arbitrary message limit */
  WAISMSGBUFF response_msg = NULL; /* arbitrary message limit */
  char *request_message = NULL; 
  char *response_message = NULL;
  long request_buffer_length;	/* how of the request is left */
  SearchResponseAPDU  *query_response;
  WAISSearchResponse  *query_response_info;
  long Max_Docs = 40, message_length = MAX_MESSAGE_LENGTH;
  FILE *connection = NULL;
  WAISSearchResponse	*retval;

  if (server_name[0] == '\0' && service[0] == '\0')
    connection = NULL;		/* do local searching */
  else {			/* remote search, fill in defaults*/
        if (server_name[0] == '\0')
	/*default to local machine*/
	gethostname(server_name,MAX_SERVER_LENGTH);
	if (service[0] == '\0')
	  strcpy(service, Z39_50_SERVICE); /* default */
	if ((connection = connectToServer(server_name,atoi(service))) == NULL){ 
	  	p_err_string = qsprintf_stcopyr(p_err_string,
						Err_Connect,
		     server_name, service);
	    RETURN(NULL);
	  }
      }

  request_msg = waismsgbuff_alloc();  request_message = request_msg->buff;
  response_msg = waismsgbuff_alloc(); response_message = response_msg->buff;

  gethostname(hostname, 80);
  getdomainname(domainname, 80);

  sprintf(userInfo, "prospero gateway from host: %s.%s", hostname, domainname);

  if((message_length = 
      initConnection(request_message, 
		     response_message,
		     message_length,
		     connection,
		     userInfo)) <= 0) {
    p_err_string = qsprintf_stcopyr(p_err_string, Err_Connect,
	     server_name, service);
	RETURN(NULL);
  }

  request_buffer_length = message_length; /* how of the request is left */
  if(NULL ==
     generate_search_apdu(request_message + HEADER_LENGTH, 
			  &request_buffer_length, 
			  keywords, database, NULL, Max_Docs)) {
    p_err_string = qsprintf_stcopyr(p_err_string,
	     "Error creating search APDU: request too large");
    RETURN(NULL);
  }

  if(0 ==
     interpret_message(request_message, 
		       message_length - request_buffer_length, 
		       response_message,
		       message_length,
		       connection,
		       WAIS_verbose /* true verbose */
		       )) {	/* perhaps the server shut down on us */
	p_err_string = qsprintf_stcopyr(p_err_string, 
				"Unable to deliver message");
	RETURN(NULL);
  }

  readSearchResponseAPDU(&query_response, response_message + HEADER_LENGTH);
  query_response_info = 
    ((WAISSearchResponse *)query_response->DatabaseDiagnosticRecords);
  freeSearchResponseAPDU( query_response);
  RETURN (query_response_info);

cleanup:
  	if (connection) fclose(connection);
	waismsgbuff_free(request_msg);
	waismsgbuff_free(response_msg);
	return(retval);

}

/* Retrieve next part of DocumentID, assumes open connection to host */
int
waisRequestNext(FILE *connection,
	char *request_message, char *response_message,
	long message_length, any *DocumentID,
	char *DocumentType, char *database, 
	long count)
{
  long request_buffer_length;	/* how of the request is left */

    request_buffer_length = message_length; /* how often the request is left */
    if(0 ==
       generate_retrieval_apdu(request_message + HEADER_LENGTH,
			       &request_buffer_length, 
			       DocumentID, 
			       CT_byte,
			       count * CHARS_PER_PAGE,
			       (count + 1) * CHARS_PER_PAGE,
			       DocumentType,
			       database)) {
      p_err_string = qsprintf_stcopyr(p_err_string,
	      "Error generating retrieval APDU: request too long");
      return(-1);
    }
	     
    if(0 ==
       interpret_message(request_message, 
			 message_length - request_buffer_length, 
			 response_message,
			 message_length,
			 connection,
			 WAIS_verbose	/* true verbose */
			 )) {	/* perhaps the server shut down, let's see: */
	p_err_string = qsprintf_stcopyr(p_err_string,
		"Error delivering message");
	return(-1);
  }
    return(0);
}

any* waisRetrieve(char* server_name, 
		  char* service, 
		  char* database, 
		  any*  DocumentID,
		  char* DocumentType,
		  long  DocumentLength)
{
  /* docHeader = query_info->DocHeaders[document_number]; */
  char userInfo[500];
  char hostname[80];
  char domainname[80];
  WAISMSGBUFF request_msg = NULL;
  WAISMSGBUFF response_msg = NULL;
  SearchResponseAPDU* retrieval_response = NULL;
  WAISDocumentText* fragmentText;
  any* fragment = NULL;
  any* document_text = NULL;
  long count = 0, message_length = MAX_MESSAGE_LENGTH;
  FILE *connection = NULL;
  any *retval;

  if (server_name[0] == '\0' && service[0] == '\0')
    connection = NULL;		/* do local searching */
  else				/* remote search, fill in defaults*/
    { if (server_name[0] == '\0')
	/*default to local machine*/
	gethostname(server_name,MAX_SERVER_LENGTH);
	if (service[0] == '\0')
	  strcpy(service, Z39_50_SERVICE); /* default */
	if ((connection = connectToServer(server_name,atoi(service))) == NULL) 
	  { p_err_string = qsprintf_stcopyr(p_err_string, Err_Connect,
		     server_name, service);
	    RETURN (NULL);
	  }
      }

  request_msg = waismsgbuff_alloc(); 
  response_msg = waismsgbuff_alloc(); 

  gethostname(hostname, 80);
  getdomainname(domainname, 80);

  sprintf(userInfo, "prospero gateway from host: %s.%s", hostname, domainname);

  if((message_length = 
      initConnection(request_msg->buff, 
		     response_msg->buff,
		     message_length,
		     connection,
		     userInfo)) <= 0) {
    p_err_string = qsprintf_stcopyr(p_err_string, Err_Connect,
	     server_name, service);
    RETURN(NULL);
  }

  if (DocumentType == NULL) DocumentType = s_strdup("TEXT");

  /* we must retrieve the document in parts since it might be very long*/
  while (TRUE) {

    if (waisRequestNext(connection, request_msg->buff, 
	response_msg->buff,
	message_length, DocumentID, DocumentType, database, count)) {
      	freeAny(document_text);
      RETURN(NULL);
	   }
    readSearchResponseAPDU(&retrieval_response, 
			   response_msg->buff + HEADER_LENGTH);

    if(NULL == 
       ((WAISSearchResponse *)
	retrieval_response->DatabaseDiagnosticRecords)->Text) {

      p_err_string = qsprintf_stcopyr(p_err_string,
		"No text was returned");
	freeAny(document_text);
      return(NULL);
    } else {
      fragmentText = ((WAISSearchResponse *)
		      retrieval_response->DatabaseDiagnosticRecords)->Text[0];
      fragment = fragmentText->DocumentText;

      if (count == 0) {
	document_text = duplicateAny(fragment);
      } else {
	/* Increase document_text size to accomodate fragment. */
	if (!(document_text->bytes =
	  s_realloc(document_text->bytes, 
	    (size_t)(document_text->size + fragment->size) * sizeof(char)))) {
		/* May never get here, since can abort if no memory */
		p_err_string = qsprintf_stcopyr(p_err_string,
			"Unable to allocate space for %d bytes", 
			document_text->size+fragment->size * sizeof(char));
		freeAny(document_text);
		RETURN(NULL);
	}
	/* Copy the fragment into the document_text.  */
	memcpy(&(document_text->bytes[document_text->size]),
	       fragment->bytes,
	       (size_t) fragment->size * sizeof(char));
	/* Adjust the size field of document_text. */
	document_text->size = fragment->size + document_text->size;
      }
    }

    /* Under normal behaviour it will loop until it encounters a diagnostic
 	saying read beyond end of document, and then return */
    if(((WAISSearchResponse *)
	retrieval_response->DatabaseDiagnosticRecords)->Diagnostics != NULL) {
      showDiags(((WAISSearchResponse *)
		 retrieval_response->DatabaseDiagnosticRecords)->Diagnostics);
	p_err_string = qsprintf_stcopyr(p_err_string, "Diagnostic records received");
      RETURN(document_text);
    }
    count++;
    freeWAISSearchResponse( retrieval_response->DatabaseDiagnosticRecords); 
    freeSearchResponseAPDU( retrieval_response);
    retrieval_response = NULL;
  }
  RETURN(document_text);

cleanup:
  freeWAISSearchResponse( retrieval_response->DatabaseDiagnosticRecords); 
  freeSearchResponseAPDU( retrieval_response);
  	if (connection) fclose(connection);
	waismsgbuff_free(request_msg);
	waismsgbuff_free(response_msg);
	return(retval);
}


int waisRetrieveFile(
		  char* server_name, 		
		  char* service, 
		  char* database, 
		  any*  DocumentID,
		  char* DocumentType,
		  long  DocumentLength,
		  char* local)
{
  /* docHeader = query_info->DocHeaders[document_number]; */
  char userInfo[500];
  char hostname[80];
  char domainname[80];
  WAISMSGBUFF request_msg = NULL;
  WAISMSGBUFF response_msg = NULL;
  SearchResponseAPDU* retrieval_response = NULL;
  WAISDocumentText* fragmentText;
  any* fragment = NULL;
  any* document_text = NULL;
  long count = 0, message_length = MAX_MESSAGE_LENGTH;
  FILE *connection = NULL;
  int retval;
  int fd = -1;

  if (server_name[0] == '\0' && service[0] == '\0')
    connection = NULL;		/* do local searching */
  else				/* remote search, fill in defaults*/
    { if (server_name[0] == '\0')
	/*default to local machine*/
	gethostname(server_name,MAX_SERVER_LENGTH);
	if (service[0] == '\0')
	  strcpy(service, Z39_50_SERVICE); /* default */
	if ((connection = connectToServer(server_name,atoi(service))) == NULL) 
	  { p_err_string = qsprintf_stcopyr(p_err_string, Err_Connect,
		     server_name, service);
	    RETURN (-1);
	  }
      }

  request_msg = waismsgbuff_alloc(); 
  response_msg = waismsgbuff_alloc(); 

  gethostname(hostname, 80);
  getdomainname(domainname, 80);

  sprintf(userInfo, "prospero gateway from host: %s.%s", hostname, domainname);

  if((message_length = 
      initConnection(request_msg->buff, 
		     response_msg->buff,
		     message_length,
		     connection,
		     userInfo)) <= 0) {
    p_err_string = qsprintf_stcopyr(p_err_string, Err_Connect,
	     server_name, service);
    RETURN(-1);
  }

  if (DocumentType == NULL) DocumentType = s_strdup("TEXT");

  /* we must retrieve the document in parts since it might be very long*/
  while (TRUE) {

    if (waisRequestNext(connection, request_msg->buff, 
	response_msg->buff,
	message_length, DocumentID, DocumentType, database, count)) {
      	freeAny(document_text);
	RETURN(-1);
    }
    readSearchResponseAPDU(&retrieval_response, 
			   response_msg->buff + HEADER_LENGTH);

    if(NULL == 
       ((WAISSearchResponse *)
	retrieval_response->DatabaseDiagnosticRecords)->Text) {

      	p_err_string = qsprintf_stcopyr(p_err_string,
		"No text was returned");
	freeAny(document_text);
      	RETURN(-1);
    } else {
      	fragmentText = ((WAISSearchResponse *)
		      retrieval_response->DatabaseDiagnosticRecords)->Text[0];
      	fragment = fragmentText->DocumentText;

      	if (fd == -1) {
		if ((fd = open(local,O_WRONLY|O_CREAT|O_TRUNC,
		       S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH))
		    == -1) {
			p_err_string = qsprintf_stcopyr(p_err_string,
				"Unable to open %s: %s", local, unixerrstr());
			RETURN(-1);
		}
      	}
      	if ((write(fd,fragment->bytes,fragment->size * sizeof(char))) == -1) {
		p_err_string = qsprintf_stcopyr(p_err_string,
			"Unable to write %d characters to %s: %s", 
			fragment->size,local,unixerrstr());
		RETURN(-1);
	}

    }


    /* Under normal behaviour it will loop until it encounters a diagnostic
 	saying read beyond end of document, and then return */
    if(((WAISSearchResponse *)
	retrieval_response->DatabaseDiagnosticRecords)->Diagnostics != NULL) {
      showDiags(((WAISSearchResponse *)
		 retrieval_response->DatabaseDiagnosticRecords)->Diagnostics);
	p_err_string = qsprintf_stcopyr(p_err_string, "Diagnostic records received");
      RETURN(0);
    }
    count++;
    freeWAISSearchResponse( retrieval_response->DatabaseDiagnosticRecords); 
    freeSearchResponseAPDU( retrieval_response);
    retrieval_response = NULL;
  }
  RETURN(0);

cleanup:
    if (fd != -1) close(fd);
		/* Dont check error - may fail if other error)*/
    if (retrieval_response) {
    	freeWAISSearchResponse( retrieval_response->DatabaseDiagnosticRecords); 
    	freeSearchResponseAPDU( retrieval_response);
    }
    if (connection) fclose(connection);
    waismsgbuff_free(request_msg);
    waismsgbuff_free(response_msg);
    return(retval);
}
/* returns a pointer into the buffer of the next free byte.
   if it overflowed, then NULL is returned
 */

char *
generate_retrieval_apdu(buff,
			buff_len,
			docID,
			chunk_type,
			start,
			end,
			type,
			database_name)
char *buff;
long *buff_len;  /* length of the buffer changed to reflect new data written */
any *docID;
long chunk_type;
long start;
long end;
char *type;
char *database_name;
{
  SearchAPDU *search;
  char  *end_ptr;

  char *database_names[2];
  char *element_names[3];
  any refID;

  DocObj *DocObjs[2];
  any *query;			/* changed from char* by brewster */

  if(NULL == type)
    type = s_strdup("TEXT");

  database_names[0] = database_name;
  database_names[1] = NULL;

  element_names[0] = " ";
  element_names[1] = ES_DocumentText;
  element_names[2] = NULL;

  refID.size = 1;
  refID.bytes = "3";
  
  switch(chunk_type){
  case CT_line: 
    DocObjs[0] = makeDocObjUsingLines(docID, type, start, end);
    break;
  case CT_byte:
    DocObjs[0] = makeDocObjUsingBytes(docID, type, start, end);
    break;
  }
  DocObjs[1] = NULL;

  query = makeWAISTextQuery(DocObjs);   
  search = makeSearchAPDU( 10L, 16L, 15L, 
			  1L,	/* replace indicator */
			  "FOO", /* result set name */
			  database_names, /* database name */   
			  QT_TextRetrievalQuery, /* query_type */
			  element_names, /* element name */
			  &refID, /* reference ID */
			  query);
  end_ptr = writeSearchAPDU(search, buff, buff_len);
  /* s_free(DocObjs[0]->Type); it's a copy of the input, don't free it! */
  CSTFreeDocObj(DocObjs[0]);
  CSTFreeWAISTextQuery(query);
  freeSearchAPDU(search);
  return(end_ptr);
}


long initConnection(char* inBuffer, 	/*!! Large malloced buffer */
		    char* outBuffer, 	/*!! Large malloced bugger */
		    long bufferSize, 
		    FILE* connection, 
		    char* userInfo)
{ 
  InitAPDU* init = NULL;
  InitResponseAPDU* reply = NULL;
  long result;
  long retval;

  /* construct an init APDU */
  init = makeInitAPDU(true,false,false,false,false,bufferSize,bufferSize,
		      userInfo,defaultImplementationID(),
		      defaultImplementationName(),
		      defaultImplementationVersion(),
		      NULL,userInfo);
  /* write it to the buffer */
  if ((result = 
	writeInitAPDU(init,inBuffer+HEADER_LENGTH,&bufferSize) - inBuffer
      ) <0)
	RETURN(-1);

  if (interpret_message(inBuffer,
		       result - HEADER_LENGTH,
		       outBuffer,
		       bufferSize,
		       connection,
		       WAIS_verbose	/* true verbose */	
		       ) == 0) 
    /* error making a connection */
    RETURN (-1);

  if ((readInitResponseAPDU(&reply,outBuffer + HEADER_LENGTH) == NULL) ||
      (reply->Result == false))
      RETURN(-1);

  /* we got a response back */
  result = reply->MaximumRecordSize;
  RETURN(result);
   
cleanup:
  if (reply) {
      freeWAISInitResponse((WAISInitResponse*)reply->UserInformationField);
      freeInitResponseAPDU(reply);
  }
	freeInitAPDU(init);
	return(retval);
}


/* returns a pointer in the buffer of the first free byte.
   if it overflows, then NULL is returned 
 */
char*
generate_search_apdu(char* buff, /* buffer to hold the apdu */
		     long* buff_len, /* buffer length changed for new data */
		     char* seed_words, /* string of the seed words */
		     char* database_name,	/* copied */
		     DocObj** docobjs,
		     long maxDocsRetrieved)
{
  /* local variables */

  SearchAPDU *search3;
  char  *end_ptr;
  /* This is copied, nothing gained if static*/
  char *database_names[2] = {"", 0};	
  any refID;
  WAISSearch *query;
  refID.size = 1;
  refID.bytes = "3";

  database_names[0] = database_name;
  query = makeWAISSearch(seed_words,	/*pointed at */
                         docobjs, /* DocObjsPtr */
                         0L,
                         1L,     /* DateFactor */
                         0L,     /* BeginDateRange */
                         0L,     /* EndDateRange */
                         maxDocsRetrieved
                         );

  search3 = makeSearchAPDU(30L, 
			   5000L, /* should be large */
			   30L,
                           1L,	/* replace indicator */
                           "",	/* result set name */
                           database_names, /* database name  (copied)*/   
                           QT_RelevanceFeedbackQuery, /* query_type */
                           0L,   /* element name */
                           NULL, /* reference ID */
                           query);

  end_ptr = writeSearchAPDU(search3, buff, buff_len);

  CSTFreeWAISSearch(query);
  freeSearchAPDU(search3);  /* frees copy of database_names */
  return(end_ptr);
}


/* returns the number of bytes written.  0 if an error */
long
interpret_message(char* request_message,
		  long request_length, /* length of the buffer */
		  char* response_message,
		  long response_buffer_length,
		  FILE* connection,
		  boolean verbose)
{
  long response_length;

  writeWAISPacketHeader(request_message,
			request_length,
			(long)'z',	/* Z39.50 */
			"wais      ", /* server name */
			(long)NO_COMPRESSION,	/* no compression */
			(long)NO_ENCODING,(long)HEADER_VERSION);

  if(connection != NULL) {
    if(0 == 
       (response_length =
	transport_message(connection, request_message,
			  request_length,
			  response_message,
			  response_buffer_length)))
      return(0);
  }
  else{
    p_err_string = qsprintf_stcopyr(p_err_string,
	    "Local search not supported in this version");
    return(0);
  }

#if !defined(IN_RMG) && !defined(PFS_THREADS)
  if(verbose){
    printf ("decoded %ld bytes: \n", response_length);
    twais_dsply_rsp_apdu(response_message + HEADER_LENGTH, 
			 request_length);
  }
#endif

  return(response_length);
}


/* this is a safe version of unix 'read' it does all the checking
 * and looping necessary
 * to those trying to modify the transport code to use non-UNIX streams:
 *  This is the function to modify!
 */
long read_from_stream(d,buf,nbytes)
long d;				/* this is the stream */
char *buf;
long nbytes;
{
  long didRead;
  long toRead = nbytes;
  long totalRead = 0;		/* paranoia */

  while (toRead > 0){
    didRead = quick_read (d, buf, toRead, TIMEOUT_WAIS_READ);
    if(didRead == -1)		/* error*/
      return(-1);
    if(didRead == 0)		/* eof */
      return(-2);		/* maybe this should return 0? */
    toRead -= didRead;
    buf += didRead;
    totalRead += didRead;
  }
  if(totalRead != nbytes)	/* we overread for some reason */
    return(- totalRead);	/* bad news */    
  return(totalRead);
}


/* returns the length of the response, 0 if an error */

long 
transport_message(FILE* connection,
		  char* request_message,
		  long request_length,
		  char* response_message,
		  long response_buffer_length)
{
  WAISMessage header;
  long response_length;

  
  /* Write out message. Read back header. Figure out response length. */
  
  if( request_length + HEADER_LENGTH
     != fwrite (request_message, 1L, request_length + HEADER_LENGTH, connection))
    return 0;

  fflush(connection);

  /* read for the first '0' */

  while(1){
    if(0 > read_from_stream(fileno(connection), response_message, 1))
      return 0;
    if('0' == response_message[0])
      break;
  }

  if(0 > read_from_stream(fileno(connection), 
				  response_message + 1, 
				  HEADER_LENGTH - 1))
    return 0;

  readWAISPacketHeader(response_message, &header);
  {
    char length_array[11];
    strncpy(length_array, header.msg_len, 10);
    length_array[10] = '\0';
    response_length = atol(length_array);
    /*
      if(verbose){
      printf("WAIS header: '%s' length_array: '%s'\n", 
      response_message, length_array);
      }
      */
    if(response_length > response_buffer_length){
      /* we got a message that is too long, therefore empty the message out,
	 and return 0 */
      long i;
      for(i = 0; i < response_length; i++){
	read_from_stream(fileno(connection), 
			 response_message + HEADER_LENGTH,
			 1);
      }
      return(0);
    }
  }
  if(0 > read_from_stream(fileno(connection), 
				  response_message + HEADER_LENGTH,
				  response_length))
    return 0;
  return(response_length);
}

/* modified from Jonny G's version in ui/question.c */
void showDiags(d)
diagnosticRecord **d;
{
  long i;

  for (i = 0; d[i] != NULL; i++) {
    if (d[i]->ADDINFO != NULL) {
      p_err_string = qsprintf_stcopyr(p_err_string,
		"Code: %s, %s", d[i]->DIAG, d[i] ->ADDINFO);
    }
  }
}

/* Convert docid into newly stalloc-ed DocumentId */
any *
un_urlascii(char *docid)
{
    int		i=0;
    int		j=0;	/* Pointer into string for writing */
    any		*DocumentId;

    DocumentId = (any *)malloc(sizeof(any));
    if (!DocumentId) out_of_memory();
    /* Converted string cant be longer than docid */
    DocumentId->bytes = stalloc(strlen(docid)+1);
    while (docid[i]) {
	if (docid[i] != '%') {
		DocumentId->bytes[j++]=docid[i++];
	} else {
		int conv;
		i++;
		sscanf(&docid[i],"%2x",&conv);
		DocumentId->bytes[j++] = (char)conv;	
		i += 2;
    }
  }
    DocumentId->size = j;
    return DocumentId;
}


int
waisRetrieveFileByHsoname(char *local,char *hsoname)
{
        any *DocumentId = NULL;
	char *host = NULL;
	char *port = NULL;
	char *type = NULL;
	char *database = NULL;
	char *query = NULL;
	int tmp;
	int retval;
   tmp = qsscanf(hsoname, "WAIS-GW/%&[^(](%&[^)])/%&[^/]/%&[^/]/%&[^/]/%[^\n]",
              &host, &port, &type, &database, &query);
	if (tmp != 5) {
		p_err_string = qsprintf_stcopyr(p_err_string,
			"Invalid WAIS hsoname: %s", hsoname);
		RETURN(-1);
	}
	DocumentId = un_urlascii(query);
	RETURN(waisRetrieveFile(host,port,database,
		DocumentId,type,0,local));

cleanup:
	if (DocumentId) {
	  stfree(DocumentId->bytes);
	  free(DocumentId);
	}
	stfree(host);
	stfree(port);
	stfree(type);
	stfree(database);
	stfree(query);
	return(retval);
}
