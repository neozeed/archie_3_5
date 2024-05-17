#ifndef FORK_WAIT_H
#define FORK_WAIT_H

#include "ansi_compat.h"
#include "defines.h"
#include "macros.h"


#ifdef PROFILE
#  define fork_return(x) break
#else
#  define fork_return(x) exit(x)
#endif

/*  
 *  Possible return values for the function 'fork_me'.
 */  
enum forkme_e
{
  _FORKME_NOT_USED = INTERNAL_ERROR,
  CHILD,
  PARENT
};

typedef enum forkme_e Forkme;


extern Forkme fork_me PROTO((int prt_status, int *ret));

#endif
