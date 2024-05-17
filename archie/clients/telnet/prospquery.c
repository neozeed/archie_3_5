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

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#ifndef SOLARIS
#include <strings.h>			/* for char *index() */
#else
#include <string.h>
#endif
#ifdef AIX
#include <sys/select.h>
#endif
#include "debug.h"
#include "macros.h"
#include "prosp.h"
#include "tellwait.h"
#include "vars.h"

#include "protos.h"

extern char *p_motd;
extern int ardp_port;


int archie_query(arq, prt_status)
  struct aquery *arq;
  int prt_status;
{
  int flags = arq->flags;
  int go = 0;
  int qpos = 0;
  int ret;
  int sys_time = 0;
  
  if (prt_status) flags |= AQ_ASYNC;

  ret = aq_query(arq, flags);
  while (ret == AQ_ACTIVE)
  {
    fd_set readfds;
    struct timeval selwait;
    time_t t = arq->retry_at.tv_sec - time((time_t *)0);

    selwait.tv_usec = 0;
    selwait.tv_sec = max(t, 0);

    if (prt_status)
    {
      if (qpos != arq->qpos)
      {
        qpos = arq->qpos;
        printf("# Your queue position: %d\n", qpos); /*FFF*/
        go = 1;
      }
      if (sys_time != arq->sys_time)
      {
        sys_time = arq->sys_time;
        if (sys_time > 0)
        {
          int min = sys_time / 60;
          int sec = sys_time % 60;
          
          printf("# Estimated time for completion: "); /*FFF*/

          if (min > 1) printf("%d minutes%s", min, sec ? ", " : ".\n");
          else if (min == 1) printf("%d minute%s", min, sec ? ", " : ".\n");
          if (sec > 1) printf("%d seconds.\n", sec);
          else if (sec == 1) printf("%d second.\n", sec);
        }
        go = 1;
      }
      if (go)
      {
        tell_parent(getppid());
        prt_status = 0;
      }
    }

    FD_ZERO(&readfds);
    FD_SET(arq->wait_fd, &readfds);
    select(arq->wait_fd+1, &readfds, (fd_set *)0, (fd_set *)0, &selwait);

    ret = aq_query(arq, AQ_ASYNC);
  }

  if (prt_status)
  {
    tell_parent(getppid());
  }
  return ret;
}


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
int aq_query(struct aquery *aq, int flags)
{
  VDIR	dir = &(aq->dirst);     /* Directory for get_vdir     */
  VLINK lowest, nextp, p, pnext, pprev, q, r;
  const char	*components = "";
  char	qstring[MAX_VPATH];     /* For construting the query  */
  int		gvdflags;
  int		tmp;

  p_clear_errors();

  /*  
   *  It seem that if `p_motd' gets set once, it is used forever after.
   *  Thus, setting the server to any machine makes it appear that it
   *  also has the old motd.
   *    - wheelan (Fri Aug 20 03:33:53 EDT 1993)
   */  
  if (p_motd)
  {
    stfree(p_motd);
    p_motd = 0;
  }

  if(aq->status == AQ_ACTIVE)
  {
    if((flags&AQ_ASYNC) == 0) aq->flags &= (~AQ_ASYNC);
    goto cont_async;
  }

  aq->status = AQ_ACTIVE;
  aq->flags = flags;            /*bug? needed?*/
  vdir_init(dir);

  /* Set the cmp_proc if not given */
  if ( ! aq->cmp_proc) aq->cmp_proc = aq_defcmplink;

  /* Make the query string */
  if(aq->query_type > ' ')
  {
    sprintf(qstring,"ARCHIE/MATCH(%d,%d,%d,%d,%c)/%s",
            aq->maxhits, aq->maxmatch, aq->maxhitpm, aq->offset, 
            aq->query_type, aq->string);
  }
  else
  {
    switch (aq->query_type)
    {
    case AQ_HOSTINFO:
      strcpy(qstring, "ARCHIE/HOST");
      components = aq->string;
      break;

    case AQ_SITELIST:           /* wheelan */
      sprintf(qstring, "ARCHIE/HOST/%s", aq->string); /* has () around it */
      break;

    case AQ_DOMAINS:
      sprintf(qstring, "ARCHIE/DOMAINS");
      break;
      
    case AQ_WHATIS:             /* wheelan */
      sprintf(qstring, "ARCHIE/WHATIS/%s", aq->string);
      break;

    case AQ_DOMAIN:             /* wheelan */
      sprintf(qstring, "ARCHIE/DOMAIN/%s", aq->string);
      break;

    case AQ_RE_DOMAIN:          /* wheelan */
      sprintf(qstring, "ARCHIE/DOMAIN/(%s)", aq->string);
      break;

    case AQ_GENERIC:            /* wheelan */
      sprintf(qstring,"ARCHIE/FIND(%d,%d,1,%s)/%s",
              aq->maxhits, aq->maxhits, aq->dbs, aq->string);
      break;

    case AQ_MOTD_ONLY:
      qstring[0] = '\0';
      break;

    default:
      break;                    /* previous code didn't catch this either */
    }
  }
  
  /* Initialize Prospero structures */
  perrno = 0;
	
  if(aq->query_type == AQ_EXP_LINK)
  {
    aq->dirquery = aq->expand;
  }
  else
  {
    if(!(aq->host)) return(perrno = aq->status = ARDP_BAD_HOSTNAME);
    aq->dirquery = vlalloc();
    aq->dirquery->host = stcopyr(aq->host, aq->dirquery->host);
    aq->dirquery->hsoname = stcopyr(qstring, aq->dirquery->hsoname);
    aq->dirquery->filters = aq->filters;
  }

 cont_async:

  gvdflags = GVD_ATTRIB|GVD_NOSORT;
  if(aq->flags & AQ_MOTD) gvdflags |= GVD_MOTD;
  if(aq->query_type == AQ_MOTD_ONLY) gvdflags |= GVD_MOTD_O;
  if(aq->flags & AQ_ASYNC) gvdflags |= GVD_ASYNC;

  /* Retrieve the list of matches, return error if there was one */
  tmp = get_vdir(aq->dirquery, components, dir, gvdflags, NULL);

  if(tmp == DQ_ACTIVE)
  {
    aq->wait_fd = ardp_port;
    aq->qpos = dir->dqs->preq->inf_queue_pos;
    aq->sys_time = dir->dqs->preq->inf_sys_time;
    aq->retry_at.tv_sec = dir->dqs->preq->wait_till.tv_sec;
    aq->retry_at.tv_usec = dir->dqs->preq->wait_till.tv_usec;
    if(!aq->motd && p_motd) aq->motd = stcopyr(p_motd, aq->motd);
    return AQ_ACTIVE;
  }

  if(aq->query_type != AQ_EXP_LINK)
  {
    aq->dirquery->filters = NULL; /* Keep them on aq->filters */
    vlfree(aq->dirquery); 
    aq->dirquery = NULL;
  }

  if(p_motd) aq->motd = stcopyr(p_motd, aq->motd);

  if(tmp) return aq->status = tmp;

  /* Save the links, and clear in dir in case it's used again   */
  aq->results = dir->links; dir->links = NULL;

  /* As returned, list is sorted by suffix, and conflicting     */
  /* suffixes appear on a list of "replicas".  We want to       */
  /* create a one-dimensional list sorted by host then filename */
  /* and maybe by some other parameter                          */

  /* First flatten the doubly-linked list */
  for (p = aq->results; p; p = nextp)
  {
    nextp = p->next;
    if (p->replicas)
    {
	    p->next = p->replicas;
	    p->next->previous = p;
	    for (r = p->replicas; r->next; r = r->next)
      /*EMPTY*/ ;
	    r->next = nextp;
	    nextp->previous = r;
	    p->replicas = NULL;
    }
  }

  /* If NOSORT given, then just hand it back */
  if (aq->flags & AQ_NOSORT)
  {
    perrno = PSUCCESS;
    aq->status = AQ_COMPLETE;
    return PSUCCESS;
  }

  /* Otherwise sort it using a selection sort and the given cmp_proc */
  for (p = aq->results; p; p = nextp)
  {
    nextp = p->next;
    lowest = p;
    for (q = p->next; q; q = q->next)
    {
      if ((*(aq->cmp_proc))(q, lowest) < 0) lowest = q;
    }
    if (p != lowest)
    {
	    /* swap the two links */
	    pnext = p->next;
	    pprev = p->previous;
	    if (lowest->next) lowest->next->previous = p;
	    p->next = lowest->next;
	    if (nextp == lowest)
      {
        p->previous = lowest;
	    }
      else
      {
        lowest->previous->next = p;
        p->previous = lowest->previous;
	    }
	    if (pprev->next) pprev->next = lowest;
	    if (nextp == lowest)
      {
        lowest->next = p;
	    }
      else
      {
        pnext->previous = lowest;
        lowest->next = pnext;
	    }
      lowest->previous = lowest == pprev ? p : pprev;
	    /* keep the head of the list in the right place */
	    if (aq->results == p) aq->results = lowest;
    }
  }
    
  /* Return the links */
  perrno = PSUCCESS;
  aq->status = AQ_COMPLETE;
  return PSUCCESS;
}


