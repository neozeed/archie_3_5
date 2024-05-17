/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>

/* Written, 9/18/92, swa@isi.edu */
/* Made use 'previous'  member 6/14/93, swa@isi.edu */
/* Got rid of double invocation of stcopy() for efficiency,. 5/15/94,
   swa@ISI.EDU */

/* Break up a string into space-separated tokens and recognize Prospero
   quoting.   This strips off the quoting in the process.  This function could
   also be called p__tokenize_newstyle_mcomp(), and it serves that function.
   It assumes the input string is correctly quoted.
 */

#include <pfs.h>
#include <pprot.h>

TOKEN
qtokenize(const char *s)
{
    TOKEN retval;
    int tmp;

    AUTOSTAT_CHARPP(bufp);

    /* We need a new interface to tkalloc() so we can avoid doing two stcopy()
       operations on the same data. */
    /* swa, 5/15/94: I decided to just make the code a bit longer, but used the
       existing TKALLOC interface instead.   This seems like less hassle than a
       new interface, since it is not used very frequently. */

    if (!s) return NULL;
    retval = NULL;
    for (;;) {
#if 1                           /* 5/15/94 change --swa */
        TOKEN tmptok;
#endif
        tmp = qsscanf(s, "%'&s %r", bufp, &s);
        assert(tmp >= 0);            /* no possible error, I hope! */
        if (tmp == 0)
            return retval;
#if 1
        tmptok = tkalloc((char *) NULL);
        tmptok->token = *bufp;

        *bufp = NULL;           /* Leaving out this step cost almost a day's
                                   work.  Since *bufp sticks around, we should
                                   mark it as free so that it is not reused
                                   inappropriately.  I now understand what
                                   broke and why. --swa, 5/16/94 */
        APPEND_ITEM(/* new */ tmptok, /* head */ retval);
        /* This version just avoided the stcopy(). */
#else
        /* This is the old less efficient version:  --swa, 5/15/94 */
        retval = tkappend(*bufp, retval);
#endif
        if (tmp == 1) return retval; /* no more to tokenize. */
    }
}
