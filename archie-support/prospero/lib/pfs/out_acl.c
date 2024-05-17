/*
 * Copyright (c) 1992 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>

#include <pfs.h>
#include <pparse.h>


static int out_ac(OUTPUT out, ACL ac);

int 
out_acl(OUTPUT out, ACL acl)
{
    int retval = PSUCCESS;
    /* This check helps us track down a possible bug involving occasional loops
       as a result of subtraction of ACL rights. */
    int num_times = 0;          /* # of times through loop */
    ACL sentinel_acl = NULL;           /* mark a sentinel after we've been
                                          through too many times */
    for (; acl; acl = acl->next) {
        if (acl == sentinel_acl) 
            internal_error("Found an ACL with a cycle in it");
        if(++num_times == 100) sentinel_acl = acl;
        if (!retval) retval = out_ac(out, acl);
    }
    return retval;
}


/* Spits out an ACL line describing the ACL entry "ac". */
static int 
out_ac(OUTPUT out, ACL ac)
{
    extern char *acltypes[];    /* defined in lib/pfs/acltypes.c */

    qoprintf(out, "ACL %'s %'s %'s", acltypes[ac->acetype], 
            (ac->atype ? ac->atype : ""),
            (ac->rights ? ac->rights : ""), 0);
    return out_sequence(out, ac->principals);
}

