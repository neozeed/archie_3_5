/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>
#include <stdio.h>
#include <stdlib.h>           /* For malloc and free */

#include <pfs.h>

static P_OBJECT	lfree = NULL;
int		p_object_count = 0;
int		p_object_max = 0;

/*
 * oballoc - allocate and initialize p_object structure
 *
 *    OBALLOC returns a pointer to an initialized structure of type
 *    P_OBJECT.  If it is unable to allocate such a structure, it
 *    signals out_of_memory();
 */
P_OBJECT
oballoc(void)
{
    P_OBJECT ob;

    p_th_mutex_lock(p_th_mutexOBALLOC);
    if(lfree) {
        ob = lfree;
        lfree = lfree->next;
    }
    else {
        ob = (P_OBJECT) malloc(sizeof(P_OBJECT_ST));
        if (!ob) out_of_memory();
        p_object_max++;
    }

    p_object_count++;
    p_th_mutex_unlock(p_th_mutexOBALLOC);

    /* Initialize and fill in default values */
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    ob->consistency = INUSE_PATTERN;
#endif
    ob->version = 0;            /* version is always zero. */
    ob->flags = 0;              /* no flags set */
    ob->inc_native = VDIN_UNINITIALIZED; /* don't write out object until this
                                            is reset.   Used only on server. */
    ob->magic_no = 0L;          /* zero means unset */
    ob->acl = NULL;             /*  */
    ob->exp = 0;
    ob->ttl = 0;
    ob->last_ref = 0;
    ob->forward = NULL;
    ob->backlinks = NULL;
    ob->attributes = NULL;
    ob->links = NULL;
    ob->ulinks = NULL;
    ob->native_mtime = 0;
    ob->shadow_file = NULL;      /* used only on server.*/
    ob->status = DQ_INACTIVE;   /* used only in client mode. */
    ob->dqs = NULL;             /* ditto */
    ob->statbuf = NULL;         /* Used only on server */
    ob->app.ptr = NULL;         /* On every architecture I ever heard of, ob->
                                   app.flg is now 0 too */
    ob->previous = NULL;
    ob->next = NULL;
    return(ob);
}

static void (*obappfreefunc)(P_OBJECT) = NULL;

/*
 * Specify which special freeing function (if any) to be used to free the
 * app.ptr member of the P_OBJECT structure, if set. 
 */
void            
obappfree(void (* appfreefunc)(P_OBJECT))
{
    obappfreefunc = appfreefunc;
}

/*
 * obfree - free a P_OBJECT structure
 *
 *    OBFREE takes a pointer to a P_OBJECT structure and adds it to
 *    the free list for later reuse.
 */
void
obfree(P_OBJECT ob)
{
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    assert(ob->consistency == INUSE_PATTERN);
    ob->consistency = FREE_PATTERN;
#endif
    if(ob->acl) aclfree(ob->acl); ob->acl = NULL;
    if(ob->forward) { vllfree(ob->forward); ob->forward = NULL;}
    if(ob->backlinks) { vllfree(ob->backlinks); ob->backlinks = NULL;}
    if(ob->attributes) { atlfree(ob->attributes); ob->attributes = NULL;}
    if(ob->links) { vllfree(ob->links); ob->links = NULL; }
    if(ob->ulinks) { vllfree(ob->ulinks); ob->ulinks = NULL; }
    if(ob->shadow_file) {stfree(ob->shadow_file); ob->shadow_file = NULL;}
    if(ob->statbuf) {stfree(ob->statbuf); ob->statbuf = NULL;}
    /* If ob->dqs signal a memory leak. */
    if(obappfreefunc && ob->app.ptr)  {
        (*obappfreefunc)(ob->app.ptr);
        ob->app.ptr = NULL;
    }

    p_th_mutex_lock(p_th_mutexOBALLOC);
    ob->next = lfree;
    ob->previous = NULL;
    lfree = ob;
    p_object_count--;
    p_th_mutex_unlock(p_th_mutexOBALLOC);
}

/*
 * oblfree - free a P_OBJECT structure list
 *
 *    OBLFREE takes a pointer to a P_OBJECT structure frees it and any linked
 *    P_OBJECT structures.  It is used to free an entire list of P_OBJECT
 *    structures.
 */
void
oblfree(P_OBJECT ob)
{
    P_OBJECT	nxt;
    
    while(ob != NULL) {
        nxt = ob->next;
        obfree(ob);
        ob = nxt;
    }
}

