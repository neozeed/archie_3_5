/*
 * aq_query.c : Programmatic Prospero interface to Archie
 *
 * Copyright (c) 1991 by the University of Washington
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 *
 * Originally part of the Prospero Archie client by Clifford 
 * Neuman (bcn@isi.edu).  Modifications, early programmatic interface,
 * and new sorting code by George Ferguson (ferguson@cs.rochester.edu) 
 * and Brendan Kehoe (brendan@cs.widener.edu).
 *
 *  - bcn 08/20/91 - make it do it properly (new invdatecmplink)
 *  - bpk 08/20/91 - made sorting go inverted as we purport it does
 */

#include "uw-copyright.h"
#include "usc-copyr.h"

#include <stdio.h>
#ifndef SOLARIS
# include <strings.h>			/* for char *index() */
#else
# include <string.h>
#endif

#include "pfs.h"
#include "perrno.h"
#include "pmachine.h"
#include "parchie.h"
#include "all.h"


extern char		*p_motd;
extern int		ardp_port;

/*
 * aq_query :     Sends a request to _host_, telling it to search for
 *                _string_ using _query_type_ as the search method.
 *                No more than _max_hits_ matches are to be returned
 *                skipping over _offset_ matches.
 *
 *		  aq_query returns a linked list of virtual links. 
 *                If _flags_ does not include AQ_NOTRANS, then the Archie
 *                responses will be translated. If _flags_ does not include 
 *                AQ_NOSORT, then the list will be sorted using _cmp_proc_ to
 *                compare pairs of links.  If _cmp_proc_ is NULL or AQ_DEFCMP,
 *                then the default comparison procedure, defcmplink(), is used
 *                sorting by host, then filename. If cmp_proc is AQ_INVDATECMP
 *                then invdatecmplink() is used, sorting inverted by date.
 *                otherwise a user-defined comparison procedure is called.
 *
 *                aq_query returns NULL and sets perrno if the query
 *                failed. Note that it can return NULL with perrno == PSUCCESS
 *                if the query didn't fail but there were simply no matches.
 *
 * 	   args:  See archie.h for form of the aquery structure
 *
 *        flags:  AQ_NOSORT      Don't sort results
 *                AQ_ASYNC	 
 */
