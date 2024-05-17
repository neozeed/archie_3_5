/*
 * Copyright (c) 1989, 1990 by the University of Washington
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#include <pfs.h>

/*
 * apply_filters - Apply filters to a directory NULL VERSION
 *
 *                 This was the null version of apply filters.  It is used
 *                 in those implementations that do not support filters.
 *
 *		Added a predefined filter, from server/list.c
 */
VDIR
dummy_filter(VDIR dir, TOKEN args)
{
	printf("Do dummy filter");
	return dir;
}

VDIR (*user_filter1)() = dummy_filter;
VDIR (*user_filter2)() = dummy_filter;
VDIR (*user_filter3)() = dummy_filter;
VDIR (*user_filter4)() = dummy_filter;
VDIR (*user_filter5)() = dummy_filter;

#define MAX_FILTER_ARGS 30		/* See note elsewhere about this*/

VDIR
apply_filters(dir,filters, dl, oa)
    VDIR	dir;
    FILTER	filters;
    VLINK       dl;
    int         oa;
{
    FILTER   	curfil;
    VDIR	result = dir;

    curfil = filters;

    while(curfil) {

#ifdef NEVER_DEFINED
	if (strequal(curfil->name, "XYZ") {
    	  char        *argarray[MAX_FILTER_ARGS];
    	  int argcount = 0;
          TOKEN argi;             /* argument index */
	  for (argi = curfil->args, argcount = 0; argi; argi = argi->next) {
            argarray[argcount++] = stcopy(argi->token);
            assert(argcount <= MAX_FILTER_ARGS);
          }
	  return(xyz_filter(dir,argcount,argarray);
	}
#endif

	if (strequal(curfil->name, "USER1")) 
		result = (user_filter1(result,curfil->args)) ;
	if (strequal(curfil->name, "USER2")) 
		result = (user_filter2(result,curfil->args)) ;
	if (strequal(curfil->name, "USER3")) 
		result = (user_filter3(result,curfil->args)) ;
	if (strequal(curfil->name, "USER4")) 
		result = (user_filter4(result,curfil->args)) ;
	if (strequal(curfil->name, "USER5")) 
		result = (user_filter5(result,curfil->args)) ;

	if (oa) 
		curfil = curfil->next;
	else 
		curfil = NULL;
    }
    if (result != dir) {
	vdir_copy(result,dir);
    };

    return(dir);
}



