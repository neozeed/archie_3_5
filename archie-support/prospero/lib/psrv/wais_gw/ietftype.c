/* Copyright (c) 1993, by Pandora Systems */
/* Author:	Mitra <mitra@path.net> */
/* Allocation code copied and adapted from:
	 prospero/alpha.5.2a+/lib/pfs/flalloc */

#include "ietftype.h"
#include <pfs.h>
#include <mitra_macros.h>
#include <psrv.h>               /* includes global declarations of
                                   ietftype_count and ietftype_max to be read.
                                   */ 

static IETFTYPE	lfree = NULL;		/* Free ietftypes */
/* These are global shared variables which will be read by dirsrv.c
   Too bad C doesn't have better methods for structuring such global data.  */
int		ietftype_count = 0;
int		ietftype_max = 0;


/* Syntactically same as atput in goph_gw_dsdb.c */
PATTRIB ietftype_atput(IETFTYPE it, char *name)
{
    PATTRIB at;

    /* Fails if no arguments --  at = vatbuild(name, ap); */
    at = atalloc();
    at->aname = stcopy(name);
    at->avtype = ATR_SEQUENCE;
    at->nature = ATR_NATURE_APPLICATION;
    at->precedence = ATR_PREC_OBJECT;
    APPEND_ITEM(at, it->prosperotype);
    return at;
}

/************* Standard routines to alloc, free and copy *************/
/*
 * ietftype_alloc - allocate and initialize IETFTYPE structure
 *
 *    returns a pointer to an initialized structure of type
 *    IETFTYPE.  If it is unable to allocate such a structure, it
 *    signals out_of_memory();
 */

IETFTYPE
ietftype_alloc()
{
    IETFTYPE	thistype;
    
    TH_STRUC_ALLOC(ietftype,IETFTYPE,thistype);
    thistype->standardtype=NULL;
    thistype->waistype=NULL;
    thistype->prosperotype=NULL;
    return(thistype);
}

/*
 * ietftype_free - free a IETFTYPE structure
 *
 *    ietftype_free takes a pointer to a IETFTYPE structure and adds it to
 *    the free list for later reuse.
 */
void
ietftype_free(IETFTYPE thistype)
{
    if (thistype->standardtype) 
	{ stfree(thistype->standardtype); thistype->standardtype = NULL;}
    if (thistype->waistype) 
	{ stfree(thistype->waistype); thistype->waistype = NULL;}
    if (thistype->prosperotype) 
	{ atfree(thistype->prosperotype); thistype->prosperotype = NULL;}
    TH_STRUC_FREE(ietftype,IETFTYPE,thistype);
}

/*
 * ietftype_lfree - free a linked list of IETFTYPE structures.
 *
 *    ietftype_lfree takes a pointer to a ietftype structure frees it and 
 *    any linked
 *    IETFTYPE structures.  It is used to free an entire list of IETFTYPE
 *    structures.
 */
void
ietftype_lfree(IETFTYPE thistype)
{
	TH_STRUC_LFREE(IETFTYPE,thistype,ietftype_free);
}

void
ietftype_freespares()
/* This is used for ietftypes to free up space in the child */
{
	IETFTYPE	instance;

	while((instance = lfree) != NULL) {
	/* Note this is slightly lazy, the thistype list is broken during loop*/
		lfree = instance->next;
		free(lfree);	/* Matches malloc in STRUC_ALLOC1*/
		ietftype_max--;
	}
}

