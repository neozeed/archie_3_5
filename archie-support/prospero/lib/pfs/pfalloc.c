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

static PFILE	lfree = NULL;
int		pfile_count = 0;
int		pfile_max = 0;

/*
 * fialloc - allocate and initialize pfile structure
 *
 *    PFALLOC returns a pointer to an initialized structure of type
 *    PFILE.  If it is unable to allocate such a structure, it
 *    signals out_of_memory();
 */
PFILE
pfalloc(void)
{
    PFILE pf;

    p_th_mutex_lock(p_th_mutexPFALLOC);
    if(lfree) {
        pf = lfree;
        lfree = lfree->next;
    }
    else {
        pf = (PFILE) malloc(sizeof(PFILE_ST));
        if (!pf) out_of_memory();
        pfile_max++;
    }
    
    pfile_count++;
    p_th_mutex_unlock(p_th_mutexPFALLOC);

    /* Initialize and fill in default values */
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    pf->consistency = INUSE_PATTERN;
#endif
    pf->version = 0;
    pf->f_magic_no = 0;
    pf->oacl = NULL;
    pf->exp = 0;
    pf->ttl = 0;
    pf->last_ref = 0;
    pf->forward = NULL;
    pf->backlinks = NULL;
    pf->attributes = NULL;
    pf->previous = NULL;
    pf->next = NULL;
    return(pf);
}

/*
 * pffree - free a PFILE structure
 *
 *    PFFREE takes a pointer to a PFILE structure and adds it to
 *    the free list for later reuse.
 */
void
pffree(PFILE pf)
{
	if (!pf) return;

#ifdef ALLOCATOR_CONSISTENCY_CHECK
    assert(pf->consistency == INUSE_PATTERN);
    pf->consistency = FREE_PATTERN;
#endif
    if(pf->oacl) aclfree(pf->oacl); pf->oacl = NULL;
    if(pf->forward) vllfree(pf->forward); pf->forward = NULL;
    if(pf->backlinks) vllfree(pf->backlinks); pf->backlinks = NULL;
    if(pf->attributes) atlfree(pf->attributes); pf->attributes = NULL;

    p_th_mutex_lock(p_th_mutexPFALLOC);
    pf->next = lfree;
    pf->previous = NULL;
    lfree = pf;
    pfile_count--;
    p_th_mutex_unlock(p_th_mutexPFALLOC);
}

/*
 * pflfree - free a PFILE structure list
 *
 *    PFLFREE takes a pointer to a PFILE structure frees it and any linked
 *    PFILE structures.  It is used to free an entire list of PFILE
 *    structures.
 */
void
pflfree(PFILE pf)
{
    PFILE	nxt;

    while(pf != NULL) {
        nxt = pf->next;
        pffree(pf);
        pf = nxt;
    }
}

