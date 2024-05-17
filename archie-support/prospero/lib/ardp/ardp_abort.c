/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * Written  by bcn 1/93     to abort pending requests
 */

#include <usc-copyr.h>

#include <posix_signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <pfs_threads.h>
#include <list_macros.h>
#include <implicit_fixes.h>
#include <ardp.h>
#include <pmachine.h>

extern RREQ	       ardp_activeQ; /* Info about active requests */
extern int	       ardp_port;    /* Opened UDP port		   */
extern int	       pfs_debug;    /* Debug level                */
extern int             ardp_activeQ_len;   /* Length of ardp_activeQ     */

/*
 * ardp_abort - abort a pending request
 *
 *   ardp_abort takes a pointer to a request structure of a request to 
 *   be aborted, sends an abort request to the server handling the pending,
 *   request, and immediately returns.  If the request is null (NOREQ), then
 *   all requests on the ardp_activeQ are aborted.
 */
int
ardp_abort(req)
    RREQ		req;		/* Request to be aborted         */
{
    PTEXT		ptmp = NOPKT;    /* Abort packet to be sent       */
    RREQ		rtmp = NOREQ;	/* Current request to abort      */
    int			ns;		/* Number of bytes actually sent */

    if(req && (req->status != ARDP_STATUS_ACTIVE)) return(ARDP_BAD_REQ);

    if(req) rtmp = req;
    else rtmp = ardp_activeQ;
    
    ptmp = ardp_ptalloc();

    /* Add header */
    ptmp->start -= 13;
    ptmp->length += 13;
    *(ptmp->start) = (char) 13;
    /* An unsequenced control packet */
    bzero(ptmp->start+3,10);
    /* Cancel flag */
    *(ptmp->start+12) = 0x01;

    while(rtmp) {
	bcopy2(&(rtmp->cid),ptmp->start+1);
	if (pfs_debug >= 6) {
	    if(rtmp->peer.sin_family == AF_INET) 
	       fprintf(stderr,"\nSending abort message (cid=%d) to %s(%d)...", 
		 ntohs(rtmp->cid),inet_ntoa(rtmp->peer_addr),PEER_PORT(rtmp));
	    else fprintf(stderr,"\nSending abort message...");
	    (void) fflush(stderr);
	}
	ns = sendto(ardp_port,(char *)(ptmp->start), ptmp->length, 0, 
		    &(rtmp->peer), S_AD_SZ);
	if(ns != ptmp->length) {
	    if (pfs_debug) {
		fprintf(stderr,"\nsent only %d/%d: ",ns, ptmp->length);
		perror("");
	    }
	    ardp_ptfree(ptmp);
	    return(ARDP_NOT_SENT);
	}
	if (pfs_debug >= 6) fprintf(stderr,"Sent.\n");

	rtmp->status = ARDP_STATUS_ABORTED;
	EXTRACT_ITEM(rtmp,ardp_activeQ);
	--ardp_activeQ_len;
	if(req) rtmp = NOREQ;
	else rtmp = rtmp->next;
    }

    ardp_ptfree(ptmp);
    return(ARDP_SUCCESS);
}

/*
 * ardp_trap_int - signal handler to abort request on ^C
 */
SIGNAL_RET_TYPE ardp_trap_int()
{
    ardp_abort(NOREQ);
    exit(1);
}

/*
 * ardp_abort_on_int - set up signal handler to abort request on ^C
 */
int
ardp_abort_on_int()
{
    signal(SIGINT,ardp_trap_int);
    return(ARDP_SUCCESS);
}
