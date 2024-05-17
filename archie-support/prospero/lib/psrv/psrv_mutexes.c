/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <pserver.h>
#include <pfs.h>
#include <psrv.h>


void
psrv_init_mutexes(void)
{
    /* retrieve_fp() is not currently called by the server, so doesn't need
       mutexing. */
    p_th_mutex_init(p_th_mutexPSRV_CHECK_ACL_INET_DEF_LOCAL);
    p_th_mutex_init(p_th_mutexPSRV_CHECK_NFS_MYADDR);
    p_th_mutex_init(p_th_mutexPSRV_LOG);
}

#ifndef NDEBUG
void
psrv_diagnose_mutexes(void)
{
  printf("{psrv_diagnose_mutexes");
      DIAGMUTEX(PSRV_CHECK_ACL_INET_DEF_LOCAL,"PSRV_CHECK_ACL_INET_DEF_LOCAL");
      DIAGMUTEX(PSRV_CHECK_NFS_MYADDR,"PSRV_CHECK_NFS_MYADDR");
      DIAGMUTEX(PSRV_LOG,"PSRV_LOG");
  printf("}");
}
#endif


