/*
 * Copyright (c) 1991       by the University of Washington
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 *
 * Written  by bcn 1991     as transmit in rdgram.c (Prospero)
 * Modified by bcn 1/93     modularized and incorporated into new ardp library
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#include <stdio.h>
#include <ardp.h>
#include <pmachine.h>           /* for bcopy() */
#include <plog.h>

EXTERN_MUTEXED_DECL(RREQ,ardp_doneQ);
EXTERN_MUTEXED_DECL(RREQ,ardp_runQ);
extern int	ardp_srvport;
extern int	ardp_prvport;
extern int	dQlen;          /* mutexed with ardp_doneQ */
extern int	dQmaxlen;
int             ardp_clear_doneQ_loginfo = 0; /* Clear log info from the doneQ.
                                                 Set by the -n option to
                                                 dirsrv. */


/*
 * ardp_respond - respond to request
 *
 *    ardp_respond takes a request block whose ->outpkt element is a pointer
 *    to a list of packets that have not yet been returned.  It moves the
 *    packets to the transmission queue of the request (->trns) and assigns a
 *    sequence number to each packet.  If options is ARDP_R_COMPLETE, it
 *    also sets the ->trns_tot field of the request indicating the total 
 *    number of packets in the response.   if the ARDP_R_NOSEND option is 
 *    not specified, the current packet is also sent to the client.
 *    
 *    ardp_respond() expects the text of the packet to have a trailing null
 *    at the end of it, which is transmitted for safety's sake.  
 *    (noted: swa, 8/5/93).  This explains some bugs...
 */
