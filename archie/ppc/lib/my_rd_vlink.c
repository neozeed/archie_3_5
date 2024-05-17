/*  
 *  The only point of having this here is to have an rd_vlink() that returns
 *  attributes.
 *
 *  - wheelan (Thu Dec  2 22:12:44 EST 1993)
 */  

/*
 * Copyright (c) 1989, 1990 by the University of Washington
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>
 */

#include <stdio.h>
# ifdef SOLARIS
#include <string.h>
#else
# include <strings.h>
#endif
#include <uw-copyright.h>
#include <usc-copyr.h>
#include <pfs.h>
#include <perrno.h>
#include <pmachine.h>
#include "ansi_compat.h"
#include "ppc_front_end.h"
#include "protos.h"
#include "all.h"

  
static VLINK rd_vslink();

/*
 * rd_vlink will read the named link, expanding any symbolic links
 * along the way.   Call rd_slink if you do not want to expand the 
 * final sym_link.
 */
VLINK
my_rd_vlink(const char *path)
{
  return rd_vslink(path,SYMLINK_NESTING);
}

extern char *p_uln_lastcomp_to_linkname(const char *s);


static VLINK
rd_vslink(path,depth)
    const char	*path;	/* Pathname for links to be returned      */
    int		depth;  /* How many levels of sym links to expand */
{
    char		pth[MAX_VPATH];  /* Working copy of pathname       */
    char		*p = pth;        /* So we can use it as a pointer  */

    char		*c;		 /* Component part of name         */
    char		*d;		 /* Directory part of name	   */
    char		*slash;		 /* Position of slash              */
    char		*colon;		 /* Position of colon              */

    int		flags;

    VDIR_ST		dir_st;
    VDIR		dir = &dir_st;
    VLINK		v;

    vdir_init(dir);

    /* special case just a . as a name for the current working directory,
       by mapping it onto the empty string. . */
    if (strequal(path, ".")) ++path;
    strcpy(p,path);

    d = p;
    slash = p_uln_rindex(p,'/');
    colon = p_uln_rindex(p,':');

    if(colon && (!slash || (colon > slash))) {
        *(colon + 1) = '\0';
        c = p_uln_lastcomp_to_linkname(p_uln_rindex(path,':') + 1);
    }
    else if(slash) {
        if(slash == p) d = "/";
        *slash = '\0';
        c = p_uln_lastcomp_to_linkname(slash + 1);
    }
    else if (strequal(path, "..")) {
        /* Special case just .. as a name for the superior directory. */
        d = "..";
        c = NULL;
    } else {
        d = "";
        c = p_uln_lastcomp_to_linkname(p);
    }

    if(c && *c) 	flags = RVD_FIND;
    else flags = RVD_DFILE_ONLY;

#if 1
    perrno = ppc_rd_vdir(d,c,dir,flags | GVD_ATTRIB|GVD_EXPAND|GVD_NOSORT);
#else
    perrno = ppc_rd_vdir(d,c,dir,flags);
#endif

    if(perrno || !dir->links) {
        if (!perrno) perrno = PFAILURE;
        vllfree(dir->links);
        vllfree(dir->ulinks);
        return(NULL);
    }

    vllfree(dir->ulinks);

    if((strncmp(dir->links->target,"SYMBOLIC",8) == 0) &&
       (strncmp(dir->links->hosttype,"VIRTUAL-SYSTEM",14) == 0)) {
        if(depth > 0) {
            if (sizeof pth < 
                qsprintf(pth, sizeof pth,
                         "/VIRTUAL-SYSTEMS/%s/ROOT%s%s",dir->links->host,
                         ((*(dir->links->hsoname) == '/') ? "" : "/"),
                         dir->links->hsoname))
                internal_error("pth too small!");
            v = rd_vslink(pth,depth-1);
            vllfree(dir->links);
            if (!perrno) perrno = PFAILURE;
            return(v);
        }
        else if(depth == 0) {
            vllfree(dir->links);
            perrno = PFS_SYMLINK_DEPTH;
            return(NULL);
        } /* depth < 0 means don't expand symlink. */
    }
    return(dir->links);
}