/*
 * aq_defcmplink: The default link comparison function for sorting. Compares
 *	         links p and q first by host then by filename. Returns < 0 if p
 *               belongs before q, > 0 if p belongs after q, and == 0 if their
 *               host and filename fields are identical.
 */
int aq_defcmplink(p, q)
    VLINK p, q;
{
	char *phost;
	char *phson;
  char qhostbuf[100];
	char phostbuf[100];
  char *qhost;
  char *qhson;
	int result;

#ifdef AQFASTCOMP
	/* This is designed to be a fast check for EXTERNAL, but */
	/* not necessarily correct.  It will break if archie     */
	/* servers start to return other than external links     */
	/* a link type that begins with an E                     */

	if (*(p->target) == 'E')
  {
    phost = p->host;
    phson = p->hsoname;
	}
  else
  {
    aq_lhost(p, phostbuf, sizeof(phostbuf));
    phost = phostbuf;
    phson = aq_lhsoname(p);
	}

	if (*(q->target) == 'E')
  {
    qhost = q->host;
    qhson = q->hsoname;
	}
  else
  {
    aq_lhost(q, qhostbuf, sizeof(qhostbuf));
    qhost = qhostbuf;
    qhson = aq_lhsoname(q);
	}
#else
	aq_lhost(p, phostbuf, sizeof(phostbuf));
	phost = phostbuf;
	phson = aq_lhsoname(p);
	aq_lhost(q, qhostbuf, sizeof(qhostbuf));
	qhost = qhostbuf;
	qhson = aq_lhsoname(q);
#endif                          /* AQFASTCOMP */

	if ((result = strcmp(phost, qhost)) != 0) return result;
	else return strcmp(phson, qhson);
}

