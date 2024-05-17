
/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 *
 * Written  by bcn 1991     as check_for_messages in rdgram.c (Prospero)
 * Modified by bcn 1/93     modularized and incorporated into new ardp library
 * Modified by swa 12/93    made handle arbitrary length ardp_runQ
 * Modified by swa 3/95     for bunyip; trapping errno 9 in bad rcvfrom.
 */

/* This version of lib/ardp/ardp_accept.c will work both in Prospero 
   Prealpha.17May94 and in the current (as of 3/21/95) working sources at ISI.
   */ 

#include <usc-license.h>

#include <netdb.h>
#include <stdio.h>
#include <sys/param.h>
/* Need types.h for u_short */
#include <sys/types.h>
#include <sys/socket.h>
#ifdef AIX
#include <sys/select.h>
#endif
#ifdef SOLARIS
#include <sys/signal.h>
#endif
#include <errno.h>
#include <ardp.h>
#include <pserver.h>
#include <plog.h>
#include <pprot.h>
#include <pmachine.h>
#include <pfs_threads.h>	/* Mutex stuff */

#define LOG_PACKET(REQ,QP) \
    if((REQ)->priority || (pQlen > 4)) \
      if(pQNlen && (REQ)->priority) \
         plog(L_QUEUE_INFO, REQ, "Queued: %d of %d (%d) - Priority %d", \
              QP, pQlen, pQNlen, (REQ)->priority, 0); \
      else if(pQNlen) \
         plog(L_QUEUE_INFO, REQ, "Queued: %d of %d (%d)", \
              QP, pQlen, pQNlen, 0); \
      else if((REQ)->priority) \
         plog(L_QUEUE_INFO, REQ, "Queued: %d of %d - Priority %d", \
               QP, pQlen, (REQ)->priority, 0); \
      else if(QP != pQlen) \
         plog(L_QUEUE_INFO, REQ, "Queued: %d of %d", QP, pQlen, 0); \
      else plog(L_QUEUE_INFO, REQ, "Queued: %d", pQlen, 0); 


EXTERN_MUTEXED_DEF_INITIALIZER(RREQ,ardp_pendingQ,NOREQ); /*Pending requests*/ 
int		pQlen = 0;               /* Length of pending queue         */
int		pQNlen = 0;              /* Number of niced requests        */

static RREQ	ardp_partialQ = NOREQ;	 /* Incomplete requests             */
int		ptQlen = 0;      	 /* Length of incomplete queue      */
int		ptQmaxlen = 20;		 /* Max length of incomplete queue  */
EXTERN_MUTEXED_DEF_INITIALIZER(RREQ,ardp_runQ,NOREQ); /* Requests currently in
                                                          progress  */ 
EXTERN_MUTEXED_DEF_INITIALIZER(RREQ,ardp_doneQ,NOREQ); /* Processed requests 
                                                         */
int		dQlen = 0;      	 /* Length of reply queue           */
                                /* Mutexed with ardp_doneQ */
int		dQmaxlen = 20;		 /* Max length of reply queue       */

static struct timeval  zerotime = {0, 0}; /* Used by select                 */

#define max(x,y) (x > y ? x : y)

extern int (*ardp_pri_func)();            /* Function to compare priorities */
extern int ardp_pri_override;             /* If 1, overide value in request */

/* C library error facility */

extern int		ardp_srvport;
extern int		ardp_prvport;
#ifdef ARCHIE
#ifdef __STDC__
extern int arch_get_etime(RREQ);
#else
extern int arch_get_etime();
#endif
#endif


static void ardp_update_cfields(RREQ existing, RREQ newvalues);
static void ardp_header_ack_rwait(PTEXT tpkt, RREQ nreq, int is_ack_needed, 
                                  int is_rwait_needed); 


