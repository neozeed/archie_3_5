/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 *
 * Written  by bcn 1/93     to send a list of packets to a destination
 */

#include <usc-license.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ardp.h>
#include <pmachine.h>           /* for bcopy() and select() */

extern int	       pfs_debug;            /* Debug level                */
extern int	       ardp_port;	     /* Opened UDP port		   */

/*
 * ardp_xmit - transmits packets in req->trns
 *
 *   ardp_xmit takes a pointer to a request structure of type RREQ and a
 *   window size.  It then sends the packet on the request structure
 *   transmit queue to the peer address in the request structure starting after
 *   the indicated peer rcvd_thru value, and up to the number of packets 
 *   specified by the window size.  It returns ARDP_SUCCESS on success and
 *   returns an error code on failure.
 *
 *   This is called on both the client and server side, called respectively by
 *   ardp_send() and ardp_pr_actv(). 
 */

int
ardp_xmit(RREQ	req,            /* Request structure with packets to send   */
	  int	window)         /* Max packets to send at once.  Note that
                                   right now this is always identical to
                                   req->pwindow_sz.  
                                   0 means infinite window size
                                   -1 means just send an ACK; don't send any
                                      data packets. */    
{
    PTEXT 	 	ptmp;	/* Packet pointer for stepping through trns */
    unsigned short	stmp;	/* Temp short for conversions    */
    int		 	ns;	/* Number of bytes actually sent            */
    static PTEXT ack = NOPKT;	/* Only an ack to be sent                   */

    if(window < 0 || req->prcvd_thru >= req->trns_tot) { 
	/* All our packets got through, send acks only */
	if(ack == NOPKT) {
	    ack = ardp_ptalloc();
	    /* Add header */
	    ack->start -= 9;
	    ack->length += 9;
	    *(ack->start) = (char) 9;
	    /* An unsequenced control packet */
	    bzero4(ack->start+3);
	}
	/* Received Through */
	stmp = htons(req->rcvd_thru);
	bcopy2(&stmp,ack->start+7);
	/* Connection ID */
	bcopy2(&(req->cid),ack->start+1);
	ptmp = ack;
    }
    else ptmp = req->trns;

    /* Note that we don't want to get rid of packts before the */
    /* peer received through since the peer might later tell   */
    /* us it forgot them and ask us to send them again         */
    /* XXX whether this is allowable should be an application  */
    /* specific configration option.                           */
    while(ptmp) {
	if((window > 0) && (ptmp->seq > req->prcvd_thru + window)) break;
	if((ptmp->seq == 0) || (ptmp->seq > req->prcvd_thru)) {
	    if (pfs_debug >= 6) {
		if (req->peer.sin_family == AF_INET) 
		    fprintf(stderr,
                            "Sending message%s (cid=%d) (seq=%d) to %s(%d)...",
                            (ptmp == ack) ? " (ACK only)" : "", 
			    ntohs(req->cid), ntohs(ptmp->seq),
                            inet_ntoa(req->peer_addr), PEER_PORT(req));
		else fprintf(stderr,"Sending message...");
		(void) fflush(stderr);
	    }
	    ns = sendto(ardp_port,(char *)(ptmp->start), ptmp->length, 0, 
			&(req->peer), S_AD_SZ);
	    if(ns != ptmp->length) {
		if (pfs_debug >= 1) {
		    fprintf(stderr,"\nsent only %d/%d: ",ns, ptmp->length);
		    perror("");
		}
		return(ARDP_NOT_SENT);
	    }
	    if (pfs_debug >= 6) fprintf(stderr,"Sent.\n");
	}
	ptmp = ptmp->next;
    }
    return(ARDP_SUCCESS);
}


