/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
#include <pfs.h>

char *
p__qbstprintf_stcopyr(char *buf, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    buf = p__vqbstprintf_stcopyr(buf, fmt, ap);
    va_end(ap);
    return buf;
}


char *
p__vqbstprintf_stcopyr(char *buf, const char *fmt, va_list ap)
{
    int tmp;
    assert(p__bst_consistent(buf));
 again:
    tmp = qsprintf(buf, p__bstsize(buf), fmt, ap);
    if (tmp > p__bstsize(buf)) {
        stfree(buf);
        buf = stalloc(tmp);
        goto again;
    }
    /* Tmp is now the size of the total output area, including a trailing null.
       Need to set the size to tmp -1, since trailing null is not included. */
    p_bst_set_buffer_length_nullterm(buf, tmp - 1);
    return buf;
}

