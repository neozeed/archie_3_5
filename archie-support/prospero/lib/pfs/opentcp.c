/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>             /* for fcntl() */
#include <fcntl.h>              /* for fcntl() */
#include <errno.h>              /* for EINPROGRESS. etc.*/
#ifdef AIX			/* lucb */
#include <sys/select.h>
#endif

#include <errno.h>
#include <pfs_threads.h>
#include <ardp.h>
#include <perrno.h>
#include <pfs.h>
#include <sys/time.h>         /* For gettimeofday */
#include <sockettime.h>       /* for xxx_APPROACH */

/* Open a TCP stream from here to the HOST at the PORT. */
/* Return socket descriptor on success, or -1 on failure. */
/* This is mostly swiped from user/vcache/gopherget.c */
/* It is part of the Prospero library.  It includes special goodies to keep
   the connect() operation from blocking in a multi-threaded environment.
   Non-blocking or briefly-blocking TCP opens are important here. */

/* Uses hostname2addr() in the ARDP library. */

/* Like close, but try again if EINTR */
static int
sure_close(int s)
{
	int retval;
	while ((((retval = close(s)) == -1) && (errno == EINTR)));
	return(retval);
}

/* Wait till the file descriptor is writable or times out */
/* Returns: 1 success, 0, timeout, -1 (& errno) error */
static int
wait_till_writable(int s,int timeout)
{
  struct timeval time_out;
  fd_set writefds;
  int retval;
  int soerror;
  int lensoerrors = sizeof(soerror);

  time_out.tv_sec = timeout;
  time_out.tv_usec = 0;

  FD_ZERO(&writefds);
  FD_SET(s,&writefds);
     
  switch (retval = select(FD_SETSIZE, NULL, &writefds, NULL, &time_out)) {
    case 1:
      retval = getsockopt(s, SOL_SOCKET, SO_ERROR, 
		       (char *)&soerror, &lensoerrors);
      /* While we'd like the error back in soerror, 
	 it fails when there is an error*/
      if (retval) return -1;
       return 1;

    case 0:
    case -1:            
    default:
        return(retval);
    }
}

/* Wait till the file descriptor is readable or times out */
/* Returns: 1 success, 0, timeout, -1 (& errno) error */
int
wait_till_readable(int s,int timeout)
{
  struct timeval time_out;
  fd_set readfds;

  time_out.tv_sec = timeout;
  time_out.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(s,&readfds);
    return(select(FD_SETSIZE, &readfds, NULL, NULL, &time_out));
}

int
quick_open_tcp_stream(const char host[], int port, int timeout)
{
    extern int ardp_hostname2addr(const char *hostname, struct sockaddr_in *hostaddr);
    struct sockaddr_in server;	/* server side socket. */
    int s;			/* the socket */
    int tmp;                    /* return from subfunctions. */

    if(ardp_hostname2addr(host, &server)) {
	p_err_string = qsprintf_stcopyr(p_err_string,
					"%s: unknown host\n", host);
        return -1;              /* p_err_string not set in callee */
    }
    server.sin_port = htons(port);
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	p_err_string = qsprintf_stcopyr(p_err_string,
            "Couldn't reach host %s: Call to socket(AF_INET, SOCK_STREAM,\
 0) failed: errno %d: %s", host, errno, unixerrstr());
	return -1;
    }

#ifdef SETSOCKOPTS
/* Actually, these may be generally usefull. */
#define M_SETSOCKOPT(f,sol,p1,p2,p3) \
    if (setsockopt(f,sol,p1,p2,p3)) { \
      p_err_string = qsprintf_stcopyr(p_err_string, \
	  "INTERNAL: setsockopt failure: %s %s", "##p1##",unixerrstr()); \
      sure_close(f); \
	      return -1; \
				    }
#ifndef UCX
    M_SETSOCKOPT(s, SOL_SOCKET, ~SO_LINGER, 0, 0);
#endif
/* Allow address reuse, I forget exactly why, but its something to do 
   with a time delay before address:port pairs can be reused otherwise */ 
    M_SETSOCKOPT(s, SOL_SOCKET, SO_REUSEADDR, 0, 0);
/* Attempt to keep a silent connection alive, and so fail if it goes down */
    M_SETSOCKOPT(s, SOL_SOCKET, SO_KEEPALIVE, 0, 0); 
#endif /*SETSOCKOPTS*/

#if defined(NONBLOCKING_APPROACH) || defined(SELECT_APPROACH)
    /* This is one approach to multi-threading.  The problem with it is 
       that we seem to get 'address in use' errors.  */
    /* Set the socket non-blocking if we're running multithreaded.  The main
       downside of this approach is that we're not going to be able to get a
       great error message if the call to connect() fails, which is why we
       don't normally enable this in the non-threaded case. */ 
    /* This code has not been fully tested.  If the FNDELAY operation is
       unavailable on your version of unix, you will need an alternative
       approach.   Mitra has one in his TCPTIME patch. */
    /* This may be FDELAY on some systems - 
       its O_NODELAY or O_NONBLOCK on Solaris2.3 & POSIX*/
    if(fcntl(s, F_SETFL, O_NONBLOCK) == -1) {
	p_err_string = qsprintf_stcopyr(p_err_string,
					"call to fcntl(s, F_SETFL, \
FNDELAY) failed: errno %d: %s", errno, unixerrstr());
        /* Close the socket so we don't have trash data hanging around. */
        sure_close(s);
	return -1;
    }
#endif /*NONBLOCKING_APPROACH||SELECT_APPROACH*/

    /* Connect will do the bind for us!  Hooray!  That's a relief. */
    /* connect() will return -1 if the connection is already in progress. */
#ifdef TIMEOUT_APPROACH
    if ((quick_connect(s, (struct sockaddr *) &server, sizeof server, timeout))
	==-1) {
#else /*!TIMEOUT_APPROACH*/
redo_connect:
    tmp = connect(s, (struct sockaddr *) &server, sizeof server);
    if (tmp == -1) {
      /* Ick - doing an fprintf, meant that in the non-debugging mode, 
	 where there is no stderr, this will fail and CHANGE errno to "Bad
	 FileNo */
      
	switch (errno) {
	case  EINPROGRESS:     
#ifdef SELECT_APPROACH
	     switch (wait_till_writable(s,timeout)) {
	     case -1:      p_err_string = qsprintf_stcopyr(p_err_string,
		      "unable to connect to: %s(%d)", host, port);
/*		      "call to select failed: %s", unixerrstr()); */
	              sure_close(s);	
	              return -1;
	     case 0:       
	       p_err_string = qsprintf_stcopyr(p_err_string,
		    "took more than %d secs to connect to: %s(%d)",
		      timeout, host, port);
	       sure_close(s);
	              return -1;
	     }
	     /* Default is going to be 1 - which is success */
	       
#endif /*SELECT_APPROACH*/
		                return s;		/* Non-blocking*/
	case EINTR:		goto redo_connect;	/* Interrupted*/
	}
#endif /*!TIMEOUT_APPROACH*/
	/* I've changed this cos users will see it all the time, lets make 
	   it friendly  - this has all the relevant info! - Mitra */
	p_err_string = qsprintf_stcopyr(p_err_string,
		"Couldn't connect to host %s: %s", host, unixerrstr());
        /* Now close it so we don't have the descriptor and associated data
           hanging around. */
    	sure_close(s);
	return -1;
    }
    return s;
}

