/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>

#include <pfs.h>
#include <pparse.h>


int 
out_atrs(OUTPUT out, PATTRIB at, int nesting)
{
    int retval = PSUCCESS;
    for (; at; at = at->next)
        if (!retval) retval = out_atr(out, at, nesting);
    return retval;
}
