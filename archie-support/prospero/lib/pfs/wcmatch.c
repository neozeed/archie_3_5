/*
 * Copyright (c) 1989, 1990 by the University of Washington
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 *
 * Written  by bcn 1989     To match wildcarded strings
 * Modified by bcn 1993     To add support for regular expressions inside ()
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#include <pmachine.h>

#define TRUE 	1
#define FALSE   0

#ifdef NOREGEX
#include "../../misc/regex.c"
#endif
#include <string.h>

/* 
 * wcmatch - Match string s against template containing widlcards
 *
 *	     WCMATCH takes a string and a template, and returns
 *	     true if the string matches the template, and 
 *	     FALSE otherwise.
 *
 *    ARGS:  s        - string to be tested
 *           template - Template containing optional wildcards
 *
 * RETURNS:  TRUE (non-zero) on match.  FALSE (0) otherwise.
 *
 *    NOTE:  If template is NULL, will return TRUE.
 *
 */
char Empty_String[] = "";

int
wcmatch(char	*s,          /* String to be checked             */
	char	*template)   /* Wildcard string to check against */
{
    char	temp[200];
    char	*p = temp;

    if(!template) return(TRUE);
    if(!s) s = Empty_String; /* Work round segv's */

    if((*template == '(') && (*(template + strlen(template)-1) == ')')) {
	if(strcmp(s,template) == 0) return(TRUE);
	strncpy(temp,template+1,sizeof(temp)-1);
	temp[strlen(temp)-1] = '\0';
    }
    else {
	*p++ = '^';

	while(*template) {
	    if(*template == '*') {*(p++)='.'; *(p++) = *(template++);}
	    else if(*template == '?') {*(p++)='.';template++;}
	    else if(*template == '.') {*(p++)='\\';*(p++)='.';template++;}
	    else if(*template == '[') {*(p++)='\\';*(p++)='[';template++;}
	    else if(*template == '$') {*(p++)='\\';*(p++)='$';template++;}
	    else if(*template == '^') {*(p++)='\\';*(p++)='^';template++;}
	    else if(*template == '\\') {*(p++)='\\';*(p++)='\\';template++;}
	    else *(p++) = *(template++);
	}
	
	*p++ = '$';
	*p++ = '\0';
	
    }
    
    return(p__re_comp_exec(temp,s));           /* Thread safe version */
    
}

