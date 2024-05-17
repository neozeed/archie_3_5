/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * Written  by swa 1992   To manipulate lists in prospero and ardp 
 * Modified by bcn 1/93   Added EXTRACT ITEM and fixed several bugs
 * Modified by swa 9/93   Added APPEND_LISTS.
 * Modified by swa 10/93  Quick hack to make assert() in APPEND_LISTS ok.
 * Modified by swa 2/94   Added PREPEND_ITEM, renamed APPEND_LISTS to
 *                          CONCATENATE_LISTS 
 * Modified by swa 4/94   Got rid of mitra changes to check_offlist.
 */

#include <usc-copyr.h>

#ifndef LIST_MACROS_H
#define LIST_MACROS_H
#include <pfs_threads.h>		/* for mutex stuff */
/* All the macros in this file are documented in the Prospero library reference
   manual. */

#ifndef NDEBUG
#ifndef PARANOID                /* could be defined on a per-file basis */
#define PARANOID
#endif
/* See comment below about why this is normally not enabled.  Turn on at your
   own risk.  --swa, April 15, 1994 */
/* #define MITRA_PARANOID */
#endif

#ifdef PARANOID
#define CHECK_HEAD(head)    do {						\
	if (head) {							\
		assert(!(head)->previous->next)	;			\
		/*assert((head)->consistency == INUSE_PATTERN);*/	\
	} \
} while(0)

#ifdef MITRA_PARANOID
/* This is actually not a very cool macro to  use because 
   much of prospero does not explicitly reset items off the lists to have NULL
   NEXT and PREVIOUS pointers. */
/* Mitra notes that he likes resetting items of the list to have NULL next
   and previous pointers.
   However, this is causing efficiency problems for some ISI applications.
   Therefore, this option is normally disabled.  If you enable it, you are
   warned that there is likely to be code in the base Prospero release which
   does not follow the Mitra convention of setting items off the lists to have
   null NEXT and PREVIOUS pointers. */
#define CHECK_OFFLIST(n) do {						\
	/*assert(n->consistency == INUSE_PATTERN);*/			\
	assert(n);                                                    \
	assert(!(n)->next && !(n)->previous);				\
} while(0)

/* Ditto for previous comment. */
#define CHECK_NEXT(m) assert(!(m)->next || ((m)->next->previous == (m)))
#else /*MITRA_PARANOID*/
/* not MITRA_PARANOID */
#define CHECK_OFFLIST(nonmemb) do { } while(0)
#define CHECK_NEXT(memb) do { } while(0)
#endif

#define CHECK_PTRinBUFF(b,p) assert(((b) <= (p)) && ((p) <= (b)+strlen(b)))
void check_mem(void);           /* declare it */
#define CHECK_MEM() check_mem()
#else /*!PARANOID*/
#define CHECK_HEAD(head) do { } while(0)
#define CHECK_OFFLIST(nonmemb) do { } while(0)
#define CHECK_NEXT(memb) do { } while(0)
#define CHECK_PTRinBUFF(b,p) do { } while(0)
#define CHECK_MEM() do { } while(0)
#endif /*!PARANOID*/

/* 
 * This macro appends an item to a doubly linked list.  The item goes
 * at the TAIL of the list. This macro modifies its second argument.
 * if head is non-null, head->previous will always be the TAIL of
 * the list. 
 */
#define APPEND_ITEM(new, head) do {         \
    CHECK_HEAD(head);			    \
    CHECK_OFFLIST(new);			    \
    if((head)) {                            \
        (new)->previous = (head)->previous; \
        (head)->previous = (new);           \
        (new)->next = NULL;                 \
        (new)->previous->next = (new);      \
    } else /* !(head) */ {                  \
        (head) = (new);                     \
        (new)->previous = (new);            \
        (new)->next = NULL;                 \
    }                                       \
} while(0) 

/* Add new to a currently empty list */
#define NEW_LIST(new,head) do {				\
	assert(!head);					\
	CHECK_OFFLIST(new);				\
	new->previous = head = new;			\
	new->next = NULL;				\
} while(0)


/* Put item on head of the queue */
/* This is mitra's implemtation.  It fails when the queue is empty (i.e., when
   "head' is NULL.).  I have commented it out in favor of the one I've been
   using.  */
#if 0                           /* leaving it in for a while. */
#define PREPEND_ITEM(new, head) do {			\
	CHECK_HEAD(head);				\
	CHECK_OFFLIST(new);				\
	new->previous = head->previous;			\
	new->next = head;				\
	head = head->previous = new;			\
} while(0)
#endif                          /* 0 */
	
#ifndef PREPEND_ITEM
/* Alternative implementation.  Either should work. */
/*
 * This macro prepends the item NEW to the doubly-linked list headed by HEAD.
 * NEW should not be a member of HEAD or the results are unpredictable. 
 */

#define PREPEND_ITEM(new, head) do {                            \
    /* Make the item a one-element list. */                     \
    (new)->next = NULL;                                         \
    (new)->previous = (new);                                    \
    /* Concatenate the full list to the one-element list. */    \
    CONCATENATE_LISTS((new), (head));                           \
    (head) = (new);  /* reset the list head. */                 \
} while(0)
#endif

