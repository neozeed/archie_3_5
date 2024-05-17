/* p__qbstscanf.c
   Author: Steven Augart (swa@isi.edu)
   Designed, Documented, and Written: 7/18/92 -- 7/27/92
   Ported from Gnu C to full ANSI C & traditional C, 10/5/92
   & modifier added, detraditionalized: 2/16/93.
   Made a wrapper around qscanf(), 3/2/93
   Turned into p__qbstscanf(), 19 Nov 1993
*/
/* This file is not a final interface; use it for internal purposes only. */
/* Copyright (c) 1992, 1993 by the University of Southern California. */
/* For copying and distribution information, see the file <usc-license.h> */

#include <usc-license.h>
#include <stdarg.h>             /* ANSI variable arguments facility. */
#include <pfs.h>
#include <pparse.h>

int
p__qbstscanf(const char *sthead, /* Start of the string stpos is in */
             const char *stpos, /*  position for the read ptr. */
             const char *format, /* What to scan for. */
             ...) /* remaining args: pointers to places to store
                                 the data we read, or integers (field widths).
                                 */ 
{
    va_list ap;                 /* for varargs */
    int     retval;
    INPUT_ST in_st;
    INPUT in = &in_st;

#ifdef P_ALLOCATOR_CONSISTENCY_CHECK
    assert(p__bst_consistency_fld(sthead) == P__INUSE_PATTERN);
#endif
	CHECK_PTRinBUFF(sthead,stpos);
    in->sourcetype = IO_BSTRING;
    in->rreq = NULL;
    in->s = stpos;
    in->file = NULL;
    in->bstring_length = p_bstlen(sthead);
    assert(sthead <= stpos);
    in->offset = stpos - sthead;
    assert(in->offset <= in->bstring_length); /* make sure consistent. */
    in->flags = PERCENT_R_TARGET_IS_STRING;

    va_start(ap, format);    

    retval = vqscanf(in, format, ap);
    va_end(ap);
    return retval;
}
