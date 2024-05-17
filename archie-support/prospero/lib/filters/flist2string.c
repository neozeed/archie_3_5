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
#include <perrno.h>

#ifdef NEVERDEFINED_NOTUSED
char *
flist2string(flist)
    VLINK	flist;
    {
	static char	fstring[1000];
	char	*p = fstring;

	*p = '\0';
	
	while(flist) {
	    sprintf(p,"{%c,%s,%s,%s,%s,%s}",flist->linktype,flist->hosttype,
		    flist->host,flist->hsonametype,flist->hsoname,flist->args);
	    p += strlen(p);
	    flist = flist->next;
	}

	return(fstring);

    }
#endif /*NEVERDEFINED*/
#ifdef NEVERDEFINEDBUGGYASANYTHING
VLINK
fstring2list(st)
    char	*st;
    {
	VLINK	flist = NULL;
	VLINK	fl = NULL;
	VLINK   nf;

	char	*sp = st;
	
	while(*sp) {		/* This is bizarre sp never changes!! */
	    nf = vlalloc();
	    if(fl) fl->next = nf;
	    else flist = nf;
	    fl = nf;

	    sscanf(sp,"{%c,%s,%s,%s,%s, %[^}]",&(fl->linktype),fl->hosttype,
		    fl->host,fl->hsonametype,fl->hsoname,fl->args);
	    flist = flist->next;
	}

	return(flist);		/* Also bizarre - always null!! */

    }
#endif /*NEVERDEFINED*/
