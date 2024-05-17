#ifndef _rmg_macros_h
#define _rmg_macros_h

#include <stdlib.h>          /* For malloc free */
#include <pfs.h>		/* For ALLOCATOR_CONSISTENCY_CHECK*/
#include <pfs_threads.h>	/* for mutex stuff */

#define TH_STRUC_ALLOC1(prefix,PREFIX,instance) { 			\
	p_th_mutex_lock(p_th_mutex##PREFIX);				\
	if(lfree) {							\
		instance = lfree;					\
		lfree = instance->next;					\
	} else {							\
		instance = (PREFIX) malloc(sizeof(PREFIX##_ST));	\
		if (!instance) out_of_memory();				\
		prefix##_max++;						\
	}								\
	prefix##_count++;						\
    /* Initialize and fill in default values */				\
    instance->previous = NULL;						\
    	instance->next = NULL; 						\
	p_th_mutex_unlock(p_th_mutex##PREFIX);				\
	}

#ifdef ALLOCATOR_CONSISTENCY_CHECK
#define TH_STRUC_ALLOC(prefix,PREFIX,instance) {			\
	TH_STRUC_ALLOC1(prefix,PREFIX,instance); 			\
    	instance->consistency = INUSE_PATTERN;				\
	}
#else /*!ALLOCATOR_CONSISTENCY_CHECK*/
#define TH_STRUC_ALLOC(prefix,PREFIX,instance) 				\
	TH_STRUC_ALLOC1(prefix,PREFIX,instance); 
#endif /*ALLOCATOR_CONSISTENCY_CHECK*/



#define TH_STRUC_FREE2(prefix,PREFIX,instance) {			\
    p_th_mutex_lock(p_th_mutex##PREFIX);				\
    instance->next = lfree;						\
    instance->previous = NULL;						\
    lfree = instance;							\
    prefix##_count--;							\
    p_th_mutex_unlock(p_th_mutex##PREFIX);				\
    }

/* Note - no ";" on last item */
#ifdef ALLOCATOR_CONSISTENCY_CHECK
#define TH_STRUC_FREE(prefix,PREFIX,instance)				\
	if (!instance) return;						\
    	assert(instance->consistency == INUSE_PATTERN);			\
    	instance->consistency = FREE_PATTERN;				\
	TH_STRUC_FREE2(prefix,PREFIX,instance)
#else
#define TH_STRUC_FREE(prefix,PREFIX,instance)				\
	if (!instance) return;						\
	TH_STRUC_FREE2(prefix,PREFIX,instance)
#endif

/*
 * instance_lfree - free a linked list of ATYPEDEF structures.
 *
 *    instance_lfree takes a pointer to a channel structure frees it and 
 *    any linked
 *    ATYPEDEF structures.  It is used to free an entire list of ATYPEDEF
 *    structures.
 */
#define TH_STRUC_LFREE(PREFIX,instance,instance_free)			\
  {									\
    PREFIX	this;							\
    PREFIX	nxt;							\
    /* Set instance to 0 so others can access it while we trash the list */\
    nxt = instance; instance = NULL;					\
    while((this=nxt) != NULL) {						\
        nxt = this->next;						\
        instance_free(this);						\
    }									\
  } 

/* temp should be set to point to a list, cnt should be 0
   temp will be NULL afterwards and cnt the count*/
#define COUNT_LISTS(temp,cnt) 					\
	while (temp) {						\
		cnt++;						\
		temp = temp->next;				\
	}


/* Find val in field, instance set to match or NULL */
/* Need to mutex this, cos if struc changes while walking it could fail */
#define	FIND_LIST(instance,field,val) \
    while (instance != NULL)  {				\
	if (instance->field == val) 			\
		break;					\
	instance = instance->next;			\
    }	

#define	TH_FIND_LIST(instance,field,val,PREFIX) { 	\
    p_th_mutex_lock(p_th_mutex##PREFIX);		\
    FIND_LIST(instance,field,val);			\
    p_th_mutex_unlock(p_th_mutex##PREFIX) ; }

#define FIND_FNCTN_LIST(instance, field, val, fnctn) 	\
	while(instance != NULL) {			\
		if (fnctn(instance->field, val))	\
			break;				\
		instance = instance->next;		\
	}

#define FIND_OBJFNCTN_LIST(instance, ob2, fnctn)	\
	while(instance != NULL) {			\
		if (fnctn(instance, ob2))		\
			break;				\
		instance = instance->next;		\
	}

#define TH_FIND_FNCTN_LIST(instance, field, val, fctn, PREFIX) { 	\
	p_th_mutex_lock(p_th_mutex##PREFIX);				\
	FIND_FNCTN_LIST(instance, field, val, fctn)			\
	p_th_mutex_unlock(p_th_mutex##PREFIX);}	
#define TH_FIND_OBJFNCTN_LIST(instance, ob2, fctn, PREFIX) { 	\
	p_th_mutex_lock(p_th_mutex##PREFIX);				\
	FIND_OBJFNCTN_LIST(instance, ob2, fctn)				\
	p_th_mutex_unlock(p_th_mutex##PREFIX);}	

#define TH_FIND_STRING_LIST_CASE(instance, field, val, PREFIX) 		\
	TH_FIND_FNCTN_LIST(instance, field, val, stcaseequal,PREFIX)
#define TH_FIND_STRING_LIST(instance, field, val, PREFIX) 		\
	TH_FIND_FNCTN_LIST(instance, field, val, stequal,PREFIX)


#define TH_FREESPARES(prefix,PREFIX)	{				\
    PREFIX	this, next;						\
    p_th_mutex_lock(p_th_mutex##PREFIX);				\
    next = lfree ; lfree = NULL;					\
    p_th_mutex_unlock(p_th_mutex##PREFIX);				\
    while((this = next) != NULL) {					\
	next = this->next;						\
	free(this);	/* Matches malloc in STRUC_ALLOC1*/		\
	prefix##_max--;							\
    }									\
    }

/* Cant take an offset from a null value, shortcuts to return NULL */
#define L2(a,b) (a ? a->b : NULL)
#define L3(a,b,c) (a ? (a->b ? a->b->c : NULL ) : NULL)
#define L4(a,b,c,d) (a ? (a->b ? (a->b->c ? a->b->c->d \
		: NULL) : NULL ) : NULL)
	  
/* String functions on some systems dont like operating on NULLS */
#define Strlen(a) (a ? strlen(a) : 0)

#endif /*_rmg_macros_h*/

