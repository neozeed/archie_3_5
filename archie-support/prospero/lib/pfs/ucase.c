/*
 * Copyright (c) 1989 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>
#include <stdio.h>

/*
 *    ucase - Change the case of a string to upper case
 *
 *            UCASE takes a string as an arugment and changes all
 *	      lowercase characters in the string to upper case.
 *	      The argument is modified
 *
 *     ARGS:   s - String to be modified
 * MODIFIES:   s
 *  RETURNS:   0 (always)
 */
int
ucase(s)
    char	*s;	/* String to have case changed */
    {
	while (*s != '\0')
		{
		    if (('a' <= *s) && (*s <= 'z'))
			*s &= 0xdf; /* Make upper case */
		    s++;
		}
	return(0);
    }
	