/*
 * aq_invdatecmplink: An alternative comparison function for sorting that
 *	             compares links p and q first by LAST-MODIFIED date, 
 *                   if they both have that attribute. If both links
 *                   don't have that attribute or the dates are the
 *                   same, it then calls defcmplink() and returns its 
 *		     value.
 */
int aq_invdatecmplink(p, q)
    VLINK p, q;
{
  PATTRIB pat, qat;
  char *pdate, *qdate;
  int result;
	
  pdate = qdate = NULL;
  for (pat = p->lattrib; pat; pat = pat->next)
	if(strcmp(pat->aname, "LAST-MODIFIED") == 0)
  pdate = pat->value.sequence->token;
  for (qat = q->lattrib; qat; qat = qat->next)
	if(strcmp(qat->aname, "LAST-MODIFIED") == 0)
  qdate = qat->value.sequence->token;
  if(!pdate && !qdate) return aq_defcmplink(p, q);
  if(!pdate) return 1; 
  if(!qdate) return -1;
  if((result=strcmp(qdate, pdate)) == 0) return aq_defcmplink(p, q);
  else return result;
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
    strncpy(argbuf, farg, sizeof(argbuf)-1);
    argbuf[sizeof(argbuf)-1] = '\0';
    argp = argbuf;
  }

  /* See if filter already exists */
  while(cfil)
  {
    if(cfil->name && (strcmp(cfil->name, fname) == 0))
    {
	    /* Found it, add argument */
	    if(sep)
      {
        while((argsep = index(argp, sep)))
        {
          *argsep = '\0';
          cfil->args = tkappend(argp, cfil->args);
          argp = argsep+1;
        }
        cfil->args = tkappend(argp, cfil->args);
	    }
	    else cfil->args = tkappend(farg, cfil->args);
    }
    cfil = cfil->next;
  }
  /* Add a new filter */
  cfil = flalloc();
  cfil->name = stcopyr(fname, cfil->name);
  cfil->type = FIL_DIRECTORY;
  cfil->execution_location = FIL_SERVER;
  cfil->pre_or_post = FIL_PRE;
  if(sep)
  {
    while((argsep = index(argp, sep)))
    {
	    *argsep = '\0';
	    cfil->args = tkappend(argp, cfil->args);
	    argp = argsep+1;
    }
    cfil->args = tkappend(argp, cfil->args);
  }
  else cfil->args = tkappend(farg, cfil->args);
  APPEND_ITEM(cfil, query->filters);
  return PSUCCESS;
}


char *aq_lhsoname(VLINK l)
{
  char *slash;
  char *hostp;

  if(strcmp(l->target, "EXTERNAL") == 0) return l->hsoname;

  /* Need check for FILE, but with FTP prefix */

  if(strcmp(l->target, "DIRECTORY") != 0) return l->hsoname;

  if(strncmp(l->hsoname, "ARCHIE/HOST", 11) == 0)
  {
    hostp = l->hsoname + 12;
    slash = index(hostp, '/');
    if (slash) return slash;
    else return "";
  }
  /* If not ARCHIE/HOST, don't change */
  return l->hsoname;
}

char *
aq_lhost(VLINK l, char *host, int hlen)
{
  char *slash;

  strncpy(host, l->host, (unsigned)hlen-1);

  if(strcmp(l->target, "EXTERNAL") == 0) return host;

  /* Need check for FILE, but with FTP prefix */

  if(strcmp(l->target, "DIRECTORY") != 0) return host;

  if (strncmp(l->hsoname, "ARCHIE/HOST", 11) == 0) {
    *(host+hlen-1) = '\0';
    strncpy(host, l->hsoname+12, (unsigned)hlen-1);
    slash = index(host, '/');	
    if (slash) *slash = '\0';
    return host;
  }
  /* If not ARCHIE/HOST, don't change */
  return host;
}
