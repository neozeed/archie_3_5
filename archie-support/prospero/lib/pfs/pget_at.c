/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1992, 1993       by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>
#include <stdio.h>
#include <string.h>

#include <ardp.h>
#include <pfs.h>
#include <pprot.h>
#include <perrno.h>
#include <pparse.h>

extern int	pfs_debug;

/* Obtain object attributes from a remote host.. */
/* Use pget_linkat() to obtain link attributes. */
PATTRIB pget_at(vl,atname)
    VLINK	vl;
    const char	*atname;
{
    RREQ	req;		/* Text of request to dir server             */
    PATTRIB	retval = NULL;	/* Start of list of attributes               */
    INPUT_ST    in_st;
    INPUT       in = &in_st;
    int		tmp;            /* return value from subfunctions.  Value is
                                   only  used for a couple of lines after it's
                                   set each time. */

    int	fwdcnt = MAX_FWD_DEPTH;

    perrno = PSUCCESS;          /* must set perrno, since that's how we report
                                   success or failure. */

    /* It's not an error to ask; However, we already know that there won't be
       any object attributes, so we just report that fact and don't waste the
       time for the request to time out. */
    if (vl->target && !strequal(vl->target, "OBJECT") 
        && !strequal(vl->target, "FILE") 
        && !strequal(vl->target, "DIRECTORY")
        && !strequal(vl->target, "DIRECTORY+FILE"))
        return NULL; 
startover:
    CHECK_MEM();
    req = p__start_req(vl->host);
    p__add_req(req, "GET-OBJECT-INFO %'s ASCII %'s 0\n",
	      atname, vl->hsoname);
    if (vl->f_magic_no)
        p__add_req(req, "SELECT OBJECT FIELD ID REMOTE %ld\n", 
               vl->f_magic_no);

    tmp = ardp_send(req,vl->host,0,ARDP_WAIT_TILL_TO);

    /* If the request fails or if we don't get a response, then return error */
    if(tmp ||  req->rcvd == NULL) return(NULL);

    rreqtoin(req, in);

    while (!in_eof(in)) {
        char		*line;
        char            *next_word;

        if (in_line(in, &line, &next_word)) {
            ardp_rqfree(req);
            return(NULL);
        }
        switch (*line) {
        case 'A': /* ATTRIBUTE */
            /* If anything but ATTRIBUTE scan error */
            if(!strnequal(line,"ATTRIBUTE", 9)) 
                goto scanerr;

            if (in_ge1_atrs(in, line, next_word, &retval)) {
                ardp_rqfree(req);
                return NULL; /* perrno will be set. */
            }
            break;
            
        case 'N': /* NONE-FOUND */
            /* NONE-FOUND, we just have no attributes to insert.  No error;
               just don't do anything. */
            if(strncmp(line,"NONE-FOUND",10) == 0) 
                break;
            goto scanerr;

        case 'F':/* FORWARDED, FAILURE */
            if(strncmp(line,"FORWARDED",9) == 0) {
                if(fwdcnt-- <= 0) {
		    ardp_rqfree(req);
                    perrno = PFS_MAX_FWD_DEPTH;
                    return(NULL);
                }
                /* parse and start over */
                tmp = qsscanf(line,"FORWARDED %&'s %&'s %&'s %&'s %ld %ld", 
                             &vl->hosttype, &vl->host, &vl->hsonametype,
                             &vl->hsoname, 
                             &(vl->version), &(vl->f_magic_no));

                if(tmp < 2) {
                    perrno = DIRSRV_BAD_FORMAT;
                    break;
                }

		ardp_rqfree(req);
                goto startover;
            }
            /* If FAILURE or anything but FORWARDED, scan error */
            goto scanerr;

        scanerr:
        default:
            if(*line && (tmp = scan_error(line, req))) {
		ardp_rqfree(req);
                perrno = tmp;
                return(NULL);
            }
            break;
        }                       /* end of moby switch */
    }                           /* while in_nextline(in) */
    ardp_rqfree(req);
    return(retval);
}


