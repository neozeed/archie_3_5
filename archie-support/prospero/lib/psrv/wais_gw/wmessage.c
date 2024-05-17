/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.
*/

/*----------------------------------------------------------------------*/

#include <string.h>
#include "wmessage.h"
#include "cutil.h"

/*---------------------------------------------------------------------*/

void 
readWAISPacketHeader(msgBuffer,header_struct)
char* msgBuffer;
WAISMessage *header_struct;
{
  
		    
  memmove(header_struct->msg_len,msgBuffer,(size_t)10); 
  header_struct->msg_type = char_downcase((unsigned long)msgBuffer[10]);
  header_struct->hdr_vers = char_downcase((unsigned long)msgBuffer[11]);
  memmove(header_struct->server,(void*)(msgBuffer + 12),(size_t)10);
  header_struct->compression = char_downcase((unsigned long)msgBuffer[22]);
  header_struct->encoding = char_downcase((unsigned long)msgBuffer[23]);
  header_struct->msg_checksum = char_downcase((unsigned long)msgBuffer[24]);
}
 
/*---------------------------------------------------------------------*/

long
getWAISPacketLength(header)
WAISMessage* header;

{ 
  char lenBuf[11];
  memmove(lenBuf,header->msg_len,(size_t)10);
  lenBuf[10] = '\0';
  return(atol(lenBuf));
}

/*---------------------------------------------------------------------*/

#ifdef NOTUSEDYET

static char checkSum _AP((char* string,long len));

static char
checkSum(string,len)
char* string;
long len;

{
  register long i;
  register char chSum = '\0';
	  
  for (i = 0; i < len; i++)
    chSum = chSum ^ string[i];
	    
  return(chSum);
}	
#endif 


 
void
writeWAISPacketHeader(header,
		      dataLen,
		      type,
		      server,
		      compression,
		      encoding,
		      version)
char* header;
long dataLen;
long type;
char* server;
long compression;
long encoding;
long version;

{
  char lengthBuf[11];
  char serverBuf[11];

  long serverLen = strlen(server);
  if (serverLen > 10)
    serverLen = 10;

  sprintf(lengthBuf, "%010ld", dataLen);  
  strncpy(header,lengthBuf,10);

  header[10] = type & 0xFF; 
  header[11] = version & 0xFF;

  strncpy(serverBuf,server,serverLen);       
  strncpy((char*)(header + 12),serverBuf,serverLen);

  header[22] = compression & 0xFF;    
  header[23] = encoding & 0xFF;    
  header[24] = '0'; 	
}              
              
/*---------------------------------------------------------------------*/




