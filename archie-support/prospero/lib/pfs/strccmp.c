/*
 * Copyright (c) 1989, 1990 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define toupper(x) (islower(x) ? (x & 0xdf) : x)

/*
 * strccmp - Compare two strings ignoring case
 *
 *	      STRCCMP compare two strings ignoring case.  It
 *            returns 0 if the strings are equal, negative is
 *            string 1 is less than string 2, and positive
 *            if string 1 is greater than string 2.
 *    ARGS:   s1 - first string
 *            s2 - second string
 *
 * RETURNS:   +,- or 0 depending on comparison
 */
int
strccmp(s1,s2)
    char	*s1;		/* First String            */
    char	*s2;		/* Second String           */
    {
	int	tmp;

	while ((*s1 != '\0') && (*s2 != '\0')) {
	    tmp = toupper(*s1) - toupper(*s2);
	    if(tmp) return (tmp);
	    s1++; s2++;
	}
	return(toupper(*s1) - toupper(*s2));
    }	    

/*
 * strcncmp - Compare two strings ignoring case and stoping after n chars
 *
 *	      STRCCMP compare two strings ignoring case.  It
 *            returns 0 if the strings are equal, negative is
 *            string 1 is less than string 2, and positive
 *            if string 1 is greater than string 2.
 *    ARGS:   s1 - first string
 *            s2 - second string
 *            n  - number of characters to check
 *
 * RETURNS:   +,- or 0 depending on comparison
 */
int
strcncmp(s1,s2,n)
    char	*s1;		/* First String            */
    char	*s2;		/* Second String           */
    int		n;              /* Number of chars         */
    {
	int	tmp;

	while ((n-- > 0) && (*s1 != '\0') && (*s2 != '\0')) {
	    tmp = toupper(*s1) - toupper(*s2);
	    if(tmp) return (tmp);
	    s1++; s2++;
	}

	if(n >= 0) return(toupper(*s1) - toupper(*s2));
	return(0);
    }	    



