#ifndef MEM_DEBUG_H
#define MEM_DEBUG_H

#ifdef __STDC__
#include <stddef.h>
#endif

#ifdef MEMORY_DEBUG
#define free Free
#define malloc Malloc
#define memcpy Memcpy

#ifdef __STDC__

extern int Free(void *ptr) ;
extern void *Malloc(size_t size) ;
extern void *Memcpy(void *dst, void *src, size_t size) ;

#else

extern int Free(/* void *ptr */) ;
extern void *Malloc(/* size_t size */) ;
extern void *Memcpy(/* void *dst, void *src, size_t size */) ;

#endif

#define mck \
  do { \
    fprintf(stderr, "%s: malloc_verify at line %d in file %s: ", prog, __LINE__, __FILE__) ; \
    fprintf(stderr, "%s\n", malloc_verify() ? "okay" : "failed") ; \
  } while(0)

#else
#define mck
#endif

#endif
