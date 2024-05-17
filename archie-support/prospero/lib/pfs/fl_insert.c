/*
 * Copyright (c) 1989, 1990 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>
#include <stdio.h>

#include <pfs.h>
#include <perrno.h>

/*
 * fl_insert - Add a filter to a link
 *
 *             FL_INSERT takes a filter, and a link that is to receive
 *             the filter.  The filter is then appended to the linked list 
 *             of filters associated with the link. 
 *
 *    ARGS:    fil - Filter to be inserted, lin - Link to get filter
 *
 * RETURNS:    PSUCCESS - always
 */
int
fl_insert(fil,lin)
#if 0
    VLINK	fil;		/* Filter to be inserted             */
#endif
    FILTER      fil;
    VLINK	lin;		/* Link to receive filter            */

{
    APPEND_ITEM(fil, lin->filters);
#if 0
    VLINK	current;	/* To step through list		     */

    /* If this is the first filter in the link */
    if(lin->filters == NULL) {
        lin->filters = fil;
        fil->previous = NULL;
        fil->next = NULL;
        return(PSUCCESS);
    }

    /* Otherwise, find the last filter */

    current = lin->filters;

    while(current->next) current = current->next;

    /* insert the new filter */
    current->next = fil;
    fil->next = NULL;
    fil->previous = current;
#endif
    return(PSUCCESS);

}
