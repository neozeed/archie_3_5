/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>
#include <pfs.h>
#include <pparse.h>

/* This function will be reset by the server to srv_qoprintf.c, which is found
   in libpsrv.  This hack lets us avoid linking the client with
   srv_qoprintf.c, and thereby cuts down on the client code size. */

/* This function, in its client and server incarnations, qprintfs to an OUTPUT
   structure. */

#ifdef __STDC__
int (*qoprintf)(OUTPUT out, const char format[], ...) = cl_qoprintf;
#else
int (*qoprintf)() = cl_qoprintf;
#endif

