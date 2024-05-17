/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file 
 * <usc-license.h>
 *
 * Written  by bcn 1991     as part of the Prospero distribution
 * Modified by bcn 1/93     extended rreq structure
 * Modified by swa 11/93    multithreaded
 */

#include <usc-license.h>
#include <stdio.h>
#include <ardp.h>
#include <stdlib.h>             /* malloc or free */
#include <pmachine.h>           /* for bzero() */
#include <pfs.h>		/* for pafree and stfree */

/* When debugging the ARDP window flow control stuff, use these definitions. */
/* Defaults are set here. */
#if 0
#define ARDP_MY_WINDOW_SZ   2 /* Our window size;currently only implemented on
                                 the client.  Here, the client asks for a
                                 window size of 2, for testing. */
#undef ARDP_DEFAULT_WINDOW_SZ   /* Peer window size the server will use for its
                                   default flow-control strategy. */ */
#define ARDP_DEFAULT_WINDOW_SZ 1 /* for testing. */
#endif

static ardp_default_timeout = ARDP_DEFAULT_TIMEOUT;
static ardp_default_retry = ARDP_DEFAULT_RETRY;
int    ardp_priority = 0; /* Default priority */

static RREQ	lfree = NOREQ;
int 		rreq_count = 0;
int		rreq_max = 0;

#ifndef NDEBUG
static int rreq_member_of_list(RREQ rr, RREQ list);
#endif

extern RREQ ardp_pendingQ, ardp_runQ, ardp_doneQ;


/*
 * rralloc - allocate and initialize an rreq structure
 *
 *    ardp_rqalloc returns a pointer to an initialized structure of type
 *    RREQ.  If it is unable to allocate such a structure, it
 *    returns NOREQ (NULL).
 */
RREQ
ardp_rqalloc()
{
    RREQ	rq;

    p_th_mutex_lock(p_th_mutexARDP_RQALLOC);
    if(lfree) {
	rq = lfree;
	lfree = lfree->next;
    }
    else {
	rq = (RREQ) malloc(sizeof(RREQ_ST));
	if (!rq) return(NOREQ);
	rreq_max++;
    }
    rreq_count++;
    p_th_mutex_unlock(p_th_mutexARDP_RQALLOC);

    rq->status = ARDP_STATUS_NOSTART;
#ifdef ARDP_MY_WINDOW_SZ
    rq->flags = ARDP_FLAG_SEND_MY_WINDOW_SIZE; /*  used by clients. */
#else
    rq->flags = 0;
#endif
    rq->cid = 0;
    rq->priority = ardp_priority;
    rq->pf_priority = 0;
    rq->peer_ardp_version = 0;
    rq->inpkt = NOPKT;
    rq->rcvd_tot = 0;
    rq->rcvd = NOPKT;
    rq->rcvd_thru = 0;
#ifdef ARDP_MY_WINDOW_SZ        /* This sets the window size we'll ask for */
    rq->window_sz = ARDP_MY_WINDOW_SZ;
#else
    rq->window_sz = 0;
#endif
    rq->pwindow_sz = ARDP_DEFAULT_WINDOW_SZ; /* sets the window size we assume
                                                our peer will accept in the
                                                absence of an explicit
                                                request.  */
    rq->comp_thru = NOPKT;
    rq->outpkt = NOPKT;
    rq->trns_tot = 0;
    rq->trns = NOPKT;
    rq->prcvd_thru = 0;
    bzero(&(rq->peer),sizeof(rq->peer));
    rq->rcvd_time.tv_sec = 0;
    rq->rcvd_time.tv_usec = 0;
    rq->svc_start_time.tv_sec = 0;
    rq->svc_start_time.tv_usec = 0;
    rq->svc_comp_time.tv_sec = 0;
    rq->svc_comp_time.tv_usec = 0;
    rq->timeout.tv_sec = ardp_default_timeout;
    rq->timeout.tv_usec = 0;
    rq->timeout_adj.tv_sec = ardp_default_timeout;
    rq->timeout_adj.tv_usec = 0;
    rq->wait_till.tv_sec = 0;
    rq->wait_till.tv_usec = 0;
    rq->retries = ardp_default_retry;
    rq->retries_rem = ardp_default_retry;
    rq->svc_rwait = 0;
    rq->svc_rwait_seq = 0;
    rq->inf_queue_pos = 0;
    rq->inf_sys_time = 0;
#ifdef PROSPERO
    rq->auth_info = NULL;
#endif /* PROSPERO */
    rq->client_name = NULL;
    rq->peer_sw_id = NULL;
    rq->cfunction = NULL;
    rq->cfunction_args = NULL;
    rq->previous = NOREQ;
    rq->next = NOREQ;
    return(rq);
}

