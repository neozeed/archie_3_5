/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>
#include <pfs.h>
#include <pparse.h>

#ifndef FILE
#include <stdio.h>
#endif

void
filetoin(FILE *f, INPUT in)
{
    in->sourcetype = IO_FILE;
    in->rreq = NULL;
    in->inpkt = NULL;
    in->ptext_ioptr = NULL;
    in->s = NULL;
    in->file = f;
    in->offset = ftell(f);
    in->flags = JUST_INITIALIZED | SERVER_DATA_FILE;
}
