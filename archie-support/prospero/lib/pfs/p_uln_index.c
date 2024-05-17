/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>.
 */

#include <usc-copyr.h>

#include <pfs.h>

char *
p_uln_rindex(const char *s, char c)
{
    const char *lastmatch = NULL;

    for (; *s; ++s) {
        if (*s == '\\') {
            if (*++s == '\0')   /* special case this error condition */
                return (char *) lastmatch; /* done. Flush the CONST.*/
            else
                continue;       /* don't match a quoted symbol */
        }
        if (*s == c) lastmatch = s;
    }
    return (char *) lastmatch;  /* done.  flush the CONST. */
}

char *
p_uln_index(const char *s, char c)
{
    for (; *s; ++s) {
        if (*s == '\\') {
            if (*++s == '\0')   /* special case this error condition */
                return NULL;          /* done */
            else
                continue;       /* don't match a quoted symbol */
        }
        if (*s == c) return (char *) s; /* done.  flush the CONST. */
    }
    return NULL;
}

/* Get a linkname from the last component of a user level name. 
   Returns memory that will be freed on the next call. */
char *
p_uln_lastcomp_to_linkname(const char *s)
{
    static char *buf = NULL;
    char *outp;
    
  assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
    if (p__bstsize(buf) < strlen(s) + 1) {
        stfree(buf);
        buf = stalloc(strlen(s) + 1);
    }
    outp = buf;
    while (*s) {
        if (*s == '\\')
            ++s;
        *outp++ = *s++;
    }
    *outp = '\0';
    return buf;
}
