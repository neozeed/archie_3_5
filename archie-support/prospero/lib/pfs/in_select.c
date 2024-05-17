/* author: swa@isi.edu */
/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>
#include <pfs.h>
#include <perrno.h>
#include <pparse.h>
#include <pprot.h>

/* The interface to this function will change.  The long * must become a
   pointer to a list of attributes (PATTRIB *).  But for now, this is correct,
   since dsrfinfo() and the name searching code don't pay attention to such
   information. */
/*   We simply ignore ID types that we don't understand.  I believe this is
     correct. */ 
int
in_select(INPUT in, long *magic_nop)
{
    char *command, *next_word;
    long atol();

    while (in_nextline(in) && strnequal(in_nextline(in), "SELECT", 6)) {
        char t_id_type[MAX_DIR_LINESIZE];
        int retval;                /* retval from subfunctions. */
        PATTRIB at = atalloc();

        if(retval = in_line(in, &command, &next_word)) {
            atfree(at);
            return retval;
        }
	assert(next_word >= command);
        if (retval = in_atr_data(in, command, next_word, 0, at)) {
            atfree(at);
            return retval;
        }
        if (at->avtype == ATR_SEQUENCE && at->nature == ATR_NATURE_FIELD
            && strequal(at->aname, "ID") && length(at->value.sequence) == 2
            && strequal(at->value.sequence->token, "REMOTE")) {
            /* XXX should use qsscanf() for the overflow checking. */
            *magic_nop = atol(at->value.sequence->next->token);
        }
        /* Just ignore other attribute and ID types. */
        atfree(at);
    }
    return PSUCCESS;
}

