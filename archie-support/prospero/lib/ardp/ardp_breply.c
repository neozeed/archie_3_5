/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 *
 * Written  by bcn 2/93     to send reply to request
 * Hacked   by swa 11/94    to send length-coded reply to request.
 */

#include <usc-license.h>

#include <stdio.h>
#include <ardp.h>

/*
 * ardp_breply - send a reply to a request that might contain binary data
 * 
 *   ardp_breply reply takes request to which a reply is to be sent
 *   a flags field, and a message to be sent to the client in response
 *   to the request, and a length count for the message. ardp_breply then adds
 *   the response to the output   queue for the request.
 *
 *   A length count of 0 indicates that the buffer is null-terminated.
 *   (implementation note: by passing the 0 down to ardp_add2req, we
 *    allow there to be only one pass through the buffer instead of 2.)
 *
 *   If space remains in the packet, the response is buffered pending
 *   subsequent replies, unless the ARDP_REPL_COMPLETE has been specified, 
 *   in which case all data is sent.  If the size of the buffered response 
 *   approaches the maximum packet size, the buffered data is sent, and
 *   and a new packet is started to hold any remaining data from the current 
 *   message.
 *
 *   The lower level function (ardp_respond) assigns packet numbers and 
 *   tags outgoing packets if necessary or a multi packet response.
 */

int
ardp_breply(RREQ req,           /* Request to which this is a response */
            int flags,          /* Whether this is the final message   */
            char *message,      /* The data to be sent. */
            int len)            /* message length; 0 means null-terminated */
{
    int		tmp;

    tmp = ardp_add2req(req,flags,message,len);
    if (tmp) return tmp;
    if(flags&ARDP_R_COMPLETE) return(ardp_respond(req,ARDP_R_COMPLETE));
    
    /* Check to see if any packets are done */
#if 1
    if(req->outpkt && req->outpkt->next) { 
#else
    /* This always yields the same results as the previous line but appears
       clunkier to me. */
    if(req->outpkt && (req->outpkt->previous != req->outpkt)) {
#endif
        PTEXT	tpkt;	   /* Temp to hold active packet          */
	/* Hold out final packet */
	tpkt = req->outpkt->previous;
	EXTRACT_ITEM(tpkt,req->outpkt);
	tmp = ardp_respond(req,ARDP_R_INCOMPLETE);
	APPEND_ITEM(tpkt,req->outpkt);
        if (tmp) return tmp;
    }
    return(ARDP_SUCCESS);
}
