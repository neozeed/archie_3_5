/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 */

#include <usc-copyr.h>
#include <pfs.h>

char *
qsprintf_stcopyr(char *buf, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    buf = vqsprintf_stcopyr(buf, fmt, ap);
    va_end(ap);
    return buf;
}


char *
vqsprintf_stcopyr(char *buf, const char *fmt, va_list ap)
{
    int tmp;
 again:
    tmp = vqsprintf(buf, p__bstsize(buf), fmt, ap);
    if (tmp > p__bstsize(buf)) {
        stfree(buf);
        buf = stalloc(tmp);
        goto again;
    }
    /* The count returned by vqsprintf includes a trailing null. */
    /* Mark this so that vqsprintf_stcopyr() returns a properly sized bstring.
       */ 
    p_bst_set_buffer_length_nullterm(buf, tmp - 1);
    return buf;
}

