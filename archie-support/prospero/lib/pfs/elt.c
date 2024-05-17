/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */
/* Author: Steven Augart, swa@isi.edu */

#include <usc-copyr.h>
#include <stdio.h>
#include <pfs.h>

extern int pfs_debug;

/* Return the NTH element of a sequence S.  Indexing starts at zero.  Works
   just like the Common LISP ELT function. */
char *
elt(TOKEN s, int nth)
{
    if (nth < 0) {
        if (pfs_debug)
            fprintf(stderr, "elt() called with illegal negative index %d\n",
                    nth);
        return NULL;
    }
    for (; s && nth > 0; s = s->next, --nth)
        ;
    return s ? s->token : NULL;
}
