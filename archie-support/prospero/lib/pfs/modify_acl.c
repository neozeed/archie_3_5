/*
 * Copyright (c) 1991       by the University of Washington
 * Copyright (c) 1992, 1993 by the University of Southern California
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
extern char	*acltypes[];

/*
 *
 *   Flags:      
 */
int
modify_acl(dlink,lname,a,flags)
    VLINK	dlink;		/* Directory link                 */
    const char *lname;		/* Link name.  Must be empty for DIRECTORY
                                   option. OBJECT option not implemented, so
                                   don't worry about it. */
    ACL		a;		/* ACL entry to add/delete/modify */
    int		flags;		/* Flags                          */
{
    RREQ	req;		/* Text of request to dir server             */
    char	options[100];   /* List of options                           */
    char	*optptr;        /* Options minus leading + */
    int		tmp;           
    OUTPUT_ST   out_st;
    OUTPUT      out = &out_st;
    INPUT_ST    in_st;
    INPUT       in = &in_st;

    int	fwdcnt = MAX_FWD_DEPTH;

    *options = '\0';

    if(flags&EACL_NOSYSTEM) strcat(options,"+NOSYSTEM");
    if(flags&EACL_NOSELF) strcat(options,"+NOSELF");

    if((flags&EACL_OP) == EACL_DEFAULT) strcat(options,"+DEFAULT");
    else if((flags&EACL_OP) == EACL_SET) strcat(options,"+SET");
    else if((flags&EACL_OP) == EACL_INSERT) strcat(options,"+INSERT");
    else if((flags&EACL_OP) == EACL_DELETE) strcat(options,"+DELETE");
    else if((flags&EACL_OP) == EACL_ADD) strcat(options,"+ADD");
    else if((flags&EACL_OP) == EACL_CREATE) strcat(options,"+CREATE"); 
    else if((flags&EACL_OP) == EACL_DESTROY) strcat(options,"+DESTROY");
    else if((flags&EACL_OP) == EACL_SUBTRACT) strcat(options,"+SUBTRACT");
    else RETURNPFAILURE;       /* bad operation */

    if((flags&EACL_OTYPE) == EACL_LINK) strcat(options,"+LINK");
    else if((flags&EACL_OTYPE) == EACL_DIRECTORY) strcat(options,"+DIRECTORY");
    else if((flags&EACL_OTYPE) == EACL_OBJECT) strcat(options,"+OBJECT");
    else if((flags&EACL_OTYPE) == EACL_INCLUDE) strcat(options,"+INCLUDE");
    else if((flags&EACL_OTYPE) == EACL_NAMED) strcat(options,"+NAMED");
    else RETURNPFAILURE;       /* bad target */

    /* CREATE and DESTROY are only valid with NAMED. */
    if (((flags & EACL_OP) == EACL_CREATE || (flags & EACL_OP) == EACL_DESTROY)
        && (flags & EACL_OTYPE) != EACL_NAMED)
        RETURNPFAILURE;
    optptr = options + 1;

    if(lname == NULL) lname = "";

    if(strcmp(dlink->target,"NULL") == 0) return(PFS_EXT_USED_AS_DIR);
    if(strcmp(dlink->target,"EXTERNAL") == 0) return(PFS_EXT_USED_AS_DIR);

startover:

    req = p__start_req(dlink->host);
    /* XXX need to add capability for ID specification.. */
    requesttoout(req,out);

    if(((flags&EACL_OTYPE) == EACL_DIRECTORY) ||
	((flags&EACL_OTYPE) == EACL_LINK)) {
	p__add_req(req,"DIRECTORY ASCII %'s\n", dlink->hsoname);
    }

    if((flags&EACL_OTYPE) == EACL_OBJECT)
	p__add_req(req,"EDIT-ACL %s %'s\n",optptr, dlink->hsoname);
    else p__add_req(req,"EDIT-ACL %s %'s\n",optptr, lname);

    out_acl(out, a);

    perrno = 0;

    tmp = ardp_send(req,dlink->host,0,ARDP_WAIT_TILL_TO);

    if(tmp) {
        if (pfs_debug) fprintf(stderr,"ardp_send failed: %d\n",tmp);
	perrno = tmp;
    }

    /* If we don't get a response, then return error */
    if(req->rcvd == NOPKT) {
        ardp_rqfree(req);
        return(perrno);
    }
    /* Here we must parse reponse - While looking at each packet */

    rreqtoin(req, in);
    while(!in_eof(in)) {
        char		*line;
        char            *next_word;

        if (tmp = in_line(in, &line, &next_word)) {
            ardp_rqfree(req);
            return(tmp);
        }
        switch (*line) {

        case 'F': /* FAILURE or FORWARDED */
            /* FORWARDED */
            if(qsscanf(line, "FORWARDED%~%r", &next_word) == 1) {
                if(fwdcnt-- <= 0) {
                    ardp_rqfree(req);
                    perrno = PFS_MAX_FWD_DEPTH;
                    return(perrno);
                }
                if (tmp = in_forwarded_data(in, line, next_word, dlink)) {
                    perrno = DIRSRV_BAD_FORMAT;
                    ardp_rqfree(req);
                    break;
                }
                /* Success; scan again. */
                ardp_rqfree(req);
                goto startover;
            }
            /* If FAILURE or anything else scan error */
            goto scanerr;

        case 'S': /* SUCCESS */
            if(strncmp(line,"SUCCESS",7) == 0) {
                ardp_rqfree(req);
                return(PSUCCESS);
            }
            goto scanerr;

        scanerr:
        default:
            if(*line && (tmp = scan_error(line, req))) {
                ardp_rqfree(req);
                return(tmp);
            }
            break;
        }
    }
    perrno = DIRSRV_BAD_FORMAT;
    ardp_rqfree(req);
    return(perrno);
}
