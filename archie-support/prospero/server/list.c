/*
 * Copyright (c) 1992, 1993  by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 *
 * Written  by bcn 1989     modified 1989-1992
 * Modified by swa 3Q/92    V5 support, modularize, quoting, multiple names
 * Modified by bcn 1/20/93  support multiple database prefixes
 */

#include <usc-license.h>

#define SERVER_SEND_NEW_MCOMP
#define REALLY_NEW_FIELD

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>

#include <pmachine.h>
#include <ardp.h>
#include <pfs.h>
#include <pparse.h>
#include <pserver.h>
#include <psrv.h>
#include <plog.h>
#include <pprot.h>
#include <perrno.h>

#include "dirsrv.h"

TOKEN p__tokenize_midstyle_mcomp(char *nextname);

/* Uncomment this for debugging and demonstration purposes. */
/* Leave it enabled for now, since it won't hurt anything. */
#define SERVER_PREDEFINED_IDENTITY_FILTER

#ifdef  SERVER_PREDEFINED_IDENTITY_FILTER
static identity_filter(P_OBJECT dir, TOKEN args);
#endif

static int list_name(RREQ req, char *command, char arg_client_dir[],
        int dir_magic_no, TOKEN components, int verify_dir,
        int localexp, int non_object_attrib_fl, int alllinks, FILTER filters);

#ifdef ARCHIE
extern int archie_supported_version;
#endif

int list(RREQ req, char **commandp, char **next_wordp, INPUT in,
        char client_dir[], int dir_magic_no)
{
 /* Flags */
    int non_object_attrib_fl;	/* Send back atribues in list  */
    int localexp;	/* OK to exp ul for remcomp    */
    int verify_dir;	/* Only verifying the directory */
    int filterfl = 0;	/* specified the filter flag. */
    FILTER filters = NULL;	/* filters to apply */
    optdecl;
    int tmp;
    AUTOSTAT_CHARPP(t_optionsp);
    char *nextname;	/* Next name to be resolved. */
    int retval;	/* Value returned by list_name.  */
    long dummy_magic_no;	/* XXX currently ignored; shouldn't be. */
    CHECK_MEM();
    nextname = NULL;
 /* Assume we can freely overwrite the buffer. */
    tmp = qsscanf(*next_wordp, "%'&s COMPONENTS %r",
	&*t_optionsp, &nextname);
    if (tmp < 0)
	interr_buffer_full();

    if (in_select(in, &dummy_magic_no))
	return error_reply(req, "LIST: %'s", p_err_string);
 /* At this point, *t_optionsp is a '+'-separated list of options and nextname
    should point to a space-separated list of names. */

    CHECK_MEM();
    /* Parse the options. */
    optstart(*t_optionsp);
    verify_dir = opttest("VERIFY") ? 1 : 0;
    /* If EXPAND specified, remeber that fact */
    CHECK_MEM();
    if (opttest("EXPAND") || opttest("LEXPAND"))
	localexp = 2;
    else
	localexp = 0;
    if (opttest("ATTRIBUTES"))
	non_object_attrib_fl = DSRD_ATTRIBUTES;
    else
	non_object_attrib_fl = 0;
    CHECK_MEM();
    if (opttest("FILTER")) {
    /* leave commandp set the way it was, since we need it to report errors. */
	char *line, *next_word;
	filterfl = 1;
        if (localexp)
            return error_reply(req, "FILTER flag cannot be specified with \
LEXPAND or EXPAND flags: %'s\n", *commandp); 
    CHECK_MEM();
        while (in_nextline(in) && strnequal(in_nextline(in), "FILTER", 6)) {
	    FILTER cur_fil;

	    if (in_line(in, &line, &next_word)) {
		fllfree(filters);
		RETURNPFAILURE;	/* error reporting done by in_line() */
	    }
            CHECK_MEM();
	    if (in_filter(in, line, next_word, 0, &cur_fil)) {
		fllfree(filters);
		return error_reply(req, "Could not parse a filter: %'s",
		    p_err_string, 0);
	    }
            CHECK_MEM();
	    if (cur_fil->execution_location != FIL_SERVER || cur_fil->link
		|| (cur_fil->type != FIL_DIRECTORY
		    && cur_fil->type != FIL_HIERARCHY)
		|| cur_fil->pre_or_post != FIL_PRE) {
		fllfree(filters);
		flfree(cur_fil);
		return error_reply(req, "All filters to be applied at the\
 server must have an execution location of SERVER and be PRE-expansion and be\
 DIRECTORY or HIERARCHY filters and be PREDEFINED.  This one\
 does not meet those criteria: %'s", line);
	    }
	    APPEND_ITEM(cur_fil, filters);
	/* Checking for whether this filter is defined on this server occurs
	   in list_name(), when we dispatch to the particular filter. */
	}
        CHECK_MEM();
	if (!filters)
	    return error_reply(req, "LIST had FILTER option specified, but no \
filters were given.");
    } else {
	filterfl = 0;
    }

    CHECK_MEM();
    plog(L_DIR_REQUEST, req, "L%s %s %s",
         (verify_dir ? "V" : " "), client_dir, (nextname ? nextname : "*"));
    /* Specifying no component is exactly equivalent to specifying '*' */
    {
        int listall_flag = 0;
        TOKEN comps;

#ifdef DONT_ACCEPT_OLD_MCOMP
         /* No clients left that transmit old styles, so guaranteed not to need
            to adapt to both. */
        if (nextname) comps = qtokenize(nextname);
#else   /* There are some old-style clients.  (accept both styles; transmit
           old or new). */ 
        if (nextname) comps = p__tokenize_midstyle_mcomp(nextname);
#endif        
        else {
            listall_flag = 1;
            /* COMPS shouldn't ever be looked at when listall flag is passed,
               but it is. */ 
            comps = tkalloc("*");                      
        }
	VLDEBUGBEGIN;
        retval = list_name(req, *commandp, client_dir, dir_magic_no, comps, verify_dir, 
                           localexp, non_object_attrib_fl, listall_flag, filters); 
	VLDEBUGEND;
        tklfree(comps);
    }
    fllfree(filters);
    return retval;
}


