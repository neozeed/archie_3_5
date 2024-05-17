/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>

#include <stdio.h>              /* for fputs() and EOF */
#include <ardp.h>
#include <pfs.h>
#include <pparse.h>
#include <psrv.h>

/* Returns PFAILURE or PSUCCESS. */
int 
srv_qoprintf(OUTPUT out, const char fmt[], ...)
{
    va_list ap;
    int retval;

    assert(out->req || out->f);
    va_start(ap, fmt);

    if (out->f) {
        retval = vqfprintf(out->f, fmt, ap);
    } else {
        retval = vreplyf(out->req, fmt, ap);
    }
    va_end(ap);
    return retval;
}

void
reqtoout(RREQ req, OUTPUT out)
{
    out->f = NULL;
    out->request = NULL;
    out->req = req;
}

void
filetoout(FILE *file, OUTPUT out)
{
    out->req = NULL;
    out->request = NULL;
    out->f = file;
}
