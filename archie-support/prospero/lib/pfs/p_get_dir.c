/*
 * Copyright (c) 1992, 1993       by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 *
 * Written  by bcn 1989     modified 1989-1992
 * Modified by swa 1992     to use qsscanf()
 * Modified by bcn 1/93     to use ardp library
 * bug fix by cheang 11/93   pre server filters.
 */

#include <usc-license.h>

#include <stdio.h>
#include <string.h>
#include <pprot.h>

#include <ardp.h>
#include <pfs.h>
#include <perrno.h>
#include <pparse.h>
#include <pmachine.h>

extern int	pfs_debug;
extern TOKEN p__tokenize_midstyle_mcomp(char *nextname);

static void build_out(VDIR dir, OUTPUT out, char *options, FILTER *ptr_filters) {
/* Build dir->dqs->preq and out */
/* Note side effect, filters updated to point to first client filter */
    struct dqstate *dqs = dir->dqs;
    FILTER filters = *ptr_filters;

    dir->status = DQ_ACTIVE;
    dqs->preq = p__start_req(dqs->dl->host);
    requesttoout(dqs->preq,out); /* two ways to output to this. */
    if(dqs->flags & GVD_MOTD) p__add_req(dqs->preq, "PARAMETER GET MOTD\n");
    if(dqs->flags & GVD_MOTD_O) return; /* Skip over query */

    /* The Server FILTER option should be applied only once before */
    /* union link expansion.                                       */
    if(filters && filters->execution_location == FIL_SERVER) {
        static char * fil_option = NULL;
      assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
        fil_option = qsprintf_stcopyr(fil_option, "%s+FILTER", options);

        p__add_req(dqs->preq, "DIRECTORY ASCII %'s\nLIST %s COMPONENTS",
               dqs->dl->hsoname, fil_option + 1);
    }
    else
        p__add_req(dqs->preq, "DIRECTORY ASCII %'s\nLIST %s COMPONENTS",
               	dqs->dl->hsoname, 
		(*options) ? options+1 : "''" );
    if(dqs->comp) {
        p__add_req(dqs->preq, " %'s", dqs->comp);
        
        if (dqs->mcomp) {
#ifdef CLIENT_SEND_NEW_MCOMP
            out_sequence(out, *dqs->acomp); /* terminates with a \n. */
#else
            TOKEN acp;

            for (acp = *dqs->acomp; acp; acp = acp->next) {
		if(!(acp->token) || (!*(acp->token)))
		    p__add_req(dqs->preq, "/*", acp->token);
                else p__add_req(dqs->preq, "/%'s", acp->token);
	    }
            p__add_req(dqs->preq, "\n");
#endif
        } else {
            p__add_req(dqs->preq, "\n");
        }
    } else {
        p__add_req(dqs->preq, "\n");
    }
    /* Append any appropriate filters to the request. */
    while (filters && filters->execution_location == FIL_SERVER
           && (filters->type == FIL_DIRECTORY 
               || filters->type == FIL_HIERARCHY)) {
        qoprintf(out, "FILTER");
        out_filter(out, filters, 0);
        filters = filters->next;
    }
}

