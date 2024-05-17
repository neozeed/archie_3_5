/* qindex.c
   This program works just like "index()", but it recognizes Prospero quoting.
   Looks for first unquoted instance of "c" in string s.
   Returns NULL if none found.
   If it finds a mis-quoted string, returns NULL and sets perrno to
   PARSE_ERROR.
   Author: swa@isi.edu, 8/17/92
*/
#include <pfs.h>                /* to make sure we match definition. */
#include <perrno.h>

char *
qindex(const char *s, char c)
{
    enum { OUTSIDE_QUOTATION, IN_QUOTATION, 
               SEEN_POSSIBLE_CLOSING_QUOTE } state; 
    const char *start = s;
    
    state = OUTSIDE_QUOTATION;
    
    for (; *s; ++s) {
        switch (state) {
        case OUTSIDE_QUOTATION:
            if (*s == '\'')
                state = IN_QUOTATION;
            else if (*s == c)
                return (char *) s; /* flush CONST */
            break;
        case IN_QUOTATION:
            if (*s == '\'')
                state = SEEN_POSSIBLE_CLOSING_QUOTE;
            break;
        case SEEN_POSSIBLE_CLOSING_QUOTE:
            if (*s == '\'') {
                if (c == '\'')
                    return (char *) s; /* flush CONST */
                state = IN_QUOTATION;
            } else {
                state = OUTSIDE_QUOTATION;
                if (*s == c)
                    return (char *) s; /* flush CONST */
            }
            break;
        default:
            internal_error("qindex(): impossible state!");
        }
    }
    if (state == IN_QUOTATION) {
        perrno = PARSE_ERROR;
        p_err_string = qsprintf_stcopyr(p_err_string,
            "qindex(): encountered text with unbalanced quoting: %'s", start);
    }
    return NULL;
}

