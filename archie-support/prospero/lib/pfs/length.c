/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */
/* Author: Steven Augart, swa@isi.edu */

#include <usc-copyr.h>
#include <pfs.h>

/* Return the length of a sequence.  Works like the Common Lisp LENGTH function
   on sequences. */
int
length(TOKEN s)
{
    int len;
    for (len = 0; s; s = s->next, ++len)
        ;
    return len;
}