/*
 * ardp_rqfree - free a RREQ structure
 *
 *    ardp_rqfree takes a pointer to a RREQ structure and adds it to
 *    the free list for later reuse.
 */
void ardp_rqfree(rq)
    RREQ	rq;
{
#ifndef NDEBUG
    /* This is different from the way we do consistency checking in the PFS
       library, because here the status field is just sitting to be used. */
    assert(rq->status != ARDP_STATUS_FREE);
    rq->status = ARDP_STATUS_FREE;
    CHECK_OFFLIST(rq);          /* MITRA macro; probably does the same as those
                                   listed below.  Might never be defined. */
    assert(!rreq_member_of_list(rq, ardp_pendingQ));
    assert(!rreq_member_of_list(rq, ardp_runQ));
    assert(!rreq_member_of_list(rq, ardp_doneQ));
#endif
    /* Don't free inpkt or comp_thru, already on rcvd     */
    if(rq->rcvd) ardp_ptlfree(rq->rcvd);
    /* But outpkt has not been added to trns */
    if(rq->outpkt) ardp_ptlfree(rq->outpkt);
    if(rq->trns) ardp_ptlfree(rq->trns);
#ifdef PROSPERO
    if (rq->auth_info) pafree(rq->auth_info);
#endif /* PROSPERO */
    if (rq->client_name) stfree(rq->client_name);
    if (rq->peer_sw_id) stfree(rq->peer_sw_id);

    p_th_mutex_lock(p_th_mutexARDP_RQALLOC);
    rq->next = lfree;
    rq->previous = NOREQ;
    lfree = rq;
    rreq_count--;
    p_th_mutex_unlock(p_th_mutexARDP_RQALLOC);
}



#ifndef NDEBUG
/* Free just the fields that are needed only while processing the request
   but not the fields that will be used for a request that is on the ardp_doneQ
   so that the results can be retransmitted.  This is only needed when
   debugging the server.  It is used in ardp_respond() and is #ifdef'd NDEBUG.
*/
void
ardp_rq_partialfree(RREQ rq)
{
#ifdef PROSPERO
    if (rq->auth_info) { pafree(rq->auth_info); rq->auth_info = NULL; }
#endif /* PROSPERO */
    if (rq->client_name) { stfree(rq->client_name);  rq->client_name = NULL; }
    if (rq->peer_sw_id) { stfree(rq->peer_sw_id); rq->peer_sw_id = NULL; }
}
#endif


/*
 * ardp_rqlfree - free many RREQ structures
 *
 *    ardp_rqlfree takes a pointer to a RREQ structure frees it and any linked
 *    RREQ structures.  It is used to free an entrie list of RREQ
 *    structures.
 */

void 
ardp_rqlfree(rq)
    RREQ	rq;
{
    RREQ	nxt;

    while(rq != NOREQ) {
	nxt = rq->next;
	ardp_rqfree(rq);
	rq = nxt;
    }
}


/*
 * ardp_set_retry - change default values for timeout
 *
 *    ardp_set_retry takes a value for timout in seconds and a count
 *    of the number of retries to allow.  It sets static variables in this
 *    file used by pralloc to set the default values in request structures
 *    it allocates.
 */
int
ardp_set_retry(int to, int rt)
{
    /* XXX This is a critical section, but it is safe as long as we are not on
       a multiprocessor and don't block.  So no mutexes. */
    ardp_default_timeout = to;
    ardp_default_retry = rt;
    /*** XXX End Critical section */
    return(ARDP_SUCCESS);
}


#ifndef NDEBUG
/* Return 1 if the RREQ rr is a member of the list LIST. */
static int 
rreq_member_of_list(RREQ rr, RREQ list)
{
    for ( ; list; list = list->next) {
        if (rr == list) return 1;
    }
    return 0;                   /* false */
}
#endif
