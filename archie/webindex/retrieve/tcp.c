#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <ansi_compat.h>

#ifndef INADDR_NONE
# define INADDR_NONE 0xffffffff
#endif

static jmp_buf jbuf;
typedef void Sigfunc PROTO((int));
extern Sigfunc *bsdSignal PROTO((int sig, Sigfunc *fn));


extern char *sys_errlist[];
extern int sys_nerr;

extern int errno;

const char *errnoString PROTO((void))
{
  /* bug: well, this isn't ANSI or POSIX, but... */
  if (errno >= 0 && errno < sys_nerr) {
    return sys_errlist[errno];
  } else {
    return "*** unknown error ***";
  }
}


/*  
 *  From Stevens' "Advanced Programming in the UNIX Environment".
 *
 *  This will ensure we get reasonable SIGCHLD semantics.
 */  
Sigfunc *bsdSignal(sig, fn)
  int sig;
  Sigfunc *fn;
{
  struct sigaction act, oact;

  act.sa_handler = fn;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if (sig == SIGALRM)
  {
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT; /* SunOS */
#endif
  }
  else
  {
#ifdef SA_RESTART
    act.sa_flags |= SA_RESTART; /* SVR4, 4.3+BSD */
#endif
  }

  if (sigaction(sig, &act, &oact) < 0)
  {
    return SIG_ERR;
  }

  return oact.sa_handler;
}



extern char *prog;
extern int debug;

static void sigAlrm(sig)
int sig;
{
  longjmp(jbuf, 1);
}


/*  
 *  Open a TCP connection to a `service' on `host'.  Return file pointers for
 *  reading and writing.  `host' may be a domain name or an IP address, and
 *  `service' may be a service name or a port number.  Allow `timeout'
 *  seconds for the connection to be established.
 */
int openTCP(host,service, timeout, ifp, ofp)
const char *host;
const char *service;
int timeout;
FILE **ifp, **ofp;
{
  Sigfunc *ofunc;
  int ifd;
  volatile int ofd = -1;
  int osecs;
  struct sockaddr_in server_addr;

  *ifp = 0; *ofp = 0;
  
  if (timeout > 0 && setjmp(jbuf) != 0) {
    fprintf(stderr, "openTCP: host `%s', port `%s', timeout %d.\n",
            host, service, timeout); /* wheelan */
    if (ofd != -1) close(ofd);
    if (debug) fprintf(stderr, "%s: openTCP: timed out [longjmp()ed].", prog);
    return 0;
  }

  /* Race conditions abound, but what the hell... */
  ofunc = bsdSignal(SIGALRM, sigAlrm);
  osecs = alarm(timeout);
/*fprintf(stderr,"timeout %d, osecs %d\n",timeout,osecs); */

  memset(&server_addr, 0, sizeof server_addr);
  server_addr.sin_family = AF_INET;

  /*  
   *  Fill in the remote address.
   */

  {
    struct hostent *hent;
    unsigned long inaddr;

    if ((inaddr = inet_addr(host)) != INADDR_NONE) {
      memcpy((void *)&server_addr.sin_addr, (void *)&inaddr, sizeof inaddr);
    } else if ((hent = gethostbyname(host))) {
      server_addr.sin_addr = *(struct in_addr *)(hent->h_addr);
    } else {
      if (debug) fprintf(stderr, "%s: openTCP: can't get address from `%s'.\n", prog, host);
      return 0;
    }
  }

  /*  
   *  Fill in the remote port.
   */

  {
    struct servent *sent;
    unsigned short port;        /* port number in network byte order */

    if ((port = atoi(service)) != 0) {
      server_addr.sin_port = htons(port);
    } else if ((sent = getservbyname(service, "tcp"))) {
      server_addr.sin_port = sent->s_port;
    } else {
      if (debug) fprintf(stderr, "%s: openTCP: can't get port from `%s'.\n", prog, service);
      return 0;
    }
  }
  
  if ((ofd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    if (debug) fprintf(stderr, "%s: openTCP: can't create socket: %s.\n", prog, errnoString());
    return 0;
  }

#if 0
  if (timeout > 0) {
    if (debug) fprintf(stderr, "%s: openTCP: pausing (timeout test).\n", prog);
    pause();                    /* for testing time outs */
  }
#endif
  
  if (connect(ofd, (struct sockaddr *)&server_addr, sizeof server_addr) < 0) {
    if (debug) fprintf(stderr, "%s: openTCP: can't connect: %s.\n", prog, errnoString());
    close(ofd);
    if (timeout > 0) {
      bsdSignal(SIGALRM, ofunc);
      alarm(osecs);
    }
    return 0;
  }
  
  if (timeout > 0) {
    bsdSignal(SIGALRM, ofunc);
    alarm(osecs);
  }
/*fprintf(stderr,"--timeout %d, osecs %d\n",timeout,osecs); */
  /*  
   *  For the benefit of Tcl, dup() the descriptor.  Tcl bases its file
   *  handles on the descriptor number, so we need to ensure that the input
   *  and output file pointers get different handles.
   */  

  if ((ifd = dup(ofd)) == -1) {
    if (debug) fprintf(stderr, "%s: openTCP: can't dup() socket descriptor: %s.\n",
                       prog, errnoString());
    close(ofd);
    return 0;
  }
  
  if ( ! (*ifp = fdopen(ifd, "r"))) {
    close(ifd); close(ofd);
    return 0;
  }

  if ( ! (*ofp = fdopen(ofd, "w"))) {
    fclose(*ifp); *ifp = 0;
    close(ofd);
    return 0;
  }

  /*  
   *  Line buffer output so Tcl script writers don't have to `flush' after
   *  every `puts'.
   */
  
  setvbuf(*ofp, (char *)0, _IOLBF, 0);
  
  return 1;
}






