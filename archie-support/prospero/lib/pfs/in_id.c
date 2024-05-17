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
   pointer to some more generic ID storage type.  But for now, this is correct.
   We simply ignore ID types that we don't understand.  I believe this is
   correct. 
   */ 
int
in_id(INPUT in, long *magic_nop)
{
    char *command, *next_word;
    long atol();

    while (in_nextline(in) && strnequal(in_nextline(in), "ID", 2)) {
        char t_id_type[MAX_DIR_LINESIZE];
        int retval;                /* retval from subfunctions. */
        int tmp;                /* # of tokens matched by qsscanf() */
        TOKEN seq = NULL;

        if(retval = in_line(in, &command, &next_word)) {
            return retval;
        }
        tmp = qsscanf(next_word, "%!!s %r", t_id_type, sizeof t_id_type,
                      &next_word);
        if (tmp < 1) {
            p_err_string = qsprintf_stcopyr(p_err_string,
                     "Malformed ID line received: %s", command);
            return PARSE_ERROR;
        }
        if (strequal(t_id_type, "REMOTE")) {
            if(retval = in_sequence(in, command, next_word, &seq))
                return retval;
            if (!seq || seq->next) {
                p_err_string = qsprintf_stcopyr(p_err_string,
                         "Malformed REMOTE ID type received; must be a single \
integer: %s", command);
                return PARSE_ERROR;
            }
            /* XXX should use qsscanf() for the overflow checking. */
            *magic_nop = atol(seq->token);
        }
        /* Just ignore other ID types. */
        tklfree(seq);
    }
    return PSUCCESS;
}

