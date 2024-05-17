/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#include <stdio.h>
#include <string.h>

#include <ardp.h>
#include <pfs.h>
#include <plog.h>
#include <psrv.h>
#include <pparse.h>
#include <perrno.h>
#include <pmachine.h>

#include "dirsrv.h"

static int vl_delete_attributes(), vl_delete_all_attributes(),
    equal_attribute_names_and_nature();

int
edit_link_info(RREQ req, char **commandp, char **next_wordp, INPUT in,
               char client_dir[], int dir_magic_no)
{
    VLINK		clink;            /* For stepping through links   */
    PATTRIB             at;     /* Attributes */
    int		tmp;            /* Temporary returns from qsscanf() */
    char        t_mod_request[ARDP_PTXT_LEN_R];
    enum    {ADD, REPLACE, DELETE, DELETE_ALL} e_mod_request;
    char 	t_name[ARDP_PTXT_LEN_R];
    long        n_magic_no = 0;
    VDIR_ST		dir_st;            /* Directory contents used ... */
    VDIR		dir = &dir_st;     /* by individual lines         */
    int         retval;
    vdir_init(dir);
    
    if(qsscanf(*next_wordp, "%!!s %!!'s",
               t_mod_request, sizeof t_mod_request, t_name, sizeof t_name) < 2)
        return error_reply(req, "Malformed command: %'s", *commandp);
    if(in_select(in, &n_magic_no))
        return error_reply(req, "EDIT-LINK-INFO: %'s", p_err_string);
    
    if (strequal(t_mod_request, "ADD"))
        e_mod_request = ADD;
    else if (strequal(t_mod_request, "DELETE"))
        e_mod_request = DELETE;
    else if (strequal(t_mod_request, "REPLACE"))
        e_mod_request = REPLACE;
    else if (strequal(t_mod_request, "DELETE-ALL"))
        e_mod_request = DELETE_ALL;
    else
        return error_reply(req, "Malformed command: %'s", *commandp);
    
    if(in_atrs(in, 0, &at))
        return error_reply(req, "EDIT-LINK-INFO: Could not read attributes \
from request packet:  %'s", p_err_string);
    plog(L_DIR_UPDATE,req,"ELI %s %s %s",client_dir,t_name,t_mod_request,0);
    

    /* XXX: The next 2 screens of code would make a nice routine called
       "find_named_link".  Modularize! */
    /* Open the directory. */
    retval = dsrdir(client_dir,dir_magic_no,dir,NULL,0);
    if(retval == DSRFINFO_FORWARDED) {
        dforwarded(req, client_dir, dir_magic_no, dir);
        return PSUCCESS;
    }

    /* If not a directory, say so */
    if(retval == DSRDIR_NOT_A_DIRECTORY) {
        creply(req,"FAILURE NOT-A-DIRECTORY\n");
        plog(L_DIR_ERR, req, "Invalid directory name: %s", client_dir,0);
        RETURNPFAILURE;
    }

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
        plog(L_DIR_ERR,req,"Link not found: %s %s",client_dir, t_name,0);
        vdir_freelinks(dir);
        RETURNPFAILURE;
    }
    /* CLINK is now set to the link we want to modify. */


    /* If not authorized, say so */
    if(!srv_check_acl(clink->acl,dir->dacl,req,"m",SCA_LINK,NULL,NULL)) {
        creply(req,"FAILURE NOT-AUTHORIZED\n");
        plog(L_AUTH_ERR,req,"Unauthorized EDIT-LINK-INFO: %s %s",client_dir,
             clink->name,0); 
        vdir_freelinks(dir);
        RETURNPFAILURE;
    }
    /* Now that we have the link, and permission to modify it, and the
       attributes, let's go do the modification! */
    switch(e_mod_request) {

        /* The cases in this switch are ALL RESPONSIBLE FOR DOING THEIR OWN
           ERROR REPORTING upon failure. */
    case ADD:
        /* XXX this isn't really an ADD operation, because we overwrite
           attributes that can only have single instances, without
           complaining.  However, it will suffice, since it handles all legal
           requests correctly. */
        /* this reports errors in p_err_string */
        retval = vl_add_atrs(at, clink);
        if (retval) {
            creplyf(req, "FAILURE BAD-VALUE %'s\n", p_err_string);
            plog(L_DIR_ERR, req, "Bad Value for MODIFY-LINK ADD: %'s", p_err_string);
        }
        break;
    case DELETE:
        retval = vl_delete_attributes(req, at, clink);
        atlfree(at);
        break;
    case DELETE_ALL:
        retval = vl_delete_all_attributes(req, at, clink, TRUE);
        atlfree(at);
        break;
    case REPLACE:
        retval = vl_delete_all_attributes(req, at, clink, FALSE);
        if (!retval) {
            retval = vl_add_atrs(at, clink);
            if (retval) {
                creplyf(req, "FAILURE BAD-VALUE %'s\n", p_err_string);
                plog(L_DIR_ERR, req, "Bad Value for MODIFY-LINK REPLACE: %'s",
                     p_err_string);
            }
        }
        break;
    default:
        internal_error("reached default case");
    }
    if(!retval) {
        retval = dswdir(client_dir,dir);
        if(!retval)
            reply(req,"SUCCESS\n");
        else
            creplyf(req,"FAILURE SERVER-FAILED EDIT-LINK-INFO Couldn''''t \
write the changes to %'s\n", client_dir);
    }
    vdir_freelinks(dir);

    return retval;
}