static int 
list_process_filters_err_reply(RREQ req, P_OBJECT ob, FILTER filters);
static int 
list_expand_local_ulinks(RREQ req, P_OBJECT ob);

static int 
list_name(RREQ req, char *command, char arg_client_dir[], int dir_magic_no,
          TOKEN remcomp, int verify_dir,
          int localexp, int non_object_attrib_fl, int alllinks, FILTER filters)
{
    char *thiscomp;             /* component being worked on */
    AUTOSTAT_CHARPP(client_dirp);	/* Current directory.  We make this a
                                   temporary, because it is modified by
                                   list_name. */ 
    int item_count = 0;	/* Count of returned items                 */
    FILTER cfil;	/* For iterating through filters.          */
    VLINK clink;	/* For stepping through links              */
    VLINK crep;	/* For stepping through replicas           */
    long dir_version = 0;	/* Directory version nbr-currently ignored */
    AUTOSTAT_CHARPP(dir_typep); /* Type of dir name (Currently, the only
                                   supported value is ASCII)  */
    P_OBJECT      ob = oballoc(); /* allocate an object to read into.       */
    P_OBJECT      lastob = NULL; /* last object used. */
    int retval;	/* Return value from subfunctions          */
    int laclchkl;	/* Cached ACL check                        */
    int daclchkl;	/* Cached ACL check                        */
    int laclchkr;	/* Cached ACL check                        */
    int daclchkr;	/* Cached ACL check                        */
    PATTRIB ca;	/* Current Attribute                       */
    OUTPUT_ST out_st;
    OUTPUT out = &out_st;
    int orig_localexp = localexp;	/* don't change this cached copy. */
    struct dsrobject_list_options listopts;

    reqtoout(req, out);

    *client_dirp = stcopyr(arg_client_dir, *client_dirp);
    *dir_typep = stcopyr("ASCII", *dir_typep); /* XXX must generalize this. */

list_start:

    /* list_start always is entered with a clean directory. */
    thiscomp = remcomp->token;
    remcomp = remcomp->next;
    /* THISCOMP refers to a single name-component.  REMCOMP may refer to
       additional components of a multiple-component name that should be
       resolved. */

    /* If only expanding last component, clear the flag */
    if (localexp == 1)
	localexp = 0;
    /* If remaining components, expand union links for this component only */
    if (remcomp && !localexp)
	localexp = 1;

    p_clear_errors();

    dsrobject_list_options_init(&listopts);
    listopts.thiscompp = &thiscomp;
    listopts.remcompp = &remcomp;
    listopts.requested_attrs = "#INTERESTING";
    listopts.req_link_ats.interesting = 1; /* just request INTERESTING. */
    listopts.filters = filters;
    /* dsrobject() might expand multiple components. */
    VLDEBUGBEGIN;
    retval = dsrobject(req, *dir_typep, *client_dirp, dir_version, dir_magic_no, 
                       verify_dir? DRO_VERIFY_DIR : 0, &listopts, ob);
    VLDEBUGOB(ob);
    VLDEBUGEND;

#ifdef DIRECTORYCACHING
    if (retval) dsrobject_fail++;
#endif
    if (retval == DSRFINFO_FORWARDED) {
        /* We are NOT in the process of expanding a union link, since if we
           were, the above test would have caught it. */
        if (lastob) {           /* if exploring a subcomponent */
            /* XXX We should update the local link to point to the new location
               too */
            if (verify_dir) {
                reply(req, "NONE-FOUND\n");
                if (lastob) obfree(lastob);
                if (ob) obfree(ob);
                return PSUCCESS;
            }
            retval = oblinkforwarded(req, out, *client_dirp, dir_magic_no, ob,
                                     thiscomp);
            if (retval) {
                if (lastob) obfree(lastob);
                obfree(ob);
                return retval;
            }
            if (remcomp) {
#ifdef SERVER_SEND_NEW_MCOMP
                replyf(req, "UNRESOLVED");
                out_sequence(out, remcomp);
#else
                TOKEN acp;

                replyf(req, "UNRESOLVED %'s", remcomp->token);
                for (acp = remcomp->next; acp; acp = acp->next)
                    replyf(req, "/%'s", acp->token);
                reply(req, "\n");
#endif
            }
        } else {
            retval = obforwarded(req, *client_dirp, dir_magic_no, ob);
            if (lastob) obfree(lastob);
            obfree(ob);
            return retval;
        }
        if (lastob) obfree(lastob);
        obfree(ob);
        return PSUCCESS;
    }
    /* Even if we were exploring a subcomponent, still cool to return these
       error messages. */
    /* If not a directory, say so */
    if (retval == DIRSRV_NOT_FOUND || 
        retval == PSUCCESS && !(ob->flags & P_OBJECT_DIRECTORY)) {
	creply(req, "FAILURE NOT-A-DIRECTORY\n");
        if (lastob) obfree(lastob);
        obfree(ob);
	RETURNPFAILURE;
    }
    /* If not authorized, say so */
    if (retval == DIRSRV_NOT_AUTHORIZED) {
	if (*p_err_string)
	    creplyf(req, "FAILURE NOT-AUTHORIZED %'s\n", p_err_string, 0);
	else
	    creply(req, "FAILURE NOT-AUTHORIZED\n");
        if (lastob) obfree(lastob);
        obfree(ob);
	RETURNPFAILURE;
    }
    /* If too many links in response, say so */
    if (retval == DIRSRV_TOO_MANY) {
	if (*p_err_string)
	    creplyf(req, "FAILURE TOO-MANY %'s\n", p_err_string, 0);
	else
	    creply(req, "FAILURE TOO-MANY\n");
        if (lastob) obfree(lastob);
        obfree(ob);
	RETURNPFAILURE;
    }
    /* If some other failure, say so */
    if (retval) {
	if (*p_err_string)
	    creplyf(req, "FAILURE SERVER-FAILED %'s %'s\n", p_err_text[retval],
                    p_err_string);
	else
	    creplyf(req, "FAILURE SERVER-FAILED %'s\n", p_err_text[retval]);
        if (lastob) obfree(lastob);
        obfree(ob);
	return (retval);
    }
    if (*p_warn_string) {
	replyf(req, "WARNING MESSAGE %'s\n", p_warn_string);
	*p_warn_string = '\0';
    }
    list_expand_local_ulinks(req, ob); /* pass request just to give the security context. */
    /* Cache the default answers for ACL checks */
    daclchkl = srv_check_acl(ob->acl, ob->acl, req, "l",
			     SCA_LINKDIR,*client_dirp,NULL);
    daclchkr = srv_check_acl(ob->acl, ob->acl, req, "r",
			     SCA_LINKDIR,*client_dirp,NULL);
    if (retval = list_process_filters_err_reply(req, ob, filters)) {
        if (lastob) obfree(lastob);
        obfree(ob);
        return retval;
    }
    /* When we reach this point, there's no going back.  
       Just spit out the links and clean up. */

    /* Here we must send back the links, excluding those that do */
    /* not match the component name. For each link, we must also */
    /* send back any replicas or links with conflicting names    */
    for (clink = ob->links; clink; clink = clink->next) {
	crep = clink;
	while (crep) {
            /* Check individual ACL only if necessary */
	    if (crep->acl) {
		laclchkl = srv_check_acl(crep->acl, ob->acl, req, "l",
					 SCA_LINK,NULL,NULL);
		laclchkr = srv_check_acl(crep->acl, ob->acl, req, "r",
					 SCA_LINK,NULL,NULL);
	    } else {
                laclchkl = daclchkl;
                laclchkr = daclchkr;
            }
	    if (!verify_dir && wcmatch(crep->name, thiscomp) &&
                crep->linktype != '-' &&
		(laclchkl || (laclchkr &&
			(strcmp(crep->name, thiscomp) == 0)))) {
		if (laclchkr) {
		    if (remcomp && strequal(crep->host, hostwport) &&
			!(crep->filters) && !item_count) {
                        /* If components remain on this host    */
                        /* And links haven't already been returned */
                        /* don't reply, but continue searching  */
                        /* Here's where we start to resolve additional
                           components */ 
                        /* Set the directory for the next component */

                        /* At this point, clink contains the link for the next
                           directory, and the directory itself is still filled
                           in.  We should save away the old directory
                           information in case we need to backtrack */
                        dir_version = clink->version;
                        dir_magic_no = clink->f_magic_no;

                        *dir_typep = stcopyr(clink->hsonametype, *dir_typep);
                        *client_dirp = stcopyr(clink->hsoname, *client_dirp);

                        if (lastob) obfree(lastob);
                        lastob = ob;
#if 0
                        /* Save last remcomp in case this lookup fails.  This
                           is actually not currently being used. */  
                        last_remcomp = remcomp;     
#endif
                        ob = oballoc();
                        
                        goto list_start;
		    }
		    qoprintf(out, "LINK ");
		    out_link(out, crep, 0, (TOKEN) NULL);
		/* If link attributes are to be returned, do so */
		/* For now, only link attributes returned       */
		/* XXX this is not all of what we want.  We need to be able to
		   explicitly read the values of attributes such as DEST-EXP
		   and ID.  But this is all we need to get Gopher running. */
		    if (non_object_attrib_fl) {
			for (ca = crep->lattrib; ca; ca = ca->next) {
			/* For now return all attributes. To be done: */
			/* return only those requested                */
			    if (1) {
				out_atr(out, ca, 0);
			    }
			}
		    } else {
		    /* No attribute flag specified.   Return ACCESS-METHOD for
		       EXTERNAL links anyway. */
			if (strequal(crep->target, "EXTERNAL"))
			    for (ca = crep->lattrib; ca; ca = ca->next)
				if (strequal(ca->aname, "ACCESS-METHOD"))
				    out_atr(out, ca, 0);
		    }
		/* If there are any filters, send them back too */

		    if (laclchkr) {
			for (cfil = crep->filters; cfil; cfil = cfil->next) {
#ifdef REALLY_NEW_FIELD
			    qoprintf(out,
                                     "ATTRIBUTE LINK FIELD FILTER FILTER ");
#else
			    qoprintf(out, "ATTRIBUTE LINK FIELD FILTER ");
#endif
			    out_filter(out, cfil, 0);
			}
		    }
		} else {
		/* If list access but no read access, just acknowledge that
		   the link exists. */
		    replyf(req, "LINK L NULL %s NULL NULL NULL NULL 0\n",
			crep->name);
		}
		item_count++;
	    }
	/* Replicas are linked through next, not replicas */
	/* But the primary link is linked to the replica  */
	/* list through replicas                          */
	    if (crep == clink)
		crep = crep->replicas;
	    else
		crep = crep->next;
	}
    }

    /* here we must send back the unexpanded union links */
    for(clink = ob->ulinks; clink && !verify_dir; clink = clink->next) {
        /* Neither return nor expand links that have been successfully expanded
           or that are dummies. 
           */ 
	if (clink->expanded == ULINK_PLACEHOLDER || clink->expanded == TRUE)
	    continue;	/* never return or process this link. */
        /* If the user explicitly requested that union links be expanded
           (original localexp == 2) or if we ended up expanding these links
           as part of the intermediate stages of processing a multi-
           component name (localexp == 1) then the user doesn't care about
           seeing union links for the current directory, nor does he or she
           care about getting two union links that agree in hsoname and
           host but disagree in their NAME field.   However, if the user
           wants to see all the contents of the current directory, then
           they very much do care to see conflicting union links. */
        if (clink->expanded == ULINK_DONT_EXPAND && orig_localexp)
            continue;
        /* Return any readable union links. */
	if (srv_check_acl(clink->acl, ob->acl, 
                          req, "r", SCA_LINK,NULL,NULL)) {
            /* Once one union link is going to be returned, don't do any more
               expanding. */ 
	    localexp = 0;
	    qoprintf(out, "LINK ");
	    out_link(out, clink, 0, (TOKEN) NULL);
            /* If link attributes are to be returned, do so */
            /* For now, only link attributes returned       */
            /* XXX this is not all of what we want.  We need to be able to
               explicitly read the values of attributes such as DEST-EXP
               and ID.  But this is all we need to get Gopher running. */
            if (non_object_attrib_fl) {
                for (ca = clink->lattrib; ca; ca = ca->next) {
                    /* For now return all attributes. To be done: */
                    /* return only those requested                */
                    if (1) {
                        out_atr(out, ca, 0);
                    }
                }
            } 
	    item_count++;
            /* if there are any filters */
	    for (cfil = clink->filters; cfil; cfil = cfil->next) {
#ifdef REALLY_NEW_FIELD
		qoprintf(out, "ATTRIBUTE LINK FIELD FILTER FILTER ");
#else
		qoprintf(out, "ATTRIBUTE LINK FIELD FILTER ");
#endif
		out_filter(out, cfil, 1);
	    }
	}
    }

 /* If none match, say so */
    if (!item_count)
	reply(req, "NONE-FOUND\n");
 /* Otherwise, if components remain say so */
    else if (remcomp) {
#ifdef SERVER_SEND_NEW_MCOMP
	replyf(req, "UNRESOLVED");
        out_sequence(out, remcomp);
#else
        TOKEN acp;

        replyf(req, "UNRESOLVED %'s", remcomp->token);
        for (acp = remcomp->next; acp; acp = acp->next)
            replyf(req, "/%'s", acp->token);
        reply(req, "\n");
#endif
    }

   /* Free the directory links */
    if (lastob) obfree(lastob);
    if (ob) obfree(ob);
    return PSUCCESS;
}


