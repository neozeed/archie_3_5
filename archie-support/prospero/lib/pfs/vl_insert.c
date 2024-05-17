/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
#include <stdio.h>

#include <pfs.h>
#include <perrno.h>

/*
 * vl_insert - Insert a directory link at the right location
 *
 *             VL_INSERT takes a directory and a link to be added to a 
 *             directory and inserts it in the linked list of links for
 *             that directory.  
 *
 *             If a link already exists with the same name, and if the
 *             information associated with the new link matches that in
 *             the existing link, an error is returned.  If the information
 *             associated with the new link is different, but the magic numbers
 *             match, then the new link will be added as a replica of the
 *             existing link.  If the magic numbers do not match, the new
 *             link will only be added to the list of "replicas" if the
 *             allow_conflict flag has been set.
 * 
 *             If the link is not added, an error is returned and the link
 *             is freed.  Ordering for the list of links is by the link name.  
 *        
 *             If vl is a union link, then VL_INSERT calls ul_insert with an
 *	       added argument indicating the link is to be included at the
 *             end of the union link list.
 *
 *             If the VLI_NOCONFLICT flag is specified, duplicate links with
 *             conflicting important information (link name, hsonametype,
 *             hsoname, hosttype, and host) cannot be inserted unless they have
 *             the same non-zero magic number (REMOTE ID).  Otherwise,
 *             VL_INSERT_CONFLICT is returned.
 *
 *             If the VLI_NONAMECONFLICT flag is specified, links
 *             with conflicting names cannot be inserted.  Otherwise,
 *             VL_INSERT_CONFLICT is returned.
 *
 * 
 *    ARGS:    vl - Link to be inserted, vd - directory to get link
 *             allow_conflict - insert links with conflicting names
 *
 * RETURNS:    Success, or VL_INSERT_ALREADY_THERE, or VL_INSERT_CONFLICT
 */
int
vl_insert(vl,vd,allow_conflict)
    VLINK	vl;		/* Link to be inserted               */
    VDIR	vd;		/* Directory to receive link         */
    int		allow_conflict;	/* Allow duplicate names             */
{
    VLINK	current;	/* To step through list		     */
    VLINK	crep;		/* To step through list of replicas  */
    int	vl_comp_retval = 1;		/* Temp for checking returned values.
Initialize to 1 meaning 'not equal'. */

    /* This can also be used to insert union links at end of list */
    if(vl->linktype == 'U') return(ul_insert(vl,vd,NULL));

    /* If this is the first link in the directory */
    if(vd->links == NULL ) {
        vd->flags &= ~VDIR_UNSORTED; /* turn off flag in case reusing dir */
        APPEND_ITEM(vl, vd->links);
        return(PSUCCESS);
    }
    /* If no sorting is to be done, just insert at end of list */
    if(allow_conflict == VLI_NOSORT) {
        vd->flags |= VDIR_UNSORTED;
        APPEND_ITEM(vl, vd->links);
        return(PSUCCESS);
    }

    if (vd->flags & VDIR_UNSORTED) {
        for (current = vd->links; ; current = current->next)  {
            if(!current) break;
            if (current->linktype == '-') continue;
            if ((vl_comp_retval = vl_comp(vl, current)) == 0) break;
        }
        if (!current) {         /* insert at end. */
            assert(vl_comp_retval);
            APPEND_ITEM(vl, vd->links);
            return PSUCCESS;
        } else {
            assert(!vl_comp_retval);    /* fall to next test */
        }
    } else {
        /* If it is to be inserted at start of list */
        if(vl_comp(vl,vd->links) < 0 && vd->links->linktype != '-') {
            vl->next = vd->links;
            vl->previous = vl->next->previous; /* point to tail */
            vl->next->previous = vl;
            vd->links = vl;
            return(PSUCCESS);
        }

        /* Otherwise, we must find the right spot to insert it */
        for(current = vd->links; ; current = current->next) {
            if(!current) {
                /* insert at end */
                APPEND_ITEM(vl, vd->links);
                return(PSUCCESS);
            }
            if (current->linktype == '-') continue;
            if((vl_comp_retval = vl_comp(vl,current)) <= 0) break;
        }
    }
    /* If we found an entry with the same name already in the list */
    if(!vl_comp_retval) {               /* current points to entry with same name */
        if(vl_equal(vl,current)) {
            vlfree(vl);
            return(VL_INSERT_ALREADY_THERE);
        }
        if((allow_conflict == VLI_NOCONFLICT) &&
           ((vl->f_magic_no != current->f_magic_no) ||
            (vl->f_magic_no==0)))
            return(VL_INSERT_CONFLICT);
        /* Insert the link into the list of "replicas" */
        /* If magic is 0, then create a pseudo magic number */
        if(vl->f_magic_no == 0) vl->f_magic_no = -1;
        crep = current->replicas;
        if(!crep) {
            current->replicas = vl;
            vl->next = NULL;
            vl->previous = vl;
        }
        else {
            while(crep->next) {
                /* If magic was 0, then we need a unique magic number */
                if((crep->f_magic_no < 0) && (vl->f_magic_no < 1))
                    (vl->f_magic_no)--;
                crep = crep->next;
            }
            /* If magic was 0, then we need a unique magic number */
            if((crep->f_magic_no < 0) && (vl->f_magic_no < 1))
                (vl->f_magic_no)--;
            crep->next = vl;
            vl->previous = crep;
            vl->next = NULL;
            /* tail of list.  Not used now, but might be one day. */
            current->replicas->previous = vl;
        }
        return(PSUCCESS);
    }

    /* We found the spot where vl is to be inserted */
    /* Appending to the very end of the list is taken care of above.
       So we don't have to reset vd->links->previous. */
    vl->next = current;
    vl->previous = current->previous;
    current->previous = vl; 
    vl->previous->next = vl;
    return(PSUCCESS);
}
