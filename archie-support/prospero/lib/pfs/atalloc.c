/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>
#include <stdio.h>

#include <pfs.h>

static PATTRIB	lfree = NULL;
int		pattrib_count = 0;
int		pattrib_max = 0;

/*
 * atalloc - allocate and initialize vlink structure
 *
 *    ATALLOC returns a pointer to an initialized structure of type
 *    PATTRIB.  If it is unable to allocate such a structure, it
 *    returns NULL.
 */
PATTRIB
atalloc(void)
{
    PATTRIB	at;
    p_th_mutex_lock(p_th_mutexATALLOC);
    if(lfree) {
        at = lfree;
        lfree = lfree->next;
    } else {
        at = (PATTRIB) malloc(sizeof(PATTRIB_ST));
        if (!at) out_of_memory();
        pattrib_max++;
    }

    pattrib_count++;
    p_th_mutex_unlock(p_th_mutexATALLOC);

#ifdef ALLOCATOR_CONSISTENCY_CHECK
    at->consistency = INUSE_PATTERN;
#endif
    /* Initialize and fill in default values */
    at->precedence = ATR_PREC_OBJECT;
    at->nature =      ATR_NATURE_UNKNOWN;
    at->avtype = ATR_UNKNOWN;
    at->aname = NULL;
    at->value.sequence = NULL;
    at->app.ptr = NULL;         /* On every architecture I ever heard of, 
                                   at->app.flg is now 0 too */
    at->previous = NULL;
    at->next = NULL;
    return(at);
}

static void (*atappfreefunc)(PATTRIB) = NULL;

/*
 * Specify which special freeing function (if any) to be used to free the
 * app.ptr member of the PATTRIB structure, if set. 
 */
void            
atappfree(void (* appfreefunc)(PATTRIB))
{
    atappfreefunc = appfreefunc;
}


/*
 * atfree - free a PATTRIB structure
 *
 *    ATFREE takes a pointer to a PATTRRIB structure and adds it to
 *    the free list for later reuse.
 */
void
atfree(PATTRIB at)
{
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    assert(at->consistency == INUSE_PATTERN);
    at->consistency = FREE_PATTERN;
#endif
    if(at->aname) stfree(at->aname);

    switch(at->avtype) {
    case ATR_UNKNOWN:
        break;
    case ATR_SEQUENCE:
        if (at->value.sequence)
            tklfree(at->value.sequence);
        break;
    case ATR_FILTER:
        if(at->value.filter)
            flfree(at->value.filter);
        break;
    case ATR_LINK:
        if (at->value.link)
            vlfree(at->value.link);
        break;
    default:
        internal_error("Illegal avtype");
    }

    if(atappfreefunc && at->app.ptr)  {
        (*atappfreefunc)(at->app.ptr); at->app.ptr = NULL;
    }
    p_th_mutex_lock(p_th_mutexATALLOC);
    at->next = lfree;
    at->previous = NULL;
    lfree = at;
    pattrib_count--;
    p_th_mutex_unlock(p_th_mutexATALLOC);
}

/*
 * atlfree - free a PATTRIB structure
 *
 *    ATLFREE takes a pointer to a PATTRIB structure frees it and any linked
 *    PATTRIB structures.  It is used to free an entrie list of PATTRIB
 *    structures.
 */
void
atlfree(PATTRIB at)
{
    PATTRIB	nxt;

    while(at != NULL) {
        nxt = at->next;
        atfree(at);
        at = nxt;
    }
}



PATTRIB
atcopy(PATTRIB at)
{
    PATTRIB newat;

    if (!at) return(at);         /* Copying NULL gets NULL */
       /* Of course, many callers wont do anything sensible if returned NULL!*/
    newat = atalloc();

#ifdef ALLOCATOR_CONSISTENCY_CHECK
    assert(at->consistency == INUSE_PATTERN);
#endif
    newat->precedence = at->precedence;
    newat->nature = at->nature;
    newat->avtype = at->avtype;
    newat->aname = stcopyr(at->aname, newat->aname);
#ifdef NEVERDEFINED
    /* WARNING - this typicaly copies a pointer to a token, leaving two pattrib
	pointing into the same token, free-ing one will leave the other 
	with an invalid (and core-dump-able) pointer - Mitra*/
    newat->value = at->value;
#else
    switch(at->avtype) {
    case ATR_UNKNOWN:
        break;
    case ATR_SEQUENCE:
        if (at->value.sequence)
            newat->value.sequence = tkcopy(at->value.sequence);
        break;
    case ATR_FILTER:
        if(at->value.filter)
            newat->value.filter = flcopy(at->value.filter,TRUE);
        break;
    case ATR_LINK:
        if (at->value.link)
            newat->value.link = vlcopy(at->value.link,TRUE);
        break;
    default:
        internal_error("Illegal avtype");
    }
#endif
    /* leave previous and next unset. */
    return newat;
}


PATTRIB
atlcopy(PATTRIB atl)
{
    PATTRIB retval = NULL;

    for ( ; atl; atl = atl->next) {
        PATTRIB newat = atcopy(atl); /*Note atcopy fixed to copy data */
        APPEND_ITEM(newat, retval);
    }

    return retval;
}

#ifndef NDEBUG
int
at_nlinks(PATTRIB atl)
{
  int i;
  for (i=0; atl; atl=atl->next) {
    if (atl->avtype == ATR_LINK) 
      i++;
  }
  return i;
}
#endif /*NDEBUG*/

