/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 *
 * Written  by bcn 2/93     to send reply to request
 * Gutted   by swa 11/93    functionality moved to ardp_breply().
 */

#include <usc-license.h>

#include <stdio.h>
#include <ardp.h>

/*
 * ardp_reply - send a null-terminated string as a reply to a request
 * 
 *   ardp_reply reply takes request to which a reply is to be sent
 *   a flags field, and a message to be sent to the client in response
 *   to the request. ardp_reply then calls ardp_reply() to do the work.
 *
 *   If space remains in the packet, the response is buffered pending
 *   subsequent replies, unless the ARDP_REPL_COMPLETE has been specified, 
 *   in which case all data is sent.  If the size of the buffered response 
 *   approaches the maximum packet size, the buffered data is sent, and
 *   and a new packet is started to hold any remaining data from the current 
 *   message.  (This is implemented by ardp_breply()).
 */

int
ardp_reply(RREQ req, 	   /* Request to which this is a response */
	   int flags,      /* Whether this is the final message   */
	   char *message)  /* The data to be sent                 */
{
    return ardp_breply(req, flags, message, 0);
}

