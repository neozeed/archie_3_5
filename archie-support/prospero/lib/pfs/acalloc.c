/*
 * Copyright (c) 1992, 1993, 1994 by the University of Southern California
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */
/* Munged by swa to handle multiple principals. */

#include <pmachine.h>
#ifdef SOLARIS                  /* don't know why this was included. */
#include <sys/select.h>
#endif
#include <usc-license.h>
#include <stdio.h>
#include <stdlib.h>		/* For malloc */

#include <pfs.h>

static ACL	aclflist = NULL;
int		acl_count = 0;
int		acl_max = 0;

/*
 * acalloc - allocate and initialize access control list structure
 *
 *    ACALLOC returns a pointer to an initialized structure of type
 *    ACL.  If it is unable to allocate such a structure, it
 *    returns NULL.
 */
ACL
acalloc()
{
    ACL	acl_ent;

    p_th_mutex_lock(p_th_mutexACALLOC);
    if(aclflist) {
        acl_ent = aclflist;
        aclflist = aclflist->next;
    }
    else {
        acl_ent = (ACL) malloc(sizeof(ACL_ST));
        if (!acl_ent) out_of_memory();
        acl_max++;
    }

    acl_count++;
    p_th_mutex_unlock(p_th_mutexACALLOC);

    /* Initialize and fill in default values */
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    acl_ent->consistency = INUSE_PATTERN;
#endif
    acl_ent->acetype = 0;
    acl_ent->atype = NULL;
    acl_ent->rights = NULL;
    acl_ent->principals = NULL;
    acl_ent->restrictions = NULL;
    acl_ent->app.ptr = NULL;         /* On every architecture I ever heard of,
                                        acl_ent->app.flg is now 0 too */
    acl_ent->previous = NULL;
    acl_ent->next = NULL;
    return(acl_ent);
}
    
static void (*acappfreefunc)(ACL) = NULL;

/*
 * Specify which special freeing function (if any) to be used to free the
 * app.ptr member of the ACL structure, if set. 
 */
void            
acappfree(void (* appfreefunc)(ACL))
{
    acappfreefunc = appfreefunc;
}

/*
 * acfree - free an ACL structure
 *
 *    ACFREE takes a pointer to an ACL structure and adds it to
 *    the free list for later reuse.
 */
void
acfree(acl_ent)
    ACL		acl_ent;
{
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    assert(acl_ent->consistency == INUSE_PATTERN);
    acl_ent->consistency = FREE_PATTERN;
#endif
    acl_ent->acetype = 0;
    if(acl_ent->atype) stfree(acl_ent->atype);
    if(acl_ent->rights) stfree(acl_ent->rights);
    if(acl_ent->principals)  {
        tklfree(acl_ent->principals);
        acl_ent->principals = NULL;
    }
    /* If acl_ent->restrictions, error since not yet implemented */
    assert(!acl_ent->restrictions);
    if(acappfreefunc && acl_ent->app.ptr)  {
        (*acappfreefunc)(acl_ent->app.ptr); acl_ent->app.ptr = NULL;
    }
    p_th_mutex_lock(p_th_mutexACALLOC);
    acl_ent->next = aclflist;
    if(aclflist) aclflist->previous = acl_ent;
    acl_ent->previous = NULL;
    aclflist = acl_ent;
    acl_count--;
    p_th_mutex_unlock(p_th_mutexACALLOC);
}

/*
 * aclfree - free an ACL structure
 *
 *    ACLFREE takes a pointer to an ACL structure frees it and any linked
 *    ACL structures.  It is used to free an entrie list of ACL
 *    structures.
 */
void
aclfree(acl_ent)
    ACL		acl_ent;
{
    ACL	nxt;

    while(acl_ent != NULL) {
        nxt = acl_ent->next;
        acfree(acl_ent);
        acl_ent = nxt;
    }
}


ACL 
aclcopy(ACL acl)
{
    ACL retval = NULL;
    for ( ;acl ; acl = acl->next) {
        ACL new = acalloc();
        new->acetype = acl->acetype;
        if(acl->atype) new->atype = stcopyr(acl->atype,new->atype);
        if(acl->rights) new->rights = stcopyr(acl->rights,new->rights);
        if(acl->principals) new->principals = tkcopy(acl->principals);
        /* If struct restrict ever used, will need to copy that too. */
        assert(!acl->restrictions);
        APPEND_ITEM(new, retval);
    }        
    return(retval);
}


