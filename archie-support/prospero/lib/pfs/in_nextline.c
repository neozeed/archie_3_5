/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h> 
 */

#include <usc-license.h>

#include <ardp.h>
#include <pfs.h>
#include <pparse.h>
#include <ctype.h>     /* For isascii */
/* isascii() is not provided in POSIX, so we just do it here. */
#ifndef isascii
#define isascii(c)	((unsigned)(c)<=0177)
#endif

/* XXX Assumes that the longest possible word we could read in is
   ARDP_PTXT_LEN_R.  This is actually a bogus assumption; one can trigger the
   assertion by sending a packet with a first word longer than that.  
   We don't cache the value returned from in_nextline() so it's safe to use
   static data.  Keywords (1st word in a line) are never quoted; this is safe
   then.  
*/
/* This function returns a pointer to internal data which will be overwritten
   on the next call to in_nextline().
   This function is used for lookahead in the parsing, and could be replaced
   with other routines to do that.
*/

char *
in_nextline(INPUT oldin)
{
    INPUT_ST in_st;
    INPUT in = &in_st;
    AUTOSTAT_CHARPP(bufp);
    char *cp;				/* Pointer into the buffer*/
    int c;
    
    if (!*bufp) *bufp = stalloc(ARDP_PTXT_LEN_R);
    cp = *bufp;
    
    input_copy(oldin, in);
    if((c = in_readc(in)) == EOF) /* test for EOF */
        return NULL;
    while ((c != EOF) && isascii(c) && !isspace(c)) {
        *(cp++) = c;
        in_incc(in);
        c = in_readc(in);
        assert(cp - *bufp < p__bstsize(*bufp));
    }
    *cp = '\0';
    return *bufp;
}
