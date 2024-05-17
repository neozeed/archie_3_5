/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * Written  by bcn 2/93     to add text to a request
 */

#include <usc-copyr.h>

#include <string.h>
#include <stdio.h>

#include <ardp.h>
#include <sys/types.h>          /* so pmachine.h won't redefine it. */
#include <pmachine.h>           /* for bcopy() */

#define max(x,y) (x > y ? x : y)
#define min(x,y) (x < y ? x : y)

/*
 * ardp_add2req - add text to request structure 
 *
 *   ardp_add2req takes a request structure, flags, a buffer containing text
 *   to be added to the output queue of the request, and the length of the
 *   buffer.
 *
 *   If the specified length of the buffer is 0, ardp_add2req calculates
 *   it using strlen (if the lenght is zero, the assumption is that the
 *   buffer is null terminated).
 *   If the buffer is a NULL pointer and the specified length is 0, then 
 *   it treated as a true zero-length buffer.
 *
 *   If the flag ARDP_A2R_NOSPLITB is set, the request will not be split
 *   across packets.  If ARDP_A2R_NOSPLITL, the lines may not be split
 *   across packets, and text added to a request without a trailing new
 *   line will be deferred until a newline is added or ardp_add2req is called
 *   without ARDP_A2R_NOSPLITL set.  If ARDP_A2R_COMPLETE is specified, it 
 *   indicates this is the final text to be added to the request structure, 
 *   and no text will remain buffered.  If there is still deferred text,
 *   a newline will be added.  If the flag ARDP_A2R_TAGLENGTH is specified, 
 *   the data written to the request will be tagged with a four octet length
 *   field in network byte order.
 *
 *   NOTE: If you use this function to add text to a request, then you
 *         should use only this function.  If this function is called on
 *         a request previously constructed by hand, unexpected failures
 *         may result.
 *
 *   BUGS: ARDP_A2R_NOSPLITL is not currently supported unless
 *         ARDP_A2R_NOSPLITB is also set.
 *         ARDP_A2R_TAGLENGTH is not presently supported.
 *         An ardp_flush_partial_line() is needed to handle some manifestations
 *         of the ARDP_TOOLONG error.  Alternatively, we might specify an 
 *         additional flag to ardp_add2req() to specify the behavior on an 
 *         ARDP_TOOLONG error.
 */
int
ardp_add2req(RREQ	req,    /* Request to get the text */
	     int	flags,  /* Options to add2req      */
	     char	*buf,   /* New text for request    */
	     int	buflen) /* Length of the new text  */
{
    int		remaining;      /* Space remaining in pkt  */
    int		written;	/* # Octets written        */
    char	*newline;	/* last nl in string       */
    int		deflen;		/* Length of deferred text */
    int		extra;		/* Extra space needed      */
    long	taglen;		/* Length in net byte ord  */
    PTEXT	pkt = NOPKT;	/* Current packet          */
    PTEXT	ppkt;		/* Previous packet         */

    /* Note: New text gets written after pkt->ioptr.  If   */
    /* there is text waiting for a newline, then ->ioptr   */
    /* may be beyond ->start+->length, which will be the   */
    /* text up to the final newline seen so far            */

    /* XXX For now if NOSPLITL, must also specify NOSPLITB */
    if((flags & ARDP_A2R_NOSPLITL) && !(flags & ARDP_A2R_NOSPLITB))
       return(ARDP_FAILURE);

    if(buf && !buflen) buflen = strlen(buf);
    if(!buf) buf = "";

 keep_writing:

    /* Find ot allocate last packet in ->outpkt */
    if(req->outpkt) pkt = req->outpkt->previous; 
    if(!pkt) {
	if((pkt = ardp_ptalloc()) == NOPKT) return(ARDP_FAILURE);
	APPEND_ITEM(pkt,req->outpkt);
    }

    deflen = pkt->ioptr - (pkt->start + pkt->length);

    extra = deflen;
    if(flags & ARDP_A2R_NOSPLITL) extra += 1;
    if(flags & ARDP_A2R_TAGLENGTH) extra += 4;

    if((flags & ARDP_A2R_NOSPLITB) && (buflen + extra > ARDP_PTXT_LEN))
	return(ARDP_TOOLONG);

    remaining = ARDP_PTXT_LEN - (pkt->ioptr - pkt->start);

    if(((flags & ARDP_A2R_NOSPLITB) && (remaining < buflen + 1)) ||
       ((flags & ARDP_A2R_TAGLENGTH) && (remaining < 4)) ||
       (remaining == 0)) {
	ppkt = pkt;
	if((pkt = ardp_ptalloc()) == NOPKT) return(ARDP_FAILURE);
	if(ppkt->start + ppkt->length != ppkt->ioptr) {
	    /* Must move deferred text to new packet */
	    bcopy(ppkt->start+ppkt->length, pkt->start, deflen);
	    ppkt->ioptr = ppkt->start + ppkt->length;
	    *(ppkt->ioptr) = '\0';
	    pkt->ioptr += deflen;
	}
	APPEND_ITEM(pkt,req->outpkt);
	remaining = ARDP_PTXT_LEN;
    }

    if(flags & ARDP_A2R_TAGLENGTH) {
	taglen = htonl(buflen);
	bcopy4(&buflen,pkt->ioptr);
	pkt->ioptr += 4;
	remaining -= 4;
    }

    written = min(remaining,buflen);
    bcopy(buf,pkt->ioptr,written);
    pkt->ioptr += written;
    if(flags & ARDP_A2R_NOSPLITL) {
	*(pkt->ioptr) = '\0'; /* To catch runaway strrchar */
	/* Find last newline */
	newline = strrchr(pkt->start + pkt->length,'\n');
	if(newline) pkt->length = newline + 1 - pkt->start;
    }
    else pkt->length = pkt->ioptr - pkt->start;
    /* Always need to stick on trailing NULL for sake of ardp_respond(),
       which expects it. */
    *pkt->ioptr = '\0';

    /* In this case, ARDP_A@R_NOSPLITB was not specified, split it */
    if(written != buflen) {
	buf += written;
	buflen -= written;
	goto keep_writing;
    }

    if(flags & ARDP_A2R_COMPLETE) {
	deflen = pkt->ioptr - (pkt->start + pkt->length);
	/* If deferred, write a newline */
	if(deflen) {
	    *(pkt->ioptr++) = '\n';
	    pkt->length = pkt->ioptr - pkt->start;
            /* Always need to stick on trailing NULL for sake of
               ardp_respond(), which expects it. */
            *pkt->ioptr = '\0';
	}
    }
    return ARDP_SUCCESS;
}