static int
list_process_filters_err_reply(RREQ req, P_OBJECT ob, FILTER filters)
{
#ifdef ARCHIE
    int already_applied_ar_domain = 0;
    int already_applied_ar_pathcomp = 0;
#endif

    /* PUT IN CODE HERE TO PROCESS PREDEFINED SERVER FILTERS */

    for (; filters; filters = filters->next) {
        if (filters->pre_or_post == FIL_ALREADY) {
#ifdef ARCHIE
            if (strequal(filters->name, "AR_DOMAIN")) 
                ++already_applied_ar_domain;
            if (strequal(filters->name, "AR_PATHCOMP")) 
                ++already_applied_ar_pathcomp;
#endif
            continue;
        }
        if (filters->pre_or_post == FIL_FAILED) {
            if (filters->errmesg) {
                creplyf(req, "FAILURE FILTER-APPLICATION Applying the %'s \
PREDEFINED SERVER filter yielded an error: %'s\n",
                        filters->name, filters->errmesg);
            } else {
                creplyf(req, "FAILURE FILTER-APPLICATION Applying the %'s \
PREDEFINED SERVER filter yielded an error.\n",
                        filters->name);
            }
        }
#ifdef SERVER_PREDEFINED_IDENTITY_FILTER
        /* this only exists for debugging and demonstration purposes.  */
        if (strequal(filters->name, "IDENTITY")) {
            int retval = identity_filter(ob, filters->args);
            if (retval) {
                creplyf(req, "FAILURE FILTER-APPLICATION Applying the %'s \
PREDEFINED SERVER filter yielded an error: %'s\n",
                        filters->name, p_err_string);
                return retval;
            }
        } else
#endif

#ifdef ARCHIE
        /* If specified should have been applied as part of query. 
           If it were applied as part of the archie database query, it would 
           have been taken care of by the test above against FIL_ALREADY. */
        if (strequal(filters->name, "AR_DOMAIN") ||
            strequal(filters->name, "AR_PATHCOMP")) {
            if (!already_applied_ar_domain || !already_applied_ar_pathcomp)
                creplyf(req, "FAILURE NOT-FOUND FILTER version %d archie servers do not support the PREDEFINED SERVER filter named %'s.\n",
                        archie_supported_version, filters->name);
            else
                creplyf(req, "FAILURE FILTER-APPLICATION The filter %'s \
cannot be applied more than once.\n", filters->name);
            RETURNPFAILURE;
        } else
#endif	/* ARCHIE */

        {
        /* Filter not found. */
            creplyf(req, "FAILURE NOT-FOUND FILTER This server does not \
support the PREDEFINED SERVER filter named %'s.\n", filters->name);
            RETURNPFAILURE;
        }
    }
    return PSUCCESS;            /* all were succesfully processed */
}