int aq_query(struct aquery *aq,int flags)
{
  char	qstring[MAX_VPATH];     /* For construting the query  */
  VDIR	dir = &(aq->dirst);     /* Directory for get_vdir     */
  int		gvdflags;
	
  VLINK	p,q,r,lowest,nextp,pnext,pprev;
  char	*components = "";
  int		tmp;

  if(aq->status == AQ_ACTIVE) {
    if((flags&AQ_ASYNC) == 0) aq->flags &= (~AQ_ASYNC);
    goto cont_async;
  }

  aq->status = AQ_ACTIVE;
  aq->flags = flags;
  vdir_init(dir);

  /* Set the cmp_proc if not given */
  if (aq->cmp_proc == NULL) aq->cmp_proc = aq_defcmplink;

  /* Make the query string */
  if(aq->query_type > ' ') {
    sprintf(qstring,"ARCHIE/MATCH(%d,%d,%d,%d,%c)/%s",
            aq->maxhits, aq->maxmatch, aq->maxhitpm, aq->offset,
            aq->query_type, aq->string);
  }
  else
  {
    switch (aq->query_type)
    {
    case AQ_HOSTINFO:
      strcpy(qstring,"ARCHIE/HOST");
      components = aq->string;
      break;

    case AQ_SITELIST: /* wheelan */
      sprintf(qstring,"ARCHIE/HOST/%s", aq->string); /* has () around it */
      break;
      
    case AQ_WHATIS: /* wheelan */
      sprintf(qstring, "ARCHIE/WHATIS/%s", aq->string);
      break;

    case AQ_DOMAIN: /* wheelan */
      sprintf(qstring, "ARCHIE/DOMAIN/%s", aq->string);
      break;

    case AQ_RE_DOMAIN: /* wheelan */
      sprintf(qstring, "ARCHIE/DOMAIN/(%s)", aq->string);
      break;

    case AQ_GENERIC: /* wheelan */
      sprintf(qstring, "ARCHIE/FIND/(%d,%d)/%s/%s", aq->maxhits, aq->maxhits,
              aq->dbs, aq->string);
      break;

    case AQ_MOTD_ONLY:
      qstring[0] = '\0';
      break;

    default:
      break; /* previous code didn't catch this either */
    }
  }
  
  /* Initialize Prospero structures */
  perrno = 0; *p_err_string = '\0';
	
  if(aq->query_type == AQ_EXP_LINK) aq->dirquery = aq->expand;
  else {
    aq->dirquery = vlalloc();
    aq->dirquery->host = stcopyr(aq->host,aq->dirquery->host);
    aq->dirquery->hsoname = stcopyr(qstring,aq->dirquery->hsoname);
    aq->dirquery->filters = aq->filters;
  }

cont_async:

  gvdflags = GVD_ATTRIB|GVD_NOSORT;
  if(aq->flags & AQ_MOTD) gvdflags |= GVD_MOTD;
  if(aq->query_type == AQ_MOTD_ONLY) gvdflags |= GVD_MOTD_O;
  if(aq->flags & AQ_ASYNC) gvdflags |= GVD_ASYNC;

  /* Retrieve the list of matches, return error if there was one */
  tmp = get_vdir(aq->dirquery,components,dir,gvdflags,NULL);

  if(tmp == DQ_ACTIVE) {
    aq->wait_fd = ardp_port;
    aq->qpos = dir->dqs->preq->inf_queue_pos;
    aq->sys_time = dir->dqs->preq->inf_sys_time;
    aq->retry_at.tv_sec = dir->dqs->preq->wait_till.tv_sec;
    aq->retry_at.tv_usec = dir->dqs->preq->wait_till.tv_usec;
    if(!aq->motd && p_motd) aq->motd = stcopyr(p_motd,aq->motd);
    return(AQ_ACTIVE);
  }

  if(aq->query_type != AQ_EXP_LINK) {
    aq->dirquery->filters = NULL; /* Keep them on aq->filters */
    vlfree(aq->dirquery); 
    aq->dirquery = NULL;
  }

  if(p_motd) aq->motd = stcopyr(p_motd,aq->motd);

  if(tmp) return(aq->status = tmp);

  /* Save the links, and clear in dir in case it's used again   */
  aq->results = dir->links; dir->links = NULL;

  /* As returned, list is sorted by suffix, and conflicting     */
  /* suffixes appear on a list of "replicas".  We want to       */
  /* create a one-dimensional list sorted by host then filename */
  /* and maybe by some other parameter                          */

  /* First flatten the doubly-linked list */
  for (p = aq->results; p != NULL; p = nextp) {
    nextp = p->next;
    if (p->replicas != NULL) {
	    p->next = p->replicas;
	    p->next->previous = p;
	    for (r = p->replicas; r->next != NULL; r = r->next)
      /*EMPTY*/ ;
	    r->next = nextp;
	    nextp->previous = r;
	    p->replicas = NULL;
    }
  }

  /* If NOSORT given, then just hand it back */
  if (aq->flags & AQ_NOSORT) {
    perrno = PSUCCESS;
    aq->status = AQ_COMPLETE;
    return(PSUCCESS);
  }

  /* Otherwise sort it using a selection sort and the given cmp_proc */
  for (p = aq->results; p != NULL; p = nextp) {
    nextp = p->next;
    lowest = p;
    for (q = p->next; q != NULL; q = q->next)
    if ((*(aq->cmp_proc))(q,lowest) < 0) lowest = q;
    if (p != lowest) {
	    /* swap the two links */
	    pnext = p->next;
	    pprev = p->previous;
	    if (lowest->next != NULL)
      lowest->next->previous = p;
	    p->next = lowest->next;
	    if (nextp == lowest) {
        p->previous = lowest;
	    } else {
        lowest->previous->next = p;
        p->previous = lowest->previous;
	    }
	    if (pprev->next != NULL)
      pprev->next = lowest;
	    if (nextp == lowest) {
        lowest->next = p;
	    } else {
        pnext->previous = lowest;
        lowest->next = pnext;
	    }
	    if(lowest == pprev) lowest->previous = p;
	    else lowest->previous = pprev;
	    /* keep the head of the list in the right place */
	    if (aq->results == p) aq->results = lowest;
    }
  }
    
  /* Return the links */
  perrno = PSUCCESS;
  aq->status = AQ_COMPLETE;
  return(PSUCCESS);
}


