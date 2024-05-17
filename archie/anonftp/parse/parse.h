#ifndef DEFS_H
#define DEFS_H

#include "mem_debug.h"

#define MAX_FIELDS	16
#define MAX_LINE_LEN	512
#define MAX_PATH_ELTS	50
#define MAX_RE_LEN	512

#define WHITE_STR	" \t"
#define DEV_WHITE_STR	" \t"


/*
  ANSI C.
*/

#if 0

#ifdef __STDC__

#ifndef PARAM_CHECK
#   define ptr_check(ptr, type, func, ret_val)
#else
#   define ptr_check(ptr, type, func, ret_val) \
    do \
    { \
      if((ptr) == (type *)0) \
      { \
        fprintf(stderr, "%s: %s: parameter '" #ptr "' is a null pointer.\n", prog, func) ; \
        return ret_val ; \
      } \
    } while(0)
#endif

extern char *prog ;
extern void regerr(int c) ;

typedef void *Void_ptr ;

#else


/*
  K&R C.
*/

#ifndef PARAM_CHECK
#   define ptr_check(ptr, type, func, ret_val)
#else
#   define ptr_check(ptr, type, func, ret_val) \
	do { if((ptr) == (type *)0) { \
		fprintf(stderr, "%s: %s: a parameter is a null pointer.\n", prog, func) ; \
		return ret_val ; \
	     } \
	} while(0)
#endif

#endif

extern char *prog ;
extern void regerr(/* int c */) ;

#define const
typedef char *Void_ptr ;


#endif
#endif

extern int verbose ;
#define vfprintf if(verbose) fprintf
