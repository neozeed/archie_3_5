/*
 * Copyright (c) 1992, 1993,1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

/* Originally written by BCN */
/* Seriously mutilated by swa@isi.edu, 9/29/92 -- 9/30/92 */
/* Memory leak fixed, swa, 3/31/94, thanks to mitra for noticing a potential
   problem.  */

#include <ardp.h>
#include <pfs.h>
#include <plog.h>
#include <psrv.h>
#include <perrno.h>
#include <pprot.h>
#include <pmachine.h>
#include "dirsrv.h"

/* list_acl.c */

extern ACL	default_acl;
extern ACL	system_acl;
extern ACL	override_acl;
extern ACL      maint_acl;
extern ACL	nullobj_acl;
extern ACL	nullcont_acl;
extern ACL	nulllink_acl;
extern ACL	nulldir_acl;

extern char	*acltypes[];

ACL get_container_acl();

int 
list_acl(RREQ req, char * command, char *next_word, INPUT in, 
         char client_dir[], int dir_magic_no)
{
    optdecl;
    AUTOSTAT_CHARPP(t_namep);
    AUTOSTAT_CHARPP(t_optionsp);
    int		retval = PSUCCESS;/* Return value from subfunctions */
    ACL		wacl;		  /* Working access control list  */
    int		aclchk;	          /* Cached ACL check             */
    VLINK       clink;            /* For stepping through links   */
    P_OBJECT    ob = oballoc();
    int         tmp;
    OUTPUT_ST   out_st;
    OUTPUT      out = &out_st;
    long        n_magic_no;     /* magic # of link. XXX currently ignored.*/
    
    reqtoout(req, out);

    if (!*t_namep) *t_namep = stcopy("");
    if (!*t_namep) out_of_memory();

    /* First arg is options.  All others are optional. */
    /* If a second argument is specified, it is the    */
    /* link for which the ACL is to be returned        */
    /* if the OBJECT option is specified, then args    */
    /* 2-5 identify the object instead of the link.  Object ACLs unimplemented,
       so we currently suppress those arguments. */
    tmp = qsscanf(next_word, "%&'s %&'s %'*s %*d", &*t_optionsp, &*t_namep);
    /* Log and return a better message */
    if (tmp < 0)
        interr_buffer_full();
    if((tmp < 1) || 
       ((tmp < 2) && (!strequal(*t_optionsp,"DIRECTORY"))))  {
        creply(req,"ERROR too few arguments\n");
        plog(L_DIR_PERR,req,"Too few arguments: %s", command);
        obfree(ob); ob = NULL;
        RETURNPFAILURE;
    }

    if (in_select(in, &n_magic_no))
        return error_reply(req, "LIST-ACL %'s", p_err_string);

    optstart(*t_optionsp);
    /* Do we need a better log message? */
    plog(L_DIR_REQUEST,req,"LA %s %s", client_dir,*t_namep);

    if(opttest("INCLUDE")) {
	if(strequal(*t_namep,"SYSTEM")) wacl = system_acl;
	else if(strequal(*t_namep,"DEFAULT")) wacl = default_acl;
	else if(strequal(*t_namep,"OVERRIDE")) wacl = override_acl;
	else if(strequal(*t_namep,"MAINTENANCE")) wacl = maint_acl;
        else {
            creplyf(req,"FAILURE NOT-FOUND ACL INCLUDED %'s\n",*t_namep);
            plog(L_DIR_ERR,req,"INCLUDED ACL not found: %s", *t_namep);
            obfree(ob); ob = NULL;
            RETURNPFAILURE;
        }
        aclchk = srv_check_acl(wacl,NULL,req,"Y",SCA_MISC,NULL,NULL);
	/* If not authorized, say so */
	if(!aclchk) {
	    creply(req,"FAILURE NOT-AUTHORIZED\n");
	    plog(L_AUTH_ERR,req,"Unauthorized LIST-ACL: INCLUDED %s", *t_namep);
            obfree(ob); ob = NULL;
	    RETURNPFAILURE;
	}
	if(wacl == NULL) retval = reply(req,"ACL NONE '' ''\n");
	else out_acl(out, wacl);
        obfree(ob); ob = NULL;
	return(retval);
    }
    else if(opttest("NAMED")) {
        tmp = get_named_acl(*t_namep, &wacl);
        if (tmp) {
            creplyf(req,"FAILURE NOT-FOUND ACL NAMED %'s\n",*t_namep);
            plog(L_DIR_ERR,req,"NAMED ACL not found: %s", *t_namep);
            obfree(ob); ob = NULL;
            RETURNPFAILURE;
        }
        aclchk = srv_check_acl(wacl,NULL,req,"Y",SCA_MISC,NULL,NULL);
	/* If not authorized, say so */
	if(!aclchk) {
	    creply(req,"FAILURE NOT-AUTHORIZED\n");
	    plog(L_AUTH_ERR,req,"Unauthorized LIST-ACL: NAMED %s", *t_namep);
            obfree(ob); ob = NULL;
	    RETURNPFAILURE;
	}
	if(wacl == NULL) retval = reply(req,"ACL NONE '' ''\n");
	else out_acl(out, wacl);
        aclfree(wacl);
        obfree(ob); ob = NULL;
	return(retval);
    }
    else if(opttest("OBJECT")||opttest("CONTAINER")) {
	int		rsinfo_ret;

        rsinfo_ret = dsrobject(req, "ASCII", *t_namep, 0L, 0L, 0, 
                              (struct dsrobject_list_options *) NULL, ob);
#ifdef DIRECTORYCACHING
	if (rsinfo_ret) dsrobject_fail++;
#endif
	if(rsinfo_ret == DSRFINFO_FORWARDED) {
	    VLINK fl;               /* List of forwarding pointers */
	    VLINK fp;               /* Current forwarding pointer */
	    fl = ob->forward; ob->forward = NULL;
	    fp = check_fwd(fl,*t_namep,0);


	    /*  Return forwarded message */
	    forwarded(req, fl, fp, *t_namep);
	    /* Free what we don't need */
            obfree(ob); ob = NULL;
	    return PSUCCESS;
	}

	if(opttest("CONTAINER")) {
	    wacl = get_container_acl(*t_namep);
	}
	else {
	    wacl = ob->acl; ob->acl = NULL;
	}


        aclchk = srv_check_acl(wacl,NULL,req,"Y",SCA_OBJECT,NULL,NULL);
	/* If not authorized, say so */
	if(!aclchk) {
	    creply(req,"FAILURE NOT-AUTHORIZED\n");
	    plog(L_AUTH_ERR,req,"Unauthorized LIST-ACL: OBJECT %s", *t_namep);
	    aclfree(wacl);
            obfree(ob); ob = NULL;
	    RETURNPFAILURE;
	}

	if(wacl == NULL) {
	    if(opttest("CONTAINER")) out_acl(out, nullcont_acl);
	    else out_acl(out, nullobj_acl);
	}
	else out_acl(out, wacl);
        obfree(ob); ob = NULL;
	return(retval);
    }

    retval = dsrobject(req, "ASCII", client_dir, 0L, dir_magic_no,
                       0, (struct dsrobject_list_options *) NULL, ob);
#ifdef DIRECTORYCACHING
    if (retval) dsrobject_fail++;
#endif
    if(retval == DSRFINFO_FORWARDED) {
        obforwarded(req, client_dir, dir_magic_no, ob);
        obfree(ob); ob = NULL;
        return PSUCCESS;
    }

    /* If not a directory, say so */
    if(retval == DSRDIR_NOT_A_DIRECTORY) {
        creply(req,"FAILURE NOT-A-DIRECTORY\n");
        plog(L_DIR_ERR,req,"Invalid directory name: %s", client_dir);
        obfree(ob); ob = NULL;
        RETURNPFAILURE;
    }
    if (retval) {
        creplyf(req, "FAILURE SERVER-FAILED Could not read directory %'s\n",
               client_dir);
        obfree(ob); ob = NULL;
        RETURNPFAILURE;
    }

    wacl = NULL;

    if(opttest("LINK")) {
        /* Need to find the link so we can check its ACL */
        clink = ob->links;
        while(clink) {
            if(strequal(clink->name,*t_namep) && clink->linktype != '-')
                break;
            clink = clink->next;
        }
        if(!clink) {
            clink = ob->ulinks;
            while(clink) {
                if(strcmp(clink->name,*t_namep) == 0)
                    break;
                clink = clink->next;
            }		 
        }
        if(!clink) {
            creplyf(req,"FAILURE NOT-FOUND LINK %'s",*t_namep);
            plog(L_DIR_ERR,req,"Link not found: %s %s", client_dir, *t_namep);
            obfree(ob); ob = NULL;
            RETURNPFAILURE;
        }
        wacl = clink->acl;
        aclchk = srv_check_acl(clink->acl,ob->acl,req,"v",SCA_LINK,NULL,NULL);
    } else if(opttest("DIRECTORY")) {
        wacl = ob->acl;
        aclchk = srv_check_acl(ob->acl,NULL,req,"V",SCA_DIRECTORY,client_dir,NULL);
    } else {
        creply(req,"ERROR invalid option\n");
        plog(L_DIR_PERR,req,"Invalid option: %s", command);
        obfree(ob); ob = NULL;
        RETURNPFAILURE;
    }

    /* If not authorized, say so */
    if(!aclchk) {
        creply(req,"FAILURE NOT-AUTHORIZED\n");
        plog(L_AUTH_ERR,req,"Unauthorized LIST-ACL: %s %s", client_dir,*t_namep);
        obfree(ob); ob = NULL;
        RETURNPFAILURE;
    }

    if(wacl == NULL) {
        if(opttest("LINK")) /* Link default is diracl */
	    out_acl(out, nulllink_acl);
        else {
	    out_acl(out, nulldir_acl);
        }
    } else {
        out_acl(out, wacl);
    }
    obfree(ob); ob = NULL;
    return retval;
}                           /* What does this go to? */
