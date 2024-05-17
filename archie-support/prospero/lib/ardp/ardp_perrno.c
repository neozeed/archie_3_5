/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 *
 * Written  by swa 11/93     to handle error reporting consistently in libardp
 */

#include <usc-license.h>
#include <ardp.h>
#include <pfs_threads.h>

#ifdef NEVERDEFINED
int perrno = 0;                 /* Declare it. */
#endif

EXTERN_INT_DEF(perrno);

