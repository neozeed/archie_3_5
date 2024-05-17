/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-copyr.h>
#include <stdio.h>
#include <stdlib.h>           /* For malloc and free */

#include <pfs.h>

static FILTER	lfree = NULL;
/* These are global variables which will be read by dirsrv.c
   Too bad C doesn't have better methods for structuring such global data.  */

int		filter_count = 0;
int		filter_max = 0;

/*
 * flalloc - allocate and initialize FILTER structure
 *
 *    FLALLOC returns a pointer to an initialized structure of type
 *    FILTER.  If it is unable to allocate such a structure, it
 *    signals out_of_memory();
 */
FILTER
flalloc()
{

    FILTER	fil;
    p_th_mutex_lock(p_th_mutexFLALLOC);
    if(lfree) {
        fil = lfree;
        lfree = lfree->next;
    }
    else {
        fil = (FILTER) malloc(sizeof(FILTER_ST));
        if (!fil) out_of_memory();
        filter_max++;
    }

    filter_count++;
    p_th_mutex_unlock(p_th_mutexFLALLOC);

    /* Initialize and fill in default values */
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    fil->consistency = INUSE_PATTERN;
#endif
    fil->name = NULL;
    fil->link = NULL;
    fil->type = 0;              /* Illegal value */
    fil->execution_location = 0; /* Neither -- illegal value. */
    fil->pre_or_post = 0;       /* Neither -- illegal value */
    fil->args = NULL;
    fil->errmesg = NULL;        /* error message from filter application, if
                                   any.  */
    fil->app.ptr = NULL;         /* On every architecture I ever heard of, 
                                    fil->app.flg is now 0 too */
    fil->previous = NULL;
    fil->next = NULL;
    return(fil);
}

static void (*flappfreefunc)(FILTER) = NULL;

/*
 * Specify which special freeing function (if any) to be used to free the
 * app.ptr member of the FILTER structure, if set. 
 */
void            
flappfree(void (* appfreefunc)(FILTER))
{
    flappfreefunc = appfreefunc;
}

/*
 * flfree - free a FILTER structure
 *
 *    FLFREE takes a pointer to a FILTER structure and adds it to
 *    the free list for later reuse.
 */
void
flfree(FILTER fil)
{
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    assert(fil->consistency == INUSE_PATTERN);
    fil->consistency = FREE_PATTERN;
#endif
    if(fil->name) stfree(fil->name);
    if(fil->link) vlfree(fil->link);
    if(fil->args) tklfree(fil->args);
    if (fil->errmesg) stfree(fil->errmesg);
    if(flappfreefunc && fil->app.ptr) {
        (*flappfreefunc)(fil->app.ptr); fil->app.ptr = NULL;
    }
    p_th_mutex_lock(p_th_mutexFLALLOC);
    fil->next = lfree;
    fil->previous = NULL;
    lfree = fil;
    filter_count--;
    p_th_mutex_unlock(p_th_mutexFLALLOC);
}

/*
 * fllfree - free a linked list of FILTER structures.
 *
 *    FLLFREE takes a pointer to a FILTER structure frees it and any linked
 *    FILTER structures.  It is used to free an entrie list of FILTER
 *    structures.
 */
void
fllfree(fil)
    FILTER	fil;
{
    FILTER	nxt;

    while((fil != NULL) /* && !fil->dontfree */) {
        nxt = fil->next;
        flfree(fil);
        fil = nxt;
    }
}



/*
 * flcopy - allocates a new filter structure and
 *          initializes it with a copy of another 
 *          filter, v.
 *
 *          If r is non-zero, successive filters will be
 *          iteratively copied.
 *
 *          fl-previous will always be null on the first link, and
 *          will be appropriately filled in for iteratively copied links. 
 *
 *          FLCOPY returns a pointer to the new structure of type
 *          FILTER.  If it is unable to allocate such a structure, it
 *          returns NULL.
 *
 *          FLCOPY will recursively copy the link associated with a filter and
 *          its associated attributes. 
 */

FILTER
flcopy(FILTER f, int r)
{
    FILTER	nf;
    FILTER	snf;  /* Start of the chain of new links */
    FILTER	tf;   /* Temporary link pointer          */

    if (!f) return NULL;	/* If called with NULL return it */
    nf = flalloc();
    snf = nf;

copyeach:

    /* Copy f into nf */
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    assert(f->consistency == INUSE_PATTERN);
#endif
    if(f->name) nf->name = stcopyr(f->name,nf->name);
    if(f->link) nf->link = vlcopy(f->link, 1);
    nf->type = f->type;
    nf->execution_location = f->execution_location;
    nf->pre_or_post = f->pre_or_post;
    nf->args = tkcopy(f->args);
    f->errmesg = stcopy(f->errmesg);
    if(r && f->next) {
        f = f->next;
        tf = nf;
        nf = flalloc();
        nf->previous = tf;
        tf->next = nf;
        goto copyeach;
    }

    return(snf);
}

