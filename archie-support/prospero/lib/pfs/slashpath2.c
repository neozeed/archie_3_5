/*
 * Copyright (c) 1993 by the University of Southern California
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 */

#include <usc-copyr.h> 

#include <pfs.h>

/* Used by ARCHIE database code to convert slashpath to token lists.
   We can use the same regular p__slashpath2tkl to convert back. */
/* Jan 10, 1994: I believe this code is now vestigial.  If it isn't, it must
   be rewritten. */

char *
p__tkl_2slashpath(TOKEN nextcomp_tkl)
{
    char buf[MAX_VPATH];

    assert("p__tkl_2slashpath() must be rewritten.");
    *buf = '\0';
    for(; nextcomp_tkl; nextcomp_tkl = nextcomp_tkl->next) {
        strcat(buf, nextcomp_tkl->token);
        strcat(buf, "/");
    }
    return buf;
}
