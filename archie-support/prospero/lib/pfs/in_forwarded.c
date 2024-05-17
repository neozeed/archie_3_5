/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>
#include <pfs.h>
#include <pparse.h>
#include <pprot.h>
#include <perrno.h>

/* Assumes next_word points to the word following a FORWARDED response. */
int
in_forwarded_data(INPUT in, char *command, char *next_word, VLINK dlink)
{
    int tmp;
    int moretext;               /* flag we use to check if there's more text on
                                   the line to examine.  */
    int retval = PSUCCESS;                 /* return value from functions */

    tmp = qsscanf(next_word,"%~%'&s %'&s %'&s %'&s %ld %r", 
                  &dlink->hosttype, &dlink->host, 
                  &dlink->hsonametype, &dlink->hsoname,
                  &dlink->version, &next_word);
    /* Log and return a better message */
    if(tmp < 5) {
        p_err_string = qsprintf_stcopyr(p_err_string, "Too few arguments: %'s",
                 command, 0); 
        return perrno = PARSE_ERROR;
    }
    moretext = (tmp == 6);
    /* search for DEST_EXP, if set. */
    if (moretext) {
        AUTOSTAT_CHARPP(t_destexpp);
        tmp = qsscanf(next_word, "DEST-EXP %'&s %r", t_destexpp, &next_word);
        if (tmp >= 1)
            dlink->dest_exp = asntotime(*t_destexpp);
        moretext = (tmp == 2);
    }
    if (moretext) {
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "Unknown tokens at end of FORWARDED specification: %'s",
                 command);
        return perrno = PARSE_ERROR;
    }
    /* Look for any and all following ID lines and merge them with the link. */
    retval = in_id(in, &dlink->f_magic_no);
    if (retval) return perrno = retval;
    return PSUCCESS;
}