int
ardp_respond(req, options)
    RREQ		req;
    int			options;
{
    char 		buf[ARDP_PTXT_DSZ];
    int			sent;
    int			retval = ARDP_SUCCESS; /* Return value */
    unsigned short	cid = htons(req->cid);
    unsigned short	nseq = 0;
    unsigned short	ntotal = 0;
    unsigned short	rcvdthru = 0;
    unsigned short	bkoff = 0;
    PTEXT			tpkt; /* Temporary pkt pointer */

    *buf = '\0';

    if(req->peer_ardp_version < 0) cid = 0;

    if(req->status == ARDP_STATUS_FREE)
        internal_error("Attempt to respond to free RREQ\n");

    if(!req->outpkt) return(ARDP_BAD_REQ);

 more_to_process:

    if(!req->trns) req->outpkt->seq = 1;
    else req->outpkt->seq = req->trns->previous->seq + 1;

    if((options & ARDP_R_COMPLETE) && (req->outpkt->next == NOPKT)) {
        /* It's the last packet to send. */
	req->trns_tot = req->outpkt->seq; /* set the total # of packets; now
                                             known.  */
    
        /* Test this condition later to see if the server (us) needs to request
           that the client ACK.  The server will request an ACK if we had
           previously told the client to back off and now are rescinding that.
           */
    }
    nseq = htons((u_short) req->outpkt->seq);
    ntotal = htons((u_short) req->trns_tot);

    if((req->peer_ardp_version < 0)&&(req->peer_ardp_version != -2)) {
	if(req->trns_tot) sprintf(buf, "MULTI-PACKET %d OF %d",
				  req->outpkt->seq, req->trns_tot);
	else sprintf(buf,"MULTI-PACKET %d\n",req->outpkt->seq);
    }

    /* Note that in the following, we don't have to make sure  */
    /* there is room for the header in the packet because we   */
    /* are the only one that moves back the start, and ptalloc */
    /* allocates space for us in all packets it creates        */
    
    /* If a single message and no connection ID to return, */
    /* and no pending server requested wait pending then   */
    /* we can leave the control fields out                 */
    if((req->trns_tot == 1) && (req->svc_rwait == 0)) {
	/* Once V4 clients gone, the next if condition can be removed */
	if((req->peer_ardp_version > -1)||(req->peer_ardp_version == -2)) {
	    if(req->cid == 0) {
		req->outpkt->start -= 1;
		req->outpkt->length += 1;
		*req->outpkt->start = (char) 1;
	    }
	    else {
		req->outpkt->start -= 3;
		req->outpkt->length += 3;
		*req->outpkt->start = (char) 3;
		bcopy2(&cid,req->outpkt->start+1);     /* Conn ID */
	    }
	}
    }
    /* Fill in the control fields */
    else {	    
        /* For old versions, assume that svc_rwait of > 5 is too long. */
        /* For new versions, if we set the svc_rwait before to any non-default
           value, now set it to 0. (client's default). */ 

	/* if we might have to set octet 11 flags (in this case, the ACK flag),
           then we need to send an 11-byte request. 
           This is the case either:
            (a) if we need to clear a service requested wait or
            (b) if we are sending the last packet of a window, so that the
            client should ACK (or request a retry) before the next one goes.
            */
	if (/* Case (A) -- clear wait */
            ((req->peer_ardp_version < 0 && req->svc_rwait > 5)
             || (req->peer_ardp_version == 0 && req->svc_rwait)
             || (req->svc_rwait_seq > req->prcvd_thru))
            || /* Case (B) -- sending last packet of a window */
            (req->outpkt->seq == req->prcvd_thru + req->pwindow_sz)) {
            req->outpkt->start -= 12;
            req->outpkt->length += 12;
	    *req->outpkt->start = (char) 12;
            
	    rcvdthru = htons((u_short) req->rcvd_thru);
	    bcopy2(&rcvdthru,req->outpkt->start+7); /* Recvd Through   */

	    /* XXX Note that when all clients V5, new wait will be 0    */
	    /* Assume we should cancel the wait and remember when reset */
	    if((req->peer_ardp_version < 0 && req->svc_rwait > 5)
               || (req->peer_ardp_version == 0 && req->svc_rwait)) {
                if(req->peer_ardp_version < 0) 
                    req->svc_rwait = 5;
                else
                    req->svc_rwait = 0;
		req->svc_rwait_seq = req->outpkt->seq; /* This was the sequence
                                                          number it was reset
                                                          on. */ 
                /* We'll ask for an acknowledgement when we send the last
                   packet of the response. */
	    }
	    bkoff = htons((u_short) req->svc_rwait);
	    bcopy2(&bkoff,req->outpkt->start+9);     /* New ttwait  */
            /* Send an ACK IF: */
            if ((/* (A1) It's the last packet */
                 (req->trns_tot && req->outpkt->seq == req->trns_tot)
                 && /* (A2) If we reduced a previously requested wait, and haven't
                       yet had that reduction acknowledged, then ask for an
                       acknowledgement. */ 
                 (req->svc_rwait_seq > req->prcvd_thru))
                || /* OR: Case (B) -- sending last packet of a window */
                (req->outpkt->seq == req->prcvd_thru + req->pwindow_sz)) {

                req->retries_rem = 4;
                req->wait_till.tv_sec = time(NULL) + req->timeout_adj.tv_sec;
                *(req->outpkt->start+11) = 0x80;          /* Request ack */
            } else {
                *(req->outpkt->start+11) = 0x00;        /* Don't request ack */
            }
	}
	else {
	    req->outpkt->start -= 7;
	    req->outpkt->length += 7;
	    *req->outpkt->start = (char) 7;
	}

	bcopy2(&cid,req->outpkt->start+1);     /* Conn ID */
	bcopy2(&nseq,req->outpkt->start+3);     /* Pkt no  */
	bcopy2(&ntotal,req->outpkt->start+5);   /* Total   */
    }	

#if 0
    /* commented out by swa@isi.edu, since nulls in packets are now significant
       to the ARDP library.  Therefore, one should NOT be appended to each
       packet in the data area. */
    /* Make room for the trailing null */
    req->outpkt->length += 1;
#endif

    /* Only send if packet not yet received. */
    /* Do honor the window of req->pwindow_sz packets.  */
    if((!(options & ARDP_R_NOSEND)) && 
       (req->outpkt->seq <= (req->prcvd_thru + req->pwindow_sz)) &&
       (req->outpkt->seq > req->prcvd_thru) /* This packet not yet received */
                                               ) { 
	retval = ardp_snd_pkt(req->outpkt,req);
    }


    /* Add packet to req->trns */
    tpkt = req->outpkt;
    EXTRACT_ITEM(tpkt,req->outpkt);
    APPEND_ITEM(tpkt,req->trns);

    if(req->outpkt) goto more_to_process;

    /* If complete then add request structure to done  */
    /* queue in case we have to retry.                 */
    if(options & ARDP_R_COMPLETE) {
        RREQ match_in_runQ;     /* Variable for indexing ardp_runQ */
        
	/* Request has been processed, here you can accumulate */
	/* statistics on system time or service time           */
	plog(L_QUEUE_COMP, req, "Requested service completed"); 

	arch_set_etime(req);  /* bajan */

        EXTERN_MUTEXED_LOCK(ardp_runQ);
        for (match_in_runQ = ardp_runQ; match_in_runQ; match_in_runQ = match_in_runQ->next) {
            if(match_in_runQ == req) {
                EXTRACT_ITEM(req, ardp_runQ);
                break;
            }
        }
        /* At this point, 'req' is the completed request structure.  It is
           definitely not on the ardp_runQ, and if it was, it's been removed.
           */ 
        EXTERN_MUTEXED_UNLOCK(ardp_runQ);
	if((req->cid == 0) || (dQmaxlen <= 0)) {
            /* If request has no CID (can't be matched on a retry) or
               if done-Q max length is <= 0 (i.e., don't keep a doneQ), then
               just throw away the request now that it's done. --swa */
#if 0                           /* ask BCN about this. Should never be
                                   necessary, since req->outpkt is always going
                                   to be NULL or we wouldn't be here. */
	    req->outpkt = NULL;
#endif
	    ardp_rqfree(req);
	}
	else {
            /* This item should be put on the doneQ; don't just throw it away.
               */
            /* Note that this code will not handle a reduction in the size of
               dQmaxlen; if anywhere you reset it to a smaller value, that code
               will have to truncate the queue. */
            EXTERN_MUTEXED_LOCK(ardp_doneQ);
            if(dQlen <= dQmaxlen) {
                /* Add to start */
#ifndef NDEBUG                  /* this helps debugging and slightly cuts down
                                 on memory usage of the doneq. */
                if (ardp_clear_doneQ_loginfo)
                    ardp_rq_partialfree(req);
#endif
                PREPEND_ITEM(req, ardp_doneQ);
                dQlen++;
            } else {
                /* The last item in the queue is ardp_doneQ->previous. */
                /* Use a variable to denote it, so that the EXTRACT_ITEM macro
                   doesn't encounter problems (since it internally modifies the
                   referent of the name ardp_doneQ->previous). */
                register RREQ doneQ_lastitem = ardp_doneQ->previous;
                /* Add new request to start */
#ifndef NDEBUG                  /* this helps debugging and slightly cuts down
                                   on memory usage of the doneq. */
                if (ardp_clear_doneQ_loginfo)
                    ardp_rq_partialfree(req);
#endif
                PREPEND_ITEM(req, ardp_doneQ);

                /* Now, prepare to remove the last item in the queue */
                /* If the request to remove was not acknowledged by the
                   clients, log the fact */ 
                if(doneQ_lastitem->svc_rwait_seq > 
                   doneQ_lastitem->prcvd_thru) {
                    plog(L_QUEUE_INFO,doneQ_lastitem,
                         "Requested ack never received (%d of %d/%d ack)",
                         doneQ_lastitem->prcvd_thru, 
                         doneQ_lastitem->svc_rwait_seq, 
                         doneQ_lastitem->trns_tot);
                }
                /* Now do the removal and free it. */
                EXTRACT_ITEM(doneQ_lastitem, ardp_doneQ);
                ardp_rqfree(doneQ_lastitem);
            }
            EXTERN_MUTEXED_UNLOCK(ardp_doneQ);
        }
    }
    return(retval);
}

