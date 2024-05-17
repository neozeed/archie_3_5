/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/file.h>
#include <sys/param.h>
#include <errno.h>

#include <pfs.h>
#include <pcompat.h>
#include <perrno.h>
#include <pmachine.h>

typedef char *(*STR_function)();

/*!! No way is this thread safe!!! */
STR_function filter;
STR_function select_filter();

/* Variables that may be accessed by the filter */
char	*FlT_htype = P_MACHINE_TYPE;     /* Current Host Type          */
char	*FlT_ostype = P_OS_TYPE;         /* Current Host Type          */

/*
 * apply_filters - Apply filters to a directroy
 *
 *	      APPLY_FILTERS takes a pointer to a directory, a list of
 *            filters to be to be applied to the directory.
 *            It also takes a pointer to the link to the directory
 *            which is made available to the filter.  The last
 *            argument specifies whether a single filter is to
 *            be applied, or all filters in a list.
 *
 *            The filters are then applied one by one
 *            to the directory, and the result returned.  Filters may
 *            be defined by the user.  The arguments may be modified,
 *            and the returned directory will be the same directory as
 *            passed as an argument. 
 *
 *      ARGS:   dir       - The directory to be filtered (will be modified)
 *              filters   - A list of the filters to be applied
 *              dl        - Directory link
 *              oa        - 0 = One, 1 = All
 *
 *  MODIFIES:   apply_filters might modify any of its arguments.
 *              'dir' is always modified to contain the result of
 *              applying the filters.  The return value is a pointer
 *              to dir.
 *
 *              apply_filters might also modify any of the following global
 *              variables:
 *
 *   FILTERS:   Have access to read or modify the arguments, the
 *              global variables listed above, and the following
 *              local variables:
 *              
 *
 *   RETURNS:   A pointer to the resulting directory.
 *              0 on failure with the error code in perrno.
 *
 *
 *      BUGS:   Doesn't trap failures in filters
 */

#define MAX_FILTER_ARGS     20  /* good round number for starters.  We should
                                   dynamically allocate the array instead, of
                                   course. */ 

VDIR
apply_filters(dir,filters,dl,oa)
    VDIR	dir;
    FILTER	filters;
    VLINK	dl;
    int		oa;
{
    VLINK   curfil;
    VDIR	result = dir;

    char	npath[MAXPATHLEN];
    char	*argarray[MAX_FILTER_ARGS];
    char	*miscarray[2];
    int	argcount = 0;
    int	retval;

    /* "" means don't load a symbol table */
    DISABLE_PFS(initialize_loader(""));   

    curfil = filters;

    while(curfil) {
        TOKEN argi;             /* argument index */

        assert(curfil->execution_location == FIL_CLIENT);
        assert(curfil->type == FIL_DIRECTORY || curfil->type == FIL_HIERARCHY);
        argarray[0] = NULL;

        /* Additional arguments to the filter */

        miscarray[0] = (char *) dl;
        miscarray[1] = (char *) filters;

        for (argi = curfil->args, argcount = 0; argi; argi = argi->next) {
            argarray[argcount++] = stcopy(argi->token);
            assert(argcount <= MAX_FILTER_ARGS);
        }
        
        retval = mapname(curfil,npath,MAP_READONLY);
        if(retval) {
            perrno = retval;
            return(0);
        }

        DISABLE_PFS(load_filter(npath));

	/* I'm amazed this works - we have a struc filter in pfs.h, 
	   and a variable filter hereh*/
        filter = select_filter("filter");
        result = (VDIR) filter(result,miscarray,argcount,argarray);

        if(oa) curfil = curfil->next;
        else curfil = NULL;
    }

    if(result != dir) vdir_copy(result,dir);

    reorder_dir(dir);
    return(dir);
}


