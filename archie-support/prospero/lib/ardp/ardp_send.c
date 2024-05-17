/*
 * Copyright (c) 1991-1993 by the University of Southern California
 *
 * Written  by bcn 1989-92  as dirsend.c in the Prospero distribution
 * Modified by bcn 1/93     separate library and add support for asynchrony 
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include <ardp.h>
#include <perrno.h>
#include <pmachine.h>

#define OLD_GETHOSTBYNAME       /* do it the old way, since we're on the client
                                   */ 
#ifdef PROSPERO
#include <pcompat.h>
#else /* not PROSPERO */
#define DISABLE_PFS_START()
#define DISABLE_PFS_END()
#endif /* not PROSPERO */

RREQ		       ardp_activeQ = NOREQ;  /* Info about active requests */
RREQ		       ardp_completeQ = NOREQ;/* Completed requests         */
static unsigned short  ardp_def_port_no = 0;  /* Default UDP port to use    */
int		       ardp_port = -1;	      /* Opened UDP port	    */

extern int	       pfs_debug;             /* Debug level                */

static ardp_init();
static short ardp_next_cid();

/*
 * ardp_send - send a request and possibly wait for response
 *
 *   ardp_send takes a pointer to a structure of type RREQ, an optional 
 *   hostname, an optional pointer to the desination address, and the time to
 *   wait before returning in microseconds.
 *
 *   If a destination address was specified, the address is inserted
 *   into the RREQ structure.  If not, but a hostname was specified, the
 *   hostname is resolved, and its address inserted into the RREQ
 *   structure.  The hostname may be followed by a port number in
 *   parentheses in which case the port field is filled in.  If not
 *   specified, the Prospero directory server port is used as the default. 
 *   If the host address is a non-null pointer to an empty address, then
 *   the address is also filled in.
 *
 *   ardp_send then sends the packets specified by the request structure 
 *   to the address in the request structure.  If the time to wait is
 *   -1, it waits until the complete response has been received and
 *   returns PSUCCESS or PFAILURE depending on the outcome.  Any
 *   returned packets are left in the RREQ strucure.  If the time to
 *   wait is 0, ardp_send returns immediately.  The prereq strucure will
 *   be filled in as the response is received if calls are made to
 *   ardp_check_messages (which may be called by an interrupt, or
 *   explicitly at appropriate points in the application.  If the time
 *   to wait is positive, then ardp_send waits the specified lenght of
 *   time before returning.
 *
 *   If ardp_send returns before the complete response has been
 *   received, it returns ARDP_PENDING (-1).  This means that only
 *   the status field in the RREQ structure may be used until the status
 *   field indicates ARDP_STATUS_COMPLETE.  In no event shall it be legal 
 *   for the application to modify fields in the RREQ structure while a
 *   request is pending.  If the request completes during the call to
 *   ardp_send, it returns ARDP_SUCCESS (0).  On error, a positive
 *   return or status value indicates the error that occured.
 *
 *   In attempting to obtain the response, the ARDP library will wait
 *   for a response and retry an appropriate number of times as defined
 *   by timeout and retries (both static variables).  It will collect
 *   however many packets form the reply, and return them in the
 *   request structue.
 *
 *   ARGS:      req        Request structure holding packets to send 
 *			   and to receive the response
 *              hname      Hostname including optional port in parentheses
 *              dest       Pointer to destination address
 *              ttwait     Time to wait in microseconds
 *
 *   MODIFIES:  dest	   If pointer to empty address
 *              req	   Fills in ->recv and frees ->trns
 *
 *   NOTE:      In preparing packets for transmission, the packets
 *              are modified.  Once the full response has been received,
 *              the packets that were sent are freed.
 */
