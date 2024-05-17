/*
 * Copyright (c) 1993 by the University of Southern Calfornia
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
#include <stdio.h>
#include <pfs.h>                /* for prototypes */

/* Return non-zero if string ends in a newline. */
/* Like fputs, but for bstrings. */
int
p__fputbst(const char *bst, FILE *out)
{
    int len = p_bstlen(bst);
    int need_newline = 0;

    while (len-- > 0) {
        need_newline = (*bst != '\n'); /* Does the bstring end in a newline?  */
        putc(*bst++, out);
    }
    return need_newline;
}
