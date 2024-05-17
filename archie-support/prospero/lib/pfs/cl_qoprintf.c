/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>
#include <ardp.h>
#include <pfs.h>
#include <pparse.h>

int 
cl_qoprintf(OUTPUT out, const char fmt[], ...)
{
    va_list ap;

    va_start(ap, fmt);
    assert(! out->req && out->request);
    
    return vp__add_req(out->request, fmt, ap);
}


int
requesttoout(RREQ req, OUTPUT out)
{
    out->request = req;
    out->req = NULL;
    out->f = NULL;
    return PSUCCESS;
}
