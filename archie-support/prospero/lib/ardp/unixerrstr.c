/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>


#include <pmachine.h>
#if defined(HAVESTRERROR)
#include <errno.h>
#include <string.h>			/* For stringerr*/
#else
#include <errno.h>			/* For sys_nerr  and sys_errlist*/
#ifndef SOLARIS
/* definitely needed under SunOS; probably under almost everything right 
   now.   Solaris's <errno.h> declares these; others don't. */
extern int sys_nerr;
extern char *sys_errlist[];
#endif                          /* ndef SOLARIS */
#undef HAVESTRERROR
#endif



const char *
unixerrstr(void)
{
#ifdef HAVESTRERROR
    /* sys_errlist is not in SOLARIS, replace with strerror() 
       which may not be thread safe in some applications
       there doesnt appear to be a posix compliant equivalent */
    return strerror(errno);
#else
    /* Would be nice to have the message include the error #. */
    return errno < sys_nerr ? sys_errlist[errno] : "Unprintable Error";
#endif
}

