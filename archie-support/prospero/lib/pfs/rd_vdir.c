/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>

#include <stdio.h>
#include <string.h>

#include <pfs.h>
#include <perrno.h>
#include <pmachine.h>

static TOKEN p__rvdslashpath2tkl(char *nextcomp);
static void p__tkl_back_2rvdslashpath(TOKEN nextcomp_tkl, char *nextcomp);

/*
 */
int
rd_vdir(dirarg,comparg,dir,flags)
    const char	*dirarg;	/* Virtual name for the directory	     */
    const char	*comparg;	/* Component name (wildcards allowed)        */
    VDIR	dir;		/* Structure to be filled in		     */
    long	flags;		/* Flags			             */
{
    /* Note: We use a working copy of dirarg because we want to        */
    /*       modify the working copy, and we would prefer not to       */
    /*       modify the directory name passed to this procedure.       */
    /*       dirnm is required because we need a character pointer     */
    /*       later on (dirname), but the compiler only allows          */
    /*       automatic allocation for a character array.               */
    /*       components is a working copy of comparg which might be    */
    /*       modified if a magic number has been specified.            */
    char		dirnm[MAX_VPATH];  /* Working copy of dirarg       */
    char		*dirname = dirnm;  /* Pointer to dirnm             */
    char		components[MAX_VPATH];/* Working copy of comparg   */

    char		*dirhst;	   /* Host of current directory    */
    char		*remdir;	   /* Dir on remote host	   */
    char		*homedir;	   /* Name of home directory       */
    char		*workdir;	   /* Name of working directory    */
    int		hdlen;             /* Length of homedir name       */
    int	        wdlen;             /* Length of workdir name       */
    char		relativeto;	   /* 0 = name relative to root    */
    FILTER		filters = NULL;	   /* Filters to be applied        */
    char		*ndname;	   /* Next component of dirname    */
    char		*magic_str;	   /* Magic number part of ndname  */
    int		magic_no;	   /* Magic number from magic_str  */
    int		poundsign;         /* Flag, nextcomp contains #    */
    char		wdname[MAX_VPATH]; /* Working directory name	   */
    VLINK		ltmp;		   /* To look for unexpanded links */
    PATTRIB		closure;           /* Namespace from closed object */
    int		retval;		   /* Return value		   */
    int		depth;		   /* Depth for sym-link expansion */
    char		stmp[MAX_VPATH];   /* Temporary string             */
    char		*colon;		   /* Pointer to colon in path     */
    char		*nsname;	   /* Possible name of new ns      */
    int		beyondcolon;	   /* Length of text after colon   */
    char		*nextcomp;	   /* 4 steppng through components */
    long		intflags;	   /* Flags for intermediate query */

    depth = SYMLINK_NESTING;

    strcpy(dirname,dirarg);
    strcpy(components,(comparg ? comparg : ""));

    nsname = dirname;

    colon = p_uln_index(dirname,':');
    if(colon) {
        /* Name can not start with a ":" */
        if(colon == nsname) return(RVD_NS_NOT_FOUND);

        /* Don't support name space aliases yet, so return an error */
        /* if the colon is preceded by a "#".                       */
        if(*(colon-1) == '#') return(RVD_NO_NS_ALIAS);

    }

    /* In most cases, we start from a specific directory      */
    /* and don't care about the full pathname from root       */
    /* We only want to deal with the pathname for the root    */
    /* directory if the name starts with ../'s or ~/../'s     */
    *wdname = '\0';

    if(*dirname == '\0') {
        relativeto = 2;
        dirhst = pget_wdhost();
        remdir = pget_wdfile();
    }
    else if(*dirname == '/') {
        relativeto = 0;
        dirhst = pget_rdhost();
        remdir = pget_rdfile();
        dirname++;
    }
    else if((strncmp(dirname,"~/",2) == 0)||(strcmp(dirname,"~") == 0)) {
        /* Set wdname to homedir and later check for ../'s */
        relativeto = 1;
        strcpy(wdname,pget_hd());
        dirhst = pget_hdhost();
        remdir = pget_hdfile();
        dirname++;
        /* We only want to step over the next byte if it is not the */
        /* end of the string.  It will be if string was ~           */
        if(*dirname) dirname++;
    }
    else {
        /* Set wdname and later check for ../'s */
        relativeto = 2;
        workdir = pget_wd();
        if(workdir) strcpy(wdname,workdir);
        else return(PFS_ENV_NOT_INITIALIZED);
        dirhst = pget_wdhost();
        remdir = pget_wdfile();
    }

    /* If Prospero environment has not been initialized return error */
    if((dirhst == NULL) || (*dirhst == '\0')) 
        return(PFS_ENV_NOT_INITIALIZED);

    /* If we still allow ../'s and the dirname starts  */
    /* as such, determine the correct name for the new */
    /* file relative to VSROOT, and use that instead   */
    if(*wdname && (strncmp(dirname,"..",2) == 0)) {
        char	*slash;

        relativeto = 0;
        dirhst = pget_rdhost();
        remdir = pget_rdfile();
        while((strncmp(dirname,"../",3) == 0) || 
              (strcmp(dirname,"..") == 0)) {

            dirname += 2;
            if(*dirname == '/') dirname++;

            slash = p_uln_rindex(wdname,'/');
            colon = p_uln_rindex(wdname,':');

            if(slash && (!colon || (slash > colon))) *slash = '\0';
            else if(colon) *(colon+1) = '\0';
            else *wdname = '\0';
            if(!*wdname) strcpy(wdname,"/");
        }

        /* Remove a ./ if it caused termination of ../s */
        if(strncmp(dirname,"./",2) == 0) {
            dirname++;
            dirname++;
        }

        if(*dirname && (*(wdname + strlen(wdname)-1) != '/'))
            strcat(wdname,"/");
        strcat(wdname,dirname);
        dirname = wdname;
    }

    /* Special case "." if appears by itself */
    if(strcmp(dirname,".") == 0) {
        dirname++;
    }

    /* And finally, remove a ./ if it appears at start */
    else if(strncmp(dirname,"./",2) == 0) {
        dirname++;
        dirname++;
    }

    /* Check to see if working or home directory is a */
    /* prefix of the directory to be listed           */
    if(! (flags & RVD_NOCACHE) ) {
        workdir = pget_wd();
        homedir = pget_hd();
        if((relativeto == 0) && (*dirname != '/')) {
            workdir++;
            homedir++;
        }
        wdlen = strlen(workdir);
        hdlen = strlen(homedir);
        /* Do not cache if remaining part of name has any       */
        /* single colons.  Right now the check disables caching */
        /* if it finds any colons.  It should allow it if it is */
        /* a double colon.                                      */
        if((strncmp(dirname,workdir,wdlen) == 0) &&
           ((hdlen <= wdlen) || (strncmp(dirname,homedir,hdlen) != 0)) &&
           ((*(dirname+wdlen) == '/') || (*(dirname+wdlen) == '\0'))&&
           (p_uln_index(dirname+wdlen,':') == 0)) {
            dirname = dirname + wdlen;
            if(*dirname == '/') dirname++;
            relativeto = 2;
            dirhst = pget_wdhost();
            remdir = pget_wdfile();
        }
        else if((strncmp(dirname,homedir,hdlen) == 0) &&
                ((*(dirname+hdlen) == '/') || (*(dirname+hdlen) == '\0'))&&
                (p_uln_index(dirname+hdlen,':') == 0)) {		    
            dirname = dirname + hdlen;
            if(*dirname == '/') dirname++;
            relativeto = 1;
            dirhst = pget_hdhost();
            remdir = pget_hdfile();
        }
    }
    else flags &= (~ RVD_NOCACHE);

startover:

    /* The target of a symbolic link must be an absolute path */
    /* from the root of the specified virtual system.  Thus   */
    /* if chasing a link, we skip the code above that checks  */
    /* for ../s, and ~/s.                                     */

    nsname = dirname;

    /* If the directory includes :s, find correct namespace */
    while(colon = p_uln_index(dirname,':')) {

        /* If the vs name is null, return an error */
        if(colon == nsname) return(RVD_NS_NOT_FOUND);

        /* If an alias, return error.  Aliases are only allowed for */
        /* the first : and if an alias, it was resolved previously  */
        if(*(colon-1) == '#') return(RVD_NO_NS_ALIAS);

        /* object closure */
        if(*(colon+1) == ':') {
            *(colon++) = '\0';
            colon++;
            ltmp = rd_vlink(dirname);
            if(!ltmp) return(RVD_NO_CLOSED_NS);
            closure = pget_at(ltmp,"CLOSURE");
            vllfree(ltmp);
            if(!closure) return(RVD_NO_CLOSED_NS);
            /* XXX This must change! */
            if (closure->avtype != ATR_SEQUENCE) {
                atlfree(closure);
                return(RVD_NO_CLOSED_NS);
            }
            if (qsprintf(stmp,sizeof stmp,
                         "%s:%s",closure->value.sequence->token,colon) > 
                sizeof stmp)
                internal_error("stmp too small!");
            atlfree(closure);
            strcpy(dirnm,stmp);
            dirname = dirnm;
            nsname = dirname;
            continue;
        }

        /* text between nsname and colon is name space */
        *(colon++) = '\0';
        if (sizeof stmp < 
            qsprintf(stmp, sizeof stmp, 
                     "/VIRTUAL-SYSTEMS/%s/ROOT%s%s",nsname,
                     ((!*colon || (*colon == '/')) ? "" : "/"),colon))
            internal_error("stmp too small!");

        /* Remember how much text after colon before we overwite dirname */
        beyondcolon = strlen(colon);

        /* Keep part of dirname that defines root of old namespace */
        if((nsname > dirname) && (*(nsname-1) == '/')) *(nsname-1) = '\0';
        else *nsname = '\0';

        /* And append path to root of new one */
        strcat(dirname,stmp);

        /* Keep track of the root of the new name space */
        nsname = dirname + strlen(dirname) - beyondcolon;

        /* Start search from root of active name space */
        relativeto = 0;
        dirhst = pget_rdhost();
        remdir = pget_rdfile();
    }

    ndname = dirname;
    while(*ndname == '/') ndname++; /* multiple slashes treated as one slash */
    nextcomp = p_uln_index(ndname,'/');
    if(nextcomp) {
        *(nextcomp++) = '\0';
        if(!*nextcomp) nextcomp = NULL; /* trailing slash ignored. */
    }
    if(!*ndname) ndname = NULL;

    /* If we only want the directory file, and we have a null   */
    /* dirname (ndname is first component), then we must create */
    /* a fictitious directory entry from dirhst and remdir      */
    if(!ndname && (flags & RVD_DFILE_ONLY)) {
        VLINK vl;
        char *tmp_wdname = pget_wd();   /* working directory. */
        char *s;                /* link name */
        /* If links remain in dir, free them */
        vdir_freelinks(dir);

        /* Allocate the pseudo link */
        dir->links = vl = vlalloc();
        if (!vl) out_of_memory();

        /* and fill it in */
        vl->host = stcopy(dirhst);
        vl->hsoname = stcopy(remdir);
        vl->target = stcopyr("DIRECTORY", vl->target);
        /* Set a NAME field of some sort. */
        if (tmp_wdname && *tmp_wdname) {
            char *sp = p_uln_rindex(tmp_wdname, '/'); /* slashptr */
            char *cp = p_uln_rindex(tmp_wdname, ':'); /* colonptr */
            char *lastwdcomp;            /* last comp in tmp_wdname */
            s = ((cp < sp) ? sp : cp) + 1;
            if (s && *s) vl->name = stcopyr(s, vl->name);
            else vl->name = stcopyr(tmp_wdname, vl->name);
        } else if (s = strrchr(remdir, '/')) {
            vl->name = stcopyr(s, vl->name);
        } else {
            vl->name = stcopyr(remdir, vl->name);
        }
    }

    while(ndname) {
        TOKEN   nextcomp_tkl;   /* Token list form of nextcomp variable;
                                   a temporary var.  */
        TOKEN thiscomp_tkl; /* used to convert quoted component strings to
                                    normal format. */

        VLINK dlink = vlalloc();
        
        dlink->host = stcopyr(dirhst, dlink->host);
        dlink->hsoname = stcopyr(remdir, dlink->hsoname);
        dlink->filters = flcopy(filters, 1);

        /* Check for magic number */
        magic_no = 0;
        magic_str = p_uln_rindex(ndname,'#');
        /* Make sure that the rest of magic str is digits */
        if(magic_str && *(magic_str + 1) &&
           (strspn(magic_str+1,"-0123456789") == strlen(magic_str+1))) {
            *(magic_str++) = '\0';
            sscanf(magic_str,"%d",&magic_no);
        }

        /* Find out if there is a # in any remaining components */
        /* and if so, don't reolve multiple components          */
        if(nextcomp && p_uln_index(nextcomp,'#')) poundsign = 1;
        else poundsign = 0;

        intflags = (magic_no ? GVD_LREMEXP : GVD_FIND);
        if((*ndname == '#')&&(*(ndname+1))) {
            intflags = GVD_UNION;
            poundsign = 1;
        }
        /* if((flags & RVD_DFILE_ONLY) && (flags & RVD_ATTRIB))   */
        /*      intflags |= GVD_ATTRIB;                           */
        /* It might not be a directory in which case we want the  */
        /* attributes                                             */
        if(flags & RVD_ATTRIB) intflags |= GVD_ATTRIB;

        if ((*ndname != '#') || (!*(ndname + 1))) {
            thiscomp_tkl = p__rvdslashpath2tkl(ndname);
            assert(thiscomp_tkl && !thiscomp_tkl->next);
        } else
            thiscomp_tkl = NULL;
        if (!magic_no && !poundsign) {
            nextcomp_tkl = p__rvdslashpath2tkl(nextcomp);
            retval = p_get_dir(dlink, 
                              thiscomp_tkl? thiscomp_tkl->token : (char *)NULL,
                              dir, intflags, &nextcomp_tkl);
            p__tkl_back_2rvdslashpath(nextcomp_tkl, nextcomp);
            tklfree(nextcomp_tkl);
        } else {
            retval = p_get_dir(dlink, 
                              thiscomp_tkl? thiscomp_tkl->token : (char *)NULL,
                              dir, intflags, NULL);
        }
        if (thiscomp_tkl) tklfree(thiscomp_tkl);

        if(retval) {
	    vlfree(dlink);
            if (retval ==  DIRSRV_NOT_DIRECTORY) {
                p_err_string = qsprintf_stcopyr(p_err_string,
			"Directory %s not found.",ndname);
                return(PFS_DIR_NOT_FOUND);
            }
            else return(retval);
        }

        if(*ndname == '#' && *(ndname+1)) {
            vllfree(dir->links);
            dir->links = dir->ulinks;
            dir->ulinks = NULL;
            while(dir->links && (strcmp(dir->links->name,ndname+1) != 0)) {
                ltmp = dir->links;
                dir->links = dir->links->next;
                vlfree(ltmp);
            }
        }

        /* Find the one with the correct magic number. */
        /* We can get rid of those that don't match    */
        if(dir->links && magic_no && (magic_no != dir->links->f_magic_no)){
            ltmp = dir->links->replicas;
            while(ltmp && (ltmp->f_magic_no != magic_no)) {
                dir->links->replicas = ltmp->next;
                vlfree(ltmp);
                ltmp = dir->links->replicas;
            }
            /* found it, replace primary link */
            if(ltmp) {
                ltmp->replicas = ltmp->next;
                if(ltmp->replicas) ltmp->replicas->previous = NULL;
                ltmp->next = dir->links->next;
                ltmp->previous = dir->links->previous;
                dir->links->replicas = NULL;
                vlfree(dir->links);
                dir->links = ltmp;
            }
            /* Otherwise get rid of primary link */
            else {
                ltmp = dir->links;
                dir->links = ltmp->next;
                vlfree(ltmp);
                dir->links->previous = NULL;
            }
        }

        /* Can't find next directory link */
        if(dir->links == NULL) {
            p_err_string = qsprintf_stcopyr(p_err_string,
		"Directory %s not found.",ndname);

            /* If unexpanded ulinks, return temporary error */
            ltmp = dir->ulinks;
            while(ltmp) {
                if(ltmp->expanded != TRUE) {
		    vlfree(dlink);
                    return(RVD_DIR_NOT_THERE);
		}
                ltmp = ltmp->next;
            }
            vlfree(dlink);
            return(PFS_DIR_NOT_FOUND);
        }

        /* If a symbolic link, expand it */
        if((strncmp(dir->links->target,"SYMBOLIC",8) == 0) &&
           (strncmp(dir->links->hosttype,"VIRTUAL-SYSTEM",14) == 0)) {
            if(depth-- > 0) {
                sprintf(stmp,"%s:%s",dir->links->host,
                        dir->links->hsoname);
                if(nextcomp && !*nextcomp) nextcomp = NULL;
                while(ndname = nextcomp) {
                    nextcomp = p_uln_index(ndname,'/');
                    if(nextcomp) {
                        *(nextcomp++) = '\0';
                        if(*nextcomp == '\0') nextcomp = NULL;
                    }
                    strcat(stmp,"/");
                    strcat(stmp,ndname);
                }
                strcpy(dirnm,stmp);
                dirname = dirnm;
		vlfree(dlink);	/* Goto's considered harmfull! */
                goto startover;
            }
            else {
                vlfree(dlink);
                return(PFS_SYMLINK_DEPTH);
            }
            /* NOTREACHED */
            assert(0);          /* should never get here. */
        }

        /* found next directory - update dirhost and remdir - continue */
        dirhst = dir->links->host;
        remdir = dir->links->hsoname;
        filters = dir->links->filters;
        /* remove the next line */
        dir->links->filters = NULL;

        /* Find the next component of the path name */
        if(nextcomp && !*nextcomp) nextcomp = NULL;
        ndname = nextcomp;
        if(ndname) {
            nextcomp = p_uln_index(ndname,'/');
            if(nextcomp) {
                *(nextcomp++) = '\0';
                if(*nextcomp == '\0') nextcomp = NULL;
            }
        }

        /* If more components, and not a real link, then */
        /* return an error                               */
        if (ndname && (strcmp(dir->links->target,"DIRECTORY") != 0) &&
            (strcmp(dir->links->target,"FILE") != 0))  {
	    /* Ooops - Release does vlfree but not return, good
	       reason to bracket EVERY if  - Mitra  */
            vlfree(dlink);
            return (PFS_EXT_USED_AS_DIR);
    }
	vlfree(dlink); /* For drop-thru or loop */
    } /*while */

    /* if magic number specified, chop it off before making request */
    magic_no = 0;
    magic_str = p_uln_rindex(components,'#');
    /* Make sure that the rest of magic str is digits */
    if(magic_str && *(magic_str + 1) &&
       (strspn(magic_str+1,"-0123456789") == strlen(magic_str+1))) {
        *(magic_str++) = '\0';
        sscanf(magic_str,"%d",&magic_no);
        /* If alternate version specified, don't stop looking */
        if(flags & GVD_FIND) flags = (flags & (~GVD_FIND)) | GVD_LREMEXP;
    }

    /* If a normal link, then we do a listing of the */
    /* directory.  If not a directory, then get_vidr */
    /* will fail We will pass through the error code */
    /* Dir will contain the single directory link    */
    /* corresponding to the file                     */
    if(!dir->links || (strcmp(dir->links->target,"DIRECTORY") == 0) ||
       (strcmp(dir->links->target,"FILE") == 0)) {
        VLINK dlink = vlalloc();
        
        dlink->host = stcopyr(dirhst, dlink->host);
        dlink->hsoname = stcopyr(remdir, dlink->hsoname);
        dlink->filters = flcopy(filters, 1);
        retval = p_get_dir(dlink,components,dir,flags, NULL);

        /* Find the one with the correct magic number.      */
        /* We can get rid of those that don't match         */
        /* This code is not applicable for all flags combos */
        if(dir->links && magic_no && (magic_no != dir->links->f_magic_no)){
            ltmp = dir->links->replicas;
            while(ltmp && (ltmp->f_magic_no != magic_no)) {
                dir->links->replicas = ltmp->next;
                vlfree(ltmp);
                ltmp = dir->links->replicas;
            }
            /* found it, replace primary link */
            if(ltmp) {
                ltmp->replicas = ltmp->next;
                if(ltmp->replicas) ltmp->replicas->previous = NULL;
                ltmp->next = dir->links->next;
                ltmp->previous = dir->links->previous;
                dir->links->replicas = NULL;
                vlfree(dir->links);
                dir->links = ltmp;
            }
            /* Otherwise get rid of primary link */
            else {
                ltmp = dir->links;
                dir->links = ltmp->next;
                vlfree(ltmp);
                dir->links->previous = NULL;
            }
        }
        vlfree(dlink);
        return(retval);
    }
    else return(DIRSRV_NOT_DIRECTORY);

}

