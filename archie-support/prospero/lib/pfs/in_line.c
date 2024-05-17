/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h> 
 */

#include <usc-license.h>
#include <ctype.h>
#include <pfs.h>
#include <pparse.h>
#include <perrno.h>



/* in_line reads quoted Prospero lines from a number of sources, up to the next
   unquoted newline.   Upon return, *thislinep points to the start of a
   NUL-terminated string which contains a Prospero protocol command line.   
   *next_wordp points to the first word in that string after command. 

   in_line passes through the error codes returned by its subsidiary functions.
   The only error they return is PARSE_ERROR.  If pfs_debug is set, then
   explanatory detail describing the malformed line will be reported on 
   stderr. 

   *thislinep and *next_wordp point to a buffer private to in_line().  This
   buffer may be rewritten after each new line is read.
 */

int 
in_line(INPUT in, char **thislinep, char **next_wordp)
{
    int tmp;                    /* temp return value. */
    INPUT_ST eol_st;      /* temp. variable for end of line. */
    INPUT eol = &eol_st;      /* temp. variable for end of line. */
    int linebuflen;                 /* length to copy into linebuf  */
    int i;                      /* temporary index */
    /* A local static variable.  Points to the line in progress. */
    AUTOSTAT_CHARPP(linebufp);
    char *cp;            /* temp. index */

    if (!in_nextline(in)) internal_error("in_line() called with no data");
    /* Ok, set EOL to the end of this input line. */
    tmp = qscanf(in, "%~%r%'*(^\n\r)%r", in, eol);
    if (tmp < 2) {
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "Read Prospero message with an unterminated quote");
        return PARSE_ERROR;
    }
    /* Make sure that linebuf points to a string with enough room to hold the
       current line. */
    if (in->sourcetype == IO_STRING) {
        linebuflen = eol->s - in->s; /* string type doesn't use offset member.
                                        */
    } else {
        linebuflen = eol->offset - in->offset; /* size that strlen() would
                                                  return */ 
    }
    if (!*linebufp)
        assert(*linebufp = stalloc(linebuflen + 1));
    else if (p__bstsize(*linebufp) < linebuflen + 1) {
        stfree(*linebufp);
        assert(*linebufp = stalloc(linebuflen + 1));
    }
    /* Now copy the bytes from the input stream to the linebuf.  This preserves
       quoting, which is very important. */
    for(cp = *linebufp, i = 0; i < linebuflen; ++cp, ++i, in_incc(in))
        *cp = in_readc(in);
    *cp = '\0';
    assert((in->sourcetype != IO_STRING) ? in->offset == eol->offset : 1);
    /* I need to trim off trailing spaces here.  Or else there might be
       trouble.  Hmm. */
    while (cp > *linebufp && isspace(*--cp)) {
        *cp = '\0', --linebuflen;
    }
    p_bst_set_buffer_length_nullterm(*linebufp, linebuflen);
    /* push on the read pointer for in to the start of the next line. */
    tmp = qscanf(in, "%R", in);
    if (tmp < 1) {
#if 0                           /* gripe about unterminated lines */
        p_err_string = qsprintf_stcopyr(p_err_string, 
                 "Read Prospero message with a line that was not LF \
terminated.");
        return PARSE_ERROR;
#else
        while (!in_eof(in))     /* Gobble up anything else that remains.
                                   I don't expect there to be anything. */
            in_incc(in);
#endif
    }
    /* linebuf now points to a complete line of still quoted text. */
    *thislinep = *linebufp;
    if(p__qbstscanf(*thislinep, *thislinep, "%*'s%~%r", next_wordp) < 1
       || *thislinep == *next_wordp) {
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "Read Prospero message with an empty line.");
        return PARSE_ERROR;
    }
    return PSUCCESS;
}


void
rreqtoin(RREQ rreq, INPUT in)
{
    in->sourcetype = IO_RREQ;
    in->offset = 0;         /* on byte 0. */
    if(in->rreq = rreq) {       /* might be NULL.  */
        in->inpkt = rreq->rcvd;
        in->ptext_ioptr = rreq->rcvd->text;
        /* Do a loop because there might be a crazy client that sends some
           packets in a sequence with empty length fields.  Skip any of them we
           encounter; go to the next packet with some content. */
        while (in->ptext_ioptr >= in->inpkt->text + in->inpkt->length) {
            in->inpkt = in->inpkt->next;
            if (in->inpkt == NULL)
                break;
            in->ptext_ioptr = in->inpkt->text;
        }
    } else {
        in->inpkt = NULL;
        in->ptext_ioptr = NULL;
    }
#ifndef NDEBUG
    in->file = NULL;
    in->s = NULL;
#endif
    in->flags = JUST_INITIALIZED; /* is this needed? */
}

