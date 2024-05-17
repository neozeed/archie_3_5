/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 *
 * $Id: stubs.c,v 2.0 1996/08/20 22:52:26 archrlse Exp $
 */

/* Miscellaneous function calls that are provided by some OS's. */
#ifdef SOLARIS
#include <unistd.h>

/* Available only with the optional C compiler option */
#ifdef __GNUC__

int getdtablesize (void)
{
    
    return(sysconf(_SC_OPEN_MAX));
    
}

#endif
#endif






    
