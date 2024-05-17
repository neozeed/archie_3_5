#define IETFTYPEFILE "/usr/local/lib/ietftypes"

#include <stdio.h>
#include <pfs.h>
#include "ietftype.h"
#include <string.h>
#include <pfs_threads.h>
#include <mitra_macros.h>

/* Shared across threads */
IETFTYPE IETFTypes = NULL;
void ietftype_init();

/* Return the IETF type corresponding to the waistype - or NULL if not found*/
IETFTYPE wais_2_ietftype(char *waistype)
{
	IETFTYPE	thistype;

	if (IETFTypes == NULL) {
#ifndef PFS_THREADS
/* In threaded version, this is done at init time */
		ietftype_init();
		if (IETFTypes == NULL) 
#endif
			return NULL;
	}
	
	thistype = IETFTypes;
	TH_FIND_STRING_LIST_CASE(thistype, waistype, waistype, IETFTYPE);
	/* temp = NULL or ietftype to return */
	return (thistype);
}

/* Open file of types and initialise data structure */
void ietftype_init() {
    FILE	*st;
    IETFTYPE	thistype = NULL;
    char	ourlinebuf[1024];
    int		i;
    char	*tag = NULL;
    char	*value = NULL;
    char	*cp;

    assert(P_IS_THIS_THREAD_MASTER());

    if ((st = fopen(IETFTYPEFILE, "r"))  == NULL)        /* Not locked_fclose - before mutexes */
	/* Should set an error string */
	return;

    while ((cp = fgets(ourlinebuf, sizeof(ourlinebuf), st)) != NULL) {
	if (ourlinebuf[0] == '\n') 
		continue;
	if (ourlinebuf[0] == '#') {	/* Seperator  & comment*/
		thistype = NULL;
		continue;
	}
	if (thistype == NULL) {
		thistype = ietftype_alloc();
		APPEND_ITEM(thistype, IETFTypes);
	}
	/*Would be nice if could skip trailing white space in qsscanf*/
	for (	i = strlen(ourlinebuf) - 1;
		strchr(" \t\n\r",ourlinebuf[i]);
		i--)
	    ourlinebuf[i] = '\0';

	if (qsscanf(ourlinebuf, "%&[^:= \t]%*[:= \t]%r",&tag,&value) == 2) {
#define tt1(tgstr,field)  						\
		if (stcaseequal(tag,tgstr)) {				\
		  thistype->field = stcopyr(value,thistype->field);	\
		  continue;						\
		}

	    tt1("ietf",standardtype);
	    tt1("wais",waistype);
	    if (stcaseequal(tag,"prospero")) {
	    	PATTRIB at=ietftype_atput(thistype,"OBJECT-INTERPRETATION");
		assert(P_IS_THIS_THREAD_MASTER());
		for(cp=strtok(value,"/");cp != NULL; cp=strtok(NULL,"/"))
			tkappend(cp,at->value.sequence);
	    }
	    /* Add other fields here as we need them */
	    /* Ignores unrecognized fields */
	}
	/* Ignores syntactically incorrect lines */
    }
	stfree(tag);
    fclose(st);
}

