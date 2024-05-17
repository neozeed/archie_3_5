/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>
 */

#include <usc-copyr.h>

#include <ardp.h>
#include <pfs.h>
#include <perrno.h>
#include <plog.h>
#include <pparse.h>
#include <pprot.h>
#include <psrv.h>

#include "dirsrv.h"

static int pf_delete_attributes(), pf_delete_all_attributes(),
    equal_attribute_names_and_nature();
static int pf_add_atrs();

int 
srv_edit_object_info(RREQ req, char *command, char *next_word, INPUT in)
{
    char t_mod_request[40];
    AUTOSTAT_CHARPP(t_handlep);
    long    t_magic_no = 0;     /* should be used. */
    enum    {ADD, REPLACE, DELETE, DELETE_ALL} e_mod_request;
    int rsinfo_ret;
    PFILE fi;
    int retval = PSUCCESS;                 /* return value from modification functions. */
    PATTRIB at;
    char *dummy_cp;

    /* First, parse the input. */
    if (qsscanf(next_word, "%!!s %*s %&'s %r", 
                t_mod_request, sizeof t_mod_request, 
                &*t_handlep, &dummy_cp) != 2)
        return error_reply(req, "Malformed command: %'s", command);
    if (strequal(t_mod_request, "ADD"))
        e_mod_request = ADD;
    else if (strequal(t_mod_request, "DELETE"))
        e_mod_request = DELETE;
    else if (strequal(t_mod_request, "REPLACE"))
        e_mod_request = REPLACE;
    else if (strequal(t_mod_request, "DELETE-ALL"))
        e_mod_request = DELETE_ALL;
    else
        return error_reply(req, "Malformed command: %'s", command);

    if (in_select(in, &t_magic_no))
        return error_reply(req, "EDIT-OBJECT-INFO: %'s", p_err_string);
    if(in_atrs(in, 0, &at))
        return error_reply(req, "EDIT-OBJECT-INFO: Could not read attributes \
from request packet:  %'s", p_err_string);
    if (!at)
        return error_reply(req, "No attributes provided to EDIT-OBJECT-INFO.");
    plog(L_DIR_UPDATE, req, "EOI %s %s", *t_handlep, t_mod_request, 0);
    
    fi = pfalloc();
    if (!fi) out_of_memory();
    /* Second, get the attribute list from the file. */
    rsinfo_ret = dsrfinfo(*t_handlep,t_magic_no,fi);
    if(rsinfo_ret == DSRFINFO_FORWARDED) {
        VLINK fl;               /* List of forwarding pointers */
        VLINK fp;               /* Current forwarding pointer */
        fl = fi->forward; fi->forward = NULL;
        fp = check_fwd(fl,*t_handlep, t_magic_no);
        
        /* Free what we don't need */
        pffree(fi);
        /* Reply with FORWARDED message. */
        forwarded(req, fl, fp, *t_handlep);
        return PSUCCESS;
    }
    
    if (rsinfo_ret > 0) {
        pffree(fi);
        /* We claim it was not found; the logfile tells the true tale. */
        creplyf(req, "FAILURE NOT-FOUND FILE %'s\n", *t_handlep);
        RETURNPFAILURE;
    }    

    /* Third: make the modifications. */
    switch(e_mod_request) {

    case ADD:
	/* If not authorized, say so */
	if(!srv_check_acl(fi->oacl,NULL,req,"i",SCA_OBJECT,*t_handlep,NULL)) {
	    creply(req,"FAILURE NOT-AUTHORIZED\n");
	    plog(L_AUTH_ERR,req,"Unauthorized EDIT-OBJECT-INFO ADD: %s",*t_handlep);
	    pffree(fi);
	    RETURNPFAILURE;
	}
        /* Always returns SUCCESS */
        /* XXX this isn't really an ADD operation, because we overwrite
           attributes that can only have single instances, without
           complaining.  However, it will suffice for this Alpha release,
           until a need for something better comes along. --swa */ 
        retval = pf_add_atrs(at, fi);
        break;
    case DELETE:
	/* If not authorized, say so */
	if(!srv_check_acl(fi->oacl,NULL,req,"z",SCA_OBJECT,*t_handlep,NULL)) {
	    creply(req,"FAILURE NOT-AUTHORIZED\n");
	    plog(L_AUTH_ERR,req,"Unauthorized EDIT-OBJECT-INFO DELETE: %s",*t_handlep);
	    pffree(fi);
	    RETURNPFAILURE;
	}
        /* does its own error reporting. */
        retval = pf_delete_attributes(req, at, fi);
        atlfree(at);
        break;
    case DELETE_ALL:
	/* If not authorized, say so */
	if(!srv_check_acl(fi->oacl,NULL,req,"z",SCA_OBJECT,*t_handlep,NULL)) {
	    creply(req,"FAILURE NOT-AUTHORIZED\n");
	    plog(L_AUTH_ERR,req,"Unauthorized EDIT-OBJECT-INFO DELETE ALL: %s",*t_handlep);
	    pffree(fi);
	    RETURNPFAILURE;
	}
        /* Does its own error reporting. */
        retval = pf_delete_all_attributes(req, at, fi, TRUE);
        atlfree(at);
        break;
    case REPLACE:
	/* If not authorized, say so */
	if(!srv_check_acl(fi->oacl,NULL,req,"u",SCA_OBJECT,*t_handlep,NULL)) {
	    creply(req,"FAILURE NOT-AUTHORIZED\n");
	    plog(L_AUTH_ERR,req,"Unauthorized EDIT-OBJECT-INFO DELETE: %s",*t_handlep);
	    pffree(fi);
	    RETURNPFAILURE;
	}
        /* delete and add do their own error reporting. */
        if(!(retval = pf_delete_all_attributes(req, at, fi, FALSE)))
            retval = pf_add_atrs(at, fi);
        else
            atlfree(at);
        break;
    default:
        internal_error("");
    }

    /* 
    /* Save the results, if it worked. */
    if (!retval)  {
        retval = dswfinfo(*t_handlep, fi);
        if(!retval)
            reply(req,"SUCCESS\n");
        else
            creplyf(req,"FAILURE SERVER-FAILED EDIT-OBJECT-INFO Couldn''''t \
write the changes to %'s\n", *t_handlep);
    }
    pffree(fi);
    return retval;

}