/*
 * aq_defcmplink: The default link comparison function for sorting. Compares
 *	         links p and q first by host then by filename. Returns < 0 if p
 *               belongs before q, > 0 if p belongs after q, and == 0 if their
 *               host and filename fields are identical.
 */
int aq_defcmplink(p,q)
    VLINK p,q;
{
	int result;
	char *phost, *qhost;
	char *phson, *qhson;
	char phostbuf[100],qhostbuf[100];

#ifdef AQFASTCOMP
	/* This is designed to be a fast check for EXTERNAL, but */
	/* not necessarily correct.  It will break if archie     */
	/* servers start to return other than external links     */
	/* a link type that begins with an E                     */

	if (*(p->target) == 'E') {
    phost = p->host;
    phson = p->hsoname;
	} else {
    aq_lhost(p,phostbuf,sizeof(phostbuf));
    phost = phostbuf;
    phson = aq_lhsoname(p);
	}

	if (*(q->target) == 'E') {
    qhost = q->host;
    qhson = q->hsoname;
	} else {
    aq_lhost(q,qhostbuf,sizeof(qhostbuf));
    qhost = qhostbuf;
    qhson = aq_lhsoname(q);
	}
#else
	aq_lhost(p,phostbuf,sizeof(phostbuf));
	phost = phostbuf;
	phson = aq_lhsoname(p);
	aq_lhost(q,qhostbuf,sizeof(qhostbuf));
	qhost = qhostbuf;
	qhson = aq_lhsoname(q);

#endif                          /* AQFASTCOMP */
	if ((result = strcmp(phost,qhost)) != 0)
  return(result);
	else
  return(strcmp(phson,qhson));
}

/*
 * aq_invdatecmplink: An alternative comparison function for sorting that
 *	             compares links p and q first by LAST-MODIFIED date,
 *                   if they both have that attribute. If both links
 *                   don't have that attribute or the dates are the
 *                   same, it then calls defcmplink() and returns its 
 *		     value.
 */
int aq_invdatecmplink(p,q)
    VLINK p,q;
{
  PATTRIB pat,qat;
  char *pdate,*qdate;
  int result;
	
  pdate = qdate = NULL;
  for (pat = p->lattrib; pat; pat = pat->next)
	if(strcmp(pat->aname,"LAST-MODIFIED") == 0)
  pdate = pat->value.sequence->token;
  for (qat = q->lattrib; qat; qat = qat->next)
	if(strcmp(qat->aname,"LAST-MODIFIED") == 0)
  qdate = qat->value.sequence->token;
  if(!pdate && !qdate) return(aq_defcmplink(p,q));
  if(!pdate) return(1); 
  if(!qdate) return(-1);
  if((result=strcmp(qdate,pdate)) == 0) return(aq_defcmplink(p,q));
  else return(result);
}


/*
 * aq_restrict - add filter to query
 *
 *        aq_restrict will add a filter with name fname to an archie
 *        query.  If a filter with the same name has already been applied, 
 *        then the new farg is added to the argument for the existing 
 *        filter.
 */