/*
 * ardp_rwait - sends a message to a client requesting
 *   that it wait for service.  This message indicates that
 *   there may be a delay before the request can be processed.
 *   The recipient should not attempt any retries until the
 *   time specified in this message has elapsed.  Depending on the
 *   protocol version in use by the client, the additional
 *   queue position and system time arguments may be returned.
 */
int
ardp_rwait(RREQ		req,        /* Request to which this is a reply  */
	   int		timetowait, /* How long client should wait       */
	   short	qpos,       /* Queue position                    */
/* Note stime is a System call in solaris, changed to systemtime */
	   int		systemtime)      /* System time                  */
{
    PTEXT	r = ardp_ptalloc();
    short	cid = htons(req->cid);
    short	nseq = 0;
    short	zero = 0;
    short	backoff;
    short	stmp;
    int		ltmp;
    int		tmp;
    
    if(req->peer_ardp_version < 0) cid = 0;

    /* Remember that we sent a server requested wait and assume  */
    /* it took effect, though request will be unsequenced        */
    /* This seems bogus to me.  Rather, more appropriate would be to send an
       unsequenced control packet if the requested wait is being extended and
       a sequenced packet if the wait is being shortened, since the shortening
       of the wait is an event which requires an acknowledgement. */
    /* Note, however, that this shortening is a way the current function is not
       used. */
    req->svc_rwait_seq = req->prcvd_thru; /* Mark the latest requested wait as
                                             definitely having been received.
                                             This is not necessary, actually --
                                             leaving this set to zero is ok. */
    req->svc_rwait = timetowait;
    
       
       
    *r->start = (char) 11;
    r->length = 11;

    backoff = htons((u_short) timetowait);

    bcopy2(&cid,r->start+1);		/* Connection ID     */
    bcopy2(&nseq,r->start+3);		/* Packet number     */
    bcopy2(&zero,r->start+5);		/* Total packets     */
    bcopy2(&zero,r->start+7);		/* Received through  */
    bcopy2(&backoff,r->start+9);	/* Backoff           */
    
    if((req->peer_ardp_version >= 0) && (qpos || systemtime)) {
	r->length = 14;
	bzero3(r->start+11); 
	*(r->start+12) = (unsigned char) 254;
	if(qpos) *(r->start+13) |= (unsigned char) 1;
	if(systemtime) *(r->start+13) |= (unsigned char) 2;
	if(qpos) {
	    stmp = htons(qpos);
	    bcopy2(&stmp,r->start+r->length);
	    r->length += 2;
	}
	if(systemtime) {
	    ltmp = htonl(systemtime);
	    bcopy4(&ltmp,r->start+r->length);
	    r->length += 4;
	    
	}
	*r->start = (char) r->length;
    }

    tmp = ardp_snd_pkt(r,req);
    ardp_ptfree(r);
    return(tmp);
}