int
ardp_send(RREQ		req,	/* Request structure to use (in, out) */
	  char		*dname,	/* Hostname (and port) of destination */
	  struct sockaddr_in *dest, /* Pointer to destination address */
	  int		ttwait) /* Time to wait in microseconds       */
{
    char	hostnoport[400];/* Hostname without port              */
    char	*openparen;	/* Start of port in dname             */
    int		req_udp_port;	/* UDP port from hostname             */
#ifdef OLD_GETHOSTBYNAME        /* unused */
    struct hostent *host;	/* Host info from gethostbyname	      */
#endif

    PTEXT	ptmp;		/* Temporary packet pointer	      */
    int		DpfStmp;        /* Used when disabling prospero       */
    int		tmp;		/* To temporarily hold return values  */

    p_clear_errors();

    if((ardp_port < 0) && (tmp = ardp_init())) return(tmp);

    if(req->status == ARDP_STATUS_FREE) {
	fprintf(stderr,"Attempt to send free RREQ\n");
	abort();
	return(perrno = ARDP_BAD_REQ);
    }

    while(req->outpkt) {
        req->outpkt->seq = ++(req->trns_tot);
        ptmp = req->outpkt;
        EXTRACT_ITEM(ptmp,req->outpkt);
        APPEND_ITEM(ptmp,req->trns);
    }

    if(pfs_debug >= 9) {
	fprintf(stderr, "In ardp_send - sending to %s\n", dname);
        ptmp = req->trns;
        while(ptmp) {
            fprintf(stderr,"Packet %d:\n",ptmp->seq);
            ardp_showbuf(ptmp->text, ptmp->length, stderr);
            putc('\n', stderr);
            ptmp = ptmp->next;
        }
    }

    /* Assign connection ID */
    req->cid = ardp_next_cid();

    /* Resolve the host name, address, and port arguments          */
    
    /* If we were given the host address, then use it.  Otherwise  */
    /* lookup the hostname.  If we were passed a host address of   */
    /* 0, we must lookup the host name, then replace the old value */
    if(!dest || (dest->sin_addr.s_addr == 0)) {
        /* I we have a null host name, return an error */
        if((dname == NULL) || (*dname == '\0')) {
            if (pfs_debug >= 1)
                fprintf(stderr, "ardp_send: Null hostname specified\n");
            return(perrno = ARDP_BAD_HOSTNAME);
        }
        /* If a port is included, save it away */
	if(openparen = strchr(dname,'(')) {
            sscanf(openparen+1,"%d",&req_udp_port);
            if(req_udp_port) req->peer_port = htons(req_udp_port);
            strncpy(hostnoport,dname,399);
            if((openparen - dname) < 400) {
                *(hostnoport + (openparen - dname)) = '\0';
                dname = hostnoport;
            }
        }
#ifdef OLD_GETHOSTBYNAME
        DISABLE_PFS_START();
	assert(P_IS_THIS_THREAD_MASTER());
        if((host = gethostbyname(dname)) == NULL) {
            DISABLE_PFS_END();
            /* Check if a numeric address */
            req->peer.sin_family = AF_INET;
            req->peer_addr.s_addr = inet_addr(dname);
            if(req->peer_addr.s_addr == -1) {
                if (pfs_debug >= 1)
                    fprintf(stderr, "ardp: Can't resolve host %s\n", dname);
                return(perrno = ARDP_BAD_HOSTNAME);
            }
        }
        else {
            DISABLE_PFS_END();
            req->peer.sin_family = host->h_addrtype;
            bcopy(host->h_addr, (char *)&(req->peer_addr), 
                  host->h_length);
        }
#else
        /* New way of doing things */
        if (ardp_hostname2addr(dname, &req->peer_addr))
            return perrno = ARDP_BAD_HOSTNAME;
        
#endif
    }
    else bcopy(dest, &(req->peer), S_AD_SZ);

    /* If no port set, use default port */
    if(req->peer_port == 0) req->peer_port = ardp_def_port_no;

    /* If dest was set, but zero, fill it in */
    if(dest && (dest->sin_addr.s_addr == 0)) 
	bcopy(&(req->peer), dest, S_AD_SZ);

    if(tmp = ardp_headers(req)) return(tmp);

    req->status = ARDP_STATUS_ACTIVE;
    APPEND_ITEM(req,ardp_activeQ);
    req->wait_till.tv_sec = time(NULL) + req->timeout_adj.tv_sec;
    ardp_xmit(req, req->pwindow_sz);
    return(ardp_retrieve(req,ttwait));
}

