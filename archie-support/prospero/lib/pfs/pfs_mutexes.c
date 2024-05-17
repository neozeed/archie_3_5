/*
 * Copyright (c) 1993-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <pfs_threads.h>
#include <pfs.h>

#ifdef PFS_THREADS
p_th_mutex        p_th_mutexATALLOC;
p_th_mutex        p_th_mutexACALLOC;
p_th_mutex        p_th_mutexFLALLOC;
p_th_mutex        p_th_mutexOBALLOC;
p_th_mutex        p_th_mutexPAALLOC;
p_th_mutex        p_th_mutexPFALLOC;
p_th_mutex        p_th_mutexPFS_VQSCANF_NW_CS;
p_th_mutex        p_th_mutexPFS_VQSPRINTF_NQ_CS;
p_th_mutex        p_th_mutexPFS_TIMETOASN;
p_th_mutex        p_th_mutexTOKEN;
p_th_mutex        p_th_mutexVLINK;
p_th_mutex        p_th_mutexREGEX;
#endif

void
p__init_mutexes(void)
{
#ifdef PFS_THREADS
    myaddress();                /* Calling myaddress() will initialize this
                                   function, which is good, since it's not
                                   inherently multithreaded when called for the
                                   1st time.  */
    p_th_mutex_init(p_th_mutexATALLOC);
    p_th_mutex_init(p_th_mutexACALLOC);
    p_th_mutex_init(p_th_mutexFLALLOC);
    p_th_mutex_init(p_th_mutexOBALLOC);
    p_th_mutex_init(p_th_mutexPAALLOC); 
    p_th_mutex_init(p_th_mutexPFALLOC); 
    p_th_mutex_init(p_th_mutexPFS_TIMETOASN);  /* does calls to gmtimes */
    p_th_mutex_init(p_th_mutexPFS_VQSCANF_NW_CS);
    p_th_mutex_init(p_th_mutexPFS_VQSPRINTF_NQ_CS);
    p_th_mutex_init(p_th_mutexTOKEN);
    p_th_mutex_init(p_th_mutexVLINK);
    p_th_mutex_init(p_th_mutexREGEX);

#endif
}

#ifndef NDEBUG
void
p__diagnose_mutexes(void)
{
#ifdef PFS_THREADS
    DIAGMUTEX(ATALLOC,"ATALLOC");
    DIAGMUTEX(ACALLOC,"ACALLOC");
    DIAGMUTEX(FLALLOC,"FLALLOC");
    DIAGMUTEX(OBALLOC,"OBALLOC");
    DIAGMUTEX(PAALLOC,"PAALLOC"); 
    DIAGMUTEX(PFALLOC,"PFALLOC"); 
    DIAGMUTEX(PFS_TIMETOASN,"PFS_TIMETOASN");  /* does calls to gmtimes */
    DIAGMUTEX(PFS_VQSCANF_NW_CS,"PFS_VQSCANF_NW_CS");
    DIAGMUTEX(PFS_VQSPRINTF_NQ_CS,"PFS_VQSPRINTF_NQ_CS");
    DIAGMUTEX(TOKEN,"TOKEN");
    DIAGMUTEX(VLINK,"VLINK");
    DIAGMUTEX(REGEX,"REGEX");
#endif
}
#endif /*NDEBUG*/
