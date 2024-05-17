/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * Written  by bcn 1/93     For use in adrp_send
 */

#include <usc-copyr.h>

#include <stdio.h>

#include <ardp.h>
#include <pmachine.h>           /* for bcopy() */

extern int	ardp_priority;    /* Override for 0 priority    */
extern int	pfs_debug;

/*
 * ardp_headers - Add headers to packets to be sent to server
 *
 *   ardp_headers takes pointer to a request structure and adds headers to 
 *   all the packets in the trns list.  The fields for the headers are
 *   taken from other fields in the request structure.  If the priority is
 *   zero, it is taken from the global vairable ardp_priority.
 *
 *   If ardp_headers is called more than once, subsequent calls will
 *   update the headers to conform to changes in the request fields.
 */
int
ardp_headers(RREQ req)
{
    PTEXT	ptmp;		/* Temporary packet pointer	      */

    for(ptmp = req->trns; ptmp; ptmp = ptmp->next) {
        int		old_hlength;    /* Old header length */
        int             new_hlength;    /* New header length, whatever it may
                                           be.  */
        unsigned short ftmp;	/* Temporary for values of fields     */
        /* Boolean: do we stamp this packet with the window size? */
        /* We will add an explicity window-size stamp to the first sent, if
           an explicit window size was set.   This packet is the one stamped
           with sequence number 1.  Thus we know that the window size message
           won't be lost. */
        int  stamp_window_size = (req->flags & ARDP_FLAG_SEND_MY_WINDOW_SIZE) 
            && (ptmp->seq == 1);
        int stamp_priority = (req->priority || ardp_priority) 
            && (ptmp->seq == 1);

	old_hlength = ptmp->text - ptmp->start;

	/* XXX Should do further tests to make sure all packets present */
	if(ptmp->seq == 0) {
	    if (pfs_debug >= 1)
		fprintf(stderr, "ardp: sequence number not set in packet\n");
	    fflush(stderr);
	    return(ARDP_BAD_REQ);
	}

        /* If priority stamp or explicit client window size, need octets 11 and
           12 to be present. */
        if (stamp_priority || stamp_window_size) {
            new_hlength = 13;   /* room for octets through 12 */
            if(req->priority || ardp_priority) 
                new_hlength += 2; /* 2 octets for priority argument.  */
            if (stamp_window_size)
                new_hlength += 2; /* 2 octets for window size */
        } else {
            new_hlength = 9;    /* room for octets through 8 (received-through)
                                   */ 
        }
        /* New header length now set. */

        /* Allocate space for the new header */
        ptmp->start = ptmp->text - new_hlength;
        ptmp->length += new_hlength - old_hlength;
        /* Set the header length and version # (zeroth octet) */
        /* Version # is zero in this version of the ARDP library; last 6 bytes
           of octet are header length. */
        *(ptmp->start) = (char) new_hlength;
        /* Set octets 1 through 8 of header */
	/* Connection ID (octets 1 & 2) */
	bcopy2(&(req->cid),ptmp->start+1);
	/* Sequence number (octets 3 & 4) */
	ftmp = htons(ptmp->seq);
	bcopy2(&ftmp,ptmp->start+3);	
	/* Total packet count (octets 5 & 6) */
	ftmp = htons(req->trns_tot);
	bcopy2(&ftmp,ptmp->start+5);	
	/* Received through (octets 7 & 8) */
	ftmp = htons(req->rcvd_thru);
	bcopy2(&ftmp,ptmp->start+7);	
        /* Now set any options. */
        if (new_hlength > 9) {
            char     *       optiondata; /* where options go. */
            /* zero out octets 9 and 10 (expected time 'till response).  It is
               not defined for the client to specify this to the server,so
               it is always zero. */
            bzero2(ptmp->start + 9);
            /* set octet 11 (flags) initially clear */
            ptmp->start[11] = 0;
            /* Here Octet 12 (option) is zero (no special options). */
            ptmp->start[12] = 0;
            optiondata = ptmp->start + 13; /* additional octets start here */
            /* Priority */
            if(stamp_priority) {
                *(ptmp->start+11) |= 0x02; /* priority flag */
                if(req->priority) ftmp = htons(req->priority);
                else ftmp = htons(ardp_priority);
                bcopy2(&ftmp, optiondata);
                optiondata += 2;
            }
            if(stamp_window_size) {
                *(ptmp->start+11) |= 0x08; /* Set window size flag */
                ftmp = htons(req->window_sz);
                bcopy2(&ftmp, optiondata);
                optiondata += 2;
            }
            assert(optiondata == ptmp->start + new_hlength);
        }
    }
    return(ARDP_SUCCESS);
}
