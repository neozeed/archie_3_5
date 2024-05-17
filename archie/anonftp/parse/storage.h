#ifndef STORAGE_H
#define STORAGE_H

#ifdef __STDC__

#include <stddef.h>

extern int elt_num(void) ;
extern int free_elts(void) ;
extern int set_elt_size(size_t size) ;
extern void *first_elt(void) ;
extern void *new_elt(void) ;
extern void *next_elt(void) ;

#else

extern int elt_num(/* void */) ;
extern int free_elts(/* void */) ;
extern int set_elt_size(/* size_t size */) ;
extern void *first_elt(/* void */) ;
extern void *new_elt(/* void */) ;
extern void *next_elt(/* void */) ;

#endif
#endif