#ifdef SERVER_PREDEFINED_IDENTITY_FILTER
/* Return PSUCCESS on success; failure indication otherwise & set p_err_string.
   This is a sample filter to demonstrate how one writes such things.  It's
   also useful for debugging filters. */
static int identity_filter(P_OBJECT dirob, TOKEN args)
{
    dirob->inc_native = VDIN_MUNGED;
    return PSUCCESS;
}

#endif /* SERVER_PREDEFINED_IDENTITY_FILTER */


/* Pass the request just to give the security context.
 */
/* Expand all local union links where we have 'r' permission on the link in the
   context of this directory. */
static int
list_expand_local_ulinks(RREQ req, P_OBJECT ob)
{
    ob->inc_native = VDIN_MUNGED;
#if 0
    /* Make sure that ob has enough information in it (or enough info. is
       passed down to expand_local_ulinks) so that we can be sure to keep track
       of what the starting directory for expansion was. */
    if (localexp) {
        /* Add a placeholder union link to indicate that the current directory
           is already being expanded, so don't expand it again.  This handles
           recursive union links (a fairly frequent case) more efficiently than
           not including this check.  */
        uexp = vlalloc();
	assert(uexp);
        uexp->host = stcopyr(hostwport, uexp->host);
         uexp->hsoname = stcopyr(*client_dirp, uexp->hsoname);
        uexp->f_magic_no = dir_magic_no;
        uexp->expanded = ULINK_PLACEHOLDER; 
        dir->ulinks = uexp;
    }
#endif
    return PSUCCESS;
}
