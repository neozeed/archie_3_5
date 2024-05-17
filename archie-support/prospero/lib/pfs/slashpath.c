/*
 * Copyright (c) 1993 by the University of Southern California
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 */

#include <usc-copyr.h> 

#include <pfs.h>

static int oldpathlen;
#ifndef NDEBUG                  /* for assertions */
static char *oldnextcomp;
#endif

TOKEN
p__slashpath2tkl(char *nextcomp)
{
    TOKEN retval = NULL;
    char *cp;

    assert(P_IS_THIS_THREAD_MASTER());
    oldpathlen = 0;
#ifndef NDEBUG
    oldnextcomp = nextcomp;
#endif
    if (!nextcomp) return NULL;
    for (cp = nextcomp; *cp; cp++) {
        if (*cp == '/') {
            if (cp == nextcomp)
                /* eliminate double-slashes */
                ++nextcomp;
            else {
                *cp = '\0';
                ++oldpathlen;
                retval = tkappend(nextcomp, retval);
                *cp = '/';      /* restore it */
                nextcomp = cp + 1;
            }
        }
    }
    /* Handle the last component. */
    if (cp > nextcomp)/* More components.  (Path didn't end in a slash). */
        tkappend(nextcomp, retval);
    return retval;
}


/* Should only be called when p__slashpath2tkl was called. */
void
p__tkl_back_2slashpath(TOKEN nextcomp_tkl, char *nextcomp)
{
    char buf[MAX_VPATH];

    assert(P_IS_THIS_THREAD_MASTER());
    /* Make sure the functions were called in the proper order.  This assertion
       helps make sure we don't write too much back into the old nextcomp
       buffer. */ 
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
            
