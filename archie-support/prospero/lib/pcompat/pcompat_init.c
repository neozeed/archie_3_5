/*
 * Copyright (c) 1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <pcompat.h>            /* prototype for p__compat_initialize() */
#include <pfs.h>                /* prototype for p_initialize() */

/* Author: Steven Seger Augart, Mar. 30, 1994 */
/* This file contains automatic initialization routines for the PCOMPAT
   library.  No Prospero functions should be called before p_initialize() has
   been called.  This makes the pcompat library self-initializing. */

/* This function can be safely called more than once. */

static int initialized = 0;

void
p__compat_initialize(void)
{
    if (!initialized) {
        ++initialized;
        p_initialize("uclP", 0, (struct p_initialize_st *) NULL);
    }
}