#define TH_APPEND_ITEM(new, head, PREFIX) do {	    \
	p_th_mutex_lock(p_th_mutex##PREFIX);	    \
	APPEND_ITEM(new, head);			    \
        p_th_mutex_unlock(p_th_mutex##PREFIX);      \
} while(0)



/* 
 * This macro removes an item from a doubly-linked list headed by HEAD.
 * If ITEM is not a member of the list, the results are not defined. 
 * The extracted item will NOT be freed.
 * Minor efficiency (code size) improvement by Mitra
 */

/* Use this if only know head, and we know its the last item */
/* from mitra. */
#define  EXTRACT_HEAD_LAST_ITEM(head) do {			\
	(head)->previous = (head)->previous->previous;		\
	(head)->previous->next->previous = NULL;		\
	(head)->previous->next = NULL;				\
} while(0)
	
/* Use this if both item and head are known, and we know it's the last item */
/* from mitra. */
#define  EXTRACT_LAST_ITEM(item,head) do {		\
	(head)->previous = (item)->previous;		\
        (item)->previous = NULL;		        \
	(head)->previous->next = NULL;			\
} while(0)
	
/* swa tends to use just this one. */
/* someone else uses the above code, they just don't have it in a macro!*/
/* I do not understand the above comment --swa, 4/13/94 */
#define EXTRACT_ITEM(item, head) do {                   \
    if ((head) == (item)) {                             \
        (head) = (item)->next;                          \
        if (head) (head)->previous = (item)->previous;  \
    } else {                                            \
        (item)->previous->next = (item)->next;          \
        if ((item)->next) (item)->next->previous = (item)->previous; \
	else (head)->previous = (item)->previous;       \
    }                                                   \
    (item)->previous = NULL; (item)->next = NULL;   \
} while(0)

#define TH_EXTRACT_ITEM(item,head,PREFIX) do {		\
	p_th_mutex_lock(p_th_mutex##PREFIX);		\
	EXTRACT_ITEM(item, head);			\
	p_th_mutex_unlock(p_th_mutex##PREFIX);          \
} while(0)

/*When you dont know the head of a list, but do know that item is at the head*/
#define EXTRACT_HEAD_ITEM(item) do {				        \
    	if ((item)->next)						\
		(item)->next->previous = (item)->previous;		\
	(item)->next = (item)->previous = NULL;			        \
} while(0)

#define TH_EXTRACT_HEAD_ITEM(item,PREFIX) do {			\
	p_th_mutex_lock(p_th_mutex##PREFIX);			\
	EXTRACT_HEAD_ITEM(item);				\
	p_th_mutex_unlock(p_th_mutex##PREFIX);			\
} while(0)

/* Set list1 to point to a doubly-linked list consisting of list1 appended to
   LIST2.  
   LIST2 must not already be a part of list1 or the results are unpredictable.
   LIST2 will be a garbage value at the end of this exercise, since it will no
   longer point to a valid doubly-linked list. 
   LIST1 and LIST2 must already be valid doubly-linked lists. */
/* This performs an O(1) list appending operation. */
#define APPEND_LISTS(list1,list2) do {\
    CHECK_HEAD(list1);                                                      \
    CHECK_HEAD(list2);                                                      \
    if (!(list1))                                                           \
        (list1) = (list2);                                                  \
    else if (!(list2))                                                      \
        /* If 2nd list is empty, concatenation is a no-op. */               \
        ;                                                                   \
    else {                                                                  \
               /* Assertion removed for compilation problems. */            \
        /* assert(!(list1)->previous->next && !(list2)->previous->next); */ \
        /* OLDL1TAIL is (list2)->previous->next (scratchpad value) */       \
        /* Read next line as: OLDL1TAIL = (list1)->previous */              \
        (list2)->previous->next = (list1)->previous; /* scratchpad value  */\
        (list1)->previous->next = (list2); /* was NULL, or should've been */\
        (list1)->previous = (list2)->previous;                              \
        /* OLDL1TAIL is now list1->previous->next too */                    \
        (list2)->previous = (list1)->previous->next;                        \
        (list1)->previous->next = NULL; /* reset scratch value */           \
    }                                                                       \
/* oops - didnt set to null if first list empty - barfed in replace_cached*/ \
   (list2) = NULL;         /* for safety, to prevent abuse. */         \
} while (0)

/* XXX This is odd.  Potential bug.  Maybe Mitra meant to make this a call to 
   CONCATENATE_LISTS(), not to APPEND_ITEM()? */
#define TH_APPEND_LISTS(list1,list2,PREFIX) do {	\
	p_th_mutex_lock(p_th_mutex##PREFIX);		\
	APPEND_LISTS(list1, list2);			\
	p_th_mutex_unlock(p_th_mutex##PREFIX);          \
} while(0)



/* New name for APPEND_LISTS() */
#define CONCATENATE_LISTS(l1, l2)   APPEND_LISTS((l1), (l2))

#endif /* LIST_MACROS_H */
