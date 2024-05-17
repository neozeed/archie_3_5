/* Author: bcn@isi.edu */
/* Updated & modified by swa@isi.edu */
/* Updated: swa@ISI.EDU, 5/15/94: to change ways attribute names are parsed and
   way dsrobject() is called. */
/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <netdb.h>
#include <sgtty.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>

#include <pmachine.h>
#include <ardp.h>
#include <pserver.h>
#include <pfs.h>
#include <pparse.h>
#include <psrv.h>
#include <plog.h>
#include <pprot.h>
#include <perrno.h>

#include "dirsrv.h"

#define REALLY_NEW_FIELD

#define RETURN(rv)    { retval = (rv); goto cleanup ; }
int
get_object_info(RREQ req, char *command, char *next_word, INPUT in)
{
    VLINK	check_fwd();
    int tmp;

    optdecl;
    AUTOSTAT_CHARPP(t_requestedp); /* attributes requested (unparsed form) */
    AUTOSTAT_CHARPP(t_ntypep); /* handle type */
    AUTOSTAT_CHARPP(t_fnamep); /* obj handle */
    long                t_version = 0; /* Object version */
    long	        t_num = 0;          /* Magic #. */
    VLINK		clink = NULL;	/* Must be freed */
    int		item_count = 0;    /* Count of returned items     */
    P_OBJECT		ob = NULL;	/* Must be freed on exit */
    int		rsinfo_ret;       /* Ret Val from dsrfinfo        */
    int                 all = 0; /* List all attributes? */
    int			fp_anyway = 0;
    OUTPUT_ST           out_st;
    OUTPUT              out = &out_st;
    int                 retval;	/* Value we are returning */
    struct dsrobject_list_options listopts_st;

    dsrobject_list_options_init(&listopts_st); /* Initialize this first; then
                                                  we can safely cleanup at the
                                                  end without worrying about
                                                  encountering a wild pointer.
                                                  */  
    set_client_addr(req->peer_addr.s_addr);
    reqtoout(req, out);
    /* still have to read the remainder of the attributes */
    tmp = qsscanf(next_word,"%'&s %'&s %'&s %d %r",
                 t_requestedp,t_ntypep,t_fnamep, &t_version, &next_word);
    /* Log and return a better message */
    if(tmp != 3 && tmp != 4) {
        creply(req,"ERROR wrong number of arguments\n");
        plog(L_DIR_PERR,req,"Wrong # of arguments: %s", command, 0);
        RETURN(PFAILURE);
    }
    if(in_select(in, &t_num))
        return error_reply(req, "GET-OBJECT-INFO %'s", p_err_string);
    /* Do we need a better log message */
    plog(L_DIR_REQUEST, req, "GOI %s %s", *t_requestedp,*t_fnamep,0);

    if(check_handle(*t_fnamep) == FALSE) {
        creply(req,"FAILURE NOT-AUTHORIZED\n");
        plog(L_AUTH_ERR, req,"Invalid object name: %s",*t_fnamep,0);
        RETURN(PFAILURE);
    }

    /* Parse *t_requestedp, the list of requested attributes. */

#ifdef DSROBJECT_LIST_OPTIONS_REQUESTED_ATTRS_BACK_COMPATIBILITY
    /* If any contents to t_requestedp, then also set requested_attrs.  This
       will be cleaned up at the end. */
    if (**t_requestedp)
        listopts_st.requested_attrs = 
            qsprintf_stcopyr(NULL, "+%s+", *t_requestedp); 
#endif /* DSROBJECT_LIST_OPTIONS_REQUESTED_ATTRS_BACK_COMPATIBILITY */

    p__parse_requested_attrs(*t_requestedp, &(listopts_st.req_obj_ats) );
    
    clink = vlalloc();
    ob = oballoc();

    clink->hsonametype = stcopyr(*t_ntypep,clink->hsonametype);
    clink->hsoname = stcopyr(*t_fnamep,clink->hsoname);
    clink->f_magic_no = t_num;
    clink->version = t_version;

    rsinfo_ret = dsrobject(req, clink->hsonametype, clink->hsoname, 
                           clink->version, clink->f_magic_no, 0, 
                           &listopts_st, ob);
#ifdef DIRECTORYCACHING
    if (rsinfo_ret) 
        dsrobject_fail++;
#endif

    /* If not authorized, say so.  If able to read the file, but not */
    /* attributes, then return ACCESS-METHOD and don't complain      */
    if((!srv_check_acl(ob->acl,NULL,req,"g",SCA_OBJECT,clink->hsoname,NULL)) &&
       (!srv_check_acl(ob->acl,NULL,req,"G",SCA_OBJECT,clink->hsoname,NULL))) {
	plog(L_AUTH_ERR,req,"Unauthorized GET-OBJECT-INFO: %s", clink->hsoname);
        creply(req,"FAILURE NOT-AUTHORIZED\n");
        RETURN(PFAILURE);
    }

#ifndef SERVER_DO_NOT_SUPPORT_FORWARDING
    /* Handle FORWARDING-POINTER as a special case. */
    if(was_attribute_requested("FORWARDING-POINTER", &listopts_st.req_obj_ats)) {
        if(rsinfo_ret == DSRFINFO_FORWARDED) {
            VLINK fl;           /* List of forwarding pointers. */
            if(fl = check_fwd(ob->forward,clink->hsoname,
                              clink->f_magic_no)) {
                replyf(req,"ATTRIBUTE OBJECT INTRINSIC FORWARDING-POINTER LINK\
 L FP %'s %s %'s %s %'s %d %d\n",
                       clink->name,
                       fl->hosttype, fl->host,
                       fl->hsonametype, fl->hsoname,
                       fl->version,fl->f_magic_no,0);
                item_count++;
            }
        } else if((clink->f_magic_no == 0) && (ob->magic_no != 0)) {
            replyf(req,"ATTRIBUTE OBJECT INTRINSIC FORWARDING-POINTER LINK L \
FP %'s %s %s %s %'s %d %d\n",
                   clink->name,
                   clink->hosttype, hostwport,
                   clink->hsonametype, clink->hsoname,
                   clink->version,ob->magic_no,0);
            item_count++;
        }
    } else if(rsinfo_ret == DSRFINFO_FORWARDED) {
        VLINK fl;               /* List of forwarding pointers */
        VLINK fp;               /* Current forwarding pointer */
        fl = ob->forward; ob->forward = NULL;
        fp = check_fwd(fl,clink->hsoname,clink->f_magic_no);

        /* Return forwarded message. */
        forwarded(req, fl, fp, clink->hsoname);
        RETURN(PSUCCESS);
    }
#endif /* SERVER_DO_NOT_SUPPORT_FORWARDING */

    /* Here we must check the file info, look for matching */
    /* attributes and return them if authorized.            */
    /* Returning the ACCESS-METHOD attribute has special authorizations
       attached to it.  */ 
    if(rsinfo_ret <= 0) {
        /* Check whether to return just the ACCESS-METHOD attribute, or whether
           to return all of them. */
	int return_non_amat = srv_check_acl(ob->acl,NULL,req,"g",SCA_OBJECT,
                                            clink->hsoname,NULL);
        PATTRIB ca;
        /* Change: 5/16/94: ob->attributes is now only set for attributes that
           were specifically requested. */
        for (ca = ob->attributes; ca; ca = ca->next) {
            /* Check permissions */
            if (return_non_amat || strequal(ca->aname, "ACCESS-METHOD")) {
                out_atr(out, ca, 0);
                item_count++;
            }
        }
    } else if (rsinfo_ret == DIRSRV_NOT_FOUND) {
        creply(req,"FAILURE NOT-FOUND\n");
        RETURN(PFAILURE);
    } else {
        creplyf(req,"FAILURE SERVER-FAILED %s%s%s\n", p_err_text[rsinfo_ret],
               *p_err_string? " " : "", p_err_string);
        RETURN( PFAILURE);
    }
    /* If none match, say so */
    if(!item_count)
        reply(req,"NONE-FOUND\n");
    
    RETURN(PSUCCESS);


 cleanup:                       /* used by RETURN() */

#ifdef DSROBJECT_LIST_OPTIONS_REQUESTED_ATTRS_BACK_COMPATIBILITY
    /* This, if set, is allocated by stcopyr(), so should be freed. */
    stfree((char *) listopts_st.requested_attrs); 
    listopts_st.requested_attrs = NULL;
#endif /* DSROBJECT_LIST_OPTIONS_REQUESTED_ATTRS_BACK_COMPATIBILITY */

    /* Free up the objects. */
    tklfree(listopts_st.req_obj_ats.specific);

    if(ob)
       obfree(ob);
    if(clink)
       vlfree(clink);
    return(retval);
}


