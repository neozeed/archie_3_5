/* Copyright (c) 1992, 1993 by the University of Southern California. */
/* For copying and distribution information, see the file <usc-copyr.h> */

#include <usc-copyr.h>

#include <pfs.h>

/* Like rindex(), but respecting Prospero quoting. */

char *
qrindex(const char *s, char c)
{
    char *thisc;
    char *lastc = NULL;

    assert(c);                  /* it'll blow up otherwise, cause we might
                                   shoot off the end of the string. */
    for(thisc = qindex(s, c); thisc; 
        lastc = thisc,  thisc = qindex(thisc + 1, c))
        ;
    return lastc;
}

