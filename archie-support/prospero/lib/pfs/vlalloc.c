/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>
#include <stdio.h>
#include <stdlib.h>           /* For malloc and free */

#include <pfs.h>
#include <mitra_macros.h>

static VLINK	lfree = NULL;
int		vlink_count = 0;
int		vlink_max = 0;

/*
 * vlalloc - allocate and initialize vlink structure
 *
 *    VLALLOC returns a pointer to an initialized structure of type
 *    VLINK.  If it is unable to allocate such a structure, it
 *    signals out_of_memory().
 */
VLINK
vlalloc()
{
    VLINK	vl;

    TH_STRUC_ALLOC(vlink, VLINK, vl);

    /* Initialize and fill in default values */
    vl->dontfree = FALSE;
    vl->flags = 0;          
    vl->name = NULL;
    vl->linktype = 'L';
    vl->expanded = 0;
    vl->target = stcopy("FILE");
    vl->filters = NULL;
    vl->replicas = NULL;
    vl->hosttype = stcopy("INTERNET-D");
    vl->host = NULL;
    vl->hsonametype = stcopy("ASCII");
    vl->hsoname = NULL;
    vl->version = 0;
    vl->f_magic_no = 0;
    vl->oid = NULL;
    vl->acl = NULL;
    vl->dest_exp = 0;
#if 0
    vl->link_exp = 0;
    vl->args = NULL;
#endif
    vl->lattrib = NULL;
    vl->f_info = NULL;
    vl->app.ptr = NULL;         /* On every architecture I ever heard of, 
                                   vl->app.flg is now 0 too */
    vl->previous = NULL;
    vl->next = NULL;
    return(vl);
}

/*
 * vlcopy - allocates a new vlink structure and
 *          initializes it with a copy of another 
 *          vlink, v.
 *
 *          If r is non-zero, successive vlinks will be
 *          iteratively copied.
 *
 *          vl-previous will always be null on the first link, and
 *          will be appropriately filled in for iteratively copied links. 
 *
 *          VLCOPY returns a pointer to the new structure of type
 *          VLINK.  It calls vlalloc(), which signals out_of_memory() if none
 *              is available.
 *
 *          VLCOPY will copy the list of filters associated with a link.
 *          It will not copy the list of replicas.
 */
VLINK
vlcopy(v,r)
	VLINK	v;
	int	r;              /* Currently ignored. */
{
    VLINK	nl;
    VLINK	snl;  /* Start of the chain of new links */
    VLINK	tl;   /* Temporary link pointer          */

	if (!v) return NULL;	/* Oops - cant duplicate empty list */
    nl = vlalloc();
    snl = nl;

copyeach:

    /* Copy v into nl */
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    assert(v->consistency == INUSE_PATTERN);
#endif
    if(v->name) nl->name = stcopyr(v->name,nl->name);
    nl->linktype = v->linktype;
    nl->expanded = v->expanded;
    nl->target = stcopyr(v->target,nl->target);
    nl->hosttype = stcopyr(v->hosttype,nl->hosttype);
    if(v->host) nl->host = stcopyr(v->host,nl->host);
    nl->hsonametype = stcopyr(v->hsonametype,nl->hsonametype);
    if(v->hsoname) nl->hsoname = stcopyr(v->hsoname,nl->hsoname);
    nl->version = v->version;
    nl->f_magic_no = v->f_magic_no;
        nl->oid = atlcopy(v->oid);	/* Fixed to copy pattrib correctly*/
    nl->acl = NULL;
    nl->dest_exp = v->dest_exp;
    if(v->filters) nl->filters = flcopy(v->filters, 1);
	nl->lattrib = atlcopy(v->lattrib);
	/* Still need to handle f_info */
    if(r && v->next) {
        v = v->next;
        tl = nl;
        nl = vlalloc();
        nl->previous = tl;
        tl->next = nl;
        goto copyeach;
    }

    return(snl);
}
static void (*vlappfreefunc)(VLINK) = NULL;

/*
 * Specify which special freeing function (if any) to be used to free the
 * app.ptr member of the VLINK structure, if set. 
 */
void            
vlappfree(void (* appfreefunc)(VLINK))
{
    vlappfreefunc = appfreefunc;
}


/*
 * vlfree - free a VLINK structure
 *
 *    VLFREE takes a pointer to a VLINK structure and adds it to
 *    the free list for later reuse.
 */
void
vlfree(VLINK vl)
{
    if(vl->dontfree) return;
    stfree(vl->name); vl->name = NULL;
    stfree(vl->target);   vl->target = NULL;
    fllfree(vl->filters); vl->filters = NULL;
    vllfree(vl->replicas); vl->replicas = NULL;
    stfree(vl->hosttype); vl->hosttype = NULL;
    stfree(vl->host); vl->host = NULL;
    stfree(vl->hsonametype); vl->hsonametype = NULL;
    stfree(vl->hsoname); vl->hsoname = NULL ; 
    atlfree(vl->oid); vl->oid = NULL ;
    aclfree(vl->acl); vl->acl = NULL;
#if 0
    if(vl->args) stfree(vl->args); vl ->args = NULL;
#endif
    atlfree(vl->lattrib); vl->lattrib = NULL;
    /* No allocation routines for f_info yet */
    if (vl->f_info ) pffree(vl->f_info); vl->f_info = NULL;
    if(vlappfreefunc && vl->app.ptr)  {
        (*vlappfreefunc)(vl->app.ptr); vl->app.ptr = NULL;
    }
    assert(!vl->next);
    TH_STRUC_FREE(vlink, VLINK, vl);
}

/*
 * vllfree - free a VLINK structure
 *
 *    VLLFREE takes a pointer to a VLINK structure frees it and any linked
 *    VLINK structures.  It is used to free an entrie list of VLINK
 *    structures.
 */
void
vllfree(VLINK vl)
{
    VLINK	nxt;

    while((vl != NULL) && !vl->dontfree) {
        nxt = vl->next;
	vl->next = NULL;   /* Avoid tripping assertion in vlfree */
        vlfree(vl);
        vl = nxt;
    }
}

int
vl_nlinks(VLINK vl)
{
  int i;
  for (i=0; vl; vl=vl->next, i++);
  return i;
}