int aq_restrict(struct aquery 	*query, /* Query to restrict    */
                char		*fname, /* Name of restriction  */
                char		*farg,  /* Args for restriction */
                char		sep)    /* Separator            */
{
  FILTER	cfil = query->filters;
  char	argbuf[100];
  char	*argp = farg;
  char	*argsep;

  if(sep)
  {
    strncpy(argbuf,farg,sizeof(argbuf)-1);
    argbuf[sizeof(argbuf)-1] = '\0';
    argp = argbuf;
  }

  /* See if filter already exists */
  while(cfil)
  {
    if(cfil->name && (strcmp(cfil->name,fname) == 0))
    {
	    /* Found it, add argument */
	    if(sep)
      {
        while((argsep = index(argp,sep)))
        {
          *argsep = '\0';
          cfil->args = tkappend(argp,cfil->args);
          argp = argsep+1;
        }
        cfil->args = tkappend(argp,cfil->args);
	    }
	    else cfil->args = tkappend(farg,cfil->args);
    }
    cfil = cfil->next;
  }
  /* Add a new filter */
  cfil = flalloc();
  cfil->name = stcopyr(fname,cfil->name);
  cfil->type = FIL_DIRECTORY;
  cfil->execution_location = FIL_SERVER;
  cfil->pre_or_post = FIL_PRE;
  if(sep)
  {
    while((argsep = index(argp,sep)))
    {
	    *argsep = '\0';
	    cfil->args = tkappend(argp,cfil->args);
	    argp = argsep+1;
    }
    cfil->args = tkappend(argp,cfil->args);
  }
  else cfil->args = tkappend(farg,cfil->args);
  APPEND_ITEM(cfil,query->filters);
  return(PSUCCESS);
}


char *aq_lhsoname(VLINK l)
{
  char *slash;
  char *hostp;

  if(strcmp(l->target,"EXTERNAL") == 0) return(l->hsoname);

  /* Need check for FILE, but with FTP prefix */

  if(strcmp(l->target,"DIRECTORY") != 0) return(l->hsoname);

  if(strncmp(l->hsoname,"ARCHIE/HOST",11) == 0)
  {
    hostp = l->hsoname + 12;
    slash = index(hostp,'/');
    if (slash) return(slash);
    else return("");
  }
  /* If not ARCHIE/HOST, don't change */
  return(l->hsoname);
}

char *
aq_lhost(VLINK l,char *host,int hlen)
{
    char *slash;

    strncpy(host,l->host,hlen-1);

    if(strcmp(l->target,"EXTERNAL") == 0) return(host);

    /* Need check for FILE, but with FTP prefix */

    if(strcmp(l->target,"DIRECTORY") != 0) return(host);

    if (strncmp(l->hsoname,"ARCHIE/HOST",11) == 0) {
	*(host+hlen-1) = '\0';
	strncpy(host,l->hsoname+12,hlen-1);
	slash = index(host,'/');	
	if (slash) *slash = '\0';
	return(host);
    }
    /* If not ARCHIE/HOST, don't change */
    return(host);
}

#if 0
/*
 * translateArchieResponse: 
 *
 *   If the given link is for an archie-pseudo directory, fix it. 
 *   This is called unless AQ_NOTRANS was given to archie_query().
 */
static void
translateArchieResponse(l)
    VLINK l;
    {
	char *slash;

	if (strcmp(l->type,"DIRECTORY") == 0) {
	    if (strncmp(l->hsoname,"ARCHIE/HOST",11) == 0) {
		l->type = stcopyr("EXTERNAL",l->type);
		l->host = stcopyr(l->hsoname+12,l->host);
		slash = index(l->host,'/');
		if (slash) {
		    l->hsoname = stcopyr(slash,l->hsoname);
		    *slash++ = '\0';
		} else
		    l->hsoname = stcopyr("",l->hsoname);
	    }
	}
    }
#endif /* 0 */
