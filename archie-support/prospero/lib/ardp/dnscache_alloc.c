/* Copyright (c) 1993, by Pandora Systems */
/* Author:	Mitra <mitra@path.net> */
/* Allocation code copied and adapted from:
	 prospero/alpha.5.2a+/lib/pfs/flalloc */

#include "dnscache_alloc.h"
#include <pfs.h>
#include <pfs_threads.h>
#include <mitra_macros.h>

static DNSCACHE	lfree = NULL;		/* Free dnscaches */
/* These are global variables which will be read by dirsrv.c
   Too bad C doesn't have better methods for structuring such global data.  */

int		dnscache_count = 0;
int		dnscache_max = 0;


/************* Standard routines to alloc, free and copy *************/
/*
 * dnscache_alloc - allocate and initialize DNSCACHE structure
 *
 *    returns a pointer to an initialized structure of type
 *    DNSCACHE.  If it is unable to allocate such a structure, it
 *    signals out_of_memory();
 */

DNSCACHE
dnscache_alloc()			
{
    DNSCACHE	acache;
    
    TH_STRUC_ALLOC(dnscache,DNSCACHE,acache);
    acache->name = NULL;
    acache->usecount = 0;
    bzero(&acache->sockad,sizeof(acache->sockad));
    return(acache);
}

/*
 * dnscache_free - free a DNSCACHE structure
 *
 *    dnscache_free takes a pointer to a DNSCACHE structure and adds it to
 *    the free list for later reuse.
 */
void
dnscache_free(DNSCACHE acache)
{
    stfree(acache->name) ; acache->name = NULL;
    TH_STRUC_FREE(dnscache,DNSCACHE,acache);
}

/*
 * dnscache_lfree - free a linked list of DNSCACHE structures.
 *
 *    dnscache_lfree takes a pointer to a dnscache structure frees it and 
 *    any linked
 *    DNSCACHE structures.  It is used to free an entire list of WAISMSGBUFF
 *    structures.
 */
void
dnscache_lfree(acache)
    DNSCACHE	acache;
{
	TH_STRUC_LFREE(DNSCACHE,acache,dnscache_free);
}

void
dnscache_freespares()
{
	TH_FREESPARES(dnscache,DNSCACHE);
}
