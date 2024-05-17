/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-license.h>
 */
/* Author: Steven Augart, swa@isi.edu */
#include <usc-license.h>

#include <stdio.h>
#include <ardp.h>
#include <pfs.h>
#include <pparse.h>
#include <pprot.h>
#include <perrno.h>

static int eoi_parse_resp(RREQ resp);
extern int pfs_debug;

/* Sets OBJECT attributes appropriately. */
/* Returns PSUCCESS upon successful execution, failure code otherwise. */
int
pset_at(VLINK vl, int flags, PATTRIB attributes)
{
    RREQ req;
    OUTPUT_ST out_st;
    OUTPUT out = &out_st;
    INPUT_ST in_st;
    INPUT in = &in_st;
    int		fwdcnt = MAX_FWD_DEPTH;

    int tmp;                /* error return code from functions.  Value only
                               used a few lines after set, each time. */

    if (vl->target && !strequal(vl->target, "OBJECT") 
        && !strequal(vl->target, "FILE") 
        && !strequal(vl->target, "DIRECTORY")
        && !strequal(vl->target, "DIRECTORY+FILE")) {
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "pset_at(): Asked to munge the attributes of the object \
pointed to by a link with a target of %s.  Cannot do this.\n", vl->target);
        return perrno = PSET_AT_TARGET_NOT_AN_OBJECT;
    }
 startover:
    req = p__start_req(vl->host);
    p__add_req(req, "EDIT-OBJECT-INFO ");
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
        return perrno = PFAILURE;
    }
    p__add_req(req, " %'s %'s\n", vl->hsonametype, vl->hsoname);
    requesttoout(req,out);
    out_atrs(out, attributes, 0);
    tmp = ardp_send(req, vl->host, 0, ARDP_WAIT_TILL_TO);
    if(tmp) {
        if (pfs_debug) fprintf(stderr,"ardp_send failed: %d\n",perrno);
	return perrno = tmp;
    }
    if(req->rcvd == NULL) return(perrno);
    
    rreqtoin(req, in);
        
    while(!in_eof(in)) {
        char		*line;
        char            *next_word;

        if (tmp = in_line(in, &line, &next_word)) {
            ardp_rqfree(req);
            return(tmp);
        }
        if(strncmp(line,"FORWARDED",9) == 0) {
            if(fwdcnt-- <= 0) {
                ardp_rqfree(req);
                perrno = PFS_MAX_FWD_DEPTH;
                return(perrno);
            }
            /* parse and start over */
            tmp = qsscanf(line,"FORWARDED %&'s %&'s %&s %&'s", 
                         &vl->hosttype, &vl->host, 
                         &vl->hsonametype,&vl->hsoname);
            if(tmp < 2) {
                ardp_rqfree(req);
                perrno = DIRSRV_BAD_FORMAT;
                break;
            }
            ardp_rqfree(req);
            goto startover;
        }
        if(strncmp(line,"SUCCESS",7) == 0) {
            ardp_rqfree(req);
            return(PSUCCESS);
        }
        /* If FAILURE or anything else scan error */
        if (tmp = scan_error(line, req)) {
            ardp_rqfree(req);
            return tmp;
        }
    }
    ardp_rqfree(req);
    return PSUCCESS;
}