/* These are not thread safe */
static int oldpathlen;
#ifndef NDEBUG                  /* for assertions */
static char *oldnextcomp;
#endif

static
TOKEN 
p__rvdslashpath2tkl(char *nextcomp)
{
    TOKEN retval = NULL;
    char *cp;
    char thiscomp[MAX_VPATH];   /* buffer for current comp. strip backslashes.
                                   */ 
    char *thisp = thiscomp;

    assert(P_IS_THIS_THREAD_MASTER());
    oldpathlen = 0;
#ifndef NDEBUG
    oldnextcomp = nextcomp;
#endif
    if (!nextcomp) return NULL;
    for (cp = nextcomp; *cp; cp++) {
        if (*cp == '/') {
            *thisp = '\0';
            /* Skip multiple slashes (as rd_vdir() does.) */
            if (thisp > thiscomp) { 
                ++oldpathlen;
                retval = tkappend(thiscomp, retval);
                thisp = thiscomp;   /* set for next */
            }
            continue;
        } else if (*cp == '\\') {
            ++cp;
            if (*cp == '\0') --cp; /* special case trailing \; treat it as a
                                        literal backslash. */
        } 
        *thisp++ = *cp;         /* process quoted and literal characters */
    }
    /* Handle the last component.  Skip trailing slashes. (as rd_vdir does). */
    *thisp = '\0';
    if (thisp > thiscomp) {
        ++oldpathlen;
        retval = tkappend(thiscomp, retval);
    }
    return retval;
    
}


/* Should only be called when p__rvdslashpath2tkl was called. */
static 
void 
p__tkl_back_2rvdslashpath(TOKEN nextcomp_tkl, char *nextcomp)
{
    char buf[MAX_VPATH];

    /* Make sure the functions were called in the proper order.  This assertion
       helps make sure we don't write too much back into the old nextcomp
       buffer. */ 
    assert(P_IS_THIS_THREAD_MASTER());
    assert(oldnextcomp == nextcomp);
    *buf = '\0';
    /* Optimize common case (no change). */
    if (oldpathlen == length(nextcomp_tkl))
        return;
    assert(oldpathlen > length(nextcomp_tkl)); /* sanity */
    for(; nextcomp_tkl; nextcomp_tkl = nextcomp_tkl->next) {
        strcat(buf, nextcomp_tkl->token);
        strcat(buf, "/");
    }
    assert(strlen(buf) <= strlen(nextcomp));
    strcpy(nextcomp, buf);
}