/* I add this function here since it is used by get_obj_info().  
   Also used by list(), in server/list.c.  Perhaps I should make it a library
   function.  --swa, 5/15/94 */
/* UNPARSED is an unparsed +-separated list of the requested attributes. The
   list is not expected to be preceded and followed by a + sign.  If it is,
   then we will get attributes with empty names, which will not do anything
   useful. (Prospero does not normally use the empty-named attribute, although
   it also does not discriminate against it.)   The attribute names will not
   contain + signs. */
/* PARSED is that list in requested_attributes form. */
/* We assume that PARSED is initialized to NULL values.  You should call
   init_requested_attributes() on PARSED before invoking
   p__parse_requested_attrs(). */ 
/* This way of specifying attributes will go away in a later revision of the
   protocol.  */
void 
p__parse_requested_attrs(const char *unparsed, 
                         struct requested_attributes *parsed)
{
    
    for (;;) {
        int tmp;                /* tmp. return value from qsscanf().   Doesn't
                                   need to be initialized here; set below. */
        char *thisatstr = NULL; /* attribute name being parsed. */
        TOKEN thisattoken ;     /* processed form of above.   Doesn't need to
                                   be initialized; set below. */ 

#if 1
        /* This will not allow the empty string as an attribute name; by way of
           compensation, it will consider it unset.
           */
        /* This version strips off any leading pluses (suppresses them).  then
           it pulls in a sequence of one or more non-plus characters as an
           attribute name.  Then it saves the rest of the string for another
           feeding.  */
        tmp = qsscanf(unparsed, "%*(+)%&[^+]%r", &thisatstr, &unparsed);
#else
        tmp = qsscanf(unparsed, "%&(^+)+%r", &thisatstr, &unparsed);
#endif

        if (tmp < 1)
            break;             /* done */
        assert(tmp == 2);       /* assumption of this code. */

        /* Now do a dispatch based upon the value of thisatstr.  If this is a
           symbolic attribute name (#ALL or #INTERESTING; others will be
           recognized later), set the matching
           flag.  The only symbolic names currently used by the clients we
           provide are #ALL and "ALL" (typo in pre-5/15/94 versions of ALS.C,
           wasn't recognized by older servers, so no reason to support it now.)
           --swa  */
        /* We deliberately insert 'thisatstr' into the list, so that
           '#INTERESTING' will always match a literal '#INTERESTING', etc. */
        if (strequal(thisatstr, "#ALL")) {
            parsed->all = 1;
        } else if (strequal(thisatstr, "#INTERESTING")) {
            parsed->interesting = 1;
        } else if (strequal(thisatstr, "#OBJECT") 
                   || strequal(thisatstr, "#FIELD") 
                   || strequal(thisatstr, "#APPLICATION")) {
            /* This test catches other possible symbolic attribute names that
               are specified in the protocol but not currently used.  It used
               to be performed in get_object_info(). */
            parsed->all = 1;
        }
        /* Done with dispatch. */

        /* This fragment of 4 lines is used in at least 2 places in Prospero;
           probably more. */
        thisattoken = tkalloc(NULL);
        thisattoken->token = thisatstr;
        thisatstr = NULL;       /* flush it. */
        APPEND_ITEM(thisattoken, parsed->specific);
        /* Added this item to the list. */
    }
    /* Done; PARSED has now been set appropriately.  This function returns no
       status, since it will always succeed. */
}


