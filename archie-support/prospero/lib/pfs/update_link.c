/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1992, 1993       by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

/* This function needs to be rewritten, but we'll wait on that until we have
   link updating fully implemented.  --swa */
#include <uw-copyright.h>
#include <usc-copyr.h>

#include <stdio.h>
#include <string.h>

#include <ardp.h>
#include <pfs.h>
#include <pprot.h>
#include <perrno.h>

extern int	pfs_debug;

/*
 */
int
update_link(dirname,components)
    const char	*dirname;	/* Directory name           */
    const char	*components;	/* Components to be updated */
{

    char	*dirhst;	/* Host of current directory */
    char	*remdir;	/* Dir on remote host	     */

    RREQ	req;		/* Text of request to dir server   */

    char	fwdhst[MAX_DIR_LINESIZE];
    char	fwdfnm[MAX_DIR_LINESIZE];

    VDIR_ST	dir_st;
    VDIR	dir = &dir_st;

    int		tmp;

    vdir_init(dir);

    /* We must first find the directory containing the */
    /* links to be updated                             */

    tmp = rd_vdir(dirname,0,dir,RVD_DFILE_ONLY);
    if (tmp || (dir->links == NULL)) return(DIRSRV_NOT_DIRECTORY);
    dirhst = dir->links->host;
    remdir = dir->links->hsoname;

startover:

    req = p__start_req(dirhst);
    p__add_req(req, "DIRECTORY ASCII %'s\nUPDATE '' COMPONENTS %s\n",
           remdir, components);

    tmp = ardp_send(req, dirhst, 0, ARDP_WAIT_TILL_TO);
    if(tmp) {
        fprintf(stderr,"Dirsend failed: %d\n",perrno);
	perrno = tmp;
    }
    
    if(req->rcvd == NULL) return(perrno);

    /* Here we parse the response.  Right now we just check the */
    /* first packet.  For this request, it should not take      */
    /* any more than one. If UPDATED, then we were successful   */
    /* if FAILURE, or anything else, we failed, and should      */
    /* return an appropriate error                              */

    if(*(req->rcvd->text) != 'U') {

        if(strncmp(req->rcvd->text,"NOT-A-DIRECTORY",15) == 0) {
            ardp_rqfree(req);
            return(DIRSRV_NOT_DIRECTORY);
        }

        /* FORWARDED */
        if(strncmp(req->rcvd->text,"FORWARDED",9) == 0) {
            /* parse and start over */

            tmp = sscanf(req->rcvd->text,"FORWARDED %*s %s %*s %s %*d %*d", 
                         fwdhst,fwdfnm);

            dirhst = stcopy(fwdhst);
            remdir = stcopy(fwdfnm);

            ardp_rqfree(req);

            if(tmp < 2) return(DIRSRV_BAD_FORMAT);
            else goto startover;
        }

        /* We should return specific error codes.  Maybe have  */
        /* a routine which will parse the response and turn    */
        /* it into an error code                               */
        if(pfs_debug)
            fprintf(stderr,"UPDATE: %s\n",req->rcvd->text);

	ardp_rqfree(req);
        RETURNPFAILURE;
    }

    ardp_rqfree(req);
    return(PSUCCESS);
}

