/* Copyright (c) 1993 by the University of Southern California.
 * For copying information, see the file <usc-copyr.h>
 */

#include <usc-copyr.h>

#include <pparse.h>

extern int
qscanf(INPUT in, const char fmt[], ...)
{
    int retval;
    va_list ap;
    va_start(ap, fmt);
    retval = vqscanf(in, fmt, ap);
    va_end(ap);
    return retval;
}
