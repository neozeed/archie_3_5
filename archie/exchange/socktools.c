#include <stdio.h>
#include <sys/types.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "error.h"

#define SOCK_QUEUE 5


static int addrlen, ls ;
static struct sockaddr_in cliaddr ;


/*
   Close the listen socket.  This might be called by a signal handler.
*/

void clservsock()
{
   close( ls ) ;
}





/*
   Attempt to make a socket for the server.  If we are successful then return
 the file ( socket ) descriptor.  Otherwise, return a negative number.
*/

int mkservsock( port )
   int *port;

{
   struct sockaddr_in myaddr ;


   memset((char *)&myaddr, 0, sizeof( struct sockaddr_in )) ;
   memset((char *)&cliaddr, 0, sizeof( struct sockaddr_in )) ;
   myaddr.sin_family = AF_INET ;
   myaddr.sin_addr.s_addr = INADDR_ANY ;
   myaddr.sin_port = *port;

   if(( ls = socket( AF_INET, SOCK_STREAM, 0 )) < 0 )
   {
      fprintf( stderr, "mkservsock: can't open listen socket\n" ) ;
      return( -1 ) ;
   }

   if( bind( ls, &myaddr, sizeof( struct sockaddr_in )) < 0 )
   {
      fprintf( stderr, "mkservsock: can't bind address to listen socket\n" ) ;
      close( ls ) ;
      return( -1 ) ;
   }

   if( listen( ls, SOCK_QUEUE ) < 0 )
   {
      fprintf( stderr, "mkservsock: error listening\n" ) ;
      close( ls ) ;
      return( -1 ) ;
   }

   return( 0 ) ;
}



/*
   Check the listen socket for more requests, then accept one.
*/

int accept_req()
{
   int s ;


   if(( s = accept( ls, &cliaddr, &addrlen )) < 0 )
      fprintf( stderr, "accept_req: error accepting\n" ) ;

   return( s ) ;
}



/*
   From a socket, read a null terminated string or 'nchars' characters,
   whichever comes first.  Upon success return the length of the string
   ( including the null ).  Return 0 on end of file, and the returned error
   value on an error.
*/

int readsock( s, str, nchars )
int s ;
char *str ;
{
   int lastchar = 0 ;

   do
   {
     int n ;

     if(( n = read( s, str + lastchar, nchars )) < 1 )
        return( n ) ;
     nchars -= n ;
     lastchar += n ;
   }
   while( *( str + lastchar ) != '\0' ) ;

   return( lastchar ) ;
}