static int
retrieve_and_parse( VDIR dir) 
{
/* Retrieve and parse a ardp response, */
    INPUT_ST    in_st;          /* Input pointer for response contents */
    INPUT       in = &in_st;

    struct dqstate *dqs = dir->dqs;

    CHECK_MEM();
        rreqtoin(dqs->preq, in);
        while(!in_eof(in)) {
            int tmp;
            char		*line;
            char            *next_word;

	    CHECK_MEM();
            if (tmp = in_line(in, &line, &next_word)) 
		return(tmp);

            switch (*line) {

                char 	l_htype[MAX_DIR_LINESIZE];
                char 	l_host[MAX_DIR_LINESIZE];
                char 	l_ntype[MAX_DIR_LINESIZE];
                char 	l_fname[MAX_DIR_LINESIZE];
                int		l_version;
                int		l_magic;
                char 	t_unresolved[MAX_DIR_LINESIZE];

            case 'A':           /* ATTRIBUTE lines that apply to this
                                   directory. */
                if (!strnequal(line, "ATTRIBUTE", sizeof "ATTRIBUTE" - 1))
                    goto scanerr;
                if (tmp = in_ge1_atrs(in, line, next_word, &dir->attributes)) {
                    if (pfs_debug)
                        perrmesg("p_get_dir()", 0, NULL);
                    return(tmp);
                }
            case 'L': /* LINK or LINK-INFO */
                if(!strnequal(line,"LINK",4)) 
                    goto scanerr;

                /* If only verifying, don't want to change dir */
                if(dqs->flags & GVD_VERIFY) {
                    break;
                }
                /* If first link and some links in dir, free them */
                if(!dqs->newlinks++) {
                    if(dir->links) vllfree(dir->links); dir->links=NULL;
                    if(dir->ulinks) vllfree(dir->ulinks); dir->ulinks=NULL;
                }
		CHECK_MEM();
                if (tmp = in_link(in,line,next_word,0,&(dqs->clnk), 
                                  (TOKEN *) NULL)) {
                    if (pfs_debug)
                        perrmesg("p_get_dir()", 0, NULL);
		    return(DIRSRV_BAD_FORMAT);
                }

                /* Double check to make sure we don't get */
                /* back unwanted components		      */
                /* OK to keep if special (URP) links      */
                /* or if dqs->mcomp specified                  */
                if(!dqs->mcomp &&
                   ((dqs->clnk->linktype == 'L') ||
                    (dqs->clnk->linktype == 'I')) && dqs->comp
                   && (!wcmatch(dqs->clnk->name,dqs->comp))) {
                    vlfree(dqs->clnk); dqs->clnk = NULL;
                    break;
                }

                /* If other optional info was sent back, it must */
                /* also be parsed before inserting link     ***  */


                if(dqs->clnk->linktype == 'L' || dqs->clnk->linktype == 'I') 
                    vl_insert(dqs->clnk,dir,
			(dqs->flags & GVD_NOSORT) ? VLI_NOSORT : VLI_ALLOW_CONF
			);
                else {
                    tmp = ul_insert(dqs->clnk,dir,dqs->pulnk);

                    /* If inserted after dqs->pulnk, next one after dqs->clnk*/
                    if(dqs->pulnk && (!tmp || (tmp == UL_INSERT_SUPERSEDING)))
                        dqs->pulnk = dqs->clnk;
                }

                break;

            case 'F': /* FAILURE or FORWARDED */
                /* FORWARDED */
                if(strncmp(line,"FORWARDED",9) == 0) {
                    if((dqs->fwdcnt)-- <= 0) {
                        perrno = PFS_MAX_FWD_DEPTH;
                        if (pfs_debug)
                            perrmesg("p_get_dir()", 0, NULL);
                        return(perrno);
                    }
                    /* parse and start over */
                    tmp = qsscanf(line,"FORWARDED %s %'s %s %'s %d %d", 
                                 l_htype,l_host,l_ntype,l_fname,
                                 &l_version, &l_magic);

                    dqs->dl->host = stcopyr(l_host, dqs->dl->host);
                    dqs->dl->hsoname = stcopyr(l_fname, dqs->dl->hsoname);

                    if(tmp < 4) {
                        if (pfs_debug)
                            fprintf(stderr, 
                                   "p_get_dir(): Malformed FORWARDED line: %s",
                                   line);
                        return(DIRSRV_BAD_FORMAT);
                    }

                    ardp_rqfree(dqs->preq);
                    dqs->preq = NOREQ;
                    dir->status = DQ_INACTIVE;
                    return(DSRFINFO_FORWARDED);
                }
                goto scanerr;
                break;


            case 'N': /* NONE-FOUND */
                /* NONE-FOUND, we just have no links to insert */
                /* It is not an error, but we must clear any   */
                /* old links in the directory arg              */
                if(strncmp(line,"NONE-FOUND",10) == 0) {
                    /* If only verifying, don't want to change dir */
                    if(dqs->flags & GVD_VERIFY) {
                        break;
                    }

                    /* If first link and some links in dir, free them */
                    if(!dqs->newlinks++) {
                        if(dir->links) vllfree(dir->links);
                        if(dir->ulinks) vllfree(dir->ulinks);
                        dir->links = NULL;
                        dir->ulinks = NULL;
                    }
                    break;
                }
                /* If anything else, scan error */
                goto scanerr;

            case 'U': /* UNRESOLVED */ {
                char *p_unresolved;
                TOKEN new_acomp;
                int newlen, oldlen;

                tmp = qsscanf(line,"UNRESOLVED %r", &p_unresolved);
                if(tmp < 1) goto scanerr;
#ifndef DONT_ACCEPT_OLD_MCOMP
                new_acomp = p__tokenize_midstyle_mcomp(p_unresolved);
#else                           /* guaranteed a new reply from server */
                new_acomp = qtokenize(p_unresolved);
#endif
                newlen = length(new_acomp);
                oldlen = length(*dqs->acomp); 
                /* If multiple components were resolved */
                /* Must be a proper suffix. */
                if (newlen != oldlen) {
                    if (oldlen <= newlen) goto badunresolved;
                    /* Comp gets the last component resolved.  (NB: Indexing
                       for elt() starts at 0.) */
                    dqs->comp =  stcopy(elt(*dqs->acomp, oldlen - newlen - 1));
                    dqs->stfree_comp = 1;
                    /* Let rd_vdir know what remains */
                    tklfree(*dqs->acomp);
                    *dqs->acomp = new_acomp;
                }
                dqs->unresp = 1;
                break;
            badunresolved:
                if (pfs_debug)
                    fprintf(stderr, "p_get_dir(): Received an UNRESOLVED \
response not a proper suffix of the acomp sent.  This is not legal: %s\n", 
                            line);
#if 0                           /* mitra commented these out; unclear why */
                ardp_rqfree(dir->dqs->preq);
                if (dir->dqs->stfree_comp) stfree(dir->dqs->comp);
                stfree(dir->dqs); dir->dqs = NULL;
#endif
                return perrno = dir->status = DIRSRV_BAD_FORMAT;
            }                   /* case 'U' (unresolved) */

            scanerr:
            default:
                if(tmp = scan_error(line, dqs->preq)) {
                    /* XXX memory leak? check this mitracode. */

                    /* change 1 follows */
 		    return(tmp);
                 }
                 break;
            }                   /* switch */
         } /*while*/
 	return PSUCCESS;
}                               /* bad_p_get_dir( */
 /*
  * p_get_dir - Get contents of a directory given its location
  *
  *	      P_GET_DIR takes a directory location, a list of desired
  *	      components, a pointer to a directory structure to be 
  *	      filled in, and flags.  It then queries the appropriate 
  *	      directory server and retrieves the desired information.
  *
  *      ARGS:   dlink       - Host and host specific object name for directory
  *                            plus any filters to be applied (->filters)
  *              components  - The first component of the name we want to
  *                            resolve in that directory.   Wildcards are
  *                            allowed if there are no additional components.
  *                            NULL or the empty string means 'all'.
  *		dir	    - Structure to be filled in (or possibly in the
  *                            process of being filled in, if GVD_ASYNC)
  *	        flags       - Options.  See FLAGS
  *              acomp       - Pointer to a token list consisting of the
  *                            remaining components of the name we 
  *                            want to resolve in that directory.  This
  *                            argument might get modified.   If it is, acomp
  *                            will be left pointing to a suffix (not
  *                            necessarily a proper suffix) of the token list or
  *                            NULL, and the extra tokens will be freed by
  *                            p_get_dir().
  *
  *     FLAGS:	GVD_UNION   - Do not expand union links
  *		GVD_EXPAND  - Expand union links locally
  *		GVD_REMEXP  - Request remote expansion (& local if refused)
  *		GVD_LREMEXP - Request remote expansion of local union links
  *		GVD_VERIFY  - Only verify that args are for a directory
  *              GVD_FIND    - Stop expanding when a match is found
  *              GVD_ATTRIB  - Request attributes from directory server
  *              GVD_NOSORT  - Do not sort links when adding to directory
  *              GVD_MOTD    - Request MOTD string from server
  *              GVD_MOTD_O  - Request MOTD and nothing else
  *              GVD_ASYNC   - Process request asynchronously
  *
  *   RETURNS:   PSUCCESS (0) or error code on completion.
  *		On some codes addition information in p_err_string.
  *              DQ_ACTIVE if request if GVD_ASYNC specified and
  *              request is still active.
  *
  *     NOTES:   If acomp is non-null the string it points to might be modified
  *
  *              If the directory passed as an argument already has
  *		links or union links, then those lists will be freed
  *              before the new contents are filled in.
  *
  *              If a filter is passed to the procedure, and application of
  *              the filter results in additional union link, then those links
  *              will (or will not) be expanded as specified in the FLAGS field.
  *
  *              If the list of components in NULL, or the null string, then
  *              p_get_dir will return all links in the requested directory.
  */
 
 /* XXX
  * See bug report in /nfs/pfs/bugs/p_get_dir_modifies_argument for
  * an explanation of what needs to be done to make this code
  * more aesthetic and maintainable.  this is a quick fix to get it working.
  */
 
