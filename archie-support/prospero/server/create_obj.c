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
#include <plog.h>
#include <psrv.h>
#include <perrno.h>
#include <pmachine.h>

#include "dirsrv.h"

/*
 * Modified 6/4/93, swa: It now writes out the child directory first.
 * It only writes out the changed parent directory if the child directory
 * was successfully written.  This means that if we have trouble writing the
 * child directory, we don't end up with a parent directory containing
 * a link to a nonexistent object.
 */

/* 
 * Modified 11/2/93, swa: Now allocates VIRTUAL subdirectories out of an 
 * object pool if the parent is not NONATIVE.  This solves the previous buggy
 * method of creating VIRTUAL subdirectories of real UNIX directories. 
 */

ACL aclcopy(ACL acl);

static VLINK
gensym_child(VDIR dir, char *newdir_parent, char *child_linkname, 
             char **child_hsonamep);

/* This was create_directory */
int
create_object(RREQ req, char **commandp, char **next_wordp, INPUT in, 
              char client_dir[], int dir_magic_no)
{
    AUTOSTAT_CHARPP(t_optionsp);  /* options to command. */
    AUTOSTAT_CHARPP(child_linknamep); /* link name in parent. */
    AUTOSTAT_CHARPP(child_hsonamep); /* HSONAME for the child. */
    VLINK	clink;          /* For the child directory. */
    int         tmp;
    int		lpriv;          /* LPRIV option  */
    int		retval;
    VDIR_ST	dir_st;         /* Parent directory contents. */
    VDIR	dir = &dir_st;
    VDIR_ST	new_dir_st;     /* Child directory contents. */
    VDIR	new_dir = &new_dir_st;

    /* Now set object_pool in dirsrv.c */
    /* still have to read the remainder of the attributes */
    tmp = qsscanf(*next_wordp,"%'&s %'&s",
                 t_optionsp, child_linknamep);

    /* Log and return a better message */
    if(tmp < 2) {
        creply(req,"ERROR too few arguments");
        plog(L_DIR_PERR,req,"Too few arguments: %s",
             *commandp, 0);
        RETURNPFAILURE;
    }

    /* For now, VIRTUAL and DIRECTORY options must be specified */
    if(!sindex(*t_optionsp, "VIRTUAL")  
       || !sindex(*t_optionsp, "DIRECTORY")) {
        creply(req,"ERROR only VIRTUAL directories implemented\n");
        plog(L_DIR_PERR,req,"Tried to create non-VIRTUAL or
non-directory object: %'s",
             *commandp, 0);
        RETURNPFAILURE;
    }

    if(sindex(*t_optionsp,"LPRIV")) lpriv = 1;
    else lpriv = 0;

    plog(L_DIR_UPDATE,req,"%s", *commandp, 0);
    
    vdir_init(dir);

    retval = dsrdir(client_dir,dir_magic_no,dir,NULL,0);
    if(retval == DSRFINFO_FORWARDED) 
        return dforwarded(req, client_dir, dir_magic_no, dir);

    /* If not a directory, say so */
    if(retval == DSRDIR_NOT_A_DIRECTORY) {
        creply(req,"FAILURE NOT-A-DIRECTORY\n");
        plog(L_DIR_ERR,req,"Invalid directory name: %'s", client_dir,0);
        RETURNPFAILURE;
    }

    /* If not authorized, say so */
    if(!srv_check_acl(dir->dacl,NULL,req,"I",SCA_DIRECTORY,client_dir,NULL)) {
        creply(req,"FAILURE NOT-AUTHORIZED\n");
        plog(L_AUTH_ERR,req,"Unauthorized CREATE-DIRECTORY: %s %s",client_dir,
             *child_linknamep); 
        /* Free the directory links */
        vdir_freelinks(dir);
        RETURNPFAILURE;
    }

    clink = gensym_child(dir, client_dir, *child_linknamep, child_hsonamep);
    if (!clink) {
        creply(req, "FAILURE SERVER-FAILED Could not generate a unique \
hsoname for the new directory.\n");
        vdir_freelinks(dir);
        RETURNPFAILURE;
    }
    retval = vl_insert(clink,dir,VLI_NOCONFLICT);
    if(retval == VL_INSERT_ALREADY_THERE) {
        creplyf(req,"FAILURE ALREADY-EXISTS %'s\n",clink->name);
        /* Free the directory links */
        vdir_freelinks(dir);
        RETURNPFAILURE;
    }
    else if(retval == VL_INSERT_CONFLICT) {
        creplyf(req,"FAILURE NAME-CONFLICT %'s\n",clink->name);
        plog(L_DIR_ERR,req,"Conflicting link already exists: %s %s",client_dir,
             clink->name,0);
        /* Free the directory links */
        vdir_freelinks(dir);
        RETURNPFAILURE;
    }
    
    /* Don't write out the parent yet!  */

    /* Initialize the new directory.  The child will get a copy of the parent's
       ACL. */
    vdir_init(new_dir);
    new_dir->inc_native = VDIN_NONATIVE;

    /* Add creator to the ACL */
    if(!lpriv || (!srv_check_acl(dir->dacl,NULL,req,"BIlr",
				 SCA_DIRECTORY,client_dir,NULL))) {
	/* need to copy since EACL_ADD may change it */
	new_dir->dacl = aclcopy(dir->dacl); 

	if(lpriv) srv_add_client_to_acl("AIlr",req,&(new_dir->dacl),
					EACL_DIRECTORY);
	else srv_add_client_to_acl("ALRWDE",req,&(new_dir->dacl),
				   EACL_DIRECTORY);
    }
    else {
        new_dir->dacl = dir->dacl;
    }
    if(!retval) retval = dswdir(*child_hsonamep,new_dir);
    /* Ok, now we can write out the parent. */
    if(!retval) retval = dswdir(client_dir,dir);

    /* Free the entries */
    if (dir->dacl == new_dir->dacl) new_dir->dacl = NULL; /* don't free twice*/
    vdir_freelinks(dir);
    vdir_freelinks(new_dir);

    /* if successful say so (need to clean this up) */
    if(!retval) reply(req,"SUCCESS\n");
    else creply(req,"FAILURE\n");
    return retval;
}

