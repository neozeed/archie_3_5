/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>
#include <stdio.h>

#include <pfs.h>

reorder_dir(dir)
    VDIR	dir;
    {
	VLINK	cl;
	VLINK   tl;
	
	cl = dir->links;

	while(cl) {
	    if((cl->linktype == 'U') || (cl->linktype == '-'))
		{
		    if(cl->previous == NULL) dir->links = cl->next;
		    else cl->previous->next = cl->next;
		    if(cl->next) cl->next->previous = cl->previous;
		}
	    tl = cl;
	    cl = cl->next;
	    if(tl->linktype == 'U') ul_insert(tl,dir,NULL);
	}

	cl = dir->ulinks;

	while(cl) {
	    if(cl->linktype != 'U')
		{
		    if(cl->previous == NULL) dir->ulinks = cl->next;
		    else cl->previous->next = cl->next;
		    if(cl->next) cl->next->previous = cl->previous;
		}
	    tl = cl;
	    cl = cl->next;
	    if((tl->linktype != 'U') && (tl->linktype != '-'))
		vl_insert(tl,dir,VLI_ALLOW_CONF);
	}
    }
