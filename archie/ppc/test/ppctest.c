/*  
 *  Usage: ppctest [-host <hosname>] -port <portnum-or-service>
 *  
 *  For each line read from stdin, open a connection to the given port, write
 *  the line to the socket, read from the socket until EOF is seen, then
 *  close the connection.
 */  

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ppc.h"


#define NEXTARG if ( ! (av++, --ac)) usage()
#define NSETARG(var) NEXTARG; var = atoi(av[0])
#define SETARG(var) NEXTARG; var = av[0]


static int myconnect proto_((const char *serverhost, const char *service, FILE **ifp, FILE **ofp));
#if 0
static int readsockstr proto_((int s, char *str, int nchars));
static int writesockstr proto_((int s, const char *str));
#endif
static void usage proto_((void));


static char *bigbuf[4096];
static const char *prog;


int main(ac, av)
  int ac;
  char **av;
{
  char *port = 0;
  char *host = "localhost";
  char line[1024];
  
  prog = av[0];
  while (av++, --ac)
  {
    if (av[0][0] != '-')
    {
      usage();
    }
    else
    {
      if (strcmp(av[0], "-host") == 0)
      {
        SETARG(host);
      }
      else if (strcmp(av[0], "-port") == 0)
      {
        SETARG(port);
      }
      else
      {
        usage();
      }
    }
  }

  if ( ! port)
  {
    usage();
  }
  
  while (fgets(line, sizeof line, stdin))
  {
    FILE *ifp;
    FILE *ofp;
    
    if ( ! myconnect(host, port, &ifp, &ofp))
    {
      fprintf(stderr, "%s: myconnect() failed.\n", prog); sleep(1);
    }
    else
    {
      char *nl = strchr(line, '\n');
      if (nl) *nl = '\0';

      if (fprintf(ofp, "%s%sAccept: */*%s%s", line, CRLF, CRLF, CRLF) == EOF)
      {
        fprintf(stderr, "%s: fprintf(ofp, ...) failed: ", prog);
        perror(""); sleep(1);
      }
      else
      {
        fflush(ofp);
        while (fgets(line, sizeof line, ifp))
        {
          char *cr = strchr(line, '\r');
          if (cr) *cr = '\0';

          if ( ! *line) break;
          printf("%s\n", line);
        }

        while (fread(bigbuf, 1, sizeof bigbuf, ifp) > 0)
        { continue; }
      }

      fclose(ifp); fclose(ofp);
    }
  }

  exit(0);
}


/*  
 *  `service' can be either a name in /etc/services, or a port number.
 */    
static int myconnect(serverhost, service, ifp, ofp)
  const char *serverhost;
  const char *service ;
  FILE **ifp;
  FILE **ofp;
{
  int s ;
  short port;
  struct hostent *sh ;
  struct servent *ss ;
  struct sockaddr_in server ;

  if ( ! serverhost || ! service)
  {
    fprintf(stderr, "%s: myconnect: invalid null argument.\n", prog);
    return 0;
  }
  if ( ! (sh = gethostbyname(serverhost)))
  {
    fprintf(stderr, "%s: myconnect: error from gethostbyname.\n", prog);
    return 0;
  }
  if ((ss = getservbyname(service, "tcp")))
  {
    port = ss->s_port;
  }
  else if ((port = ntohs(atoi(service))) == 0)
  {
    fprintf(stderr, "%s: myconnect: bad port `%s'.\n", prog, service);
    return 0;
  }

  memset((char *)&server, 0, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET ;
  server.sin_addr.s_addr = ((struct in_addr *)(sh->h_addr))->s_addr ;
  server.sin_port = port;

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    fprintf(stderr, "%s: myconnect: can't open socket.\n", prog);
    return 0;
  }
  if (connect(s, (struct sockaddr *)&server, sizeof server) == -1)
  {
    fprintf(stderr, "%s: myconnect: can't connect to server: ", prog);
    perror("");
    close(s);
    return 0;
  }
  if ( ! (*ifp = fdopen(s, "r")))
  {
    fprintf(stderr, "%s: myconnect: can't open read file pointer on fd %d.\n",
            prog, s);
    close(s);
    return 0;
  }
  if ( ! (*ofp = fdopen(s, "w")))
  {
    fprintf(stderr, "%s: myconnect: can't open write file pointer on fd %d.\n",
            prog, s);
    fclose(*ifp);
    close(s);
    return 0;
  }

  return 1;
}


#if 0
/*  
 *  From a socket, read a null terminated string or 'nchars' characters,
 *  whichever comes first.  Upon success return the length of the string (
 *  including the null ).  Return 0 on end of file, and the returned error
 *  value on an error.
 */  
static int readsockstr(s, str, nchars)
  int s;
  char *str;
  int nchars;
{
  int lastchar = 0;

  do
  {
    int n ;

    if ((n = recv(s, str + lastchar, nchars, 0)) < 1)
    return n;
    nchars -= n;
    lastchar += n;
  }
  while(*(str + lastchar - 1) != '\0');

#ifdef DEBUG
  fprintf( stderr, "ts-readsockstr: read [%s]\n", str ) ;
#endif

  return lastchar;
}
#endif


#if 0
/*  
 *  Write a string to a socket, including the null terminator.  Compliments
 *  readsockstr.
 */  
static int writesockstr(s, str)
  int s ;
  const char *str ;
{
#ifdef DEBUG
  int tmp ;

  tmp = write(s, str, strlen(str) + 1);
  fprintf(stderr, "%s: ts-writesockstr: wrote [%s].\n", str, prog);
  return tmp;
#else
  return write(s, str, strlen(str) + 1);
#endif
}
#endif


static void usage()
{
  fprintf(stderr, "Usage: %s [-host <hostname>] -port <port-or-service>\n", prog);
  exit(1);
}
