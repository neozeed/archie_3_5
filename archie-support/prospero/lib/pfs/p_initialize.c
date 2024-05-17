/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
#include <pfs_threads.h>
#include <pfs.h>
#include <perrno.h>
#include <pfs_threads.h>
#include <pcompat.h>           /* For PMAP_ATSIGN*/

static void
p_thread_initialize_master_and_daughters()
{
    pfs_enable = PMAP_ATSIGN;
    if (!p_err_string) p_err_string = stcopy("");
    if (!p_warn_string) p_warn_string = stcopy("");
    p_clear_errors();
}
void 
p_thread_initialize()
{
#ifdef PFS_THREADS
#ifndef MASTER_IS_ONLY_SUBTHREAD
/* This should probably be a general purpose initialization fnctn*/
    p_th_allocate_self_num();
#endif
#endif
    p_thread_initialize_master_and_daughters();
}

/* p_initialize(), written by swa@isi.edu, 6 Nov 1993 */
void 
p_initialize(char *software_id, int flags, struct p_initialize_st * arg_st)
{
    if (pfs_debug == 0) {       /* if specified in a command line flag or
                                   otherwise by the program itself, this
                                   overrides the environment variable.  */
        char *pfs_debug_string; /* temporary */
        if(pfs_debug_string = getenv("PFS_DEBUG"))
            sscanf(pfs_debug_string, "%d", &pfs_debug);
    }
    /* Carefull not to put anything in here specific to the server */
    /* No flags currently defined. */
    /* Currently no options to set in the p_initialize_st */
#ifdef PFS_THREADS
    p__th_set_self_master();    /* must be called first. */
#endif
    p__set_sw_id(software_id);
    /* Initialize error messages to null string, if they're currently set to
       NULL ptr.  This means that it will never be wrong to dereference
       p_err_string. */
    p_thread_initialize_master_and_daughters();
    ardp_init_mutexes();        /* need not be called if not running threaded.
                                   Won't hurt though. */ 
    p__init_mutexes();          /* ditto */
}

#ifndef NDEBUG
void
p_diagnose(void)
{
  ardp_diagnose_mutexes();
  p__diagnose_mutexes();
}
#endif /*NDEBUG*/
