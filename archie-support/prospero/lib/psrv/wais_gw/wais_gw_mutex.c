/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
#include <pfs_threads.h>

#include "ietftype_parse.h"

#ifdef PFS_THREADS
p_th_mutex p_th_mutexWAISMSGBUFF; /* declaration */
p_th_mutex p_th_mutexIETFTYPE; /* declaration */
p_th_mutex p_th_mutexWAISSOURCE; /* declaration */
#endif

void
wais_gw_init_mutexes(void)
{
#ifdef PFS_THREADS
    p_th_mutex_init(p_th_mutexWAISMSGBUFF);
    p_th_mutex_init(p_th_mutexIETFTYPE);
    p_th_mutex_init(p_th_mutexWAISSOURCE);
    ietftype_init();	/* Safer than mutexing it */
#endif
}

#ifndef NDEBUG
void
wais_gw_diagnose_mutexes(void)
{
#ifdef PFS_THREADS
    DIAGMUTEX(WAISMSGBUFF,"WAISMSGBUFF");
    DIAGMUTEX(IETFTYPE,"IETFTYPE");
    DIAGMUTEX(WAISSOURCE,"WAISSOURCE");
#endif
}

#endif /*NDEBUG*/
