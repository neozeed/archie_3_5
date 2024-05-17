/* charset.h */
/* Written by Steven Augart <swa@isi.edu> July, 1992 */
/* This implements a simple character set recognizer. */
/* It is used by qsprintf.c and qsscanf.c */
/* must include limits.h to use this. */
#include <limits.h>

/* These macros define an abstraction for a set of characters where one can add
   members, remove them, and test for membership.  There are several ways in
   which this might be made more efficient, but I'm tired. */

#include <string.h>             /* This should include a definition of
                                   memset(), but the one on our system doesn't
                                   have it, so I have to define memset below.
                                   */ 
extern void *memset(void *, int, size_t);

typedef char charset[UCHAR_MAX + 1];    /* what we'll work on */
#define new_full_charset(cs)      do { memset((cs), 1, sizeof (cs)); \
                                           /* cs['\0'] = 0; */} while (0)
#define new_empty_charset(cs)     memset((cs), 0, sizeof (cs))
#define add_char(cs, c)             do {    \
         assert(c != EOF);                 \
         ((cs)[(unsigned char) (c)] = 1);   \
     } while (0)
#define remove_char(cs, c)          ((cs)[(unsigned char) (c)] = 0)
#define in_charset(cs,c)        (((c) != EOF) && (cs)[(unsigned char) (c)])

