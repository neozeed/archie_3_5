/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file 
 * <usc-license.h>
 */

#include <usc-license.h>

/* Written, 9/18/92, swa@isi.edu */
/* Made use 'previous'  member 6/14/93, swa@isi.edu */
/* Turned into qbstokenize 19 Nov 1993, swa@isi.edu */

/* Break up a string into space-separated tokens and recognize Prospero
   quoting.   This strips off the quoting in the process.  This function could
   also be called p__tokenize_newstyle_mcomp(), and it serves that function.
   It assumes the input string is correctly quoted.
 */

#include <pfs.h>

/* loc is a pointer into the string pointed to by header, where we 
   should start tokenizing, it is NOT const, but used within the loop,
   however since not passed as &loc, it wont be returned as NULL
*/
TOKEN
#ifdef OLDSWA
p__qbstokenize(const char * head, const char *loc)
#else
p__qbstokenize(const char * head, char *loc)
#endif
{
    TOKEN retval;
    int tmp;

    CHECK_MEM();
    CHECK_PTRinBUFF(head,loc);
    if (!head || !loc) return NULL;
    retval = NULL;
    for (;;) {
        TOKEN tk;
        char *buf = NULL;
        tmp = p__qbstscanf(head, loc, "%'&b %r", &buf, &loc);
        assert(tmp >= 0);            /* no possible error, I hope! */
        if (tmp == 0)
            return retval;
        tk = tkalloc(NULL);
        tk->token = buf; buf = NULL;
        APPEND_ITEM(tk, retval);
        if (tmp == 1) return retval; /* no more to tokenize. */
    }
}
