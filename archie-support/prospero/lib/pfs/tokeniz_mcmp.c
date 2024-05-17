/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>
 *
 * Written  by swa April 1993  
 */

/* This function provides common code for handling the multiple components
 * specified after an UNRESOLVED protocol message and for handling the
 * multiple components specified after a LIST '' COMPONENTS protocol message.
 * It is passed a string and returns a freshly * allocated TOKEN list (which
 * must be freed).  Used in server/list.c and in lib/pfs/p_get_dir.c.
 */

#include <pfs.h>
#include <pprot.h>

/*
 * Stages:
 * a) With no special definions, they will transmit oldstyle, accept both
 * b) With NEW_MCOMP defined (after all servers are known to be new), clients
 *    will transmit new; servers still transmit old (since old clients harder
 *    to snag than old servers), accept both.
 * c) With REALLY_NEW_MCOMP defined (after *all* are known to be new), 
 *    clients and servers will transmit new, accept only new. 
 */
/* The above documentation is now incorrect.
 * See /nfs/pfs/dev/doc/compile-options for a better explanation.
 * The old path didn't work, because old clients ended up going away before old
 * servers did.
 */


static TOKEN p__tokenize_oldstyle_mcomp(char *nextname);

#ifndef DONT_ACCEPT_OLD_MCOMP
TOKEN
p__tokenize_midstyle_mcomp(char *nextname)
{
    TOKEN retval = qtokenize(nextname);
    CHECK_MEM();
    /* This code is used by both client and server.  */
    if (retval && !retval->next) {/* Must be old-format response from server or
                                     client  */
        tkfree(retval);
        return p__tokenize_oldstyle_mcomp(nextname);
    }
    return retval;
}


/* This is also used for V1 support. */
static
TOKEN
p__tokenize_oldstyle_mcomp(char *nextname)
{
    TOKEN retval;
    char *buf = NULL;        /* this is assigned to the token before exit. */ 
    int tmp;
    
    tmp = qsscanf(nextname, "%&'[^/ \t]/%r", &buf, &nextname);
    assert(tmp >= 0);
    if (tmp == 0)
        return NULL;
    retval = tkalloc(NULL);
    retval->token = buf; buf = NULL;
    if (tmp == 2)
        retval->next = p__tokenize_oldstyle_mcomp(nextname);
    return retval;
}
#endif
