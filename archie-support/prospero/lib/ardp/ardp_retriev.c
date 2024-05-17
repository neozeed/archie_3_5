/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * Written  by bcn 1/93     to check and/or wait for completion of request
 */

#include <usc-copyr.h>

#include <stdio.h>
#ifdef AIX
#include <sys/select.h>
#endif

#include <ardp.h>
#include <perrno.h>
#include <pmachine.h>

extern RREQ	      ardp_activeQ;  /* Info about active requests */
extern RREQ	      ardp_completeQ;/* Info about completed reqs  */
extern int	      ardp_port;     /* Opened UDP port		   */
extern int	      pfs_debug;     /* Debug level                */

/*
 * ardp_retrieve - check and/or wait for completion of request
 *
 *   ardp_retrieve takes a request and a time to wait.  It will check to
 *   see if the request is complete and if so, returns ARDP_SUCCESS.
 *   If not complete,  ardp_retrieve will wait up till the time to wait
 *   before returning.  If still incomplete it will return ARDP_PENDING.
 *   A time to wait of -1 indicates that one should not return until
 *   complete, or until a timeout occurs. On any failure (other than
 *   still pending), an error code is returned.
 *
 *   BUGS: Right now, only accepts ttwait values of 0 and -1.  If
 *         positive, it is currently treated as -1.
 */
int
ardp_retrieve(req,ttwait)
    RREQ		req;	  /* Request to wait for           */
    int			ttwait;   /* Time to wait in microseconds  */
{
    fd_set		readfds;  /* Used for select               */
    struct timeval	*selwait; /* Time to wait for select       */
    int			tmp;	  /* Hold value returned by select */

    /* XXX For now only support ttwait values of 0 and -1 */
    if(ttwait > 0) return(ARDP_FAILURE);

    p_clear_errors();

    if(req->status == ARDP_STATUS_FREE) {
	fprintf(stderr,"Attempt to retrieve free RREQ\n");
	abort();
	return(ARDP_BAD_REQ);
    }

    if(req->status == ARDP_STATUS_NOSTART) {
	perrno = ARDP_BAD_REQ;
	return(perrno);
    }

 check_for_more:

    ardp_process_active();
    if((req->status == ARDP_STATUS_COMPLETE) || (req->status > 0)) {
	EXTRACT_ITEM(req,ardp_completeQ);
	if(pfs_debug >= 9) {
            PTEXT		ptmp;	  /* debug-step through req->rcvd  */
	    if(req->status > 0) fprintf(stderr,"Request failed (error %d)!",
					req->status);
	    else fprintf(stderr,"Packets received...");
	    ptmp = req->rcvd;
	    while(ptmp) {
		fprintf(stderr,"Packet %d:\n",ptmp->seq);
                ardp_showbuf(ptmp->start, ptmp->length, stderr);
                putc('\n', stderr);
		ptmp = ptmp->next;
	    }
	    (void) fflush(stderr);
	}
	if(req->status == ARDP_STATUS_COMPLETE) return(ARDP_SUCCESS);
	else {
	    perrno = req->status;   /* Specific error */
	    return(perrno);
	}
    }
    if(ttwait == 0) return(ARDP_PENDING);

    /* Here we should figure out how long to wait, a minimum of */
    /* ttwait, or the first retry timer for any pending request */
    /* For the time being, we use the retry timer of the        */
    /* current request.                                         */

    if (pfs_debug >= 6) fprintf(stderr,"Waiting for reply...");

    FD_ZERO(&readfds);
    FD_SET(ardp_port, &readfds);

    selwait = &(req->timeout_adj);

    /* select - either recv is ready, or timeout */
    /* see if timeout or error or wrong descriptor */
    tmp = select(ardp_port + 1, &readfds, (fd_set *)0, (fd_set *)0, selwait);

    /* Packet received, or timeout - both handled by ardp_process_active */
    if(tmp >= 0) goto check_for_more;

    if (pfs_debug) fprintf(stderr, "select failed: returned %d\n", tmp);
    perrno = ARDP_SELECT_FAILED;
    return(perrno);
}
