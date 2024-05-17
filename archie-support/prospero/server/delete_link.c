/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>

#include <stdio.h>
#include <string.h>

#include <ardp.h>
#include <pfs.h>
#include <pparse.h>
#include <plog.h>
#include <psrv.h>
#include <perrno.h>
#include <pmachine.h>

#include "dirsrv.h"

static void
srv_vl_delete(VDIR dir, VLINK vl);

int 
delete_link(RREQ req, char *command, char *next_word, INPUT in,
            char client_dir[], int dir_magic_no)
{
    char 	t_name[ARDP_PTXT_LEN_R];
    char 	t_options[ARDP_PTXT_LEN_R];
    long        t_magic_no = 0;  /* 0 means none specified.     */
    int         retval;
    ACL		wacl;		 /* Working access control list */
    int         tmp;             /* return value from qsscanf() */
    char        *cp;             /* dummy pointer               */
    VLINK       vl = NULL;  /* target link         */
    VDIR_ST	dir_st;          /* Directory contents used ... */
    VDIR	dir = &dir_st;   /* by individual lines         */

    vdir_init(dir);

    tmp = qsscanf(next_word,"%'s %'s %r", t_options, t_name, &cp);

    /* Log and return a better message */
    if (tmp < 0)
        return error_reply(req, "Format error: %'s", command, 0);
    if(tmp < 2)
        return error_reply(req, "Too few arguments: %'s", command, 0);
    if (tmp > 2)
        return error_reply(req, "Too many arguments: %'s", command, 0);

    if(retval = in_select(in, &t_magic_no))
        return error_reply(req, "DELETE-LINK: %'s", p_err_string);

    if (!strequal(t_options, ""))
        return error_reply(req, "DELETE-LINK: options must be empty: %'s",
                           command);
    /* Do we need a better log message */
    plog(L_DIR_UPDATE,req,"RM %s %s ID REMOTE %ld",
         client_dir,t_name,t_magic_no,0);

    retval = dsrdir(client_dir,dir_magic_no,dir,NULL,0);
    if(retval == DSRFINFO_FORWARDED) 
        return dforwarded(req, client_dir, dir_magic_no, dir);

    /* If not a directory, say so */
    if(retval == DSRDIR_NOT_A_DIRECTORY) {
        creply(req,"FAILURE NOT-A-DIRECTORY\n");
        plog(L_DIR_ERR, req,"Invalid directory name: %s", client_dir,0);
        RETURNPFAILURE;
    }

    /* Need to find the link so we can check its ACL */
    for( vl = dir->links; vl; vl = vl->next) {
        if(strequal(vl->name,t_name) && vl->linktype != '-')
            if ((t_magic_no == 0) || (t_magic_no == vl->f_magic_no))
                break;
    }
    if (!vl) {
        for( vl = dir->ulinks; vl; vl = vl->next) {
            if(strequal(vl->name,t_name))
                if ((t_magic_no == 0) || (t_magic_no == vl->f_magic_no))
                    break;
        }
    }
    if (!vl) {
        creplyf(req, "FAILURE NOT-FOUND LINK %'s\n", t_name);
        plog(L_DIR_ERR,req,"Link not found: %s %s", client_dir, t_name);
        vdir_freelinks(dir);
        RETURNPFAILURE;
    }
    wacl = vl->acl;
    /* If not authorized, say so */
    if(!srv_check_acl(wacl, dir->dacl, req,"d",SCA_LINK,NULL,NULL)) {
        creply(req,"FAILURE NOT-AUTHORIZED\n");
        plog(L_AUTH_ERR,req,"Unauthorized DELETE-LINK: %s %s",client_dir,
             t_name); 
        vdir_freelinks(dir);
        RETURNPFAILURE;
    }
    if (vl->flags & VLINK_NATIVE) {
        creplyf(req, "FAILURE SERVER-FAILED Deleting NATIVE links is not \
defined nor implemented.  You can try instead either \
(a) manually deleting the link named %'s in the native UNIX directory %'s \
on the host %'s \
or (b) setting the link invisible: \
set_atr <linkname> LINK-TYPE -linkprec -replace -field I \
(see any version of the user''''s manual dated \
11/3/93 or later for a discussion of this).\n", t_name, client_dir, hostname);
        plog(L_FAILURE, req, "Tried to DELETE-LINK on NATIVE link: %s %s",
             client_dir, t_name);
        vdir_freelinks(dir);
        RETURNPFAILURE;
    }
    srv_vl_delete(dir, vl);
        
    retval = dswdir(client_dir,dir);
    vdir_freelinks(dir);
    if(!retval) {
        reply(req,"SUCCESS\n");
    } else {
        creplyf(req, "FAILURE SERVER-FAILED Unable to write out modified directory %'s\n", client_dir);
    }
    return retval;
}

static void
srv_vl_delete(VDIR dir, VLINK vl)
{
    if (vl->linktype == 'U') {
        EXTRACT_ITEM(vl, dir->ulinks);
        vlfree(vl);
#if 0
    } else if (vl->flags & VLINK_NATIVE) {
        vl->linktype = '-';
        if (dir->inc_native == VDIN_ONLYREAL) dir->inc_native = VDIN_INCLREAL;
#endif
    } else {
        EXTRACT_ITEM(vl, dir->links);
        vlfree(vl);
    }
}
