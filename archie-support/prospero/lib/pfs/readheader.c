/*
 * Copyright (c) 1989, 1990 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>
#include <stdio.h>
#include <string.h>
#include <pfs.h>
#include <pmachine.h>

/*
 * readheader will read the f until it comes across a line with 
 * header whose name ends in h.  It will then return the next token
 * on the same line.
 *
 * IMPORTANT: The returned value is static.  It must be copied if it is
 * to be used after the next call to readheader.
 *
 * BUGS: The present implementation only looks for the first occurance of the
 * last character in h.  This may cause problems if the character is not
 * unique.  Ideally it should be something like a ":".
 */
char *readheader(f,h)
    FILE	*f;
    char	*h;
    {
	static char	hv[MAX_VPATH];

	char		temp[MAX_VPATH+40];
	char		*p;
	
	int		hl = strlen(h);     /* Length of h    */
	char		*he = h + hl - 1;   /* Last char in h */

    assert(P_IS_THIS_THREAD_MASTER());
	while(fgets(temp,sizeof(temp),f)) {
	    /* Note, for now, we only check the first occurance of */
	    /* the last character.  It had better be a uniqe one   */
	    /* such as a ":".                                      */
	    if(p = strchr(temp + hl - 1, *he)) {
		if(strcncmp(p-hl+1,h,hl) == 0) {
		    sscanf(++p,"%s",hv);
		    return(hv);
		}
	    }
	}

	return(NULL);
    }
