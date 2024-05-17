/* qsscanf.c
   Author: Steven Augart (swa@isi.edu)
   Designed, Documented, and Written: 7/18/92 -- 7/27/92
   Ported from Gnu C to full ANSI C & traditional C, 10/5/92
   & modifier added, detraditionalized: 2/16/93.
   Made a wrapper around qscanf(), 3/2/93
*/
/* Copyright (c) 1992, 1993 by the University of Southern California. */
/* For copying and distribution information, see the file <usc-copyr.h> */

#include <usc-copyr.h>
#include <stdarg.h>             /* ANSI variable arguments facility. */
#include <pfs.h>
#include <pparse.h>

int
qsscanf(const char *s, const char *fmt, ...)
/* s: source string
   fmt: format describing what to scan for.
   remaining args: pointers to places to store the data we read, or 
      integers (field widths).
*/
{
    va_list ap;                 /* for varargs */
    int     retval;
    INPUT_ST in_st;
    INPUT in = &in_st;

    /* Otherwise vqscanf will fail an assertion*/
    if (!s || s[0] == '\0') return 0;

    in->sourcetype = IO_STRING;
    in->rreq = NULL;
    in->s = s;
    in->file = NULL;
    in->flags = PERCENT_R_TARGET_IS_STRING;

    va_start(ap, fmt);    

    retval = vqscanf(in, fmt, ap);
    va_end(ap);
    return retval;
}
