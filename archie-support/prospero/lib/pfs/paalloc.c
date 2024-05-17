/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */
/* Munged by swa to handle multiple principals. */

#include <uw-copyright.h>
#include <stdio.h>
#include <stdlib.h>           /* For malloc and free */

#include <pfs.h>

static PAUTH	pauthflist = NULL;
int		pauth_count = 0;
int		pauth_max = 0;

/*
 * paalloc - allocate and initialize pacess control list structure
 *
 *    PAALLOC returns a pointer to an initialized structure of type
 *    PAUTH.  If it is unable to allocate such a structure, it
 *    signals out_of_memory();
 */
/* We dynamically allocate these because the server end may have several of
   them hanging out. */
PAUTH
paalloc()
{
    PAUTH	pauth_ent;
    p_th_mutex_lock(p_th_mutexPAALLOC);
    if(pauthflist) {
        pauth_ent = pauthflist;
        pauthflist = pauthflist->next;
    }
    else {
        pauth_ent = (PAUTH) malloc(sizeof(PAUTH_ST));
        if (!pauth_ent) out_of_memory();
        pauth_max++;
    }

    pauth_count++;
    p_th_mutex_unlock(p_th_mutexPAALLOC);

    /* Initialize and fill in default values */
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    pauth_ent->consistency = INUSE_PATTERN;
#endif
    pauth_ent->ainfo_type = 0;
    pauth_ent->authenticator = NULL;
    pauth_ent->principals = NULL;
    pauth_ent->app.ptr = NULL;         /* On every architecture I ever heard 
                                          of, pauth_ent->app.flg is now 0 too
                                          */ 
    pauth_ent->next = NULL;
    return(pauth_ent);
}

static void (*paappfreefunc)(PAUTH) = NULL;

/*
 * Specify which special freeing function (if any) to be used to free the
 * app.ptr member of the PAUTH structure, if set. 
 */
void            
paappfree(void (* appfreefunc)(PAUTH))
{
    paappfreefunc = appfreefunc;
}



/*
 * pafree - free an PAUTH structure
 *
 *    PAFREE takes a pointer to an PAUTH structure and adds it to
 *    the free list for later reuse.
 */
void
pafree(PAUTH pauth_ent)
{
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    assert(pauth_ent->consistency == INUSE_PATTERN);
    pauth_ent->consistency = FREE_PATTERN;
#endif
    pauth_ent->ainfo_type = 0;
    if(pauth_ent->authenticator) stfree(pauth_ent->authenticator);
    if(pauth_ent->principals) tklfree(pauth_ent->principals);
    if(paappfreefunc && pauth_ent->app.ptr)  {
        (*paappfreefunc)(pauth_ent->app.ptr); pauth_ent->app.ptr = NULL;
    }

    p_th_mutex_lock(p_th_mutexPAALLOC);
    pauth_ent->next = pauthflist;
    pauthflist = pauth_ent;
    pauth_count--;
    p_th_mutex_unlock(p_th_mutexPAALLOC);
}

/*
 * palfree - free a PAUTH structure
 *
 *    PALFREE takes a pointer to an PAUTH structure frees it and any linked
 *    PAUTH structures.  It is used to free an entrie list of PAUTH
 *    structures.
 */
void
palfree(PAUTH pauth_ent)
{
    PAUTH	nxt;

    while(pauth_ent != NULL) {
        nxt = pauth_ent->next;
        pafree(pauth_ent);
        pauth_ent = nxt;
    }
}

