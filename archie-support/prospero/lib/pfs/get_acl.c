/*
 * Copyright (c) 1991 by the University of Washington
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
#include <pparse.h>
#include <pprot.h>
#include <perrno.h>

extern int	pfs_debug;

/* lname is specified if we want the ACL for a link within a directory.
   It is left out when requesting the ACL for the directory itself. */
/* XXX To be done: add an ID field. */
#define LIST_ACL_SEND_OLDSTYLE_OBJECT_AND_CONTAINER /* until Alpha.5.2 servers
                                                       are gone. */
#define SEND_DIRECTORY_NOT_OBJECT /* Until Alpha.5.2 and before servers all
                                     gone.  */

ACL 
get_acl(VLINK dlink,
	const char *lname, 
        /* Note that 'directory' and 'object' flags will soon be the same. */
	int flags /* 0 = directory, 1 = link, 2 = object, 3 = included/named, 
                     4 = container 5 = named*/)
{
    RREQ	req;		/* Text of request to dir server             */
    ACL		retval = NULL;  /* ACLs being returned                       */
    ACL		ap;		/* Temporary acl pointer                     */
    char	*options = "";  /* List of options                           */

    int	fwdcnt = MAX_FWD_DEPTH;
    char	fwdhst[MAX_DIR_LINESIZE];
    char	fwdfnm[MAX_DIR_LINESIZE];
    INPUT_ST    in_st;
    INPUT       in = &in_st;
    char        *next_line;
    int		tmp;

    if(flags == 0) {
        options = "DIRECTORY";
        /* no link name should be specified with the directory option.  */
        if (lname && *lname) return NULL; 
    }
    /* If for a link, or an included ACL */
    else if ((flags == 1) || (flags == 3) || (flags == 5)) {
        /* link name must be specified with this option */ 
        if (!lname || !*lname)
            return NULL;   
        if (flags == 1) options = "LINK";
	else if (flags == 3) options = "INCLUDE"; 
	else if (flags == 5) options = "NAMED"; 
    }
    else if (flags == 2) {
        options = "OBJECT"; 
        if (lname && *lname) return NULL;
    }
    else if (flags == 4) {
        options = "CONTAINER";
        if (lname && *lname) return NULL;
    }
    if(lname == NULL) lname = "";

    if(strcmp(dlink->target,"NULL") == 0) return(NULL);
    if(strcmp(dlink->target,"EXTERNAL") == 0) return(NULL);

startover:

    req = p__start_req(dlink->host);

    if ((flags == 0) || (flags == 1)) {
#ifdef SEND_DIRECTORY_NOT_OBJECT
	p__add_req(req, "DIRECTORY %'s %'s\n", dlink->hsonametype,
                   dlink->hsoname); 
#else
	p__add_req(req, "OBJECT %'s %'s\n", dlink->hsonametype, 
                   dlink->hsoname);
#endif
    }
#ifdef LIST_ACL_SEND_OLDSTYLE_OBJECT_AND_CONTAINER
    if ((flags == 0) || (flags == 1) || (flags == 3) || (flags == 5)) {
	p__add_req(req, "LIST-ACL %s", options);
	if(*lname) p__add_req(req, " %'s", lname);
	p__add_req(req, "\n");
    }

    if ((flags == 2) || (flags == 4)) {
	p__add_req(req, "LIST-ACL %s %s\n", options, dlink->hsoname);
    }
#else
    if (flags == 2 || flags == 4) {
        p__add_req(req, "OBJECT %'s %'s\n", dlink->hsonametype, 
                   dlink->hsoname);
	p__add_req(req, "LIST-ACL %'s\n", options);
    }
    if ((flags == 0) || (flags == 1) || (flags == 3) || (flags == 5)) {
	p__add_req(req, "LIST-ACL %s", options);
	if(*lname) p__add_req(req, " %'s", lname);
	p__add_req(req, "\n");
    }
#endif
    
    perrno = 0;

    tmp = ardp_send(req,dlink->host,0,ARDP_WAIT_TILL_TO);

    if(pfs_debug && tmp) {
        fprintf(stderr,"get_acl(): Dirsend failed: %d\n",tmp);
	perrno = tmp;
    }

    /* If we don't get a response, then return error */
    if(req->rcvd == NULL) return(NULL);

    rreqtoin(req, in);
    /* Here we must parse reponse   */
    /* While looking at each packet */
    while (in_nextline(in)) {
        char *line, *next_word;
        if(tmp = in_line(in, &line, &next_word)) {
            aclfree(retval);
            perrno = tmp;
            return NULL;
        }
        switch (*line) {

            /* Temporary variables to info */
	    /* There is already a variable "tmp" in this function*/
            int		tmp1;

        case 'A': /* ACL */ 
            if(!strnequal(line,"ACL",3)) goto scanerr;
            if (retval) {/* if already saw ACL lines */
                perrno = DIRSRV_BAD_FORMAT;
                aclfree(retval);
                return NULL;
            }
            if(in_ge1_acl(in, line, next_word, &retval)) {
                aclfree(retval);
                return NULL;
            }
            break;

        case 'F':/* FORWARDED */
            if(strncmp(line,"FORWARDED",9) == 0) {
                if(fwdcnt-- <= 0) {
		    ardp_rqfree(req);
                    perrno = PFS_MAX_FWD_DEPTH;
                    return(NULL);
                }
                /* parse and start over */

                tmp1 = qsscanf(line,"FORWARDED %'&s %'&s %'&s %'&s %ld %ld", 
                             &dlink->hosttype, &dlink->host, 
                             &dlink->hsonametype, &dlink->hsoname, 
                             &dlink->version, &dlink->f_magic_no);
                if(tmp1 < 2) {
                    perrno = DIRSRV_BAD_FORMAT;
                    break;
                }

		ardp_rqfree(req);
                goto startover;
            }
            /* If FAILURE or other, then scan error */
            goto scanerr;

        scanerr:
        default:
            if(tmp1 = scan_error(line, req)) {
		ardp_rqfree(req);
                return(NULL);
            }
            break;
        }                       /* switch() */
    }                       /* while (in_nextline(in)) */
    ardp_rqfree(req);
    return(retval);
}

	    
