/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.
*/

/*----------------------------------------------------------------------*/

#ifndef WMESSAGE_H
#define WMESSAGE_H

#include "cdialect.h"

typedef struct wais_header {
        char    msg_len[10];    
        char    msg_type;       
        char    hdr_vers;       
        char    server[10];     
        char    compression;    
        char    encoding;       
        char    msg_checksum;   
        } WAISMessage;

#define HEADER_LENGTH 	25	

#define HEADER_VERSION 	(long)'2'


#define Z3950		'z'  
#define ACK			'a'  
#define	NAK			'n'  


#define NO_COMPRESSION 		' ' 
#define UNIX_COMPRESSION 	'u' 


#define NO_ENCODING		' '  
#define HEX_ENCODING	'h'  
#define IBM_HEXCODING	'i'	 
#define UUENCODE		'u'  

#ifdef __cplusplus

extern "C"
	{
#endif 

void readWAISPacketHeader _AP((char* msgBuffer,WAISMessage *header_struct));
long getWAISPacketLength _AP((WAISMessage* header));
void writeWAISPacketHeader _AP((char* header,long dataLen,long type,
				char* server,long compression,	
				long encoding,long version));

#ifdef __cplusplus
	}
#endif 

#endif 
