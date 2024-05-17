/* replyf.c */
/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>

#include <stdarg.h>

#include <ardp.h>
#include <pfs.h>
#include <plog.h>
#include <pprot.h>
#include <psrv.h>               /* includes prototypes of vreplyf(),
                                   vcreplyf(), replyf(), and creplyf() */


/*
 * Note: If you are looking for the definitions of reply and creply
 *       they are defined as macros that call ardp_reply with
 *       the ARDP_R_COMPLETE or ARDP_R_INCOMPLETE flags.  
 *       This file defines the formatted versions of these
 *       commands, replyf and creplyf.
 */

int
replyf(RREQ req, const char *format, ...)
{
    va_list ap;
    int retval;

    va_start(ap, format);
    retval = vreplyf(req, format, ap);
    va_end(ap);
    return retval;
}

int
vreplyf(RREQ req, const char *format, va_list ap)
{
    AUTOSTAT_CHARPP(bufp);
    
    *bufp = vqsprintf_stcopyr(*bufp, format, ap);
    /* Perform the reply and pass through any error codes */
    return(ardp_breply(req, ARDP_R_INCOMPLETE, *bufp, p_bstlen(*bufp)));
}


int
creplyf(RREQ req, const char *format, ...)
{
    va_list ap;
    int retval;

    va_start(ap, format);
    retval = vcreplyf(req, format, ap);
    va_end(ap);
    return retval;
}


int
vcreplyf(RREQ req, const char *format, va_list ap)
{
    AUTOSTAT_CHARPP(bufp);
    *bufp = vqsprintf_stcopyr(*bufp, format, ap);

    /* Perform the reply and pass through any error codes */
    return(ardp_breply(req, ARDP_R_COMPLETE, *bufp, p_bstlen(*bufp)));
}

