/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <usc-license.h>
 */

#include <usc-copyr.h>

#include <stdio.h>              /* declare fprintf() */
#include <string.h>		/* SOLARIS doesnt have strings.h */

#include <ardp.h>
#include <pfs.h>
#include <pprot.h>
#include <pparse.h>
#include <perrno.h>

extern int	pfs_debug;

/*
 */
int
add_vlink(
    const char	*direct,	/* Virtual name for the directory	  */
    const char	*lname,		/* Name of the new link                   */
    VLINK	l,		/* Link to be inserted                    */
    int		flags)		/* Options for link to add 		  */
{
    VLINK       dlink;          /* Link to remote directory.  This points to
                                   memory in the DIR structure; it is
                                   freed when we call vdir_freelinks(dir). */
    VDIR_ST	dir_st;
    VDIR	dir = &dir_st;

    RREQ	req;	 	/* Text of request to dir server.  Free before
                                   return. */

    int		fwdcnt = MAX_FWD_DEPTH;
    OUTPUT_ST   out_st;
    OUTPUT      out = &out_st;
    INPUT_ST    in_st;
    INPUT       in = &in_st;
    PATTRIB     ca;             /* current attribute */

    int		tmp;

    vdir_init(dir);

    /* If magic number has been modified, set it back to 0 */
    if(l->f_magic_no < 0) l->f_magic_no = 0;

    /* We must first find the directory into which the link  */
    /* will be inserted                                      */

    tmp = rd_vdir(direct,0,dir,RVD_ATTRIB|RVD_DFILE_ONLY);
    if (tmp || (dir->links == NULL)) {
        vdir_freelinks(dir);
        return(DIRSRV_NOT_DIRECTORY);
    }
    dlink = dir->links;

startover:

    req = p__start_req(dlink->host);
    requesttoout(req, out);
    p__add_req(req,
           "DIRECTORY ASCII %'s\nCREATE-LINK '' %c %'s %'s %s %'s %s %'s %d",
               dlink->hsoname, ((flags & AVL_INVISIBLE) ? 'I' : 
                        ((flags& AVL_UNION) ? 'U' : 'L')),
            l->target, lname, l->hosttype,l->host,
           l->hsonametype, l->hsoname,l->version);
    if (l->dest_exp) {
        char *cp = NULL;
        p__add_req(req, " DEST-EXP %s", 
                   cp = p_timetoasn_stcopyr(l->dest_exp, cp));
        stfree(cp);
    }
    p__add_req(req, "\n");
    if (l->f_magic_no) p__add_req(req, "ID REMOTE %d\n", l->f_magic_no);
    for (ca = l->lattrib; ca; ca = ca->next) {
        /* Convert attributes of  OBJECT precedence to CACHED precedence. */
        /* Don't leave the link modified that we passed to this function. */
        if (ca->precedence == ATR_PREC_OBJECT) {
            ca->precedence = ATR_PREC_CACHED;
            out_atr(out, ca, 0);
            ca->precedence = ATR_PREC_OBJECT;
        } else {
            out_atr(out, ca, 0);
        }
    }

    tmp = ardp_send(req,dlink->host,0,ARDP_WAIT_TILL_TO);

    if(pfs_debug && tmp) {
        fprintf(stderr,"Dirsend failed: %d\n",tmp);
    }

    if(req->rcvd == NOPKT) {
        ardp_rqfree(req);
        vdir_freelinks(dir);
        return(perrno);
    }
    /* Here we must parse reponse - While looking at each packet */
    rreqtoin(req, in);
    while(!in_eof(in)) {
        char		*line;
        char            *next_word;

        if (tmp = in_line(in, &line, &next_word)) {
            ardp_rqfree(req);
            vdir_freelinks(dir);
            return(tmp);
        }
        switch (*line) {

        case 'F': /* FAILURE or FORWARDED */
            /* FORWARDED */
            if(strncmp(line,"FORWARDED",9) == 0) {
                if(fwdcnt-- <= 0) {
                    ardp_rqfree(req);
                    vdir_freelinks(dir);
                    perrno = PFS_MAX_FWD_DEPTH;
                    return(perrno);
                }
                /* parse and start over */
                tmp = qsscanf(line,"FORWARDED %*s %'&s %*s %'&s %*d %*d", 
                             &dlink->host, &dlink->hsoname);

                if(tmp < 2) {
                    ardp_rqfree(req);
                    perrno = DIRSRV_BAD_FORMAT;
                    break;
                }
                ardp_rqfree(req);
                goto startover;
            }
            /* If FAILURE or anything else scan error */
            goto scanerr;

        case 'S': /* SUCCESS */
            if(strncmp(line,"SUCCESS",7) == 0) {
                ardp_rqfree(req);
                vdir_freelinks(dir);
                return(PSUCCESS);
            }
            goto scanerr;

        scanerr:
        default:
            if(*line && (tmp = scan_error(line, req))) {
                ardp_rqfree(req);
                vdir_freelinks(dir);
                return(tmp);
            }
            break;
        }
    }
    perrno = DIRSRV_BAD_FORMAT;
    ardp_rqfree(req);
    vdir_freelinks(dir);
    return(perrno);
}
