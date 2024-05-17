/*
 * Copyright (c) 1993             by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>
#include <stdio.h>
#include <string.h>
#include <ardp.h>               /* For ALLOCATOR_CONSISTENCY_CHECK */
#include <pfs.h>                /* for defs. of functions in this file. */
#include <stdlib.h>             /*SOLARIS: for malloc, free etc*/

/* #define DREADFUL_TEMPORARY_HACK_TO_FIX_URGENT_CRASH_5_16_94 *//* my fault --swa */

/* See pfs.h for discussion of the string format we use and for macros that
   manipulate it. . */

int	string_count = 0;
int	string_max = 0;

/*
 * stcopy - allocate space for and copy a string
 *
 *     STCOPY takes a string as an argument, allocates space for
 *     a copy of the string, copies the string to the allocated space,
 *     and returns a pointer to the copy.
 *	Cannot fail - calls out_of_memory
 */

char *
stcopy(const char *st)
{
    return(stcopyr(st,(char *)0));
}

/*
 * stcopyr - copy a string allocating space if necessary
 *
 *     STCOPYR takes a conventional string, S, as an argument, and a pointer to
 *     a second  string, R, which is to be replaced by S.  If R is long enough
 *     to hold S, S is copied.  Otherwise, new space is allocated, and R is
 *     freed.  S is then copied to the newly allocated space.  If S is
 *     NULL, then R is freed and NULL is returned.
 *
 *     In any event, STCOPYR returns a pointer to the new copy of S,
 *     or a NULL pointer.
 */
char *
stcopyr(const char *s, char *r)
{
    int	sl;
    int	rl;

    CHECK_MEM();
    assert(p__bst_consistent(r));
    if(!s && r) {
        stfree(r);
        return(NULL);
    }
    else if (!s) return(NULL);

    sl = strlen(s) + 1;

    if(r) {
        rl = p__bstsize(r);
        if(rl < sl) {
            stfree(r);
            r = (char *) malloc(sl + p__bst_header_sz);
            /* Check for out of memory.  not too cool, but better than segfaulting. */
	    if (!r) out_of_memory();
            string_count++;
            r += p__bst_header_sz;
            p__bst_size_fld(r) = sl;
            p__bst_length_fld(r) = P__BST_LENGTH_NULLTERMINATED;
#ifdef P_ALLOCATOR_CONSISTENCY_CHECK
            p__bst_consistency_fld(r) = P__INUSE_PATTERN;
#endif
        }
    }
    else {
        r = (char *) malloc(sl + p__bst_header_sz);
	if (!r) out_of_memory();
        string_count++;
        r += p__bst_header_sz;
        p__bst_size_fld(r) = sl;
        p__bst_length_fld(r) = P__BST_LENGTH_NULLTERMINATED;
#ifdef P_ALLOCATOR_CONSISTENCY_CHECK
        p__bst_consistency_fld(r) = P__INUSE_PATTERN;
#endif
        if(string_max < string_count) string_max = string_count;
    }
    strcpy(r,s);
    return(r);
}

/*
 * stalloc - Allocate space for a string
 *
 *     STALLOC allocates space for a string by calling malloc.
 *     STALLOC guarantees never to honor requests for zero or fewer bytes of
 *      memory. 
 */
char *
stalloc(int size)
{
    void	*st;

    if (size <= 0) return NULL;
    st = (void *) malloc(size + p__bst_header_sz);
    if(!st) out_of_memory();
    string_count++;
    st += p__bst_header_sz;
    p__bst_size_fld(st) = size;
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    p__bst_consistency_fld(st) = INUSE_PATTERN;
#endif
    p__bst_length_fld(st) = P__BST_LENGTH_NULLTERMINATED;
    if(string_max < string_count) string_max = string_count;
    return(st);
}

/*
 * stfree - free space allocated by stcopy or stalloc
 *
 *     STFREE takes a string that was returned by stcopy or stalloc 
 *     and frees the space that was allocated for the string.
 */
void
stfree(void *st)
{
#ifndef DREADFUL_TEMPORARY_HACK_TO_FIX_URGENT_CRASH_5_16_94 /* my fault --swa */
    assert(p__bst_consistent(st));
#else
    if (!p__bst_consistent(st))
        return;
#endif
    if(st) {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
        p__bst_consistency_fld(st) = FREE_PATTERN;
#endif
        free(st - p__bst_header_sz);
        string_count--;
    }
}


/* Take the length of a Prospero bstring.  Must be allocated by Prospero. */
int
p_bstlen(const char *s)
{
    assert(p__bst_consistent(s));
    if (!s) return 0;
    else if (p__bst_length_fld(s) == P__BST_LENGTH_NULLTERMINATED) 
        return strlen(s);
    else
    return p__bst_length_fld(s);
}


/* Mark the length field of a buffer appropriately, when BUFLEN amount of data
   has been read into it.  Give preference to using a raw length. */
/* This is what you do after allocating a buffer with stalloc(), and it might
   be seen as part of the interface. */
void
p_bst_set_buffer_length_nullterm(char *buf, int buflen)
{
    register int i;

    assert(p__bst_consistent(buf));
    assert(p__bstsize(buf) >= buflen + 1);
    for (i = 0; i < buflen; ++i) {
        if (buf[i] == '\0') {
            /* If a null is present in the data, have to set an explicit
               count. */
            p__bst_length_fld(buf) = buflen;
            goto done;
        }
    }
    p__bst_length_fld(buf) = P__BST_LENGTH_NULLTERMINATED;
    
 done:
    buf[buflen] = '\0';
}


void
p_bst_set_buffer_length_explicit(char *buf, int buflen)
{
    assert(p__bst_consistent(buf));
    assert(p__bst_size_fld(buf) < buflen + 1);
    p__bst_length_fld(buf) = buflen;
    buf[buflen] = '\0';
}


/* This is called through the CHECK_MEM macro */
void
check_mem()
{
  static int check_mem_val = 2000 ; /* Not thread safe, but not a prob */
  void *t1;

  if (++check_mem_val > 20000) 
    check_mem_val = 2000;

  t1 = malloc(check_mem_val++);
  assert(t1);
  free(t1);
}