/* This does NOT free the attributes it deletes. */

static
int
pf_delete_attributes(req, ats, fi)
    RREQ req;
    PATTRIB ats;
    PFILE fi;
{
    static int pf_delete_attribute();

    while(ats) {
        if(pf_delete_attribute(ats, fi)) {
            creplyf(req, 
                    "FAILURE Could not DELETE attribute %'s; this instance \
not found\n", ats->aname);
            RETURNPFAILURE;
        } else {
            ats = ats->next;
        }
    }
    return PSUCCESS;
}


static int
pf_delete_attribute(at, fi) 
    PATTRIB at;
    PFILE fi;
{
    return delete_matching_at(at, &(fi->attributes), equal_attributes);
}


static
int
pf_delete_all_attributes(req, at, pf, complain)
    RREQ req;
    PATTRIB at;
    PFILE pf;
    int complain;               /* TRUE if we object to deleting indelible
                                   or non-present attributes.  FALSE if we
                                   don't mind.  XXX -- currently ignored. */ 
{
    static int pf_delete_all_attribute();
    PATTRIB ind;
    for (ind = at; ind; ind = ind->next) {
        while(pf_delete_all_attribute(ind, pf) == PSUCCESS)
            ;
    }
    return PSUCCESS;
    
}

/* If there's more than  one instance of the attribute, we delete only one.
   Must be called repeatedly in order to really delete all of them. */
   
static
int
pf_delete_all_attribute(at, fi) 
    PATTRIB at;
    PFILE fi;
{
    static int equal_attribute_names_and_nature(PATTRIB, PATTRIB);

    return delete_matching_at(at, &(fi->attributes), 
                              equal_attribute_names_and_nature);
}


static
int
equal_attribute_names_and_nature(at1, at2)
    PATTRIB at1, at2;
{
    return at1->nature == at2->nature && strequal(at1->aname,at2->aname);
}


/* Merge the attributres in AT with the PFILE fi.  We need this function
   because eventually we'll support modification of PFILE attributes that are
   not stored in the attributes member.   This destructively modifies the
   attributes being merged. */

static int
pf_add_atrs(PATTRIB at, PFILE fi)
{
    while(at) {
        PATTRIB next = at->next;
        APPEND_ITEM(at, fi->attributes);
        at = next;
    }
    return PSUCCESS;
}
