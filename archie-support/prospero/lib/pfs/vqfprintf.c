/* Copyright (c) 1992 by the University of Southern California. */
/* For copying and distribution information, see the file <usc-copyr.h> */
#include <usc-copyr.h>

#include <pfs.h>
#include <pprot.h>
#include <perrno.h>

/* Returns PFAILURE or PSUCCESS. */
int
vqfprintf(FILE *outf, const char fmt[], va_list ap)
{
    int retval;
    
    AUTOSTAT_CHARPP(bufp);

    *bufp = vqsprintf_stcopyr(*bufp, fmt, ap);
    if (fputs((*bufp), outf) == EOF)
        RETURNPFAILURE;
    return PSUCCESS;
}
