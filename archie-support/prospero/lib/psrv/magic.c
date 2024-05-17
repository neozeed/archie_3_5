/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <pfs.h>                /* assert, internal_error, prototypes */

/* Generate a unique magic number (positive long).  This is based upon hashing
   the vlink VL's HSONAME field. */
long
generate_magic(VLINK vl)
{
    long retval = 0L;
    int n = p_bstlen(vl->hsoname);
    int rvoffset = 0;           /* offset for xoring with the return value. */
    while(n--) {
        rvoffset += 8;
        if (rvoffset >= 8 * sizeof retval) rvoffset = 0;
        retval ^= ((vl->hsoname[n]) << rvoffset);
    }
    if (retval < 0) return ~retval;
    if (retval == 0) return 1;
    return retval;
}

/* Is the magic number MAGIC anywhere in use in the list of vlinks LINKS? */
int
magic_no_in_list(long magic, VLINK links)
{
    for ( ;links ; links = links->next) {
        if (links->f_magic_no == magic) return TRUE;
    }
    return FALSE;
}

