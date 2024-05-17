/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.
*/

/*---------------------------------------------------------------------------*/

#define sockets_c

#include "sockets.h"

#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <posix_signal.h>

#include "panic.h"
#include "waislog.h"

/* In theory this is defined in stdio.h, but its hosed on SOLARIS */
extern FILE *fdopen(const int fd, const char *opt);

extern char *sys_errlist[];

#define QUEUE_SIZE (3L)
#define HOSTNAME_BUFFER_SIZE (120L)
#define MAX_RETRYS (10L)
#define TIMEOUT_CONNECT 5
/*---------------------------------------------------------------------------*/
#if !defined(IN_RMG) && !defined(PFS_THREADS)
static boolean 
clrSocket(struct sockaddr_in *address,long portnumber,long *sock)
{
  if (errno == EADDRINUSE) 
   { if (connect(*sock, address, sizeof (struct sockaddr_in)) == 0) 
      { close(*sock);
	waislog(WLOG_HIGH,WLOG_ERROR,
		"cannot bind port %ld (address already in use)",
		portnumber);
	waislog(WLOG_HIGH,WLOG_ERROR,
		"waisserver is already running on this system");
	panic("Exiting");
      } 
     else 
      { int one = 1;
	
	close(*sock);
	if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	  panic("Open socket failed in trying to clear the port.");
	if (setsockopt(*sock,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one)) < 0) 
	  ; /* do nothing? */
	address->sin_family = AF_INET;
	address->sin_addr.s_addr = INADDR_ANY;
	address->sin_port = htons(portnumber);
	if (bind(*sock, address, sizeof(*address)) == 0) 
	 ; /* do nothing? */
      }
   }
  return(true);
}
/*---------------------------------------------------------------------------*/

void
openServer(long port,long* fd,long size)
{ 
  struct sockaddr_in address;

  memset(&address, 0, sizeof(address));

  if ((*fd = socket(AF_INET,SOCK_STREAM,0)) < 0)
   { panic("can't get file descriptor for socket: %s", sys_errlist[errno]);
   }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);

  if (bind(*fd,(struct sockaddr*)&address,sizeof(struct sockaddr)) < 0)
    clrSocket(&address, port, fd);

  if (listen(*fd,QUEUE_SIZE) < 0)
    panic("can't open server: %s", sys_errlist[errno]);
}

/*---------------------------------------------------------------------------*/

void
fdAcceptClientConnection(long socket,long* fd)
{ 
  struct sockaddr_in source;
  int sourcelen;

#ifdef BSD
  struct in_addr {
    union {
      struct { u_char s_b1,s_b2,s_b3,s_b4; } S_un_b;
      u_long S_addr;
    } S_un;
  } addr_p;
#endif 

  sourcelen = sizeof(struct sockaddr_in);

  do {
       errno = 0;
       *fd = accept(socket, &source, &sourcelen);
     } while (*fd < 0 && errno == EINTR);

  if (source.sin_family == AF_INET) 
   { struct hostent *peer = NULL;

     peer = gethostbyaddr((char*)&source.sin_addr, 4, AF_INET);

     waislog(WLOG_MEDIUM,WLOG_CONNECT,"accepted connection from: %s [%s]",
	     (peer == NULL) ? "" : peer->h_name,
#if defined(sparc) && defined(__GNUC__) && !defined(__svr4__)
	     inet_ntoa(&source.sin_addr));
#else
             inet_ntoa(source.sin_addr));
#endif /* sparc */
   }
  if (*fd < 0)
    panic("can't accept connection");
}

/*---------------------------------------------------------------------------*/

void
acceptClientConnection(long socket,FILE** file)
{ 
  long fd; 
  fdAcceptClientConnection(socket,&fd);
  if ((*file = fdopen(fd,"r+")) == NULL)
    panic("can't accept connection");
}

/*---------------------------------------------------------------------------*/

void
closeClientConnection(FILE* file)
{ 
  fclose(file);
}

/*---------------------------------------------------------------------------*/

void
closeServer(long socket)
{
  close(socket);
}

#endif /*!IN_RMG && !PFS_THREADS*/
/*---------------------------------------------------------------------------*/


FILE *
connectToServer(char* host_name,long port)
{
  FILE* file;
  int fd;
  if ((fd = quick_open_tcp_stream(host_name, port, TIMEOUT_CONNECT)) == -1)
    return(NULL);

  if ((file = fdopen(fd,"r+")) == NULL) 
   { perror("Connect to socket did not work, fdopen failure");
     close(fd);
     return(NULL);
   }

  return(file);
}

/*---------------------------------------------------------------------------*/

void
closeConnectionToServer(FILE* file)
{
  fclose(file);
}

/*---------------------------------------------------------------------------*/


