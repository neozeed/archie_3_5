/*
 * Copyright (c) 1991, 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 *
 * Written  by bcn 1989-92  as part of dirsend.c in the Prospero distribution
 * Modified by bcn 1/93     moved to ardp library - added asynchrony 
 */

#include <usc-license.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef AIX
#include <sys/select.h>
#endif

#include <ardp.h>
#include <perrno.h> 
#include <pmachine.h>           /* for bcopy() */

extern RREQ	   ardp_activeQ;      /* Info about active requests */
extern RREQ	   ardp_completeQ;    /* Info about completed reqs  */
extern int	   ardp_port;         /* Opened UDP port	    */
extern int	   pfs_debug;         /* Debug level                */
int         ardp_activeQ_len;         /* Length of activeQ          */

static struct timeval  zerotime = {0, 0}; /* Used by select        */

#define max(x,y) (x > y ? x : y)

static ardp__pr_ferror(int errcode);

/*
 * ardp_process_active - process active requests
 *
 *   ardp_process_active takes no arguments.  It checks to see if any responses
 *   are pending on the UDP port, and if so processes them, adding them
 *   to the request structures on ardp_activeQ.  If no requests are pending
 *   ardp_process_active processes and requests requiring retries then
 *   returns.
 *
 */
int
ardp_process_active()
{
    fd_set		readfds;     /* Used for select		      */
    struct sockaddr_in	from;	     /* Reply received from	      */
    int			from_sz;     /* Size of address structure     */
    int			hdr_len;     /* Header Length                 */
    char		*ctlptr;     /* Pointer to control field      */
    unsigned short	cid = 0;     /* Connection ID from rcvd pkt   */
    unsigned char 	rdflag11;    /* First byte of flags (bit vect)*/
    unsigned char 	rdflag12;    /* Second byte of flags (int)    */
    unsigned char	tchar;	     /* For decoding extra fields     */
    unsigned short	stmp;	     /* Temp short for conversions    */
    unsigned long	ltmp;        /* Temp long for converions      */
    int			tmp;	     /* Temporaty return value        */
    int			nr;	     /* Number octest received        */
    long		now;	     /* The current time              */

    RREQ	req = NOREQ;	     /* To store request info         */
    PTEXT	pkt = NOPKT;	     /* Packet being processed        */
    PTEXT	ptmp = NOPKT;	     /* Packet being processed        */

    assert(P_IS_THIS_THREAD_MASTER()); /* something_received not MT-Safe */
 check_for_pending:

    /* Set list of file descriptors to be checked by select */
    FD_ZERO(&readfds);
    FD_SET(ardp_port, &readfds);

    /* select - either recv is ready, or not       */
    /* see if timeout or error or wrong descriptor */
    tmp = select(ardp_port + 1, &readfds, (fd_set *)0, (fd_set *)0, &zerotime);

    /* If tmp is zero, then nothing to process, check for timeouts */
    if(tmp == 0) {
	now = time(NULL);
	req = ardp_activeQ;
	while(req) {
            if (req->status == ARDP_STATUS_ACKPEND) {
                ardp_xmit(req, -1 /* just ACK; send no data packets */);
                req->status = ARDP_STATUS_ACTIVE;
            }
            if((req->wait_till.tv_sec) && (now >= req->wait_till.tv_sec)) {
                if(req->retries_rem-- > 0) {
                    req->timeout_adj.tv_sec = ARDP_BACKOFF(req->timeout_adj.tv_sec);
                    if (pfs_debug >= 8) {
                        fprintf(stderr,"Connection %d timed out - Setting timeout to %d seconds.\n",
                                ntohs(req->cid), (int) req->timeout_adj.tv_sec);
                    }
                    req->wait_till.tv_sec = now + req->timeout_adj.tv_sec;
                    ardp_xmit(req, req->pwindow_sz);
                } else { 
                    if(pfs_debug >= 8) {
                        fprintf(stderr,"Connection %d timed out - Retry count exceeded.\n",
			    ntohs(req->cid) );
                        (void) fflush(stderr);
                    }
                    req->status = ARDP_TIMEOUT;
                    EXTRACT_ITEM(req,ardp_activeQ);
                    --ardp_activeQ_len;
                    APPEND_ITEM(req,ardp_completeQ);
                }
            }
            req = req->next;
        }
        return(ARDP_SUCCESS);
    }

    /* If negative, then return an error */ 
    if((tmp < 0) || !FD_ISSET(ardp_port, &readfds)) {
	if (pfs_debug) 
		fprintf(stderr, "select failed : returned %d port=%d\n", 
		tmp, ardp_port);
		
        return(ardp__pr_ferror(ARDP_SELECT_FAILED));
    }
    
    /* Process incoming packets */
    pkt = ardp_ptalloc();
    pkt->start = pkt->dat;
    from_sz = sizeof(from);
    
    if ((nr = recvfrom(ardp_port, pkt->start, sizeof(pkt->dat),
		       0, &from, &from_sz)) < 0) {
        if (pfs_debug) perror("recvfrom");
	ardp_ptlfree(pkt); 
	return(ardp__pr_ferror(ARDP_BAD_RECV));
    }
    pkt->length = nr;
    *(pkt->start + pkt->length) = '\0';

    if (pfs_debug >= 6) 
       fprintf(stderr,"Received packet from %s\n", inet_ntoa(from.sin_addr));

    /* If the first byte is greater than 31 we have an old version        */
    /* response - return error                                            */
    if((hdr_len = (unsigned char) *(pkt->start)) > 31) {
	ardp_ptfree(pkt); 
	if(ardp_activeQ->next == NOREQ)
	    return(ardp__pr_ferror(ARDP_BAD_VERSION));
	else goto check_for_pending;
    }

    /* For the current format, the first two bits are a version number    */
    /* and the next six are the header length (including the first byte). */
    ctlptr = pkt->start + 1;

    if(hdr_len >= 3) { 	/* Connection ID */
	bcopy2(ctlptr,&stmp);
	if(stmp) cid = stmp;
	ctlptr += 2;
    }

    /* Match up response with request */
    req = ardp_activeQ;
    while(req) {
	if(cid == req->cid) break;
	req = req->next;
    }
    if(!req) { /* The request isn't on the active queue */
	if (pfs_debug>=6) 
	    fprintf(stderr,"Packet received for inactive request (cid %d)\n",
		    ntohs(cid));
	ardp_ptfree(pkt); 
	/* The following can be removed when old servers are gone */
	if((cid == 0) && (ardp_activeQ->next == NOREQ))
	    return(ardp__pr_ferror(ARDP_BAD_VERSION));
	goto check_for_pending;
    }

    if(hdr_len >= 5) {	/* Packet number */
	bcopy2(ctlptr,&stmp);
	pkt->seq = ntohs(stmp);
	ctlptr += 2;
    }
    else { /* No packet number specified, so this is the only one */
	pkt->seq = 1;
	req->rcvd_tot = 1;
    }
    if(hdr_len >= 7) {	    /* Total number of packets */
	bcopy2(ctlptr,&stmp);  /* 0 means don't know      */
	if(stmp) req->rcvd_tot = ntohs(stmp);
	ctlptr += 2;
    }
    if(hdr_len >= 9) {	/* Receievd through */
	bcopy2(ctlptr,&stmp);  /* 1 means received request */
	req->prcvd_thru = max(ntohs(stmp),req->prcvd_thru);
	ctlptr += 2;
    }
    if(hdr_len >= 11) {	/* Service requested wait */
	bcopy2(ctlptr,&stmp);
	if(stmp || (req->svc_rwait != ntohs(stmp))) {
	    now = time(NULL);
	    /* New or non-zero requested wait value */
	    req->svc_rwait = ntohs(stmp);
	    if(pfs_debug >= 8) 
		fprintf(stderr,"Service asked us to wait %d seconds\n", 
			req->svc_rwait);
	    /* Adjust out timeout */
	    req->timeout_adj.tv_sec = max(req->timeout.tv_sec,req->svc_rwait);
	    req->wait_till.tv_sec = now + req->timeout_adj.tv_sec;
	    /* Reset the retry count */
	    req->retries_rem = req->retries;
	}
	ctlptr += 2;
    }
    if(hdr_len >= 12) {	/* Flags (1st byte) */
	bcopy1(ctlptr,&rdflag11);
	if(rdflag11 & 0x80) {
	    if(pfs_debug >= 8) 
		fprintf(stderr,"Ack requested\n");
	    req->status = ARDP_STATUS_ACKPEND;
	}
	if(rdflag11 & 0x40) {
	    if(pfs_debug >= 8) 
		fprintf(stderr,"Sequenced control packet\n");
	    pkt->length = -1;
	}
	ctlptr += 1;
    }
    if(hdr_len >= 13) {	/* Flags (2nd byte) */
	/* Reserved for future use */
	bcopy1(ctlptr,&rdflag12);
	ctlptr += 1;
	if(rdflag12 == 2) {
	    /* Attempt to set back prcvd_thru */
	    bcopy2(pkt->start+7,&stmp);  
	    req->prcvd_thru = ntohs(stmp);
	}
	if(rdflag12 == 1) {
	    /* ARDP Connection Refused */
	    if(pfs_debug >= 8) 
		fprintf(stderr,"***ARDP connection refused by server***\n");
	    req->status = ARDP_REFUSED;
	    EXTRACT_ITEM(req,ardp_activeQ);
	    --ardp_activeQ_len;
	    APPEND_ITEM(req,ardp_completeQ);
	    goto check_for_pending;
	}
	if(rdflag12 == 4) {
	    /* Server Redirect */
	    bcopy4(ctlptr,&(req->peer_addr));  
	    ctlptr += 4;
	    bcopy2(ctlptr,&(req->peer_port));  
	    ctlptr += 2;
	    if(pfs_debug >= 8) 
		fprintf(stderr,"***ARDP redirect to %s(%d)***\n",
			inet_ntoa(req->peer_addr),PEER_PORT(req));
	    ardp_xmit(req, req->pwindow_sz);
	}
	if(rdflag12 == 254) {
	    tchar = *ctlptr;
	    ctlptr++;
	    if(tchar & 0x1) { /* Queue position */
		bcopy2(ctlptr,&stmp);
		req->inf_queue_pos = ntohs(stmp);
		if(pfs_debug >= 8) 
		    fprintf(stderr,"Current queue position on server is %d\n",
			    req->inf_queue_pos);
		ctlptr += 2;
	    }
	    if(tchar & 0x2) { /* Expected system time */
		bcopy4(ctlptr,&ltmp);
		req->inf_sys_time = ntohl(ltmp);
		if(pfs_debug >= 8) 
		    fprintf(stderr,"Expected system time is %d seconds\n",
			    req->inf_sys_time);
		ctlptr += 4;
	    }
	}
    }
    /* If ->seq 0, then an unsequenced control packet */
    if(pkt->seq == 0) {
	ardp_ptfree(pkt);
	goto check_for_pending;
    }
    if(pkt->length >= 0) pkt->length -= hdr_len;
    pkt->start += hdr_len;
    pkt->text = pkt->start;
    pkt->ioptr = pkt->start;

    if(pfs_debug >= 8) fprintf(stderr,"Packet %d of %d (cid=%d)\n", pkt->seq,
			       req->rcvd_tot, ntohs(req->cid));
    
    if (req->rcvd == NOPKT) {
	/* Add it as the head of empty doubly linked list */
	req->rcvd = pkt; 
	pkt->previous = pkt;
	if(pkt->seq == 1) {
	    req->comp_thru = pkt;
	    req->rcvd_thru = 1;
	    /* If only one packet, then return it.  We will assume */
	    /* that it is not a sequenced control packet           */
	    if(req->rcvd_tot == 1) goto req_complete;
	}
	goto ins_done;
    }
	
    if(req->comp_thru && (pkt->seq <= req->rcvd_thru))
	ardp_ptfree(pkt); /* Duplicate */
    else if(pkt->seq < req->rcvd->seq) { /* First (sequentially) */
	pkt->next = req->rcvd;
	pkt->previous = req->rcvd->previous;
	req->rcvd->previous = pkt;
	req->rcvd = pkt;
	if(req->rcvd->seq == 1) {
	    req->comp_thru = req->rcvd;
	    req->rcvd_thru = 1;
	}
    }
    else { /* Insert later in the packet list unless duplicate */
	ptmp = (req->comp_thru ? req->comp_thru : req->rcvd);
	while (pkt->seq > ptmp->seq) {
	    if(ptmp->next == NULL) { 
		/* Insert at end */
		ptmp->next = pkt;
		pkt->previous = ptmp;
		pkt->next = NULL;
		req->rcvd->previous = pkt;
		goto ins_done;
	    }
	    ptmp = ptmp->next;
	}
	if(ptmp->seq == pkt->seq) /* Duplicate */
	    ardp_ptfree(pkt);
	else { /* insert before ptmp (we know were not first) */
	    ptmp->previous->next = pkt;
	    pkt->previous = ptmp->previous;
	    pkt->next = ptmp;
	    ptmp->previous = pkt;
	}
    }   
    
 ins_done:
    /* Find out how much is complete and remove sequenced control packets */
    while(req->comp_thru && req->comp_thru->next && 
	  (req->comp_thru->next->seq == (req->comp_thru->seq + 1))) {
	/* We have added one more in sequence               */
	if(pfs_debug >= 8) 
	    fprintf(stderr,"Packets now received through %d\n",
		    req->comp_thru->next->seq);

	if(req->comp_thru->length == -1) {
	    /* Old comp_thru was a sequenced control packet */
	    ptmp = req->comp_thru;
	    req->comp_thru = req->comp_thru->next;
	    req->rcvd_thru = req->comp_thru->seq;
	    EXTRACT_ITEM(ptmp,req->rcvd);
	    ardp_ptfree(ptmp);
	}
	else {
	    /* Old comp_thru was a data packet */
	    req->comp_thru = req->comp_thru->next;
	    req->rcvd_thru = req->comp_thru->seq;
	}

	/* Update outgoing packets */
	ardp_headers(req);

	/* We've made progress, so reset timeout and retry count */
	req->timeout_adj.tv_sec = max(req->timeout.tv_sec,req->svc_rwait);
	req->retries_rem = req->retries;
    }

    /* See if there are any gaps - possibly toggle GAP status */
    if(!req->comp_thru || req->comp_thru->next) {
	if (req->status == ARDP_STATUS_ACTIVE) req->status = ARDP_STATUS_GAPS;
    }
    else if (req->status == ARDP_STATUS_GAPS) req->status = ARDP_STATUS_ACTIVE;

    /* If incomplete, wait for more */
    if (!(req->comp_thru) || (req->comp_thru->seq != req->rcvd_tot))
	goto check_for_pending;

 req_complete:

    if (pfs_debug >= 7) {
	fprintf(stderr,"The complete response has been received.\n");
	(void) fflush(stderr);
    }

    if(req->status == ARDP_STATUS_ACKPEND) {
	if (pfs_debug >= 7) {
	    if (req->peer.sin_family == AF_INET)
		fprintf(stderr,"Acknowledging final packet to %s(%d)\n", 
			inet_ntoa(req->peer_addr), PEER_PORT(req));
            else fprintf(stderr,"Acknowledging final packet\n");
	    (void) fflush(stderr);
	}
	ardp_xmit(req, req->pwindow_sz);
    }

    req->status = ARDP_STATUS_COMPLETE;
    req->inpkt = req->rcvd;
    EXTRACT_ITEM(req,ardp_activeQ);
    --ardp_activeQ_len;
    APPEND_ITEM(req,ardp_completeQ);
    return(ARDP_SUCCESS);
}

static int
ardp__pr_ferror(int errcode)
{
    RREQ	creq;

    while(ardp_activeQ) {
	creq = ardp_activeQ;
	ardp_activeQ->status = errcode;
	EXTRACT_ITEM(creq,ardp_activeQ);
	--ardp_activeQ_len;
	APPEND_ITEM(creq,ardp_completeQ);
    }
    perrno = errcode;
    return(errcode);
}
