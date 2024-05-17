/*
 * Copyright (c) 1991 by the University of Washington
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#define ABSOLUTE_MAX_HITS 2500
#define ABSOLUTE_MAX_GIF  100

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <stdio.h>
#include <sgtty.h>
#include <string_with_strcasecmp.h>

/* Archie defines */
#include <defines.h>
#include <structs.h>
#include <error.h>
#include <database.h>

#include "prarch.h"

#include <pserver.h>
#include <pfs.h>
#include <ardp.h>
#include <psrv.h>
#include <plog.h>
#include <pprot.h>
#include <perrno.h>
#include <pmachine.h>

int		archie_supported_version = 2;

extern char	hostname[];
extern char	hostwport[];
char		archie_prefix[] = "ARCHIE";
static int num_slashes(char *s);
static int tkllength(TOKEN tkl);
/*
 * dsdb - Make a database query as if it were a directory lookup
 *
 */
arch_dsdb(RREQ	req,           /* Request pointer                           */
	  char	*name,         /* Name of the directory                     */
	  char	**componentsp, /* Next component of name                    */
	  TOKEN	*rcompp,       /* Additional components                     */
	  VDIR	dir,           /* Directory to be filled in                 */
	  int	options,       /* Options to list command                   */
	  const char *rattrib, /* Requested attributes                      */
	  FILTER filters)       /* Filters to be applied                     */
{
    /* Note that componentspp and rcompp are pointers to */
    /* pointers.  This is necessary because    */
    /* this routine must be able to update these values  */
    /* if more than one component of the name is         */
    /* resolved.                                         */
    char 	*components = NULL;
    int		num_unresolvedcomps = 0;
    VLINK	cur_link = NULL;
    char	newdirname[MAXPATHLEN];
    static int	dbopen = 0;
    char	fullquery[MAXPATHLEN];
    char	*dbpart;
    char	dbquery[MAXPATHLEN];
    char	dbargs[MAXPATHLEN];
    char	dbarg1[MAXPATHLEN];
    char	dbarg2[MAXPATHLEN];
    char	dbarg3[MAXPATHLEN];
    char	dirlinkname[MAXPATHLEN];
    char	sep;
    char	*firstsep;
    int		tmp;
    VLINK	dirlink = NULL;
    TOKEN       tkl_tmp;

    /* Make sure NAME, COMPONENTSP, and RCOMPP arguments are correct. */

    /* Name components with slashes in them are malformed inputs to the
       ARCHIE database. */ 
    if(componentsp && (components = *componentsp)) {
        if(index(components, '/')) 
            RETURNPFAILURE;
        for (tkl_tmp = *rcompp; tkl_tmp; tkl_tmp = tkl_tmp->next)
            if (index(tkl_tmp->token, '/'))
                RETURNPFAILURE;
    } else {
        if (*rcompp) RETURNPFAILURE; /* ridiculous to specify additional comps
                                         and no initial comps.*/
    }

    /* Directory already initialized, but note that this */
    /* is not a real directory                           */
    dir->version = -1;
    dir->inc_native = 3;	   /* Not really a directory */

    /* Note that if we are resolving multiple components */
    /* (rcomp!=NULL) the directory will already be empty */
    /* since had anything been in it dirsrv would have   */
    /* already cleared it and moved on to the next comp  */

    /* Do only once */
    if(!dbopen++) {
	set_default_dir(DEFAULT_DBDIR);
	if((tmp = open_db_files(DB_RDONLY)) != A_OK) {
	    dbopen = 0;
	    plog(L_DB_ERROR,NOREQ,"Can't open archie database",0);
	    RETURNPFAILURE;
	}
    }

    /* For now, if only verifying, indicate success */
    /* We don't want to do a DB search.  Eventually */
    /* we might actually check that the directory   */
    /* is valid.                                    */
    if(options&DSDB_VERIFY) return(PSUCCESS);
    
    /* Construct the full query from the pieces passed to us */
    tmp = -1 + qsprintf(fullquery,sizeof fullquery, "%s%s%s",name,
                        ((components && *components) ? "/" : ""),
                        ((components && *components) ? components : ""));
    for (tkl_tmp = *rcompp; tkl_tmp; tkl_tmp = tkl_tmp->next)
        tmp += -1 + qsprintf(fullquery + tmp, sizeof fullquery - tmp, 
                             "/%s", (*rcompp)->token);
    if (tmp + 1 > sizeof fullquery) return DSRDIR_NOT_A_DIRECTORY;
    
    /* The format for the queries is            */
    /* DATABASE_PREFIX/COMMAND(PARAMETERS)/ARGS */
    
    /* Strip off the database prefix */
    dbpart = fullquery + strlen(archie_prefix);

    /* And we want to skip the next slash */
    dbpart++;
    
    /* Find the query (up to the next /), determine if the */
    /* / exists and then read the args                     */
    tmp = sscanf(dbpart,"%[^/]%c%s",dbquery,&sep,dbargs);
    
    /* If no separator, for now return nothing         */
    /* Eventually, we might return a list of the query */
    /* types supported                                 */
    if(tmp < 2) return(PSUCCESS);
    
    /* Check query type */
    if(strncmp(dbquery,"MATCH",5)==0) {
	char	stype = 'R';     /* search type           */
	int	maxthit = 100;   /* max entries to return */
	int	maxmatch = 100;  /* max strings to match  */
	int	maxhitpm = 100;  /* max hits per match    */
	int	offset = 0;      /* entries to skip       */
	search_sel method;	 /* Search method         */
	int	onlystr = 0;	 /* Just return strings   */
	
        /* In the MATCH querytype, the directory part of the query (the
           argument named NAME) may have no more than 3 components.  
           There are 3 possible formats:
           1) DATABASE_PREFIX (one component)
           2) (1)/MATCH(...)
           3) (2)/query-term (3 total components)
           */
	if (num_slashes(name) > 2) return DSRDIR_NOT_A_DIRECTORY;
	/* if no strings to match, return nothing */
	if(tmp < 3) return(PSUCCESS);
	
	/* Get arguments */
	tmp = sscanf(dbquery,"MATCH(%d,%d,%d,%d,%c",&maxthit,
		     &maxmatch,&maxhitpm,&offset,&stype);
	
	if(tmp < 3) {
	    sscanf(dbquery,"MATCH(%d,%d,%c",&maxthit,&offset,&stype);
	    maxmatch = maxthit;
	    maxhitpm = maxthit;
	}
	/* Note: in maxhits, 0 means use default, -1 means use max */
	
	/* Don't let the user request more than ABSOLUTE_MAX_HITS */
	if((maxthit > ABSOLUTE_MAX_HITS) || (maxthit < 1)) {
	    p_err_string = qsprintf_stcopyr(p_err_string,
	   	"Legal values for max hits are between 1 and %d ",
		ABSOLUTE_MAX_HITS);
	    return(DIRSRV_NOT_AUTHORIZED);
	}
	if(maxthit == 0) maxthit = ABSOLUTE_MAX_HITS;
	
	switch(stype) {
	case '=':
	    onlystr = 0;
	    method = S_EXACT ;
	    break;
	case 'C':
	    onlystr = 0;
	    method = S_SUB_CASE_STR ;
	    break;
	case 'c':
	    onlystr = 0;
	    method = S_E_SUB_CASE_STR ;
	    break;
	case 'K':
	    onlystr = 1;
	    method = S_SUB_CASE_STR ;
	    break;
	case 'k':
	    onlystr = 1;
	    method = S_E_SUB_CASE_STR ;
	    break;
	case 'R':
	    onlystr = 0;
	    method = S_FULL_REGEX ;
	    break;
	case 'r':
	    onlystr = 0;
	    method = S_E_FULL_REGEX ;
	    break;
	case 'X':
	    onlystr = 1;
	    method = S_FULL_REGEX ;
	    break;
	case 'x':
	    onlystr = 1;
	    method = S_E_FULL_REGEX ;
	    break;
	case 'z':
	    onlystr = 1;
	    method = S_E_SUB_NCASE_STR ;
	    break;
	case 'Z':
	    onlystr = 1;
	    method = S_SUB_NCASE_STR ;
	    break;
	case 's':
	    onlystr = 0;
	    method = S_E_SUB_NCASE_STR ;
	    break;
	case 'S':
	default:
	    onlystr = 0;
	    method = S_SUB_NCASE_STR ;
	    break;
	}
	
	*dbarg1 = *dbarg2 = *dbarg3 = '\0';
	
	tmp = sscanf(dbargs,"%[^/]%c%[^/]%c%s",dbarg1,&sep,dbarg2,
		     &sep,dbarg3); 
	
	if(tmp < 2) {
	    /* This specifies a directory, but not a link within it  */
	    /* create a pseudo directory and return a pointer        */
            /* In other words, listing a MATCH directory by itself yields
               an empty directory. */
	    if(*dbarg1 && (strcmp(dbarg1,"*")!= 0)) {
		dirlink = vlalloc();
		dirlink->target = stcopyr("DIRECTORY",dirlink->target);
		dirlink->name = stcopyr(dbarg1,dirlink->name);
		dirlink->host = stcopyr(hostwport,dirlink->host);
		sprintf(dirlinkname,"%s/%s/%s",archie_prefix,dbquery,dbarg1);
		dirlink->hsoname = stcopyr(dirlinkname,dirlink->hsoname);
		vl_insert(dirlink,dir,VLI_ALLOW_CONF);
	    }
	}
	else {
	    if(tmp > 4) {
		/* There are remaining components */
		num_unresolvedcomps = num_slashes(dbarg3);
	    }
#ifdef ABSOLUTE_MAX_GIF
	    /* If looking for GIF files (arrgh) don't allow them */
	    /* to set an unreasonable number of hits, this is    */
	    /* promted by someone who set max hits to 10,000     */
	    if((maxthit+offset > ABSOLUTE_MAX_GIF)&&(((strlen(dbarg1) >= 4)&&
		      (strcasecmp(dbarg1+strlen(dbarg1)-4,".gif") == 0)) ||
  		      (strcasecmp(dbarg1,"gif") == 0))) {
		p_err_string = qsprintf_stcopyr(p_err_string,
"Max hits for GIF searches is %d - See archie/doc/giflist.Z on \
archie.mcgill.ca for full gif list",ABSOLUTE_MAX_GIF);
		return(DIRSRV_NOT_AUTHORIZED);
	    }
#endif ABSOLUTE_MAX_GIF
	    
	    tmp = prarch_match(dbarg1,maxthit,maxmatch,maxhitpm,
			       offset,method,dir,FALSE,onlystr);
	    if(tmp) RETURNPFAILURE;
	}
    }
    else if (strncmp(dbquery,"HOST",4)==0) {
	/* First component of args is the site name    */
	/* remaining components are the directory name */
	
	*dbarg1 = *dbarg2 = '\0';
	
	tmp = sscanf(dbargs,"%[^/]%c%s",dbarg1,&sep,dbarg2);
	
	/* If first component is null, return an empty directory */
	if(tmp < 1) return(PSUCCESS);
	
	/* if first component exists, but is last component, */
	/* then it is the name of the subdirectory for the   */
	/* host, create a pseudo directory and return a      */
	/* pointer, If first component is a wildcard, and no */
	/* additional components, then return matching list  */
	/* of sites.                                         */
	if(tmp == 1) {
	    tmp = prarch_host(dbarg1,NULL,dir,A2PL_ARDIR);
	    if(tmp == PRARCH_TOO_MANY) return(DIRSRV_TOO_MANY);
	    if(tmp) return(tmp);
	}
	/* More than one component, Look up the requested directory  */
	/* Note that the since the full query is passed to us, it    */
	/* includes the component name, thus the directory name is   */
	/* what you get when you strip off the last component of the */
	/* name                                                      */
	else {
	    char *lastsep = rindex(dbarg2,'/');
	    if(lastsep) *lastsep++ = '\0';
	    else *dbarg2 = '\0';
	    tmp = prarch_host(dbarg1,dbarg2,dir,A2PL_ARDIR);
	    if(tmp == PRARCH_DONT_HAVE_SITE) 
		return(DSRDIR_NOT_A_DIRECTORY);
	    if(tmp) RETURNPFAILURE;
	}
    }
    else {
	/* Query type not supported */
	return(DSRDIR_NOT_A_DIRECTORY);
    }
    
    /* We are done, but we need to figure out if we resolved multiple
       components and reset *componentsp and *rcompp appropriately. */ 
    
    if (num_unresolvedcomps) {
        int skip = tkllength(*rcompp) - num_unresolvedcomps;
        if (skip < 0) return DSRDIR_NOT_A_DIRECTORY; /* shouldn't happen. */
        while(skip-- > 0) {
            assert(*rcompp);
            *componentsp = (*rcompp)->token;
            *rcompp = (*rcompp)->next;
        }
    } else {
        while (*rcompp) {
            *componentsp = (*rcompp)->token;
            *rcompp = (*rcompp)->next;
        }
    }
    return(PSUCCESS);
}

static int
tkllength(TOKEN tkl)
{
    int retval = 0;
    for (;tkl; tkl = tkl->next)
        ++retval;
    return retval;
}


static
int
num_slashes(char *s)
{
    int retval = 0;
    for (; *s; ++s) {
        if (*s == '/') 
            ++retval;
    }
    return retval;
}

