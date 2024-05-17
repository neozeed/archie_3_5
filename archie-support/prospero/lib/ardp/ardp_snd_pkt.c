/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * Written  by bcn 1/93     to send a single packet to a peer
 */

#include <usc-copyr.h>

#include <stdio.h>
#include <ardp.h>
#include <plog.h>
#include <errno.h>
#include <perrno.h>

extern int	ardp_srvport;
extern int	ardp_prvport;

/*
 * ardp_snd_pkt - transmits a single packet to address in req
 *
 *   ardp_snd_pkt takes a pointer to a packet of type PTEXT to be
 *   sent to a peer identified by req->peer.  It then send the packet to 
 *   the peer.  If the packet was sent successfully, ARDP_SUCCESS is
 *   returned.  Successful transmission of the packet does not provide
 *   any assurance of receipt by the peer.  If the attempt to send 
 *   the packet fails, ARDP_NOT_SENT is returned.
 */
int
ardp_snd_pkt(pkt,req)
    PTEXT		pkt; 
    RREQ		req;
{
    int	sent;

    sent = sendto(((ardp_prvport != -1) ? ardp_prvport : ardp_srvport), 
		  pkt->start, pkt->length, 0, &(req->peer), S_AD_SZ);
	    
    if(sent == pkt->length) return(ARDP_SUCCESS);
    
    plog(L_NET_ERR, req, "Attempt to send message failed (errno %d %s)",
	 errno, unixerrstr(), 0); 
	
    return(ARDP_NOT_SENT);
}
