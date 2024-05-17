/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.
*/

/*---------------------------------------------------------------------------*/

#ifndef sockets_h
#define sockets_h

#include "cdialect.h"
#include "cutil.h"

#ifndef THINK_C
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif 

/*---------------------------------------------------------------------------*/

void openServer(long port,long* socket,long size);
void acceptClientConnection(long socket,FILE** file);
void closeClientConnection(FILE* file);
void closeServer(long socket);
FILE *connectToServer(char* hostname,long port);
void closeConnectionToServer(FILE* file);

/*---------------------------------------------------------------------------*/

#endif




