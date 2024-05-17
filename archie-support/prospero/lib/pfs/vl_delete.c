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

#define VLD_NOT_FOUND 1

/*
 * vl_delete
 *
 *  RETURNS: Pointer to the modified list, or NULL on error
 */
VLINK
vl_delete(vll,lname,number)
    VLINK	vll;		/* List from which link to be removed     */
    char	*lname;		/* Name of the link to remove             */
    int		number;		/* The 1+number of matching links to skip */
    {
	VLINK	cl = vll;	/* The current link                       */

	number--;		/* Turn it into number to be skipped      */

	perrno = 0;

	if(lname == NULL) lname = "";

	while(cl) {
	    if((!strcmp(lname,cl->name)) && (number-- == 0)) {
		if(cl->previous == NULL) {
		    cl = cl->next;
		    /* should really punt the original cl */
		    vlfree(vll);
		    if(cl) cl->previous = NULL;
		    return(cl);
		}
		cl->previous->next = cl->next;
		if(cl->next) cl->next->previous = cl->previous;
		vlfree(cl);
		return(vll);
	    }
    	    cl = cl->next;
	}
	perrno = VLD_NOT_FOUND;
	return(NULL);
    }
