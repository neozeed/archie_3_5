/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
#include "gopher.h"
#include <pfs.h>
#include <pfs_threads.h>

/* This file is used by lib/psrv.  So libpgoph_gw.a should follow psrv. */


#ifdef PFS_THREADS
p_th_mutex p_th_mutexGLINK; /* declaration */
#endif

void
gopher_gw_init_mutexes(void)
{
#ifdef PFS_THREADS
    p_th_mutex_init(p_th_mutexGLINK);
#endif
}

#ifndef NDEBUG
void
gopher_gw_diagnose_mutexes(void)
{
  printf("{gopher_gw_init_mutexes ");
#ifdef PFS_THREADS
    DIAGMUTEX(GLINK,"GLINK");
#endif
  printf("}");
}
#endif /*NDEBUG*/

