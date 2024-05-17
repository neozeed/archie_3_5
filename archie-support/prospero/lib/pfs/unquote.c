/*
 * Copyright (c) 1989, 1990 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>
#include <stdio.h>
#include <string.h>
#include <pfs_threads.h>


/*
 * unquote - unquote string if necessary
 *
 *	      UNQUOTE takes a string and unquotes it if it has been quoted.
 *
 *    ARGS:   s - string to be unquoted
 *            
 * RETURNS:   The original string.  If the string has been quoted, then the
 *            result appears in static storage, and must be copied if 
 *            it is to last beyond the next call to quote.
 *
 */

/* This code exists only for the sake of server/dirsrv_v1.c,
   server/shadowcvt.c, and lib/psrv/dsdir_v0.c. */
   
char *
unquote(s)
    char	*s;		/* String to be quoted */
{
    AUTOSTAT_CHARPP(unquotedp);
    char		*c = *unquotedp;
    
    if(*s != '\'') return(s);
    
    s++;
    
    /* This should really treat a quote followed by other */
    /* than a quote or a null as an error                 */
    while(*s) {
        if(*s == '\'') s++;
        if(*s) *c++ = *s++;
    }
    
    *c++ = '\0';
    
    return(*unquotedp);
}

/*
 * unquoten - unquote string if necessary and return NULL for empty string
 *
 *	      UNQUOTEN takes a string and unquotes it if it has been quoted.
 *            If the string is the emtpy string, or the quoted empty string
 *            UNQUOTEN returns NULL.
 *
 *    ARGS:   s - string to be unquoted
 *            
 * RETURNS:   The original string.  If the string has been quoted, then the
 *            result appears in static storage, and must be copied if 
 *            it is to last beyond the next call to unquote() or unquoten().
 *
 */
char *
unquoten(s)
    char	*s;		/* String to be quoted */
    {
	char		*u;

	if(!s || !*s) return(NULL);

	u = unquote(s);

	if(!*u) return(NULL);
	else return(u);
    }

