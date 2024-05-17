/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>
#include <pfs.h>
#include <pparse.h>
#include <pprot.h>
#include <perrno.h>

/* Returns PSUCCESS or PFAILURE. 
   Stashes its results in *valuep, and in *argsp, if argsp is non-NULL. 
   Starts reading from next_word.
*/
#define RETURN(rv) { retval = rv ; goto cleanup ; }
/* This wasnt freeing clink in all cases */
int
in_link(INPUT in, char *command, char *next_word, 
           int nesting, VLINK *valuep, 
        TOKEN *argsp /* Only used by in_filter. */)
{
    VLINK		clink;
    int tmp;
    char  	t_linktype;
    char	t_type[MAX_DIR_LINESIZE];
    char 	t_name[MAX_DIR_LINESIZE];
    char 	t_htype[MAX_DIR_LINESIZE];
    char 	t_host[MAX_DIR_LINESIZE];
    char 	t_ntype[MAX_DIR_LINESIZE];
    char 	t_fname[MAX_DIR_LINESIZE];
    char        t_destexp[20];
    char        *p_args;
    PATTRIB     at;
    int moretext;               /* flag we use to check if there's more text on
                                   the line to examine.  */
    int retval = PSUCCESS;                 /* return value from functions */

    CHECK_MEM();
    clink = vlalloc();		/* free-d or returned in valuep */

    tmp = qsscanf(next_word, "%c %s %'s %s %'s %s %'s %d %r",
                  &t_linktype,t_type,t_name,t_htype,t_host,
                  t_ntype,t_fname,
                  &(clink->version), &next_word);

    /* Log and return a better message */
    if(tmp < 8) {
        p_err_string = qsprintf_stcopyr(p_err_string, "Too few arguments: %'s",
                 command, 0); 
        RETURN(perrno = PARSE_ERROR);
    }
    assert(tmp <= 9);
    moretext = (tmp == 9);
    if (t_linktype == 'U' || t_linktype == 'L' || t_linktype == 'I'
        || in->flags == SERVER_DATA_FILE
        && (t_linktype == 'n' || t_linktype == 'N'))
        clink->linktype = t_linktype;
    else {
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "Illegal link type %c specified: %'s", t_linktype, command);
        RETURN(perrno = PARSE_ERROR);
    }

    clink->target = stcopyr(t_type, clink->target);
    clink->name = stcopyr(t_name, clink->name);
    clink->hosttype = stcopyr(t_htype,clink->hosttype);
    clink->host = stcopyr(t_host,clink->host);
    clink->hsonametype = stcopyr(t_ntype,clink->hsonametype);
    clink->hsoname = stcopyr(t_fname,clink->hsoname);
    /* search for DEST_EXP, if set. */
    if (moretext) {
        tmp = qsscanf(next_word, "DEST-EXP %!!s %r", t_destexp, 
                      sizeof t_destexp, &next_word);
        if (tmp >= 1)
            clink->dest_exp = asntotime(t_destexp);
        moretext = (tmp == 2);
    }
    if (argsp) {                /* if args requested */
        /* if given */
        if (moretext) {
            tmp = qsscanf(next_word, "ARGS %r", &p_args);
            if (tmp == 1) {
                *argsp = qtokenize(p_args);
                moretext = 0;
            } else {
                p_err_string = qsprintf_stcopyr(p_err_string,
                         "Unknown tokens at end of LINK specification: %'s",
                         command);
                RETURN(perrno = PARSE_ERROR);
            }
        } else {
            /* no more text, but args requested. */
            *argsp = NULL;
        }
    }
    /* check for leftover text. */
    if (moretext) {
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "Unknown tokens at end of LINK specification: %'s",
                 command);
        RETURN( perrno = PARSE_ERROR);
    }
    /* Look for any and all following ID lines and merge them with the link. */
    retval = in_id(in, &clink->f_magic_no);
    
    /* look for ATTRIBUTE lines specifying link attributes. */
    /* errors reported in p_err_string by these subfunctions. */
    /* These subfunctions will free up memory they don't need. */
    if (!retval) retval = in_atrs(in, nesting, &at);
    if (!retval) retval = vl_add_atrs(at, clink);
    if (retval) {
        if(argsp) tklfree(*argsp);
        /* in_atrs and add_attributes free their allocated memory if they fail;
           we don't have to. */
        RETURN(retval;)          /* in_atrs && add_attributes set perrno, too.
                                   */ 
    }
    *valuep = clink;
    return PSUCCESS;

cleanup:
    vlfree(clink);
    return(retval);
}

