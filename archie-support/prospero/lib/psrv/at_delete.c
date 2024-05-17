/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>
 */

#include <usc-copyr.h>
#include <pfs.h>                /* def of PATTRIB */
#include <psrv.h>               /* prototype for delete_matching_at() */
#include <list_macros.h>        /* EXTRACT_ITEM macro */
#include <perrno.h>

/* Looks for an attribute equal to the key according to the equal function.
   Deletes & frees it. 
   Returns: PSUCCESS or PFAILURE 
   This is currently only used in ed_link_info.c and ed_obj_info.c
*/
int
delete_matching_at(PATTRIB key, PATTRIB *headp, int (*equal)(PATTRIB, PATTRIB))
{
    /* ick - this was "index" which is defined as a macro */
    PATTRIB ind;
    /* Find the match. */
    for (ind = *headp; ind; ind = ind->next) {
        if ((*equal)(key, ind)) {
            EXTRACT_ITEM(ind, (*headp));
            atfree(ind);
            return PSUCCESS;
        }
    }
    RETURNPFAILURE;
}
                   
int
delete_matching_fl(FILTER key, FILTER *headp)
{
    FILTER ind;
    /* Find the match. */
    for (ind = *headp; ind; ind = ind->next) {
        if (equal_filters(key, ind)) {
            EXTRACT_ITEM(ind, (*headp));
            flfree(ind);
            return PSUCCESS;
        }
    }
    RETURNPFAILURE;
}
