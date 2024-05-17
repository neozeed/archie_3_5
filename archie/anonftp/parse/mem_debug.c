/*
   Note: the debugging routines use the file /usr/lib/debug/malloc.o.
*/

#ifdef MEMORY_DEBUG

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_LEVEL 2

#ifdef __STDC__

extern void *memcpy(void *s1, const void *s2, size_t n) ; /* lacking real ANSI includes */
extern int malloc_debug(int level) ;
extern int malloc_verify(void) ;

#else

extern void *memcpy(/* void *s1, const void *s2, size_t n */) ; /* lacking real ANSI includes */
extern int malloc_debug(/* int level */) ;
extern int malloc_verify(/* void */) ;

#endif


static int debug_level = -1 ;


int
Free(ptr)
  void *ptr ;
{
  int rval ;

  if( ! (rval = (free)(ptr)))
  {
    error(A_SYSERR, "Free", "error from free(0x%lx)", (long)ptr) ;
  }
  else
  {
    error(A_SYSERR, "Free", "free(0x%lx)", (long)ptr) ;
  }
  return rval ;
}


void *
Malloc(size)
  size_t size ;
{
  void *rval ;

  if(debug_level == -1)
  {
    error(A_SYSERR, "Malloc", "setting debug level to %d", DEBUG_LEVEL) ;
    debug_level = DEBUG_LEVEL ;
    malloc_debug(debug_level) ;
  }
  if((rval = (malloc)(size)) == (void *)0)
  {
    error(A_SYSERR, "Malloc", "failed to malloc %ld bytes", (long)size) ;
  }
  else
  {
    error(A_SYSERR, "Malloc", "malloc\'ed %ld bytes at location 0x%lx", (long)size,
            (long)rval) ;
  }
  return rval ;
}


void *
Memcpy(dst, src, size)
  void *dst ;
  void *src ;
  size_t size ;
{
  void *rval ;
   
  if((rval = (memcpy)(dst, src, size)) != dst)
  {
    error(A_ERR, "Memcpy", "error from memcpy(%ld, %ld, %ld)",(long)dst, (long)src,
            (long)size) ;
  }
  else
  {
    error(A_ERR, "Memcpy", "memcpy(%ld, %ld, %ld) successful", (long)dst, (long)src,
            (long)size) ;
  }
  return rval ;
}

#endif