/*
 * ardp_init - Open socket and bind port for network operations
 *
 *    ardp_init attempts to determine the default destination port.
 *    It then opens a socket for network operations and attempts
 *    to bind it to an available privleged port.  It tries to bind to a
 *    privileged port so that its peer can tell it is communicating with
 *    a "trusted" program.  If it can not bind a priveleged port, then
 *    it also returns successfully since the system will automatically 
 *    assign a non-priveleged port later, in which case the peer will
 *    assume that it is communicating with a non-trusted program.  It
 *    is expected that in the normal case, we will fail to bind the
 *    port since most applications calling this routine should NOT
 *    be setuid.
 */
static int
ardp_init()
{
    struct servent 	*sp;	/* Entry from services file           */
    struct sockaddr_in  us;	/* Our address                        */
    int			DpfStmp;/* Used when disabling prospero       */
    int			tmp;	/* For stepping through ports 	      */

    /* Determine default udp port to use */
    DISABLE_PFS_START();
    assert(P_IS_THIS_THREAD_MASTER()); /*SOLARIS: getservbyname MT-Unsafe */
    if ((sp = getservbyname(ARDP_DEFAULT_PEER,"udp")) == 0) {
	if (pfs_debug >= 10)
	    fprintf(stderr, "ardp: udp/%s unknown service - using %d\n", 
		    ARDP_DEFAULT_PEER, ARDP_DEFAULT_PORT);
	ardp_def_port_no = htons((u_short) ARDP_DEFAULT_PORT);
    }
    else ardp_def_port_no = sp->s_port;
    DISABLE_PFS_END();
    if (pfs_debug >= 10)
	fprintf(stderr,"default udp port is %d\n", ntohs(ardp_def_port_no));


    /* Open the local socket from which packets will be sent */
    errno=0;
    if((ardp_port = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        if (pfs_debug >= 1)
            fprintf(stderr,"ardp: Can't open socket - %s\n",
                    unixerrstr());
        return(perrno = ARDP_UDP_CANT);
    }

#ifndef ARDP_NONPRIVED
    /* Try to bind it to a privileged port - loop through candidate   */
    /* ports trying to bind.  If failed, that's OK, we will let the   */
    /* system assign a non-privileged port later                      */
    bzero((char *)&us, sizeof(us));
    us.sin_family = AF_INET;
    for(tmp = ARDP_FIRST_PRIVP; tmp < ARDP_FIRST_PRIVP+ARDP_NUM_PRIVP;tmp++) {
	us.sin_port = htons((u_short) tmp);
	if(bind(ardp_port, (struct sockaddr *)&us, sizeof(us)) == 0)
	    return(ARDP_SUCCESS);
	if(errno != EADDRINUSE) return(ARDP_SUCCESS);
    }
#endif /* ARDP_NONPRIVED */
    return(ARDP_SUCCESS);
}

/*
 * ardp_next_cid - return next connection ID in network byte order
 *
 *    ardp_next_cid returns the next connection ID to be used
 *    after first converting it to network byte order.
 */
static short ardp_next_cid()
{
    static unsigned short	next_conn_id = 0; /* Next conn id to use  */
    static int			last_pid = 0;     /* Reset after forks    */
    int				pid = getpid();
    
    /* If we did a fork, reinitialize */
    if(last_pid != pid) {
	if(ardp_port >= 0) close(ardp_port);
	ardp_port = -1;
	next_conn_id = 0;
	ardp_init();
    }
    /* Find first connection ID */
    assert(P_IS_THIS_THREAD_MASTER()); /* rand and srand are unsafe */
    if(next_conn_id == 0) {
	srand(pid+time(0));
	next_conn_id = rand();
	last_pid = pid;
    }
    if(++next_conn_id == 0) ++next_conn_id;
    return(htons(next_conn_id));
}
