/* tkalloc.c */
/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
#include <stdio.h>
#include <stdlib.h>           /* For malloc and free */

#include <pfs.h>
#include <mitra_macros.h>

static TOKEN	lfree = NULL;
int		token_count = 0;
int		token_max = 0;

/*
 * tkalloc - allocate and initialize token structure
 *
 *    TKALLOC returns a pointer to an initialized structure of type
 *    TOKEN.  If it is unable to allocate such a structure, it
 *    returns NULL.
 */
TOKEN
tkalloc(const char *s)
{
    TOKEN	token_ent;

    CHECK_MEM();

    TH_STRUC_ALLOC(token,TOKEN,token_ent);

    /* Initialize and fill in default values */
    if(s) 
        token_ent->token = stcopy(s);
    else
        token_ent->token = NULL;

    return token_ent;
}

#if 0                           /* not currently used, I think. */
/* Prepend the new string to the token list.  The head of the list will have a
   "previous" item that is the tail of the list.  However, the tail of the list
   will have no "next" item.  In the case of a single-element list, the head's
   "previous" will be itself.*/
TOKEN
tkprepend(char *newstr, TOKEN toklist)
{
    TOKEN tok;
    tok = tkalloc(newstr);
    tok->next = toklist;
    if (toklist) {
        tok->previous = toklist->previous;
        toklist->previous = tok;
    } else {
        tok->previous = tok;
    }
    return tok;
}
#endif

/* Append the new string to the token list.  The head of the list will have a
   "previous" item that is the tail of the list.  However, the tail of the list
   will have no "next" item.  Returns the new head of the list. 
   Needed because APPEND_ITEM multiply evaluates its arguments. 
*/
TOKEN
tkappend(const char *newstr, TOKEN toklist)
{
    TOKEN tok;

    CHECK_MEM();
    tok = tkalloc(newstr);
    APPEND_ITEM(tok, toklist);
    return(toklist);
}


/*
 * tkfree - free an TOKEN structure
 *
 *    TKFREE takes a pointer to an TOKEN structure and adds it to
 *    the free list for later reuse.
 */
void
tkfree(TOKEN token_ent)
{
    if (token_ent->token)  {
        stfree(token_ent->token);
        token_ent->token = NULL; /* free it up */
    }
    TH_STRUC_FREE(token,TOKEN,token_ent);
}

/*
 * tklfree - free an TOKEN structure
 *
 *    TKLFREE takes a pointer to an TOKEN structure frees it and any linked
 *    TOKEN structures.  It is used to free an entrie list of TOKEN
 *    structures.
 */
void
tklfree(TOKEN token_ent)
{
    TH_STRUC_LFREE(TOKEN,token_ent,tkfree);
}


/*
 * tkcopy()
 * Copy a linked list of token structures.  Return the new list. 
 * Signal out_of_memory() if no more.
 */

TOKEN
tkcopy(TOKEN t)
{
    TOKEN retval = NULL;
    for ( ; t ; t = t->next) {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
        assert(t->consistency == INUSE_PATTERN);
#endif
        retval = tkappend(t->token, retval);
    }
    return retval;
}


/* return TRUE if s is a member of the list t. */
int
member(const char s[], TOKEN tklist)
{
    for ( ; tklist; tklist = tklist->next) {
        if (strequal(s, tklist->token))
            return TRUE;
    }
    return FALSE;
}