/* Dispatch table used below: */
const static struct {
    char  *name;
    int interesting:1;          /* matches #INTERESTING.  Note that until
                                   #INTERESTING is better defined, interesting
                                   is implemented as equivalent to #ALL in the
                                   code in was_attribute_requested(), below. */
    int minus:1;                /* not returned to an #ALL */
    int plus:1;                 /* always returned. */
} attribute_class_table[] = {
    /* name                         INT -   + */
    {"CONTENTS",                    0,  1,  0},
    /* FORWARDING-POINTER is a special case; if it wasn't explicitly requested
       but it is nevertheless present, we will return a FORWARDED message
       instead of returning any attributes whatsoever. */
    /* This is a change: should #ALL and #INTERESTING also match
       FORWARDING-POINTER?  They currently do*/
    /* {"FORWARDING-POINTER",          0,  1,  0}, */
    {"OBJECT-INTERPRETATION",       1,  0,  0},
    {"QUERY-METHOD",                1,  0,  0},
    /* Currently no other magic attributes.  But there will be soon. */
};


/* Question: should we go to the trouble of getting the attribute named ATNAME?
   ATNAME is the attribute we're wondering whether to return.

   This tests PARSED.  We use an internal dispatch table (Currently defined
   right here, will later be more configurable) with the following rules:
   INTERESTING only matches attributes with 'interesting' set.
   MINUS, if set, means that the attribute is not returned to a "#ALL".  PLUS
   means the attribute is always returned. 

   Returns: non-zero if should return, zero if shouldn't */

int
was_attribute_requested(char *atname, struct requested_attributes *parsed)
{
    int i;
    int matches_all = 1;
    int matches_interesting = 1;
    int always_matches = 0;
    for (i = 0; i < sizeof attribute_class_table / sizeof attribute_class_table[0];
         ++i) {
        if(strequal(atname, attribute_class_table[i].name)) {
            /* INTERESTING same as ALL for now. */
            matches_interesting     = !attribute_class_table[i].minus;
            matches_all             = !attribute_class_table[i].minus;
            always_matches          = attribute_class_table[i].plus;
            break;              /* don't need to look at the rest of the table.
                                 */ 
        }
    }
    /* Done scanning the attribute_class_table.  We might be able to decide
       just based on that information. */
    if (always_matches)
        return 1;
    /* Now look at PARSED */
    /* First, the obvious classes */
    if (matches_interesting && parsed->interesting)
        return 1;
    if (matches_all && parsed->all)
        return 1;
    /* Now check if this attribute was explicitly requested. */
    if (member(atname, parsed->specific))
        return 1;
    /* No match based on this data.  We might still match in special-case code
       in our caller.  For instance, LIST ... COMPONENTS always returns the
       ACCESS-METHOD attribute for EXTERNAL links. */
    return 0;
}
