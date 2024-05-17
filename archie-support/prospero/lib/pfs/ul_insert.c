/*
 * Copyright (c) 1989, 1990 by the University of Washington
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>
 */

#include <uw-copyright.h>
#include <usc-copyr.h>
#include <stdio.h>

#include <pfs.h>
#include <perrno.h>

/*
 * ul_insert - Insert a union link at the right location
 *
 *             UL_INSERT takes a directory and a union link to be added
 *             to a the list of union links in the directory.  It then
 *             inserts the union link in the right spot in the linked
 *             list of union links associated with that directory.
 *
 *	       If an identical link already exists, then the link which
 *             would be evaluated earlier (closer to the front of the list)
 *             wins and the other one is freed.  If this happens, an error
 *             will also be returned.
 *        
 *  If a union link agrees with another link in all respects except for NAME
 *   and p is set to a vlink, then the link is tagged with ULINK_DONT_EXPAND.
 *   This is only used on the server.
 *  
 *    ARGS:    ul    - link to be inserted
 *	       vd    - directory to get link
 *             p     - vl that this link will apper after
 *                     NULL - This vl will go at end of list
 *                     vd   - This vl will go at head of list
 *
 * RETURNS:    Success, or UL_INSERT_ALREADY_THERE or UL_INSERT_SUPERSEDING
 */
int
ul_insert(ul,vd,p)
    VLINK	ul;		/* Link to be inserted or freed          */
    VDIR	vd;		/* Directory to receive link             */
    VLINK	p;		/* Union link to appear prior to new one */
{
    VLINK	current;

    static ul_comp(VLINK vl1,VLINK vl2);
    static ul_other_fields_equal(VLINK ul1, VLINK ul2);


    /* Either (a) This is the first ul in the directory, 
       or (b) we are supposed to insert at the end anyway.  */
    if(vd->ulinks == NULL || p == NULL) {
        APPEND_ITEM(ul, vd->ulinks);
        return(PSUCCESS);
    }

    /* This ul will go at the head of the list */
    if(p == (VLINK) vd) {
        ul->next = vd->ulinks;
        ul->previous = ( (ul->next) ? ul->next->previous : ul);
        ul->next->previous = ul;
        vd->ulinks = ul;
    }
    /* Otherwise, decide if it must be inserted at all  */
    /* If an identical link appears before the position */
    /* at which the new one is to be inserted, we can   */
    /* return without inserting it 			    */
    else {
        for(current = vd->ulinks; current; current = current->next) {
            if(ul_comp(current,ul) == 0) {
                if (current->expanded == ULINK_PLACEHOLDER)  {
                    /* Remove the matching link.  This isn't really necessary,
                       but it's a harmless efficiency hack.  */
                    EXTRACT_ITEM(current, vd->ulinks);
                    vlfree(current);
                    /* Stick on the new link, but don't expand it. */
                    ul->expanded = ULINK_DONT_EXPAND;
                    APPEND_ITEM(ul, vd->ulinks);
                    return PSUCCESS;
                }
                if (ul_other_fields_equal(ul, current)) {
                    /* If the two links have the same link name */
                    vlfree(ul);
                    return(UL_INSERT_ALREADY_THERE);
                } else {
                    /* different link names; might need to return both in a
                       directory listing. */
                    ul->expanded = ULINK_DONT_EXPAND;
                    APPEND_ITEM(ul, vd->ulinks);
                    return PSUCCESS;
                }
            }

            if(current == p) break;
        }

        /* If current is null, p was not found */
        if(current == NULL) {
	    vlfree(ul);
            return(UL_INSERT_POS_NOTFOUND);
	  }
        /* Insert ul */
        ul->next = p->next;
        p->next = ul;
        ul->previous = p;
        if(ul->next) 
            ul->next->previous = ul;
        else /* appending at end of list */
            vd->ulinks->previous = ul;
    }

    /* Check for matching links after ul. */
    for(current = ul->next; current; current = current->next) {
        if (ul_comp(current, ul) == 0) {
            /* Don't expand both ulinks.  If neither expanded yet, give first
               one priority. */
            if (current->expanded) ul->expanded = ULINK_DONT_EXPAND;
            else current->expanded = ULINK_DONT_EXPAND;
            /* Placeholder union links are always superseded. */
            if (current->expanded == ULINK_PLACEHOLDER ) {
                EXTRACT_ITEM(current, vd->ulinks);
                vlfree(current);
                return PSUCCESS;
            }
            if (ul_other_fields_equal(ul, current)) {
                /* If the two links have the same link name, remove the
                   matching link */ 
                EXTRACT_ITEM(current, vd->ulinks);
                vlfree(current);
                return(UL_INSERT_SUPERSEDING);
            }
        }
    }
    return PSUCCESS;
}


/* Like VL_COMP, but only compares the fields that matter when you're expanding
   the union link (i.e., doesn't care about 'name') . */
static int
ul_comp(VLINK ul1,VLINK ul2)
{
    int	retval;
    
    retval = strcmp(ul1->hosttype,ul2->hosttype);
    if(!retval) retval = strcmp(ul1->host,ul2->host);
    if(!retval) retval = strcmp(ul1->hsonametype,ul2->hsonametype);
    if(!retval) retval = strcmp(ul1->hsoname,ul2->hsoname);
    return(retval);
}

/* This assumes a ul_comp returned 0.  Tests other particulars to see whether
   two links should be included in a listing (currently just name)
*/
static int
ul_other_fields_equal(VLINK ul1, VLINK ul2)
{
    if ((!ul1->name && !ul2->name)
        || strequal(ul1->name, ul2->name))
        return TRUE;            /* should we test other fields? */
    return FALSE;
}
