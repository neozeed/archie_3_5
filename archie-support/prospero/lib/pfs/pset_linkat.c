/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */
/* Author: Steven Augart, swa@isi.edu */
#include <usc-copyr.h>

#include <stdio.h>

#include <ardp.h>
#include <pfs.h>
#include <pparse.h>
#include <perrno.h>

static int eoi_parse_resp(RREQ resp);
extern int pfs_debug;

/* Returns PSUCCESS upon successful execution, PFAILURE for whatever failure
   reason. */
int
pset_linkat(VLINK dlink, char lname[], int flags, PATTRIB attributes)
{
    RREQ req;
    OUTPUT_ST out_st;
    OUTPUT out = &out_st;
    int tmp;

    req = p__start_req(dlink->host);
    p__add_req(req, "DIRECTORY ASCII %'s 0\nEDIT-LINK-INFO ", dlink->hsoname);
    switch(flags) {
    case EOI_ADD:
        p__add_req(req, "ADD");
        break;
    case EOI_DELETE:
        p__add_req(req, "DELETE");
        break;
    case EOI_DELETE_ALL:
        p__add_req(req, "DELETE-ALL");
        break;
    case EOI_REPLACE:
        p__add_req(req, "REPLACE");
        break;
    default:
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "edit_object_info(): Illegal flag value: %d", flags);
                 break;
        RETURNPFAILURE;
    }
    p__add_req(req, " %'s\n", lname);
    requesttoout(req,out);
    out_atrs(out, attributes, 0);
    tmp = ardp_send(req, dlink->host, 0, ARDP_WAIT_TILL_TO);
    if(tmp) {
        fprintf(stderr,"Dirsend failed: %d\n",perrno);
	perrno = tmp;
    }
    
    if(req->rcvd == NULL) return(perrno);

    return eoi_parse_resp(req);
}

static int
eoi_parse_resp(RREQ resp)
{
    INPUT_ST in_st;
    INPUT in = &in_st;

    rreqtoin(resp, in);
        
    while(!in_eof(in)) {
        char		*line;
        char            *next_word;
        int tmp;                /* error return code from functions */

        if (tmp = in_line(in, &line, &next_word)) {
            ardp_rqfree(resp);
            return(tmp);
        }
        /* Scanerr() will handle SUCCESS or FAILURE or any other error return
           codes.  It parses it all for us! */
        if (tmp = scan_error(line, resp)) {
	    ardp_rqfree(resp);
            return tmp;
        }
    }
    return PSUCCESS;
}
