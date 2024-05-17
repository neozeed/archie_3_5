/*
 * Copyright (c) 1993 by the University of Southern Calfornia
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <stdio.h>
#include <ctype.h>

static void showc(char c, FILE *outf);

/* Return non-zero if string ends in a newline. */
/* Like fputs, but for bstrings. */
void
ardp_showbuf(const char *bst, int len, FILE *out)
{
    while (len-- > 0) {
        showc(*bst++, out);
    }
}

#include <ctype.h>

extern int pfs_debug;

static void 
showc(char c, FILE *outf)
{
    if (c == '\\') {
        putc('\\', outf);
        putc('\\', outf);
    } else if (isprint(c)) {
        putc(c, outf);
    } else if ((c == '\n')) {
        if (pfs_debug >= 11) {
            putc('\\', outf);
            putc('n', outf);
        } else {
            putc('\n', outf);
        }
    } else if ((c == '\t')) {
        if (pfs_debug >= 11) {
            putc('\\', outf);
            putc('t', outf);
        } else {
            putc('\t', outf);
        }
    } else if ((c == '\r')) {
        if (pfs_debug >= 11) {
            putc('\\', outf);
            putc('r', outf);
        } else {
            putc('\r', outf);
        }
    } else {
        fprintf(outf, "\\%#03o", c);
    }
}