/*
 * ardp_refuse - sends a message to the recipient indicating
 *   that its connection attempt has been refused.
 */
int
ardp_refuse(RREQ req)
{
    PTEXT	r;
    short	cid = htons(req->cid);
    short	zero = 0;
    int		tmp;
    
    /* If old version won't understand refused */
    if(req->peer_ardp_version < 0) return(ARDP_FAILURE);

    r = ardp_ptalloc();
    *r->start = (char) 13;
    r->length = 13;

    bcopy2(&cid,r->start+1);		/* Connection ID     */
    bcopy2(&zero,r->start+3);		/* Packet number     */
    bcopy2(&zero,r->start+5);		/* Total packets     */
    bcopy2(&zero,r->start+7);		/* Received through  */
    bcopy2(&zero,r->start+9);		/* Backoff           */
    bzero2(r->start+11);		/* Flags             */
    *(r->start + 12) = (unsigned char) 1; /* Refused         */
    
    tmp = ardp_snd_pkt(r,req);
    ardp_ptfree(r);
    return(tmp);
}


/*
 * ardp_redirect - sends a message to the recipient indicating that
 * all unacknowledged packets should be sent to a new target destination.
 * The target specifies the host address and the UDP port in network
 * byte order.  For now, redirections should only occur before any
 * request packets have been acknowledged, or response packets sent.
 */
int
ardp_redirect(RREQ 		     req,     /* Request to be redirected */
	      struct sockaddr_in    *target)  /* Address of new server    */
{
    PTEXT	r;
    short	cid = htons(req->cid);
    int		tmp;

    /* Old versions don't support redirection */
    if(req->peer_ardp_version < 0) return(ARDP_FAILURE);

    r = ardp_ptalloc();

    *r->start = (char) 19;
    r->length = 19;

    bcopy2(&cid,r->start+1);  	/* Connection ID                     */
    bzero(r->start+3,9);	/* Pktno, total, rcvd, swait, flags1 */
    *(r->start + 12) = '\004';  /* Redirect option                   */
    bcopy4(&(target->sin_addr),r->start+13);
    bcopy2(&(target->sin_port),r->start+17);

    tmp = ardp_snd_pkt(r,req);
    ardp_ptfree(r);
    return(tmp);
}


/*
 * ardp_ack - sends an acknowledgment message to the client indicating
 * that all packets through req->rcvd_thru have been recived.  It is
 * only called on receipt of a multi-packet server request.
 */
int
ardp_acknowledge(RREQ	req)        /* Request to which this is a reply  */
{
    PTEXT	r = ardp_ptalloc();
    short	cid = htons(req->cid);
    short	rcvd = htons(req->rcvd_thru);
    short	zero = 0;
    int		tmp;
    
    *r->start = (char) 9;
    r->length = 9;

    bcopy2(&cid,r->start+1);		/* Connection ID     */
    bcopy2(&zero,r->start+3);		/* Packet number     */
    bcopy2(&zero,r->start+5);		/* Total packets     */
    bcopy2(&rcvd,r->start+7);		/* Received through  */

    tmp = ardp_snd_pkt(r,req);
    ardp_ptfree(r);
    return(tmp);
}


