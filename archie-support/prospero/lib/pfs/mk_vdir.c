/*
 * Copyright (c) 1989, 1990 by the University of Washington
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
#include <pmachine.h>

extern int	pfs_debug;

/*
 */
int
mk_vdir(vpath,flags)
    const char	*vpath;		/* Name of the new directory       */ 
    int		flags;		/* Options for directory           */
{

    char	*dirhst;	/* Host of current directory    */
    char	*remdir;	/* Dir on remote host	   */

    char	*cname = "";	/* Component name               */
    char	*pname = "";	/* Parent name		   */

    char	pathcpy[MAX_VPATH];

    int		fwdcnt = MAX_FWD_DEPTH;
    char	fwdhst[MAX_DIR_LINESIZE];
    char	fwdfnm[MAX_DIR_LINESIZE];

    RREQ	req;		/* Text of request to dir server             */

    VDIR_ST	dir_st;
    VDIR	dir = &dir_st;
    INPUT_ST    in_st;
    INPUT       in = &in_st;
    char        *next_line;

    int		tmp;

    vdir_init(dir);

    strcpy(pathcpy,vpath);
    cname = p_uln_rindex(pathcpy,'/');
    if(!cname)  cname = pathcpy;
    else {
        *cname++ = '\0';
        pname = pathcpy;
        if(cname == (pname + 1)) pname = "/";
    }


    /* We must first find the directory into which the link  */
    /* will be inserted                                      */

    tmp = rd_vdir(pname,0,dir,RVD_DFILE_ONLY);
    if (tmp || (dir->links == NULL)) return(DIRSRV_NOT_DIRECTORY);
    dirhst = dir->links->host;
    remdir = dir->links->hsoname;

startover:

    req = p__start_req(dirhst);

    p__add_req(req, "DIRECTORY ASCII %'s\n\
CREATE-OBJECT VIRTUAL+DIRECTORY%s %'s\n",
           remdir, ((flags&MKVD_LPRIV) ? "+LPRIV" : ""), cname);

    tmp = ardp_send(req,dirhst,0,ARDP_WAIT_TILL_TO);

    if(tmp) {
        fprintf(stderr,"ardp_send failed: %d\n",tmp);
	perrno = tmp;
    }

    /* If we don't get a response, then return error */
    if(req->rcvd == NULL) return(perrno);

    rreqtoin(req, in); /* not currently used.  */
    /* Here we must parse reponse - While looking at each packet */
    while(!in_eof(in)) {
        char		*line;
        char            *next_word; /* a dummy, here. */
        int             retval;

        /* Look at each line in packet */
        if(retval = in_line(in, &line, &next_word)) 
            return retval;
        switch (*line) {

        case 'F': /* FAILURE or FORWARDED */
                /* FORWARDED */
            if(strncmp(line,"FORWARDED",9) == 0) {
                if(fwdcnt-- <= 0) {
                    ardp_rqfree(req);
                    perrno = PFS_MAX_FWD_DEPTH;
                    return(perrno);
                }
                /* parse and start over */
                tmp = sscanf(line,"FORWARDED %*s %s %*s %s %*d %*d", 
                             fwdhst,fwdfnm);
                dirhst = stcopy(fwdhst);
                remdir = stcopy(fwdfnm);
                
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
    return(perrno);
}