/* Gensym a vlink for the child directory. */
static VLINK
gensym_child(VDIR dir, char *newdir_parent, char *child_linkname, 
             char **child_hsonamep)
{
    VDIR_ST tmp_dir_st;         /* scratch directory */
    VDIR tmp_dir = &tmp_dir_st; /* scratch directory */
    int retval;                 /* value returned by dsrdir() */
    /* Allocate a link for the child directory. */
    VLINK clink = vlalloc();
    clink->name = stcopyr(child_linkname,clink->name);
    clink->target = stcopyr("DIRECTORY",clink->target);
    clink->host = stcopyr(hostwport,clink->host);

    /* come up with an unused hsoname for the new directory. */
    *child_hsonamep = qsprintf_stcopyr(*child_hsonamep, "%s/%s", 
              dir->inc_native == VDIN_NONATIVE ? newdir_parent : object_pool,
                                     child_linkname); 
    /* Try first a name unadorned with a :# */
    vdir_init(tmp_dir);
    retval = dsrdir(*child_hsonamep, 0L, tmp_dir, (VLINK) NULL, 0);
    if (retval == PSUCCESS) {
        char *template = NULL;
        int i;
        
        template = qsprintf_stcopyr(template, "%s:%%d", *child_hsonamep);
        i = 1;
        for (;;) {
            *child_hsonamep = qsprintf_stcopyr(*child_hsonamep, template, i);
            vdir_freelinks(tmp_dir);
            retval = dsrdir(*child_hsonamep, 0L, tmp_dir, (VLINK) NULL, 0);
            if (retval != PSUCCESS)
                /* Either *child_hsonamep now has an unused name or reading that
                   directory failed for some reason we can't do much about */
                break;          
            ++i;
        }
    }
    vdir_freelinks(tmp_dir);
    if (retval != DSRDIR_NOT_A_DIRECTORY) {
        /* dsrdir() failed for some reason other than because *child_hsonamep was
           unused.  We can't do much about it here if it wasn't caught when
           reading the parent. */
        vlfree(clink);
        return NULL;
    }
    clink->hsoname = stcopyr(*child_hsonamep, clink->hsoname);
    return clink;
}

