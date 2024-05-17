/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 *
 * $Id: stubs.c,v 2.0 1996/08/20 22:48:41 archrlse Exp $
 */

/* Miscellaneous function calls that are provided by some OS's. */
#ifdef SOLARIS
#include <unistd.h>
#include "error.h"

/* Available only with the optional C compiler option */
#ifdef __GNUC__


int getpagesize(void)
{
    int pagesize;
    
    if ((pagesize = sysconf(_SC_PAGESIZE)) < 0) {
        /* Duh... */
        error(A_SYSERR, "getpagesize", "Can't get pagesize from OS");
/* # warning exit() required here        */
    }
    return (pagesize);
}

#endif
#endif





