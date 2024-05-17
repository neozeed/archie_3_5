#ifdef __STDC__
# include <stdlib.h>
# include <unistd.h>
#else
# include <memory.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "error.h"
#include "misc.h"
#include "protos.h"
#include "str.h"
#include "all.h"


extern int errno;
extern char *sys_errlist[];


/*  
 *  Return the numeric value of the port, in network byte order.
 *
 *  Return 0 upon failure.
 */  

static unsigned short nportnum(service, protocol)
  const char *service;
  const char *protocol;
{
  struct servent *pse;
  unsigned short port;

  if ((pse = getservbyname(service, protocol)))
  {
    port = (unsigned short)pse->s_port;
  }
  else if ((port = htons((unsigned short)atoi(service))) == 0)
  {
    efprintf(stderr, "%s: nportnum: can't get port for `%s/%s'.\n",
             logpfx(), service, protocol);
  }

  return port;
}


int getPeer(s, saddr)
  int s;
  struct sockaddr_in *saddr;
{
  int alen = sizeof (struct sockaddr);
  int ret;

  memset((char *)saddr, 0, alen);
  if ( ! (ret = (getpeername(s, (struct sockaddr *)saddr, &alen) != -1)))
  {
    efprintf(stderr, "%s: get_peer: getpeername: %m.\n", logpfx());
  }

  return ret;
}


/*  
 *  Open a server socket.
 *  
 *  From "Internetworking with TCP/IP" by Comer & Stevens, p. 117.
 */  

int passivesock(service, protocol, qlen)
  char *service;
  char *protocol;
  int qlen;
{
  struct protoent *ppe;
  struct sockaddr_in sin;
  int s;
  int type;

  memset((char *)&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;

  if ((sin.sin_port = nportnum(service, protocol)) == 0)
  {
    efprintf(stderr, "%s: passivesock: can't get port number for `%s'.\n",
             logpfx(), service);
    return -1;
  }

  if ((ppe = getprotobyname(protocol)) == 0)
  {
    efprintf(stderr, "%s: passivesock: can't get protocol entry for `%s'.\n",
             logpfx(), protocol);
    return -1;
  }

  if (strcmp(protocol, "udp") == 0)
  {
    type = SOCK_DGRAM;
  }
  else
  {
    type = SOCK_STREAM;
  }

  s = socket(PF_INET, type, ppe->p_proto);
  if (s < 0)
  {
    efprintf(stderr, "%s: passivesock: can't create socket: %m.\n", logpfx());
    return -1;
  }

  if (debug || reuseaddr)
  {
    int bool = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&bool, sizeof bool) == -1)
    {
      efprintf(stderr, "%s: passivesock: setsockopt() failed: ", logpfx());
      perror("");
    }
  }
  
  if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    efprintf(stderr, "%s: passivesock: can't bind socket to port (%s): %m.\n",
             logpfx(), service);
    return -1;
  }

  if (type == SOCK_STREAM && listen(s, qlen) < 0)
  {
    efprintf(stderr, "%s: passivesock: error listen()ing (%s): %m.\n",
             logpfx(), service);
    return -1;
  }

  return s;
}


int passiveTCP(service, qlen)
  char *service;
  int qlen;
{
  return passivesock(service, "tcp", qlen);
}


int portnumTCP(service)
  const char *service;
{
  return (int)ntohs(nportnum(service, "tcp"));
}


/*  
 *  Return a pointer to the (static) ASCII representation of the peer's IP
 *  address.
 *  
 *  Return NULL on error.
 */    

const char *sockAddr(s)
  int s;
{
  const char *ret = 0;
  struct sockaddr_in sa;

  if (getPeer(s, &sa))
  {
    ret = (const char *)inet_ntoa(sa.sin_addr);
  }

  return ret;
}


/*  
 *  Return the peer port number as an integer in host byte order.
 *
 *  Return 0 on error.
 */    

int sockPortNum(s)
  int s;
{
  int ret;
  struct sockaddr_in sa;

  if ((ret = getPeer(s, &sa)))
  {
    ret = (int)ntohs(sa.sin_port);
  }

  return ret;
}


/*  
 *  Return the in_addr structure corresponding to the dotted decimal address,
 *  `s'.
 *  
 *  We interpret an address of `8' as `8.0.0.0' rather than `0.0.0.8'.
 */    
struct in_addr net_addr(s)
  const char *s;
{
  char *aptr;
  char *aorig;
  struct in_addr addr;

  memset((char *)&addr, 0, sizeof addr);
  addr.s_addr = 0xffffffff; /* in case of error */
  if ((aorig = strsdup(s, ".0.0.0.0.", (char *)0)))
  {
    aptr = *aorig == '.' ? aorig+1 : aorig;
    /* toast fourth `.' */
    *strchr(strchr(strchr(strchr(aptr, '.')+1, '.')+1, '.')+1, '.') = '\0';
    addr.s_addr = inet_addr(aptr); /* Berkeley bug: should return in_addr structure. */
    free(aorig);
  }

  return addr;
}


struct in_addr network(in)
  struct in_addr in;
{
  struct in_addr net;

  memset((char *)&net, 0, sizeof net);
#if 0
  /* Well, this doesn't seem to do what I expected... */
  net = inet_makeaddr(inet_netof(in), 0);

#else

  /* so we'll just have to roll our own. */
  {
    unsigned long n = in.s_addr;

    if      ((n >> 31) == 0x00) net.s_addr = n & 0xff000000; /* class A     */
    else if ((n >> 30) == 0x02) net.s_addr = n & 0xffff0000; /* class B     */
    else if ((n >> 29) == 0x06) net.s_addr = n & 0xffffff00; /* class C     */
    else                        net.s_addr = n;              /* class D & E */
  }
#endif

  return net;
}
