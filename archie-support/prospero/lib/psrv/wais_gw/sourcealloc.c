/* Copyright (c) 1993, by Pandora Systems */
/* Author:	Mitra <mitra@path.net> */
/* Allocation code copied and adapted from:
	 prospero/alpha.5.2a+/lib/pfs/flalloc */

#include "buffalloc.h"
#include <pfs.h>
#include <pfs_threads.h>
#include <mitra_macros.h>
#include "source.h"
#define Channel_Illegal 0;

EXTERN_ALLOC_DECL(WAISSOURCE);

static WAISSOURCE	lfree = NULL;		/* Free waissources */
/* These are global variables which will be read by dirsrv.c
   Too bad C doesn't have better methods for structuring such global data.  */

int		waissource_count = 0;
int		waissource_max = 0;


/************* Standard routines to alloc, free and copy *************/
/*
 * waissource_alloc - allocate and initialize WAISSOURCE structure
 *
 *    returns a pointer to an initialized structure of type
 *    WAISSOURCE.  If it is unable to allocate such a structure, it
 *    signals out_of_memory();
 */

WAISSOURCE
waissource_alloc()			
{
    WAISSOURCE	awaissource;
    
    TH_STRUC_ALLOC(waissource,WAISSOURCE,awaissource);
    awaissource->name = NULL;
    awaissource->directory = NULL;
    awaissource->description = NULL;
    awaissource->connection = NULL;
    awaissource->maintainer = NULL;
    awaissource->subjects = NULL;
    return(awaissource);
}

/*
 * waissource_free - free a WAISSOURCE structure
 *
 *    waissource_free takes a pointer to a WAISSOURCE structure and adds it to
 *    the free list for later reuse.
 */
void
waissource_free(WAISSOURCE awaissource)
{
    stfree(awaissource->name);		awaissource->name = NULL;
    stfree(awaissource->directory);	awaissource->directory = NULL;
    stfree(awaissource->description);	awaissource->description = NULL;
    stfree(awaissource->connection);	awaissource->connection = NULL;
    stfree(awaissource->maintainer); 	awaissource->maintainer = NULL;
    stfree(awaissource->subjects);  	awaissource->subjects = NULL;
    TH_STRUC_FREE(waissource,WAISSOURCE,awaissource);
}

/*
 * waissource_lfree - free a linked list of WAISSOURCE structures.
 *
 *    waissource_lfree takes a pointer to a waissource structure frees it and 
 *    any linked
 *    WAISSOURCE structures.  It is used to free an entire list of WAISSOURCE
 *    structures.
 */
void
waissource_lfree(awaissource)
    WAISSOURCE	awaissource;
{
	TH_STRUC_LFREE(WAISSOURCE,awaissource,waissource_free);
}

void
waissource_freespares()
{
	TH_FREESPARES(waissource,WAISSOURCE);
}

#ifdef NEVERDEFINED
/*
 * waissource_copy - allocates a new waissource structure and
 *          initializes it with a copy of another 
 *          waissource, v.
 *
 *          If r is non-zero, successive waissources will be
 *          iteratively copied.
 *
 *          awaissource-previous will always be null on the first link, and
 *          will be appropriately filled in for iteratively copied links. 
 *
 *          waissource_copy returns a pointer to the new structure of type
 *          WAISSOURCE.  If it is unable to allocate such a structure, it
 *          returns NULL.
 *
 *          waissource_copy will recursively copy the link associated with
 *	    a waissource and
 *          its associated attributes. 
 */

WAISSOURCE
waissource_copy(f,r)
    WAISSOURCE	f;
    int	r;              /* Currently ignored. */
{
    WAISSOURCE	nf;
    WAISSOURCE	snf;  /* Start of the chain of new links */
    WAISSOURCE	tf;   /* Temporary link pointer          */

    nf = waissource_alloc();
    snf = nf;

copyeach:

    /* Copy f into nf */
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    assert(f->consistency == INUSE_PATTERN);
#endif
	LOCAL STUFF NOT DEFINED 
    if(r && f->next) {
        f = f->next;
        tf = nf;
        nf = waissource_alloc();
        nf->previous = tf;
        tf->next = nf;
        goto copyeach;
    }

    return(snf);
}

#endif