/* This does NOT free the attributes to be matched in the ATS list. 
   It DOES free the attributes that were deleted. */

static
int
vl_delete_attributes(req, ats, vl)
    RREQ req;
    PATTRIB ats;
    VLINK vl;
{
    static int vl_delete_attribute();

    while(ats) {
        if(vl_delete_attribute(ats, vl)) {
            creplyf(req,"FAILURE NOT-FOUND Could not DELETE attribute %'s; \
this instance not found", ats->aname); 
            plog(L_DIR_ERR,req, "Desired Instance of an attribute not found \
to delete: %s", ats->aname);
            RETURNPFAILURE;
        } else {
            ats = ats->next;
        }
    }
    return PSUCCESS;
}


static int
vl_delete_attribute(at, vl) 
    PATTRIB at;
    VLINK vl;
{
    int retval;
    /* Filter is the only one we need to special-case right now. */
    /* All of the others object attributes have exactly one instance, except
       for ID.  */
    if (strequal(at->aname, "ID") && at->nature == ATR_NATURE_FIELD) {
        retval = delete_matching_at(at, &(vl->oid), equal_attributes);
        if (retval == PSUCCESS && strequal(at->value.sequence->token, "REMOTE")
            && length(at->value.sequence) == 2)
            vl->f_magic_no = 0;
        return retval;
    } else if (strequal(at->aname, "FILTER") 
               && at->nature == ATR_NATURE_FIELD) {
        return delete_matching_fl(at->value.filter, &(vl->filters));
    } else {
        return delete_matching_at(at, &(vl->lattrib), equal_attributes);
    }
}


static
int
vl_delete_all_attributes(req, at, vl, complain)
    RREQ req;
    PATTRIB at;
    VLINK vl;
    int complain;               /* TRUE if we object to deleting indelible
                                   or non-present attributes.  FALSE if we
                                   don't mind.  XXX -- currently ignored. */ 
{
    static int vl_delete_all_attribute();
    PATTRIB ind;
    for (ind = at; ind; ind = ind->next) {
        while(vl_delete_all_attribute(ind, vl) == PSUCCESS)
            ;
    }
    return PSUCCESS;
    
}

/* If there's more than  one instance of the attribute, we delete only one.
   Must be called repeatedly in order to really delete all of them. */
   
static
int
vl_delete_all_attribute(at, vl) 
    PATTRIB at;
    VLINK vl;
{
    static int equal_attribute_names_and_nature(PATTRIB, PATTRIB);

    if (strequal(at->aname, "ID") && at->nature == ATR_NATURE_FIELD) {
        atlfree(vl->oid);
        vl->oid = NULL;
        vl->f_magic_no = 0;
    } else if (strequal(at->aname, "FILTER") 
               && at->nature == ATR_NATURE_FIELD) {
        fllfree(vl->filters);
        vl->filters = NULL;
    } else {
        return delete_matching_at(at, &(vl->lattrib), 
                                  equal_attribute_names_and_nature);
    }
    return(PSUCCESS);
}


static
int
equal_attribute_names_and_nature(at1, at2)
    PATTRIB at1, at2;
{
    return at1->nature == at2->nature && strequal(at1->aname,at2->aname);
}
