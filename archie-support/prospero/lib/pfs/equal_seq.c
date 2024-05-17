/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>
/* Written by swa@ISI.EDU, Sept. 24, 1992 */

/* This is used by equal_attributes(), and probably not by much else. */
/* Compare two token lists.  Return nonzero value if they match; zero if 
   they don't.  */

#include <pfs.h>

int
equal_sequences(t1, t2)
    TOKEN t1, t2;
{
    for (;;t1 = t1->next, t2 = t2->next) {
        if (t1 == t2)               /* Handles the case where both are NULL */
            return TRUE;
        if (!t1 || !t2)         /* handles the case where one is null and the
                                   other is not. */
            return FALSE;
        if (!strequal(t1->token, t2->token))
            return FALSE;
    }
    /* NOTREACHED */
}
