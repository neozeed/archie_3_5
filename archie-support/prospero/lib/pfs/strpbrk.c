/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>

#ifndef NULL
#define NULL 0
#endif 

/*
 * strpbrk - Find first instance of character from chrs in s
 *
 *	      SINDEX scans a string for the first instance of a character
 *            from chrs.  If found, STRPBRK returns a pointer to the
 *	      character in s.  If no instance is found STRPBRK returns 
 *            NULL (0).
 *
 *    ARGS:   s    - string to be searched
 *            chrs - string of characters we are looking for
 * RETURNS:   First instance of chrs in s, or NULL (0) if not found
 */
char *
strpbrk(s,chrs)
    char	*s;    /* String to search                         */
    char	*chrs; /* String of characters we are looking for  */
    {
	char	*cp;   /* Pointer to the current character in chrs */
	
	while(*s) {
	    for(cp = chrs;*cp;cp++)
		if(*cp == *s) return(s);
	    s++;
	}
	return(NULL);
    }
