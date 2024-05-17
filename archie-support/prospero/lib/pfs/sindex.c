/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <stdio.h>
#include <string.h>

#include <pmachine.h>

#include <pfs.h>

/*
 * sindex - Find first instance of string 2 in string 1 
 *
 *	      SINDEX scans string 1 for the first instance of string
 *	      2.  If found, SINDEX returns a pointer to the first
 *	      character of that instance.  If no instance is found, 
 *	      SINDEX returns NULL (0).
 *
 *    ARGS:   s1 - string to be searched
 *            s2 - string to be found
 * RETURNS:   First instance of s2 in s1, or NULL (0) if not found
 */
const char *
sindex(const char *s1  /* String to be searched   */,
       const char *s2  		/* String to be found */)
{
    const char	*s = s1;	/* Temp pointer to string  */
    
    /* Check for first character of s2 */
    while((s = strchr(s,*s2)) != NULL) {
        if(strncmp(s,s2,strlen(s2)) == 0)
            return(s);
        s++;
    }
    
    /* We didn't find it */
    return(NULL);
}

