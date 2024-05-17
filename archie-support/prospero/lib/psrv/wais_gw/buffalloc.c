/* Copyright (c) 1993, by Pandora Systems */
/* Author:	Mitra <mitra@path.net> */
/* Allocation code copied and adapted from:
	 prospero/alpha.5.2a+/lib/pfs/flalloc */

/*
 * Original allocation code Copyright (c) 1991-1993 by the University of
 * Southern California 
 * Modifications to Pandora code copyright (c) 1994 by the University
 * of Southern California 
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include "buffalloc.h"
#include <pfs.h>
#include <mitra_macros.h>
#define Channel_Illegal 0;
#include <psrv.h>               /* includes global declarations of
                                   waismsgbuff_count and waismsgbuff_max to be
                                   read by dirsrv.c. */ 

static WAISMSGBUFF	lfree = NULL;		/* Free waismsgbuffs */
/* These are global variables which will be read by dirsrv.c
   Too bad C doesn't have better methods for structuring such global data.  */

int		waismsgbuff_count = 0;
int		waismsgbuff_max = 0;


/************* Standard routines to alloc, free and copy *************/
/*
 * waismsgbuff_alloc - allocate and initialize WAISMSGBUFF structure
 *
 *    returns a pointer to an initialized structure of type
 *    WAISMSGBUFF.  If it is unable to allocate such a structure, it
 *    signals out_of_memory();
 */

WAISMSGBUFF
waismsgbuff_alloc()			
{
    WAISMSGBUFF	msgbuff;
    
    TH_STRUC_ALLOC(waismsgbuff,WAISMSGBUFF,msgbuff);
    return(msgbuff);
}

/*
 * waismsgbuff_free - free a WAISMSGBUFF structure
 *
 *    waismsgbuff_free takes a pointer to a WAISMSGBUFF structure and adds it to
 *    the free list for later reuse.
 */
void
waismsgbuff_free(WAISMSGBUFF msgbuff)
{
    TH_STRUC_FREE(waismsgbuff,WAISMSGBUFF,msgbuff);
}

/*
 * waismsgbuff_lfree - free a linked list of WAISMSGBUFF structures.
 *
 *    waismsgbuff_lfree takes a pointer to a waismsgbuff structure frees it and 
 *    any linked
 *    WAISMSGBUFF structures.  It is used to free an entire list of WAISMSGBUFF
 *    structures.
 */
void
waismsgbuff_lfree(msgbuff)
    WAISMSGBUFF	msgbuff;
{
	TH_STRUC_LFREE(WAISMSGBUFF,msgbuff,waismsgbuff_free);
}

void
waismsgbuff_freespares()
{
	TH_FREESPARES(waismsgbuff,WAISMSGBUFF);
}

#ifdef NEVERDEFINED
/*
 * waismsgbuff_copy - allocates a new waismsgbuff structure and
 *          initializes it with a copy of another 
 *          waismsgbuff, v.
 *
 *          If r is non-zero, successive waismsgbuffs will be
 *          iteratively copied.
 *
 *          msgbuff-previous will always be null on the first link, and
 *          will be appropriately filled in for iteratively copied links. 
 *
 *          waismsgbuff_copy returns a pointer to the new structure of type
 *          WAISMSGBUFF.  If it is unable to allocate such a structure, it
 *          returns NULL.
 *
 *          waismsgbuff_copy will recursively copy the link associated with
 *	    a waismsgbuff and
 *          its associated attributes. 
 */

WAISMSGBUFF
waismsgbuff_copy(f,r)
    WAISMSGBUFF	f;
    int	r;              /* Currently ignored. */
{
    WAISMSGBUFF	nf;
    WAISMSGBUFF	snf;  /* Start of the chain of new links */
    WAISMSGBUFF	tf;   /* Temporary link pointer          */

    nf = waismsgbuff_alloc();
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
        nf = waismsgbuff_alloc();
        nf->previous = tf;
        tf->next = nf;
        goto copyeach;
    }

    return(snf);
}

#endif
