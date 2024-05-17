/*
 * Copyright (c) 1993-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <pfs_threads.h>
#include <ardp.h>

#ifdef PFS_THREADS
p_th_mutex p_th_mutexARDP_ACCEPT; /* declaration */
p_th_mutex p_th_mutexPTEXT; /* declared in ardp_mutexes.c */
p_th_mutex p_th_mutexARDP_RQALLOC; /* declared in ardp_mutexes.c */
p_th_mutex p_th_mutexGETHOSTBYNAME; /* declared in pfs_mutexes.c */
p_th_mutex p_th_mutexARDP_SELFNUM; /* declared in pfs_mutexes.c */
p_th_mutex p_th_mutexDNSCACHE; /* declared in pfs_mutexes.c */
p_th_mutex p_th_mutexALLDNSCACHE; /* declared in pfs_mutexes.c */
p_th_mutex p_th_mutexFILES; /* declared on p__self_num.c */
p_th_mutex p_th_mutexFILELOCK; /* declared in flock.c */
#endif

void
ardp_init_mutexes(void)
{
#ifdef PFS_THREADS
    EXTERN_MUTEXED_INIT_MUTEX(ardp_doneQ);
    EXTERN_MUTEXED_INIT_MUTEX(ardp_runQ);

    p_th_mutex_init(p_th_mutexGETHOSTBYNAME);
    p_th_mutex_init(p_th_mutexARDP_ACCEPT);
    p_th_mutex_init(p_th_mutexPTEXT);
    p_th_mutex_init(p_th_mutexARDP_RQALLOC);
    p_th_mutex_init(p_th_mutexARDP_SELFNUM);
    p_th_mutex_init(p_th_mutexDNSCACHE);
    p_th_mutex_init(p_th_mutexALLDNSCACHE);
    p_th_mutex_init(p_th_mutexFILES);
    p_th_mutex_init(p_th_mutexFILELOCK);
#endif
}


#ifndef NDEBUG
void
ardp_diagnose_mutexes(void)
{
#ifdef PFS_THREADS
    DIAGMUTEX(ardp_doneQ,"ardp_doneQ");
    DIAGMUTEX(ardp_runQ,"ardp_runQ");
    DIAGMUTEX(GETHOSTBYNAME,"GETHOSTBYNAME");
    DIAGMUTEX(ARDP_ACCEPT,"ARDP_ACCEPT");
    DIAGMUTEX(PTEXT,"PTEXT");
    DIAGMUTEX(ARDP_RQALLOC,"ARDP_RQALLOC");
    DIAGMUTEX(ARDP_SELFNUM,"ARDP_SELFNUM");
    DIAGMUTEX(DNSCACHE,"DNSCACHE");
    DIAGMUTEX(ALLDNSCACHE,"ALLDNSCACHE");
    DIAGMUTEX(FILES,"FILES");
    DIAGMUTEX(FILELOCK,"FILELOCK");
#endif
}
#endif /*NDEBUG*/






















