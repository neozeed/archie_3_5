/*
 * Copyright (c) 1992, 1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>
/* Written by swa@ISI.EDU, October 2, 1992 */
/* Mutexed, swa@ISI.EDU, November 1993. */
/* Interface changed: swa@ISI.EDU, Feb. 16, 1994 */
/* got rid of appearance of number 22 anywhere here; converted back to
   qsprintf: swa, April 20, 1994. */ 

#include <time.h>
#include <pfs.h>
#include <pfs_threads.h>


/*
 * Writes timetoasn() stamp into target.
 */
char *
p_timetoasn_stcopyr(time_t ourtime, char *target)
{
    /* This mutexes GMTIME and sprintf.  These are the only 
       problematic cases. */
    p_th_mutex_lock(p_th_mutexPFS_TIMETOASN);
    {
        struct tm *mt = gmtime(&ourtime);
        target = qsprintf_stcopyr(target, "%04d%02d%02d%02d%02d%02dZ",
                                  mt->tm_year+1900,mt->tm_mon+1,mt->tm_mday,
                                  mt->tm_hour, mt->tm_min,mt->tm_sec);
    }
    p_th_mutex_unlock(p_th_mutexPFS_TIMETOASN);
    return target;
}    
    
