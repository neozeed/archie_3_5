/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 *
 * Written  by bcn 1991     as get_next_request in rdgram.c (Prospero)
 * Modified by bcn 1/93     modularized and incorporated into new ardp library
 */

#include <usc-license.h>

#include <stdio.h>
#include <sys/time.h>
#ifdef AIX
#include <sys/select.h>
#endif
#include <ardp.h>
#include <pmachine.h>

extern RREQ ardp_pendingQ;
EXTERN_MUTEXED_DECL(RREQ,ardp_runQ);
extern int	pQNlen;
extern int	pQlen;
extern int	ardp_srvport;
extern int	ardp_prvport;

#define max(x,y) (x > y ? x : y)

/*
 * ardp_get_nxt - return next request for server
 *
 *   ardp_get_nxt returns the next request to be processed by the server.
 *   If no requests are pending, ardp_get_nxt waits until a request
 *   arrives, then returns it.
 */
/* Called outside multithraded stuff; doesn't need to be mutexed. */
RREQ 
ardp_get_nxt(void)
{
    RREQ	nextreq;
    fd_set	readfds;
    int		tmp;

 tryagain:
    if (nextreq = ardp_get_nxt_nonblocking())
        return nextreq;
    /* if queue is empty, then wait till somethings comes */
    /* in, then go back to start                          */
    FD_ZERO(&readfds);
    if(ardp_srvport != -1) FD_SET(ardp_srvport, &readfds);
    if(ardp_prvport != -1) FD_SET(ardp_prvport, &readfds); 
    tmp = select(max(ardp_srvport,ardp_prvport) + 1, &readfds, 
		 (fd_set *)0, (fd_set *)0, NULL);
    goto tryagain;
}


/*
 * Nonblocking version of the above.
 * Returns  NULL if no pending items.
 */
RREQ
ardp_get_nxt_nonblocking(void)
{
    ardp_accept();

    /* return next message in queue */
    if (ardp_pendingQ) {
        /* Atomic test; save ourselves the trouble of going through the kernel
           if nothing in queue */
        EXTERN_MUTEXED_LOCK(ardp_pendingQ);
        if(ardp_pendingQ) {
            RREQ nextreq = ardp_pendingQ;
            
            EXTRACT_ITEM(nextreq, ardp_pendingQ);
            pQlen--;if(nextreq->priority > 0) pQNlen--;
            EXTERN_MUTEXED_UNLOCK(ardp_pendingQ);
            nextreq->svc_start_time.tv_sec = time(NULL);	
            EXTERN_MUTEXED_LOCK(ardp_runQ);
            APPEND_ITEM(nextreq,ardp_runQ);
            EXTERN_MUTEXED_UNLOCK(ardp_runQ);
            return(nextreq);
        }
        EXTERN_MUTEXED_UNLOCK(ardp_pendingQ);
    }
    return NOREQ;
}