#define RETURN(rv) { retval = (rv); goto cleanup ; }
 
static int
bad_p_get_dir(VLINK  dlink,        /* Directory to examine                */
              const char *components,/* Component name (wildcards allowed). */
              VDIR	dir,		/* Structure to be filled in	       */
              long	flags,		/* Flags			       */
              TOKEN	*acomp)		/* Components left to be resolved      */
{
    struct dqstate *dqs;	/* State of current query              */
     
    OUTPUT_ST	out_st;         /* Output pointer for packet contents  */
     OUTPUT	out = &out_st;
 
     int		ardptime;	/* Time to wait for ardp_send          */
 
     int		tmp;            /* To check return values              */
     VLINK	tl = NULL;       /* Temp link used for apply_filter()   */
                             /* Setting it to NULL shuts up GCC; never used
                                before set. */
     VLINK	l;		/* Temp link pointer 		       */
     FILTER      filters;        /* Filters to apply this time through  */
 
     char	options[40];    /* LIST option                         */
     char	*tchar;		/* Temporary chatacter pointer         */
    int         retval;		/* Hold return value */
 
     perrno = 0;
 
     if((dir->status == DQ_ACTIVE) && dir->dqs) {
 	dqs = dir->dqs;
         /* Turn off this query's GVD_ASYNC flag if it was set on a previous
            call to P_GET_DIR. */
 	if(!(flags & GVD_ASYNC)) dqs->flags &= (~GVD_ASYNC);
     }
     else {
         /* Initialize a new dqstate structure. */
 	dqs = (struct dqstate *) stalloc(sizeof(struct dqstate));
	if(!dqs) RETURN(PFAILURE);
 	dir->dqs = dqs;
 	dqs->dl = dlink;
 
         if (!components || (*components == '\0')) dqs->comp = NULL;
         else dqs->comp = (char *) components; /* strip off CONST.  */
         dqs->stfree_comp = 0;
 	dqs->acomp = acomp;
 
 	dqs->flags = flags;
 	dqs->fwdcnt = MAX_FWD_DEPTH;
 	dqs->newlinks = 0;
 	dqs->clnk = NULL;
 	dqs->cexp = NULL;
 	dqs->pulnk = NULL;
 	
 	/* If filters, don't do remote expansion of union links! */
 #ifdef NEVERDEFINED
 /* I cant see why not! */
 	if(dqs->dl->filters) {
 	    dqs->flags &= (~(GVD_REMEXP&(~GVD_EXPAND)));
             dqs->comp = NULL;
 	}
 #endif
 	if(acomp && *acomp && !dlink->filters) dqs->mcomp = 1;
 	else dqs->mcomp = 0;
     }
 
     *options = '\0';
     if(dqs->flags & GVD_ATTRIB) strcat(options,"+ATTRIBUTES");
     if((dqs->flags & GVD_EXPMASK) == GVD_REMEXP) strcat(options,"+EXPAND");
     if((dqs->flags & GVD_EXPMASK) == GVD_LREMEXP) strcat(options,"+LEXPAND");
 
     /* If all we are doing is verifying a directory     */
     /* we do not want a big response from the directory */
     /* server.  A NOT-FOUND is sufficient.		*/ 
     if(dqs->flags & GVD_VERIFY) strcat(options,"+VERIFY");
 
 
     if((dir->status == DQ_ACTIVE) && dir->dqs) goto cont_async;
 
 startover:
     filters = dqs->dl->filters;
 
     if(filters) {
        if(tl) vlfree(tl);      /* Loop with filter leaked old copy */
        tl = vlalloc();		/* Pased to apply_filters */
         tl->name = stcopy("UNNEEDED");
         tl->host = stcopy(dqs->dl->host);
         tl->hsoname = stcopy(dqs->dl->hsoname);
        tl->filters = flcopy(filters,1);    /* otherwise gets freed twice*/
     }
 
 get_contents:
 
     /* Build dir->dqs->preq and out */
     /* Side effect, filters now points to first client-side filter */
     build_out(dir, out, options, &filters);
 
 cont_async:
 
     if(dqs->flags & GVD_ASYNC) ardptime = 0;
     else ardptime = ARDP_WAIT_TILL_TO;
 
     if(dqs->preq->status == ARDP_STATUS_NOSTART)
 	tmp = ardp_send(dqs->preq,dqs->dl->host,0,ardptime);
     else tmp = ardp_retrieve(dqs->preq,ardptime);
 
    if((dqs->flags & GVD_ASYNC) && (tmp == ARDP_PENDING)) RETURN(DQ_ACTIVE);
 
     /* If we don't get a response, then if the requested       */
     /* directory, return error, if a ulink, mark it unexpanded */
     if(tmp != PSUCCESS) {
         if (pfs_debug)
             fprintf(stderr,"ardp_send failed: %d\n",tmp);
         if(dqs->cexp) dqs->cexp->expanded = FAILED;
 	else {
 	    ardp_rqfree(dir->dqs->preq);
                    /* change 1 done*/
                    if (dir->dqs->stfree_comp) stfree(dir->dqs->comp);

                    stfree(dir->dqs); dir->dqs = NULL;
 	    RETURN(perrno = dir->status = tmp);

                }
            }
 
 
     /* Claim that we don't have any more unresolved components to process.  If
        we get an UNRESOLVED response, we'll set this back on. */
     dqs->unresp = 0;
 
     if (tmp == PSUCCESS) {
         /* Here we must parse reponse and put in directory */
         /* While looking at each packet 		   */
 	switch (tmp = retrieve_and_parse(dir)) {
 	case PSUCCESS:			break;
 	case DSRFINFO_FORWARDED:	goto startover;
 	default:
 	    if (dqs->cexp) dqs->cexp->expanded = FAILED;
 	    else {
                     ardp_rqfree(dqs->preq);
                     if (dqs->stfree_comp) stfree(dqs->comp);
                     stfree(dqs); dqs = NULL;
                    RETURN(perrno = dir->status = tmp);
        }
 	} /*switch*/
    }
    
    /* We sent multiple components and weren't told any */
    /* were unresolved                                  */
    if(dqs->mcomp && !dqs->unresp) {
        TOKEN acp;
        for (acp = *dqs->acomp; acp->next; acp = acp->next)
            ;
        /* acp now points to the last additional component */
        dqs->comp = acp->token;
        acp->token = NULL;
        /* Free the entries. */
        tklfree(*dqs->acomp);
        /* Let rd_vdir know what remains */
        *(dqs->acomp) = NULL;
        /* If we have union links to resolve, only one component remains */
        dqs->mcomp = 0;
    }

    /* If only verifying, we already know it is a directory */
    if(dqs->flags & GVD_VERIFY) {
	ardp_rqfree(dqs->preq); 
        if (dir->dqs->stfree_comp) stfree(dir->dqs->comp);
	stfree(dir->dqs); dir->dqs = NULL;
	dir->status = DQ_COMPLETE;
	RETURN(PSUCCESS);
    }

    /* Don't return if matching was delayed by the need to filter    */
    /* if FIND specified, and dir->links is non null, then we have   */
    /* found a match, and should return.                             */
    if((dqs->flags & GVD_FIND) && dir->links && (!filters)) {
	ardp_rqfree(dqs->preq); 
        if (dir->dqs->stfree_comp) stfree(dir->dqs->comp);
	stfree(dir->dqs); dir->dqs = NULL;
	dir->status = DQ_COMPLETE;
	RETURN(PSUCCESS);
    }

    /* If expand specified, and ulinks must be expanded, making sure */
    /* that the order of the links is maintained properly            */

expand_ulinks:

    if ((dqs->flags & GVD_FIND) || 
        (((dqs->flags & GVD_EXPMASK) != GVD_UNION) && 
         !(dqs->flags & GVD_VERIFY))) {

        l = dir->ulinks;

        /* Find first unexpanded ulink */
        while(l && l->expanded && (l->linktype == 'U')) l = l->next;

        /* Only expand if a FILE or DIRECTORY -  Mark as  */
        /* failed otherwise                               */
        /* We must still add support for symbolic ulinks */
        if(l) {
            if ((strcmp(l->target,"DIRECTORY") == 0) || 
                (strcmp(l->target,"FILE") == 0)) {
                l->expanded = TRUE;
                dqs->cexp = l;
                dqs->pulnk = l;
                dqs->dl->host = stcopyr(l->host, dqs->dl->host);
                dqs->dl->hsoname = stcopyr(l->hsoname, dqs->dl->hsoname);
                /* When we're expanding union links, we need to turn off
                   support for multiple components.  Otherwise, p_get_dir() is
                   confused by a NONE-FOUND response during the expansion
                   process and zeroes out the dqs->acomp. */
                dqs->mcomp = 0;
                goto get_contents;
            }
            else l->expanded = FAILED;
        }
    }

    /* Right now, give off no error messages if inappropriate filters are
       encountered (e.g., a SERVER filter following a CLIENT filter).
       We just don't worry about it right now.   Maybe we should do error
       reporting.  Presumably, only client filters are left right now. XXX */
    if(filters && filters->execution_location == FIL_CLIENT &&
       (filters->type == FIL_DIRECTORY || filters->type == FIL_HIERARCHY)) {
        apply_filters(dir,filters,tl,0); /* Apply a single filter */
        filters = filters->next;
        goto expand_ulinks;
    }

    /* Double check to make sure we don't get */
    /* back unwanted components		  */
    /* OK to keep if special (URP) links      */
    if(dqs->comp && *(dqs->comp)) {
        l = dir->links;
        while(l) {
            VLINK	ol;
            if((l->linktype == 'L' || l->linktype == 'I') 
               && (!wcmatch(l->name,dqs->comp))) {
                if(l == dir->links)
                    dir->links = l->next;
                else l->previous->next = l->next;
                if(l->next) l->next->previous = l->previous;
                ol = l;
                l = l->next;
                vlfree(ol);
            }
            else l = l->next;
        }
    }

    ardp_rqfree(dqs->preq); 
    if (dir->dqs->stfree_comp) stfree(dir->dqs->comp);
    stfree(dir->dqs); dir->dqs = NULL;
    dir->status = DQ_COMPLETE;
    RETURN(PSUCCESS);
  cleanup:
    if (tl) vlfree(tl);
    return(retval);
}

int
p_get_dir(VLINK dlink, const char *components, VDIR dir, 
		long flags, TOKEN *acomp)
{
	VLINK vl;
	int retval;

/* the distributed p_get_dir hacks dlink if the resulting directory contains
   any union links, or is forwarded, fix this behaviour */

	vl = vlcopy(dlink,FALSE);
	retval = bad_p_get_dir(vl, components, dir, flags, acomp);
	vlfree(vl);
	return retval;
}


/* Older interfaces (backwards compatability) */

int
p_get_vdir(VLINK  dlink,        /* Directory to examine                */
	 const char *components,/* Component name (wildcards allowed). */
	 VDIR	dir,		/* Structure to be filled in	       */
	 long	flags,		/* Flags			       */
	 TOKEN	*acomp)		/* Components left to be resolved      */
{
    return p_get_dir(dlink, components, dir, flags, acomp);
}

int
get_vdir(VLINK  dlink,        /* Directory to examine                */
	 const char *components,/* Component name (wildcards allowed). */
	 VDIR	dir,		/* Structure to be filled in	       */
	 long	flags,		/* Flags			       */
	 TOKEN	*acomp)		/* Components left to be resolved      */
{
    return p_get_dir(dlink, components, dir, flags, acomp);
}






