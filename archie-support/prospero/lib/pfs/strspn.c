/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>

/*
 * strspn - Count initial characters from chrs in s
 *
 *	      STRSPN counts the occurances of chacters from chrs
 *            in the string s preceeding the first occurance of
 *            a character not in s.
 *
 *    ARGS:   s    - string to be checked
 *            chrs - string of characters we are looking for
 *
 * RETURNS:   Count of initial characters from chrs in s
 */
int
strspn(s,chrs)
    char	*s;    /* String to search                         */
    char	*chrs; /* String of characters we are looking for  */
    {
	char	*cp;   /* Pointer to the current character in chrs */
	int	count; /* Count of characters seen so far          */
	
	count = 0;

	while(*s) {
	    for(cp = chrs;*cp;cp++)
		if(*cp == *s) {
		    s++;
		    count++;
		    goto done;
		}
	    return(count);
	done:
	    ;
	}
	return(count);
    }
