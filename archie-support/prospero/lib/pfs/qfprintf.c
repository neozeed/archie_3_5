/* Copyright (c) 1992 by the University of Southern California. */
/* For copying and distribution information, see the file <usc-copyr.h> */
#include <usc-copyr.h>

#include <pfs.h>
#include <pprot.h>

/* See vqsprintf() for documentation.
   This is slightly inefficient, since it wastes a function call.  Oh well.
   Return PFAILURE or PSUCCESS.
   */
int
qfprintf(FILE *outf, const char fmt[], ...)
{
    va_list ap;
    int retval;

    va_start(ap,fmt);
    retval = vqfprintf(outf, fmt, ap);
    va_end(ap);
    return retval;
}
