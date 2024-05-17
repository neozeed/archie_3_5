/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

/* Credits: originally by bcn@cs.washington.edu 1989 -- 1991 */
/* Severely mutilated by swa@isi.edu, 9/21/92. */

#include <stdio.h>
#include <string.h>

#include <ardp.h>
#include <pfs.h>
#include <psrv.h>
#include <pparse.h>
#include <plog.h>
#include <perrno.h>
#include <pmachine.h>

#include "dirsrv.h"

extern char	    *acltypes[];

int
edit_acl(RREQ req, char **commandp, char **next_wordp, INPUT in, 
         char client_dir[], int dir_magic_no)
{
    char 		t_name[ARDP_PTXT_LEN_R];
    char		t_options[ARDP_PTXT_LEN_R];
    char        	*p_principals;
    int         	retval;
    int			n_options;
    char		insrights[ARDP_PTXT_LEN_R];
    ACL			nacl;		  /* New ACL entry                */
    int			tmp;
    PFILE		fi = NULL;
    int			rsinfo_ret;
    VDIR_ST		dir_st;           /* Directory contents used ...  */
    VDIR		dir = &dir_st;    /* by individual lines          */
    VLINK		clink;            /* For stepping through links   */
    ACL			wacl;		  /* Working access control list  */
    int			aclchk;	          /* Cached ACL check             */
    long         	t_magic_no;
    char        	*dummy_cp;
    int			i;

    vdir_init(dir);
    *t_name = '\0';             /* t_name is not sent for the DIRECTORY option.
                                   (or, if it is sent, should be sent null).
                                   OBJECT option not implemented. */

    p_principals = NULL;
    tmp = qsscanf(*next_wordp, "%'!!s %'!!s %r", t_options, sizeof t_options, 
                  t_name, sizeof t_name, next_wordp);

    /* XXX for now, we ignore the ID field. */
    if (in_select(in, &t_magic_no))
        return error_reply(req, "EDIT-ACL: %'s", p_err_string);
    /* Log and return a better message */
    if(tmp != 2) {
        creply(req,"ERROR EDIT-ACL Wrong number of arguments\n");
        plog(L_DIR_PERR,req,"Too few arguments: %s", *commandp);
        RETURNPFAILURE;
    }

    
    /* Do we need a better log message? */
    plog(L_DIR_UPDATE,req,"EA %s %s %s",client_dir,t_name,t_options);

    if(in_acl(in, &nacl)) {
        creplyf(req, "ERROR %'s\n", p_err_string);
        return perrno;
    }
    /* XXX This is a temporary limitation, although I've left it in the 
       protocol.  We can change it later, but this will get us going for now.
       */  
    if (!nacl || nacl->next) 
        return error_reply(req, 
                           "EDIT-ACL must be followed by exactly 1 ACL line.");

    n_options = 0;

    /* Parse the options */
    /* Doesn't do a whole lot of error checking. */
    if(sindex(t_options,"NOSYSTEM")) n_options|=EACL_NOSYSTEM;
    if(sindex(t_options,"NOSELF")) n_options|=EACL_NOSELF;
    if(sindex(t_options,"DEFAULT")) n_options|=EACL_DEFAULT;
    if(sindex(t_options,"SET")) n_options |= EACL_SET;
    if(sindex(t_options,"INSERT")) n_options|=EACL_INSERT;
    if(sindex(t_options,"DELETE")) n_options|=EACL_DELETE;
    if(sindex(t_options,"ADD")) n_options|=EACL_ADD;
    if(sindex(t_options,"CREATE")) n_options|=EACL_CREATE;
    if(sindex(t_options,"DESTROY")) n_options|=EACL_DESTROY;
    if(sindex(t_options,"SUBTRACT")) n_options|=EACL_SUBTRACT;
    if(sindex(t_options,"LINK")) n_options|=EACL_LINK;
    if(sindex(t_options,"DIRECTORY")) n_options|=EACL_DIRECTORY;
    if(sindex(t_options,"OBJECT")) n_options|=EACL_OBJECT;
    if(sindex(t_options,"INCLUDE")) n_options|=EACL_INCLUDE;
    if(sindex(t_options,"NAMED")) n_options|=EACL_NAMED;

    if(!((n_options&EACL_OTYPE)^EACL_OBJECT)) {
	fi = pfalloc();
	if (!fi) out_of_memory();
	retval = dsrfinfo(t_name,0,fi);
	if(retval == DSRFINFO_FORWARDED) {
	    VLINK fl;               /* List of forwarding pointers */
	    VLINK fp;               /* Current forwarding pointer */
	    fl = fi->forward; fi->forward = NULL;
	    fp = check_fwd(fl,t_name,0);
	    /* Free what we don't need */
	    pffree(fi); fi = NULL;
	    /* Got to location to return forwarded error */
	    forwarded(req, fl, fp, t_name);
	    return PSUCCESS;
	}
	wacl = fi->oacl;
    }
    else {
	retval = dsrdir(client_dir,dir_magic_no,dir,NULL,0);
	if(retval == DSRFINFO_FORWARDED) {
	    return dforwarded(req, client_dir, dir_magic_no, dir);
	}
	/* If not a directory, say so */
	if(retval == DSRDIR_NOT_A_DIRECTORY) {
	    creply(req,"FAILURE NOT-A-DIRECTORY\n");
	    plog(L_DIR_ERR, req, "Invalid directory name: %s", client_dir);
	    RETURNPFAILURE;
	}
	wacl = dir->dacl;
    }

    if(!((n_options&EACL_OTYPE)^EACL_LINK)) {
        /* Need to find the link so we can change its ACL */
        clink = dir->links;
        while(clink) {
            if(strcmp(clink->name,t_name) == 0 && clink->linktype != '-')
                break;
            clink = clink->next;
        }
        if(!clink) {
            clink = dir->ulinks;
            while(clink) {
                if(strcmp(clink->name,t_name) == 0)
                    break;
                clink = clink->next;
            }		 
        }
        if(!clink) {
            creplyf(req,"FAILURE NOT-FOUND LINK %'s\n",t_name);
            plog(L_DIR_ERR,req,"Link not found: %s %s",client_dir,t_name);
            vdir_freelinks(dir);
            RETURNPFAILURE;
        }
        if (clink->linktype == 'N') {
            creplyf(req, "FAILURE SERVER-FAILED Modifying NATIVE links is not \
implemented.  Reset this directory''''s INCLUDE-NATIVE attribute to \
NONATIVE and retry the operation.");
            plog(L_FAILURE, req, "Tried to EDIT-ACL on NATIVE link: %s %s",
                 client_dir, t_name);
            vdir_freelinks(dir);
            RETURNPFAILURE;
        }
        if(clink->acl) wacl = clink->acl;
        /* Check and update link ACL */
        aclchk = srv_check_acl(clink->acl,dir->dacl,req,"a",SCA_LINK,NULL,NULL);
        if(!aclchk && nacl->rights && *(nacl->rights) &&
           (!((n_options&EACL_OP)^EACL_ADD) ||
            !((n_options&EACL_OP)^EACL_INSERT) ||
            !((n_options&EACL_OP)^EACL_SUBTRACT) ||
            !((n_options&EACL_OP)^EACL_DELETE))) {
            if(!((n_options&EACL_OP)^EACL_ADD) ||
               !((n_options&EACL_OP)^EACL_INSERT))
                *insrights = ']';
            else *insrights = '[';
            strcpy(insrights+1,nacl->rights);
            aclchk = srv_check_acl(clink->acl,dir->dacl,req,insrights,SCA_LINK,NULL,NULL);
            /* Don't use this to upgrade [ to a */
            if(aclchk) n_options |= EACL_NOSELF;
        }
        /* If not authorized, say so */
        if(!aclchk) {
            creply(req,"FAILURE NOT-AUTHORIZED\n");
            plog(L_AUTH_ERR,req,
                 "Unauthorized EDIT-ACL: %s %s",client_dir,t_name); 
            vdir_freelinks(dir);
            RETURNPFAILURE;
        }
        retval = change_acl(&(clink->acl),nacl,req,n_options,dir->dacl);
        /* if native and successful, native no more */
        if (!retval && (clink->linktype == 'N')) 
            clink->linktype = 'L'; 
    }
    else if(!((n_options&EACL_OTYPE)^EACL_DIRECTORY)) {
        /* Check and update directory ACL */
        aclchk = srv_check_acl(dir->dacl,NULL,req,"A",SCA_DIRECTORY,
			       client_dir,NULL);
        if(!aclchk && nacl->rights && *(nacl->rights) &&
           (!((n_options&EACL_OP)^EACL_ADD) ||
            !((n_options&EACL_OP)^EACL_INSERT) ||
            !((n_options&EACL_OP)^EACL_SUBTRACT) ||
            !((n_options&EACL_OP)^EACL_DELETE))) {
            if(!((n_options&EACL_OP)^EACL_ADD) ||
               !((n_options&EACL_OP)^EACL_INSERT))
                *insrights = '>';
            else *insrights = '<';
            strcpy(insrights+1,nacl->rights);
            aclchk = srv_check_acl(dir->dacl,NULL,req,insrights,SCA_DIRECTORY,
				   client_dir,NULL);
            /* Don't use this to upgrade < to a */
            if(aclchk) n_options |= EACL_NOSELF;
        }
        /* If not authorized, say so */
        if(!aclchk) {
            creply(req,"FAILURE NOT-AUTHORIZED\n");
            plog(L_AUTH_ERR, req, "Unauthorized EDIT-ACL: %s %s",
		 client_dir,t_name); 
            /* Free the directory links */
            vdir_freelinks(dir);
            RETURNPFAILURE;
        }
        retval = change_acl(&(dir->dacl),nacl,req,n_options,dir->dacl);
    }
    else if(!((n_options&EACL_OTYPE)^EACL_OBJECT)) {
        /* Check and update directory ACL */
        aclchk = srv_check_acl(fi->oacl,NULL,req,"A",SCA_OBJECT,t_name,NULL);
        if(!aclchk && nacl->rights && *(nacl->rights) &&
           (!((n_options&EACL_OP)^EACL_ADD) ||
            !((n_options&EACL_OP)^EACL_INSERT) ||
            !((n_options&EACL_OP)^EACL_SUBTRACT) ||
            !((n_options&EACL_OP)^EACL_DELETE))) {
            if(!((n_options&EACL_OP)^EACL_ADD) ||
               !((n_options&EACL_OP)^EACL_INSERT))
                *insrights = '>';
            else *insrights = '<';
            strcpy(insrights+1,nacl->rights);
	    aclchk = srv_check_acl(fi->oacl,NULL,req,insrights,
				   SCA_OBJECT,t_name,NULL);
            /* Don't use this to upgrade < to a */
            if(aclchk) n_options |= EACL_NOSELF;
        }
        /* If not authorized, say so */
        if(!aclchk) {
            creply(req,"FAILURE NOT-AUTHORIZED\n");
            plog(L_AUTH_ERR, req, "Unauthorized EDIT-ACL: %s",
		 t_name); 
            /* Free the directory links */
	    pffree(fi); fi = NULL;
            RETURNPFAILURE;
        }
        retval = change_acl(&(fi->oacl),nacl,req,n_options,fi->oacl);
    }
    else {
        creply(req,"ERROR invalid or unimplemented option\n");
        plog(L_DIR_PERR,req,"Invalid option: %s",*commandp);
        vdir_freelinks(dir);
        RETURNPFAILURE;
    }

    /* if unsuccessfull say so (need to clean this up) */
    if(retval) creply(req,"FAILURE NOT-FOUND ACL\n");
    else { /* Otherwise write the directory and indicate success */
	if(!((n_options&EACL_OTYPE)^EACL_OBJECT)) {
	    retval = dswfinfo(t_name, fi);
	}
	else {
	    retval = dswdir(client_dir,dir);
	}
	    if(retval) creply(req,"FAILURE\n");
	    else replyf(req,"SUCCESS\n",0);
    }
    vdir_freelinks(dir);
    return(retval);            /* successfully completed. */
}