int
ardp_accept_and_wait(int timeout, int usec)
{
    struct sockaddr_in 	from;
    int			fromlen;
    int			n = 0;
    PTEXT		pkt;
    unsigned char       flags1,flags2;
    unsigned short	pid;    /* protocol ID for higher-level protocol */
    int			qpos; /* Position of new req in queue       */
    int			dpos; /* Position of dupe in queue          */
    RREQ		creq; /* Current request                    */
    RREQ		treq; /* Temporary request pointer          */
    RREQ		nreq; /* New request pointer                */
    RREQ		areq = NOREQ; /* Request needing ack        */
    RREQ                match_in_runQ = NOREQ; /* if match found in runq for
                                                  completed request. */
    int			hdr_len;
    char		*ctlptr;
    short		stmp;
    int			tmp;
    int			check_for_ack = 1; 
    fd_set		readfds;	               /* Used for select */
    long		now;		      /* Time - used for retries  */
    long		rr_time = 0; /* Time last retrans from done queue */

    struct timeval time_out; 
    time_out.tv_sec = timeout;
    time_out.tv_usec = usec;


#ifdef PFS_THREADS
    /* Changed from original interface.  This will try the lock and return 
	an error if unsuccessful.  Otherwise it will get the lock */
    if (p_th_mutex_trylock(p_th_mutexARDP_ACCEPT))
        return ARDP_SUCCESS;    /* Being retrieved */
#endif
 check_for_more:
    /* This is initialized afresh before each select().  Some operating
       systems, such as LINUX, modify the time_out parameter to SELECT().  
       --swa, 6/19/94 */

    time_out.tv_sec = timeout;
    time_out.tv_usec = usec;
    now = time(NULL);

    /* Check both the prived and unprived ports if necessary */
    FD_ZERO(&readfds);
    FD_SET(ardp_srvport, &readfds);
    if(ardp_prvport != -1) FD_SET(ardp_prvport, &readfds); 
    tmp = select(max(ardp_srvport,ardp_prvport) + 1,
		 &readfds,(fd_set *)0,(fd_set *)0,&time_out);
    
    if(tmp == 0) {
	if(areq) ardp_acknowledge(areq); areq = NOREQ;
        p_th_mutex_unlock(p_th_mutexARDP_ACCEPT);
	return ARDP_SUCCESS;
    }
    if(tmp < 0) {
        p_th_mutex_unlock(p_th_mutexARDP_ACCEPT);
        return ARDP_SELECT_FAILED;
    }
    creq = ardp_rqalloc();
    pkt = ardp_ptalloc();
    
    /* There is a message waiting, add it to the queue */
    
    fromlen = sizeof(from);
    if((ardp_prvport >= 0) && FD_ISSET(ardp_prvport,&readfds))
	n = recvfrom(ardp_prvport, pkt->dat, ARDP_PTXT_LEN_R, 0, 
		     (struct sockaddr *) &from, &fromlen);
    else {
        assert(FD_ISSET(ardp_srvport, &readfds));
        n = recvfrom(ardp_srvport, pkt->dat, ARDP_PTXT_LEN_R, 0, 
                     (struct sockaddr *) &from, &fromlen);
    }
    if (n <= 0) {
	plog(L_NET_ERR,NOREQ,"Bad recvfrom n = %d errno = %d %s",
	     n, errno, unixerrstr(), 0);

        /* I added this in response to a problem experienced by Bunyip. 
           --swa, 3/21/95 */
        if (errno == 9) {
            if ((ardp_prvport >= 0) && FD_ISSET(ardp_prvport, &readfds))
                plog(L_NET_ERR, NOREQ, "The bad descriptor was descriptor #%d,
represented internally by the variable ardp_prvport", ardp_prvport);
            else if (FD_ISSET(ardp_srvport, &readfds))
                plog(L_NET_ERR, NOREQ, "The bad descriptor was descriptor #%d,
represented internally by the variable ardp_srvport", ardp_srvport);
            else
                internal_error("Something is wrong with your system's version
of select().  No descriptors are readable; unclear how we got here.");

            plog(L_NET_ERR, NOREQ, 
                 "This should never happen.  Attempting to restart server with SIGUSR1.");
            kill(getpid(), SIGUSR1); /* sending restart signal to Prospero 
                                      server or to whatever we hand*/
            
        }
	ardp_rqfree(creq); ardp_ptfree(pkt);
        p_th_mutex_unlock(p_th_mutexARDP_ACCEPT);
	return ARDP_BAD_RECV;
    }
    
    
    bcopy(&from,&(creq->peer),sizeof(creq->peer));
    creq->cid = 0; creq->peer_ardp_version = 0;
    creq->rcvd_time.tv_sec = now;
    creq->prcvd_thru = 0;

    if ( ! verify_acl_list(&from) ) {
      ardp_breply(creq, ARDP_R_COMPLETE, acl_message(),0);
      ardp_ptfree(pkt);
      goto check_for_more;
    }
    
    pkt->start = pkt->dat;
    pkt->length = n;
    *(pkt->start + pkt->length) = '\0';
    pkt->mbz = 0; /* force zeros to catch runaway strings     */
    
    pkt->seq = 1;
    if((hdr_len = (unsigned char) *(pkt->start)) < 32) {
	ctlptr = pkt->start + 1;
	pkt->seq = 0;
	if(hdr_len >= 3) { 	/* Connection ID */
	    bcopy2(ctlptr,&stmp);
	    if(stmp) creq->cid = ntohs(stmp);
	    ctlptr += 2;
	}
	if(hdr_len >= 5) {	/* Packet number */
	    bcopy2(ctlptr,&stmp);
	    pkt->seq = ntohs(stmp);
	}
	else { /* No packet number specified, so this is the only one */
	    pkt->seq = 1;
	    creq->rcvd_tot = 1;
	}
	ctlptr += 2;
	if(hdr_len >= 7) {	    /* Total number of packets */
	    bcopy2(ctlptr,&stmp);  /* 0 means don't know      */
	    if(stmp) creq->rcvd_tot = ntohs(stmp);
	}
	ctlptr += 2;
	if(hdr_len >= 9) {	/* Receievd through */
	    bcopy2(ctlptr,&stmp);  /* 0 means don't know      */
	    if(stmp) {
		creq->prcvd_thru = ntohs(stmp);
	    }
	}
	ctlptr += 2;
	if(hdr_len >= 11) {	/* Backoff */
	    /* Not supported by server */
	}
	ctlptr += 2;
	
	flags1 = flags2 = 0;
	if(hdr_len >= 12) flags1 = *ctlptr++; /* Flags */
	if(hdr_len >= 13) flags2 = *ctlptr++; /* Flags */
	
	if(flags2 == 1) { /* Cancel request */
            EXTERN_MUTEXED_LOCK(ardp_pendingQ);
	    treq = ardp_pendingQ;
	    while(treq) {
		if((treq->cid == creq->cid) && 
		   (treq->peer_port == creq->peer_port) &&
		   (treq->peer_addr.s_addr == creq->peer_addr.s_addr)){
		    plog(L_QUEUE_INFO,treq,
			 "Request canceled by client - dequeued",0);
		    pQlen--;if(treq->priority > 0) pQNlen--;
		    EXTRACT_ITEM(treq,ardp_pendingQ);
		    ardp_rqfree(treq);
		    ardp_rqfree(creq); ardp_ptfree(pkt);
                    EXTERN_MUTEXED_UNLOCK(ardp_pendingQ);
		    goto check_for_more;
		}
		treq = treq->next;
	    }
	    plog(L_QUEUE_INFO,creq,
		 "Request canceled by client - not on queue",0);
	    ardp_rqfree(creq); ardp_ptfree(pkt);
            EXTERN_MUTEXED_UNLOCK(ardp_pendingQ);
	    goto check_for_more;
	}
	
        /* If the client specifies option flags and doesn't specify control
           info to match them, we probably should log this; right now we just
           silently ignore it. */
        if (ctlptr < pkt->start + hdr_len) /* if still control info */ {
            /* Priority specified. */
	    if(flags1 & 0x2) {
                bcopy2(ctlptr,&stmp);
                creq->priority = ntohs(stmp);
                ctlptr += 2;
            }
        }
        if (ctlptr < pkt->start + hdr_len) /* if still control info */ {
            /* Higher-level protocol ID follows. */
            /* N.B.: used to be a bug here where this was 0x1; this is in
               variance with the ARDP spec and I've corrected the
               implementation to conform to the spec. */
	    if(flags1 & 0x4) {
                bcopy2(ctlptr,&stmp);
                pid = ntohs(stmp);
                ctlptr += 2;
            }
        }
        if (ctlptr < pkt->start + hdr_len) /* if still control info */ {
            /* Window size follows (2 octets of information). */
	    if(flags1 & 0x8) {
                bcopy2(ctlptr,&stmp);
                creq->pwindow_sz = ntohs(stmp);
                ctlptr += 2;
                if (creq->pwindow_sz == 0) {
                    creq->pwindow_sz = ARDP_DEFAULT_WINDOW_SZ;
                    plog(L_NET_INFO, creq, 
                         "Client explicity reset window size to server \
default (%d)",
                         ARDP_DEFAULT_WINDOW_SZ);
                } else {
                    if (creq->pwindow_sz > ARDP_MAX_WINDOW_SZ) {
                        plog(L_NET_INFO, creq, "Client set window size \
to %d; server will limit it to %d", creq->pwindow_sz, ARDP_MAX_WINDOW_SZ);
                        creq->pwindow_sz = ARDP_MAX_WINDOW_SZ;
                    } else {
                        plog(L_NET_INFO, creq, 
                             "Client set window size to %d", creq->pwindow_sz);
                    }
                }
            }
        }
	
	if((creq->priority < 0) && (creq->priority != -42)) {
	    plog(L_NET_RDPERR, creq, "Priority %d requested - ignored", creq->priority, 0);
	    creq->priority = 0;
	}
	
	if(pkt->seq == 1) creq->rcvd_thru = max(creq->rcvd_thru, 1);
	
	pkt->length -= hdr_len;
	pkt->start += hdr_len;
        pkt->text = pkt->start;
    }
    else {
	/* When we are no longer getting any of these old format  */
	/* requests, we know that everyone is using the new       */
	/* reliable datagram protocol, and that they also         */
	/* have the singleton response bug fixed.  We can then    */
	/* get rid of the special casing for singleton responses  */
	
	/* Lower the priority to encourage people to upgrade  */
	creq->priority = 1;
	creq->peer_ardp_version = -1;
#ifdef LOGOLDFORMAT
	plog(L_DIR_PWARN,creq,"Old RDP format",0);
#endif LOGOLDFORMAT
	pkt->seq = 1;
	creq->rcvd_tot = 1;
	creq->rcvd_thru = 1;
    }
    
    /* Check to see if it is already on done, partial, or pending */
    
    /* Done queue */
    EXTERN_MUTEXED_LOCK(ardp_doneQ);
    treq = ardp_doneQ;
    CHECK_HEAD(treq);
    while(treq) {
	if((treq->cid != 0) && (treq->cid == creq->cid) &&
	   (bcmp((char *) &(treq->peer),
		 (char *) &from,sizeof(from))==0)){
	    /* Request is already on doneQ */
	    if(creq->prcvd_thru > treq->prcvd_thru) {
		treq->prcvd_thru = creq->prcvd_thru;
		rr_time = 0; /* made progress, don't supress retransmission */
	    }
	    nreq = treq; 
	    
	    /* Retransmit reply if not completely received */
	    /* and if we didn't retransmit it this second  */
	    if((nreq->prcvd_thru != nreq->trns_tot) &&
	       ((rr_time != now) || (nreq != ardp_doneQ))) {
                PTEXT		tpkt; /* Temporary pkt pointer */

		plog(L_QUEUE_INFO,nreq,"Retransmitting reply (%d of %d ack)",
		     nreq->prcvd_thru, nreq->trns_tot, 0);
		/* Transmit all outstanding packets */
		for (tpkt = nreq->trns; tpkt; tpkt = tpkt->next) {
		    if((tpkt->seq <= (nreq->prcvd_thru + nreq->pwindow_sz)) &&
		       ((tpkt->seq == 0) || (tpkt->seq > nreq->prcvd_thru))) {
                        /* (A) Request an ack at the end of a window
                           (B) Request an ack in the last packet if the 
                               service has rescinded a wait request, but we
                               aren't sure that the client knows this.
                         */
                        ardp_header_ack_rwait(tpkt, nreq, 
                                              /* do set ACK BIT: */
                                              (/*A*/ tpkt->seq == 
                                               (nreq->prcvd_thru 
                                               + nreq->pwindow_sz))
                                              || /*B*/ 
                                              ((tpkt->seq == nreq->trns_tot)
                                               /* last pkt */
                                               && (nreq->svc_rwait_seq >
                                                   nreq->prcvd_thru)),
                                              /* Might an  RWAIT be needed? */
                                              (nreq->svc_rwait_seq >
                                              nreq->prcvd_thru));
			ardp_snd_pkt(tpkt,nreq);
                    }
		}
		rr_time = now; /* Remember time of retransmission */
	    }
	    /* Move matched request to front of queue */
            /* nreq is definitely in ardp_doneQ right now. */
            /* This replaces much icky special-case code that used to be here
               -- swa, Feb 9, 1994 */
            EXTRACT_ITEM(nreq, ardp_doneQ);
            PREPEND_ITEM(nreq, ardp_doneQ);
	    ardp_rqfree(creq); ardp_ptfree(pkt);
            EXTERN_MUTEXED_UNLOCK(ardp_doneQ);
	    goto check_for_more;
	}
	/* While we're checking the done queue also see if any    */
	/* replies require follow up requests for acknowledgments */
        /* This is currently used when the server has requested that the client
           wait for a long time, and then has rescinded that wait request
           (because the message is done).  Such a rescinding of the wait
           request should be received by the client, and the server is
           expecting an ACK. */
        /* You can add additional tests here if there are other circumstances
           under which you want the server to expect the client to send an
           acknowledgement. */
	if(check_for_ack && (treq->svc_rwait_seq > treq->prcvd_thru) && 
	   (treq->retries_rem > 0) && (now >= treq->wait_till.tv_sec)) {
#define ARDP_LOG_SPONTANEOUS_RETRANSMISSIONS
#ifdef ARDP_LOG_SPONTANEOUS_RETRANSMISSIONS
	    plog(L_QUEUE_INFO,treq,"Requested ack not received - pinging client (%d of %d/%d ack)",
		 treq->prcvd_thru, treq->svc_rwait_seq, treq->trns_tot, 0);
#endif /* ARDP_LOG_SPONTANEOUS_RETRANSMISSIONS */
	    /* Resend the final packet only - to wake the client  */
	    if(treq->trns) ardp_snd_pkt(treq->trns->previous,treq);
	    treq->timeout_adj.tv_sec = ARDP_BACKOFF(treq->timeout_adj.tv_sec);
	    treq->wait_till.tv_sec = now + treq->timeout_adj.tv_sec;
	    treq->retries_rem--;
	}
        CHECK_NEXT(treq);
	treq = treq->next;
    } /*while*/
    EXTERN_MUTEXED_UNLOCK(ardp_doneQ);
    check_for_ack = 0; /* Only check once per call to ardp_accept */
    
    /* If unsequenced control packet free it and check for more */
    if(pkt->seq == 0) {
	ardp_rqfree(creq); ardp_ptfree(pkt);
	goto check_for_more;
    }

    /* Check if incomplete and match up with other incomplete requests */
    if(creq->rcvd_tot != 1) { /* Look for rest of request */
	treq = ardp_partialQ;
	while(treq) {
	    if((treq->cid != 0) && (treq->cid == creq->cid) &&
	       (bcmp((char *) &(treq->peer),
		     (char *) &from,sizeof(from))==0)) {
		/* We found the rest of the request     */
		/* coallesce and log and check_for more */
                PTEXT		tpkt; /* Temporary pkt pointer */

		ardp_update_cfields(treq,creq);
		if(creq->rcvd_tot) treq->rcvd_tot = creq->rcvd_tot;
		tpkt = treq->rcvd;
		while(tpkt) {
		    if(tpkt->seq == treq->rcvd_thru+1) treq->rcvd_thru++;
		    if(tpkt->seq == pkt->seq) {
			/* Duplicate - discard */
			plog(L_NET_INFO,creq,"Multi-packet duplicate received (pkt %d of %d)",
			     pkt->seq, creq->rcvd_tot, 0);
			if(areq != treq) {
			    if(areq) {
				ardp_acknowledge(areq);
			    }
			    areq = treq;
			}
			ardp_rqfree(creq); ardp_ptfree(pkt);
			goto check_for_more;
		    }
		    if(tpkt->seq > pkt->seq) {
			/* Insert new packet in rcvd */
			pkt->next = tpkt;
			pkt->previous = tpkt->previous;
			if(treq->rcvd == tpkt) treq->rcvd = pkt;
			else tpkt->previous->next = pkt;
			tpkt->previous = pkt;
			if(pkt->seq == treq->rcvd_thru+1) treq->rcvd_thru++;
			while(tpkt && (tpkt->seq == treq->rcvd_thru+1)) {
			    treq->rcvd_thru++;
			    tpkt = tpkt->next;
			}
			pkt = NOPKT;
			break;
		    }
		    tpkt = tpkt->next;
		}
		if(pkt) { /* Append at end of rcvd */
		    APPEND_ITEM(pkt,treq->rcvd); 
		    if(pkt->seq == treq->rcvd_thru+1) treq->rcvd_thru++;
		    pkt = NOPKT;
		}
		if(treq->rcvd_tot && (treq->rcvd_thru == treq->rcvd_tot)) {
		    if(areq == treq) areq = NOREQ;
		    creq = treq; EXTRACT_ITEM(creq, ardp_partialQ); --ptQlen;
		    plog(L_NET_INFO,creq,"Multi-packet request complete",0);
		    goto req_complete;
		}
		else {
		    if(areq != treq) {
			if(areq) {
			    ardp_acknowledge(areq);
			}
			areq = treq;
		    }
		    plog(L_NET_INFO, creq,
			 "Multi-packet request continued (rcvd %d of %d)",
			 creq->rcvd_thru, creq->rcvd_tot, 0);
		    goto check_for_more;
		}
	    }
	    treq = treq->next;
	}
 	/* This is the first packet we received in the request */
	/* log it and queue it and check for more              */
	plog(L_NET_INFO,creq,"Multi-packet request received (pkt %d of %d)",
	     pkt->seq, creq->rcvd_tot, 0);
	APPEND_ITEM(pkt,creq->rcvd);
	APPEND_ITEM(creq,ardp_partialQ); /* Add at end of incomp queue   */
	if(++ptQlen > ptQmaxlen) {       
	    treq = ardp_partialQ;        /* Remove from head of queue    */
	    EXTRACT_ITEM(ardp_partialQ,ardp_partialQ); ptQlen--;
	    plog(L_NET_ERR, treq,
		 "Too many incomplete requests - dropped (rthru %d of %d)",
		 treq->rcvd_thru, treq->rcvd_tot, 0);
	    ardp_rqfree(treq);    
	}
	goto check_for_more;
    }
    
    plog(L_NET_INFO, creq, "Received packet", 0);
    
 req_complete:

    qpos = 0; dpos = 1;

    /* Insert this message at end of pending but make sure the  */
    /* request is not already in pending                        */
    nreq = creq; creq = NOREQ;
    EXTERN_MUTEXED_LOCK(ardp_pendingQ);
    treq = ardp_pendingQ;
	
    if(pkt) {
	nreq->rcvd_tot = 1;
	APPEND_ITEM(pkt,nreq->rcvd);
    }
    
    EXTERN_MUTEXED_LOCK(ardp_runQ);
    for (match_in_runQ = ardp_runQ; match_in_runQ; 
         match_in_runQ = match_in_runQ->next) {
        if((match_in_runQ->cid == nreq->cid) && (match_in_runQ->cid != 0) &&
           (bcmp((char *) &(match_in_runQ->peer),
                 (char *) &(nreq->peer),sizeof(nreq->peer))==0)) {
            /* Request is running right now */
            ardp_update_cfields(match_in_runQ,nreq);
            plog(L_QUEUE_INFO,match_in_runQ,"Duplicate discarded (presently executing)",0);
	    /*!! This looks strange - freeing but not removing from ardp_runQ*/
            ardp_rqfree(nreq);
            nreq = match_in_runQ;
            break;              /* leave match_in_runQ set to nreq  */
        }
    }
    EXTERN_MUTEXED_UNLOCK(ardp_runQ);
    if (match_in_runQ) {
                                /* do nothing; handled above. */
    } else if(ardp_pendingQ) {
        int     keep_looking = 1; /* Keep looking for dup.  Initially say to
                                     keep on looking. */
        
	while(treq) {
	    if((treq->cid != 0) && (treq->cid == nreq->cid) &&
	       (bcmp((char *) &(treq->peer),
		     (char *) &(nreq->peer),sizeof(treq->peer))==0)){
		/* Request is already on queue */
		ardp_update_cfields(treq,nreq);
		plog(L_QUEUE_INFO,treq,"Duplicate discarded (%d of %d)",dpos,pQlen,0);
		ardp_rqfree(nreq);
		nreq = treq;
		keep_looking = 0;  /* We found the duplicate */
		break;
	    }
	    if((ardp_pri_override && ardp_pri_func && (ardp_pri_func(nreq,treq) < 0)) ||
	       (!ardp_pri_override && ((nreq->priority < treq->priority) ||
				       ((treq->priority == nreq->priority) && ardp_pri_func &&
					(ardp_pri_func(nreq,treq) < 0))))) {
		if(ardp_pendingQ == treq) {
		    nreq->previous = ardp_pendingQ->previous;
		    nreq->next = ardp_pendingQ;
		    ardp_pendingQ->previous = nreq;
		    ardp_pendingQ = nreq;
		}
		else {
		    nreq->next = treq;
		    nreq->previous = treq->previous;
		    nreq->previous->next = nreq;
		    treq->previous = nreq;
		}
		pQlen++;
		if(nreq->priority > 0) pQNlen++;
		LOG_PACKET(nreq, dpos);
		qpos = dpos;
		keep_looking = 1;  /* There may still be a duplicate */
		break;
	    }
	    if(!treq->next) {
		treq->next = nreq;
		nreq->previous = treq;
		ardp_pendingQ->previous = nreq;
		pQlen++;
		if(nreq->priority > 0) pQNlen++;
		LOG_PACKET(nreq,dpos+1);
		qpos = dpos+1;
		keep_looking = 0; /* Nothing more to check */
		break;
	    }
	    treq = treq->next;
	    dpos++;
	}
	/* Save queue position to send to client if appropriate */
	qpos = dpos;
	/* If not added at end of queue, check behind packet for dup */
	if(keep_looking) {
	    while(treq) {
		if((treq->cid == nreq->cid) && (treq->cid != 0) && 
		   (bcmp((char *) &(treq->peer),
			 (char *) &(nreq->peer),
			 sizeof(treq->peer))==0)){
		    /* Found a dup */
		    plog(L_QUEUE_INFO,treq,"Duplicate replaced (removed at %d)", dpos, 0);
		    pQlen--;if(treq->priority > 0) pQNlen--;
		    EXTRACT_ITEM(treq,ardp_pendingQ);
		    ardp_rqfree(treq);
		    break;
		}
		treq = treq->next;
		dpos++;
	    }
	} /* if keep_looking */
    }
    else {
	nreq->next = NULL;
	ardp_pendingQ = nreq;
	ardp_pendingQ->previous = nreq;
	pQlen++;if(nreq->priority > 0) pQNlen++;
	LOG_PACKET(nreq, dpos);
	qpos = dpos;
    }
    EXTERN_MUTEXED_UNLOCK(ardp_pendingQ);
    
    /* Fill in queue position and system time             */
    nreq->inf_queue_pos = qpos;
    /* nreq->inf_sys_time = **compute-system-time-here**  */

#ifdef ARCHIE
    nreq -> inf_sys_time = arch_get_etime(nreq);
#endif

    /* Here we can optionally send a message telling the  */
    /* client to back off.  How long should depend on the */
    /* queue length and the mean service time             */
    /* 15 min for now - the archie server is overloaded   */
#ifdef ARCHIE
    /* This is an old version of the Prospero clients that only archie servers
       will need to be able to talk to, so don't bother doing this test
       for anything but an archie server. */
    if((nreq->peer_ardp_version >= 0) && (sindex(nreq->rcvd->start,"VERSION 1")))
      nreq->peer_ardp_version = -2;
    /* Archie servers should request a longer wait.   Other heavily used
       databases may want to have this command defined too. */
    ardp_rwait(nreq,900,nreq->inf_queue_pos,nreq->inf_sys_time); 
#endif

    goto check_for_more;
}



static 
void
ardp_update_cfields(RREQ existing,RREQ newvalues)
{
    if(newvalues->prcvd_thru > existing->prcvd_thru)
	existing->prcvd_thru = newvalues->prcvd_thru;
}

/* Rewrite the header of TPKT to conform to the status in nreq.  The only
   fields 
   we need to set are possibly the ACK bit in octet 11 and possibly the rwait
   field in octets 9 & 10.  We always send the received-through field for the
   sake of simplicity of implementation.  */
static void ardp_header_ack_rwait(PTEXT tpkt, RREQ nreq, int is_ack_needed, 
                                  int is_rwait_needed)
{
    int new_hlength;            /* new header length. */
    int old_hlength;            /* old header length */
    unsigned short us_tmp;      /* tmp for values. */
    old_hlength = tpkt->text - tpkt->start;
    /* If rwait needed, leave room for it. */
    new_hlength = 9;            /* Include room for received-through count */
    if (is_rwait_needed) 
        new_hlength = 11;
    if (is_ack_needed) new_hlength = 12;

    /* Allocate space for new header. */
    tpkt->start = tpkt->text - new_hlength;
    tpkt->length += new_hlength - old_hlength;
    /* Set the header length and version # (zeroth octet) */
    /* Version # is zero in this version of the ARDP library; last 6 bytes
       of octet are header length. */
    *(tpkt->start) = (char) new_hlength;
    /* Set octets 1 through 8 of header */
    /* Connection ID (octets 1 & 2) */
    bcopy2(&(nreq->cid),tpkt->start+1);
    /* Sequence number (octets 3 & 4) */
    us_tmp = htons(tpkt->seq);
    bcopy2(&us_tmp,tpkt->start+3);	
    /* Total packet count (octets 5 & 6) */
    us_tmp = htons(nreq->trns_tot);
    bcopy2(&us_tmp,tpkt->start+5);	
    /* Received through (octets 7 & 8) */
    us_tmp = htons(nreq->rcvd_thru);
    bcopy2(&us_tmp,tpkt->start+7);	
    /* Now set any options. */
    /* expected time 'till response */
    if (new_hlength > 9) {
        us_tmp = htons(nreq->svc_rwait);
        bcopy2(&us_tmp, tpkt->start+9);
    }
    if (new_hlength > 11) {
        tpkt->start[11] = 
            is_ack_needed ? 0x80 /* ACK BIT */ : 0x00 /* Don't ack */;
    }
}

int
ardp_accept() {
  return ardp_accept_and_wait(0,0);
}


