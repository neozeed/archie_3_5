/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>
 */

#include <ardp.h>
#include <psrv.h>
#include <plog.h>
#include <pprot.h>
#include <perrno.h>

/* This function will execute both a vplog() and an vsendmqf() in order to
   report on an erroneous condition.
   It makes error returns easier.  It returns PFAILURE, since it should only be
   used if an error has occurred. 
   It also automatically prefixes the word ERROR to the error reply packets.
   It appends the appropriate newlines, so you don't have to.
*/

int
error_reply(RREQ req, char *format, ...)
{
    va_list ap;
    char *bufp;
    
    va_start(ap, format);
    
    
    bufp = vplog(L_DIR_PERR, req, format, ap); /* return formatted string */

    reply(req, "ERROR ");
    reply(req, bufp);
    creply(req, "\n");
    va_end(ap);
    RETURNPFAILURE;
}

