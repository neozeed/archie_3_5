/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <pfs_threads.h>

#ifdef PFS_THREADS
extern p_th_mutex p_th_mutexREGEX;
#endif

/* This routine combines RE_COMP and RE_EXEC in an appropriate thread-safe
   interface.  It currently operates by mutexing the use of the regex package.
   It will later be revised to take advantage of thread-safe library calls,
   as they become available in standard system libraries.  If you need 
   to do multithreaded matches right now, then you could do so by using the
   routine in misc/regex.c.
   This can also be linked with the versions of re_comp() and re_exec() that
   your system library provides; Prospero will do this by default.
*/
/* 
 * Returns zero if successfully compiled and matched.
 * Returns 1 if not.
 * This is probably inefficient, since we always compile, even in the common
 * case where the pattern string has not changed.  Need to rethink this at
 * a later date.  (but not right now.)
 */
int
p__re_comp_exec(char *temp, char *s)
{
    int  retval;

    p_th_mutex_lock(p_th_mutexREGEX);
    if(re_comp(temp)) { 
        /* There  should be a way to pass this error information up to the
           caller,but we don't have one right now.  Need to look at this again
           later. */
        retval= 0;
    } else {
        /* -1 is internal error for re_exec().  1 is match.  0 is failure to
           match. */
        retval = (re_exec(s) > 0);
    }
    p_th_mutex_unlock(p_th_mutexREGEX);
    
    return(retval);
}
