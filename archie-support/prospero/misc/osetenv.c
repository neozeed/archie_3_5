/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>

static setenv(n,v,o)
    char	*n;
    char	*v;
    int		o;
    {
	int	tmp;

	/* Allocate space for environment variable */
	char	*template = (char *) stalloc(strlen(n)+strlen(v)+2);
	
	sprintf(template,"%s=%s",n,v);
	tmp = putenv(template);
	return(tmp);

	/* Potential memory leak - it is not clear whether putenv     */
	/* deallocates the string with the old value of the           */
	/* envorinment variable. If not, then we should do so here.   */

    }
    
