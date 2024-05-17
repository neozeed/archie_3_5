/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>
 */

/* Seriously mutilated by swa@isi.edu, 9/21/92 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#include <stdio.h>
#include <string.h>	/* SOLARIS doesnt have strings.h */

#include <ardp.h>
#include <pfs.h>
#include <plog.h>
#include <psrv.h>
#include <pparse.h>
#include <perrno.h>

#include <pmachine.h>

#include "dirsrv.h"


int
create_link(RREQ req, char **commandp, char **next_wordp, 
            INPUT in, char client_dir[], int dir_magic_no)
{
    VLINK	clink;            /* For stepping through links   */
    PATTRIB     at;     /* Attributes */
    int tmp;
    char  	t_linktype;
    char 	t_fname[ARDP_PTXT_LEN_R];
    char	t_options[ARDP_PTXT_LEN_R];
    VDIR_ST	dir_st;            /* Directory contents used ... */
    VDIR	dir = &dir_st;     /* by individual lines         */
    int         retval;

    vdir_init(dir);
    
    tmp = qsscanf(*next_wordp, "%'s %r", t_options, next_wordp);
    /* Log and return a better message */
    if(tmp < 2)
        return error_reply(req, "Too few arguments: %'s", *commandp, 0);

    /* Read link info. and attributes. */
    retval = 
        in_link(in, *commandp, *next_wordp, 0, &clink,
                (TOKEN *) NULL);
    if (retval)
        return error_reply(req, "%s", p_err_string, 0);
    /* Do we need a better log message? */
    plog(L_DIR_UPDATE, req,"CL %s %c %s %s %s %s %s %s", client_dir,
         clink->linktype, clink->name, clink->target, clink->hosttype,
         clink->host, clink->hsonametype,clink->hsoname,0);
                          

    /* The next two tests are repeated several places throughout the code.
       Might be Nice if we made a function out of them. */
    retval = dsrdir(client_dir,dir_magic_no,dir,NULL,0);
    if(retval == DSRFINFO_FORWARDED) {
        dforwarded(req, client_dir, dir_magic_no, dir);
	vlfree(clink);
        return PSUCCESS;
    }
    /* If not a directory, say so */
    if(retval == DSRDIR_NOT_A_DIRECTORY) {
        creply(req,"FAILURE NOT-A-DIRECTORY\n");
        plog(L_DIR_ERR, req,"Invalid directory name: %s", client_dir,0);
	vlfree(clink);
        RETURNPFAILURE;
    }

    /* If not authorized, say so */
    if(!srv_check_acl(dir->dacl,NULL,req, "I",SCA_DIRECTORY,client_dir,NULL)) {
        creply(req,"FAILURE NOT-AUTHORIZED\n");
        plog(L_AUTH_ERR,req, "Unauthorized CREATE-LINK: %s %s",client_dir,
             t_fname, 0); 
        /* Free the directory links */
	vlfree(clink);
        vdir_freelinks(dir);
        RETURNPFAILURE;
    }

    /* Make sure creator has all rights to link */
    if(!srv_check_acl(dir->dacl,NULL,req, "alrmd",
		      SCA_DIRECTORY,client_dir,NULL)) 
	srv_add_client_to_acl("alrmd",req,&(clink->acl),EACL_LINK);

    retval = vl_insert(clink,dir,VLI_NOCONFLICT);
    if((retval == VL_INSERT_ALREADY_THERE) ||
       (retval == UL_INSERT_ALREADY_THERE))    {
        creplyf(req,"FAILURE ALREADY-EXISTS %'s\n",clink->name);
        /* Free the directory links */
        vdir_freelinks(dir);
        RETURNPFAILURE;
    }
    else if(retval == VL_INSERT_CONFLICT) {
        creplyf(req,"FAILURE NAME-CONFLICT %'s\n",clink->name);
        plog(L_DIR_ERR, req,"Conflicting link already exists: %'s %'s",
	     client_dir, clink->name,0);
        /* Free the directory links */
        vdir_freelinks(dir);
        RETURNPFAILURE;
    }

    if(!retval) retval = dswdir(client_dir,dir);

    /* if successful say so (need to clean this up) */
    if(!retval)
        reply(req,"SUCCESS\n");
    else 
        creplyf(req,
                "FAILURE SERVER-FAILED Unable to write server directory %s\n",
                client_dir );

    /* Free the directory links */
    vdir_freelinks(dir);
    return(retval);
}
