/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>
#include <pfs.h>
#include <pparse.h>
#include <perrno.h>

/* Merge the attributes in AT with the vlink CLINK.  We need this function
   because not all attributes are stored in the pattrib member.
   This function will destroy or free all of the attributes being merged. */
/* RETURNS PSUCCESS or PFAILURE, if could not achieve its objectives. 
   This function is NOT atomic; it may leave CLINK in an inconsistent state.
   It will do the best it can.
 */
int
vl_add_atrs(PATTRIB at, VLINK clink)
{
    while (at) {
        int retval;
        PATTRIB next = at->next;

	EXTRACT_HEAD_ITEM(at);	/* Ensure list consistency*/
        if(retval = vl_add_atr(at, clink)) {
	    atfree(at);
            atlfree(next);
            return retval;
        }
        /* do NOT free AT; it is now probably on the lattrib member of CLINK,
           or else it's already free. */ 
        at = next;
    }
    return PSUCCESS;
}

/* This function does not destroy the attribute AT if it returns PFAILURE.
   That way, the caller still has AT around in order to play with it. 
   However, if it succeeds, it frees or moves AT so that it is should NOT be
   freed by the caller. */
int
vl_add_atr(PATTRIB at, VLINK clink)
{
    int retval = PSUCCESS;      /* Change it if failure. */
    switch(at->precedence) {
    case ATR_PREC_OBJECT:
    case ATR_PREC_LINK:
    case ATR_PREC_CACHED:
    case ATR_PREC_REPLACE:
    case ATR_PREC_ADD:
        break;
    default:
        internal_error("Unknown value for at->precedence");
    }
    /* Some link attributes are special.  We also use this to reject any
       unknown link field names. */
    if(at->nature == ATR_NATURE_FIELD && at->precedence == ATR_PREC_LINK) {
        if (strequal(at->aname, "NAME-COMPONENT")) {
            if (at->avtype != ATR_SEQUENCE || length(at->value.sequence) != 1)
                goto badvalue;
            clink->name = stcopyr(at->value.sequence->token, clink->name);
            atfree(at);
        } else if (strequal(at->aname, "LINK-TYPE")) {
            if (at->avtype != ATR_SEQUENCE || length(at->value.sequence) != 1
                || (!strequal(at->value.sequence->token, "L") 
                    && !strequal(at->value.sequence->token, "U") 
                    && !strequal(at->value.sequence->token, "I") 
                    && !strequal(at->value.sequence->token, "N")))
                goto badvalue;

            clink->linktype = at->value.sequence->token[0];
            atfree(at);
        } else if (strequal(at->aname, "TARGET")) {
            if (at->avtype != ATR_SEQUENCE || length(at->value.sequence) != 1)
                goto badvalue;
            clink->target = stcopyr(at->value.sequence->token, clink->target);
            atfree(at);
        } else if (strequal(at->aname, "FILTER")) {
            if (at->avtype != ATR_FILTER)
                goto badvalue;

            APPEND_ITEM(at->value.filter, clink->filters);
            at->avtype = ATR_UNKNOWN; /* So that atfree() won't destroy the
                                         data.  */
            atfree(at);
        } else if (strequal(at->aname, "HOST-TYPE")) {
            if (at->avtype != ATR_SEQUENCE || length(at->value.sequence) != 1)
                goto badvalue;
            clink->hosttype = 
                stcopyr(at->value.sequence->token, clink->hosttype);
            atfree(at);
        } else if (strequal(at->aname, "HOST")) {
            if (at->avtype != ATR_SEQUENCE || length(at->value.sequence) != 1)
                goto badvalue;
            clink->host = stcopyr(at->value.sequence->token, clink->host);
            atfree(at);
        } else if (strequal(at->aname, "HSONAME-TYPE")) {
            if (at->avtype != ATR_SEQUENCE || length(at->value.sequence) != 1)
                goto badvalue;
            clink->hsonametype = 
                stcopyr(at->value.sequence->token, clink->hsonametype);
            atfree(at);
        } else if (strequal(at->aname, "HSONAME")) {
            if (at->avtype != ATR_SEQUENCE || length(at->value.sequence) != 1)
                goto badvalue;
            clink->hsoname = 
                stcopyr(at->value.sequence->token, clink->hsoname);
            atfree(at);
        } else if (strequal(at->aname, "VERSION")) {
            if (at->avtype != ATR_SEQUENCE || length(at->value.sequence) != 1)
                goto badvalue;
            clink->version = atoi(at->value.sequence->token);
            atfree(at);
        } else if (strequal(at->aname, "ID")) {
            /* XXX Currently, only REMOTE (numeric) IDs supported.  This must
               change soon. 

               This routine is only called when the ID is already parsed.
               in_id() will silently ignore any ID types except for REMOTE,
               since we currently do not have data structures that are capable
               of storing them in easily-accessible ways.    Therefore, this
               test should always be passed, unless the user specifies strange
               ID types in the ATTRIBUTE line.  Hmm.  If that's the case, then
               I suppose rejecting is ok.
               */
            if (at->avtype == ATR_SEQUENCE
                && strequal(at->value.sequence->token, "REMOTE")
                && length(at->value.sequence) == 2) {
                clink->f_magic_no = atoi(at->value.sequence->next->token);
                APPEND_ITEM(at, clink->oid);
            } else {
                p_err_string = qsprintf_stcopyr(p_err_string,
                         "Unknown ID type: %'s", 
                         (at->value.sequence && at->value.sequence->token) ? 
                         at->value.sequence->token :
                         "zero-length ID types are not supported");
                retval = perrno = PARSE_ERROR;
            }
        } else if (strequal(at->aname, "DEST-EXP")) {
            if (at->avtype != ATR_SEQUENCE || length(at->value.sequence) != 1)
                goto badvalue;
            clink->dest_exp = asntotime(at->value.sequence->token);
            atfree(at);
        } else {
            APPEND_ITEM(at,clink->lattrib);
        }
        return retval;
    } 
    /* Must be a normal attribute or a field which is stored in the attribute
       list.   */
    /* order is possibly significant; must maintain it */
    
    APPEND_ITEM(at, clink->lattrib);
    return retval;
 badvalue:
    p_err_string = qsprintf_stcopyr(p_err_string,
            "Illegal value for %s attribute", at->aname);
    RETURNPFAILURE;
}


/* Syntactically same as atput in goph_gw_dsdb.c */
/* creates new attribute for "name" and adds list of tokens */
/* Note caller must pass last arg as  (char *)0   */
/* Note caller can pass arg (char *)1 to force next arg to be pointed to
   rather than copied */
void vl_atput(VLINK vl, char *name, ...)
{
    va_list ap;
    PATTRIB at;

    va_start(ap, name);
    at = vatbuild(name, ap);
    /* override defaults */
    if (strequal(vl->target, "EXTERNAL"))
        at->precedence = ATR_PREC_REPLACE; /* still not clear what to use */ 
    APPEND_ITEM(at, vl->lattrib);
    va_end(ap);
}

