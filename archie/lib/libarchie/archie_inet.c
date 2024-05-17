/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <memory.h>
#include "typedef.h"
#include "archie_inet.h"
#include "error.h"
#include "lang_libarchie.h"

#include "protos.h"

/*
 * cliconnect: connecto to given <serverhost> on <port>. <socket_fd> is a
 * pointer to an integer in which the socket file descriptor is returned
 */

con_status_t cliconnect( serverhost, port, socket_fd )
   hostname_t serverhost;  /* Host to connect to */
   int port;		   /* Port to connect to */
   int *socket_fd;	   /* file descriptor to return */

{
   extern int errno;
   extern time_t time PROTO((time_t *));


   int s ;
   struct hostent *sh ;
   struct sockaddr_in server ;

   if(serverhost == (char *) NULL){

     /* "serverhost is NULL" */

     error(A_INTERR, "cliconnect", CLICONNECT_001);
     return(CON_NULL_SERVERHOST);
   }

   if(socket_fd == (int *) NULL){

      /*  "socket descriptor is NULL" */

      error(A_INTERR, "cliconnect", CLICONNECT_002);
      return(CON_NULL_SOCKET);
   }

   if(( sh = gethostbyname( serverhost )) == (struct hostent *)NULL )
   {

      /*  "Error from gethostbyname() looking for %s" */

      error( A_ERR, "cliconnect", CLICONNECT_003, serverhost);
      return( CON_HOST_UNKNOWN ) ;
   }

   memset((char *)&server, 0, sizeof( struct sockaddr_in )) ;
   server.sin_family = AF_INET ;
   server.sin_addr.s_addr = ((struct in_addr *)( sh->h_addr))->s_addr ;
   server.sin_port = htons(port);

   if(( s = socket( AF_INET, SOCK_STREAM, 0 )) < 0 )
   {

      /* "Can't open socket" */

      error( A_SYSERR, "cliconnect", CLICONNECT_004);
      return( CON_SOCKETFAILED ) ;
   }

   if( connect( s, (struct sockaddr *) &server, sizeof( server )) < 0 )
   {

      /*  "Can't connect to server %s" */

      error( A_SYSERR, "cliconnect", CLICONNECT_005, serverhost);
      return( -errno ) ;
   }

   *socket_fd = s;
   
   return(A_OK) ;
}



/*
 * get_new_port: get a new port on the local machine. The initial port
 * number is randomly generated. If it is in use, go sequentially through
 * the ports until an unoccupied one is found
 */

status_t get_new_port(port, socket_fd)
   int *port;	     /* Port returned */
   int *socket_fd;   /* Socket descriptor on that port */

{
   extern int errno;
   extern int srand PROTO((int));
   extern time_t time PROTO((time_t *));

   static struct sockaddr_in myaddr ;
   int finished;
   int count;
   u16 myport;
   int conv;

   if((*socket_fd = socket(AF_INET,SOCK_STREAM,0)) == -1){

      /* "Can't open new socket. Error from socket()" */

      error(A_SYSERR,"get_new_socket", GET_NEW_PORT_001);
      return(ERROR);

   }

   srand(time((time_t *) NULL));

   for(finished = 0; !finished;){
      long i;

      i = rand() % 65521;

      myport = (u16) i;
      if(myport > 1024)
        finished = 1;
   }
   
   memset((char *)&myaddr, 0, sizeof( struct sockaddr_in )) ;
   myaddr.sin_family = AF_INET ;
   myaddr.sin_addr.s_addr = INADDR_ANY ;
   myaddr.sin_port = myport;

   for(count = 0, finished = 0;
      (!finished) && (count < DEFAULT_TRIES);
      count++){

      if(bind( *socket_fd, (struct sockaddr *) &myaddr, sizeof(struct sockaddr_in)) == -1){

	 if(errno == EADDRINUSE){
   	    myport = ++myaddr.sin_port;
	    continue;
	 }
	 else{

	    /* "Can't bind() socket %u" */
	    
	    error(A_SYSERR,"get_new_port", GET_NEW_PORT_002, *socket_fd);
	    return(ERROR);
	 }
      }
      else
         finished = 1;
   }

   if(!finished){

      /* "All addreses in use. Giving up after %u tries" */

      error(A_INTERR,"get_new_port", GET_NEW_PORT_003, count);
      return(ERROR);
   }

   if(listen(*socket_fd, SOCK_QUEUE ) == -1){

      /* "Can't listen() on socket" */

      error(A_SYSERR,"get_new_port", GET_NEW_PORT_004);
      return(ERROR);
   }

   conv = myport;

   *port = conv;

   return(A_OK);
}


/*
 * ipaddr_to_inet: Convert the internal representation of an IP address
 * (the ip_addr_t) to the external (system) structure. Result is stored in
 * static area.
 */


struct in_addr ipaddr_to_inet(ipaddr)
   ip_addr_t ipaddr;	/* address to be converted */

{
   static struct in_addr inaddr;

   memcpy(&inaddr, &ipaddr, sizeof(inaddr));

   return(inaddr);
}

/*
 * ipaddr_to_inet: Convert the internal representation of an IP address
 * (the ip_addr_t) to the external (system) structure. Result is stored in
 * static area.
 */


ip_addr_t inet_to_ipaddr(inaddr)
   struct in_addr *inaddr;

{
   static ip_addr_t ipaddr;

   memcpy(&ipaddr, inaddr, sizeof(inaddr));

   return(ipaddr);
}



	 
char *get_conn_err(retcode)
   int retcode;
{
   static pathname_t erstr;

   switch(retcode){

	 case -CON_SOCKETFAILED:

	    strcpy(erstr, "Can't get socket to connect");
	    break;
	    
	 case -CON_UNREACHABLE:
	    strcpy(erstr, "Host unreachable");
	    break;

	 case -CON_NETUNREACHABLE:
	    strcpy(erstr, "Network unreachable");
	    break;
      
	 case -CON_TIMEOUT:
	    strcpy(erstr, "Connection timeout");
	    break;

	 case -CON_HOST_UNKNOWN:
	    strcpy(erstr, "Host unknown");
	    break;

	 case -CON_REFUSED:
	    strcpy(erstr, "Connection refused");
	    break;

	 default:
	    sprintf(erstr, "Error code returned is %d", retcode);
	    break;
   }

   return(erstr);
}
   
