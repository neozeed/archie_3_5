#include <string.h>
#include <stdio.h>
#include <stdlib.h> /* malloc? */
#include "parse.h"
#include "line_type.h"
#include "queue.h"
#include "info_cell.h"
#include "stack.h"
#include "error.h"
#include "lang_parsers.h"
#ifdef SOLARIS
#include "protos.h"
#else

#ifdef __STDC__
static Stack *new_Stack(Info_cell *ic) ;
#else
static Stack *new_Stack(/* Info_cell *ic */) ;
#endif
#endif

static Stack *stop = (Stack *)0 ;


int
empty_stack()
{
  return stop == (Stack *)0 ;
}


int
init_stack(root_dir)
  char *root_dir ;
{
  if(root_dir != (char *)0)
  {
    if(S_line_type(root_dir) != L_DIR_START)
    {

      /* "Root directory '%s' doesn't match pattern" */

      error(A_ERR, "init_stack", INIT_STACK_001, root_dir) ;
      return 0 ;
    }
    else
    {
      char **path ;
      int i ;
      int has_dev ;
      int npath ;
	    
      if( ! S_split_dir(root_dir, (char *)0, &has_dev, &npath, &path))
      {

        /* "Error from S_split_dir()" */

        error(A_ERR, "init_stack", INIT_STACK_002) ;
        return 0 ;
      }
      for(i = 0 ; i < npath ; i++)
      {
        char *s ;
        Info_cell *ic ;

        if((s = strdup(path[i])) == (char *)0)
        {

          /* "Error strdup'ing '%s'" */

          error(A_ERR, "init_stack", INIT_STACK_003, path[i]) ;
          return 0 ;
        }
        if((ic = new_Info_cell(s)) == (Info_cell *)0)
        {

          /* "Error from new_Info_cell()" */

          error(A_ERR, "init_stack", INIT_STACK_004) ;
          return 0 ;
        }
        if ( i != npath-1 || npath == 1 ) {
          ic->idx = -1 ;
          if( ! push(ic))
          {

            /* "Error from push()" */

            error(A_ERR, "init_stack", INIT_STACK_005) ;
            return 0 ;
          }
        }
        else {
          Pars_ent_holder *peh ;
          Queue_elt *qe ;

          if((peh = (Pars_ent_holder *)new_elt()) == (Pars_ent_holder *)0)
          {

            /* "Error allocating space for parser record" */

            error(A_ERR, "handle_file", HANDLE_FILE_001);
            return 0 ;
          }

          CSE_SET_DIR(peh->pent.core) ;
          qe = new_Queue_elt(strdup(s)) ;
          qe->ident->addr = peh ;
          qe->ident->idx = elt_num() ;
          if( ! addq(qe, tos()))
          {

            /* "Error adding directory to queue" */

            error(A_ERR, "handle_file", HANDLE_FILE_004);
            return 0 ;
          }

          peh->pent.core.parent_idx = tos()->ident->idx ;
          if((peh->name = strdup(s)) == (char *)0)
          {

            /* "Error strdup'ing file name" */

            error(A_ERR, "handle_file", HANDLE_FILE_005);
            return 0 ;
          }
          peh->pent.slen = strlen(peh->name);
          ic->idx = -1 ;
          if( ! push(ic))
          {

            /* "Error from push()" */

            error(A_ERR, "init_stack", INIT_STACK_005) ;
            return 0 ;
          }
          
        }
      }
    }
  }
  return 1 ;
}


/*
  Create a new stack element.  'str' is assumed to point to storage which may be freed by
  pop.
*/

static Stack *
new_Stack(ic)
  Info_cell *ic ;
{
  Stack *s ;

  ptr_check(ic, Info_cell, "new_Stack", 0) ;

  if((s = (Stack *)malloc((unsigned)sizeof(Stack))) == (Stack *)0)
  {

  /* "Error allocating %d bytes for new stack element" */

    error(A_SYSERR, "new_Stack", NEW_STACK_001, sizeof(Stack)) ;
    return((Stack *) NULL);
  }
  s->ident = ic ;
  s->q.head = s->q.tail = (Queue_elt *)0 ;
  return s ;
}


int
pop()
{
  Stack *s ;

  if(stop == (Stack *)0)
  {

#if 0
  /* "Stack is empty" */

    error(A_ERR, "pop", POP_001) ;
#endif

    return 0 ;
  }
  if( ! empty_queue(tos()))
  {

  /* "Queue is not empty" */

#if 0
    error(A_ERR, "pop", POP_002) ;
#endif

    return 0 ;
  }
  s = stop ;
  stop = stop->prev ;
  if( ! dispose_Info_cell(s->ident))
  {

  /* "Error from dispose_Info_cell()" */

    error(A_ERR, "pop", POP_003) ;
    return 0 ;
  }
  free(s) ;
  return 1 ;
}


int
print_stack()
{
  if(stop == (Stack *)0)
  {
    /* "Stack is empty" */
    fprintf(stderr, PRINT_STACK_001) ;
  }
  else
  {
    Stack *s = stop ;

    fprintf(stderr, "------ stack -----\n") ;
    while(s != (Stack *)0)
    {
      fprintf(stderr, "%s: ", s->ident->name) ;
      print_queue(s->q.tail) ;
      s = s->prev ;
    }
    fprintf(stderr, "==================\n") ;
  }
  return 1 ;
}


int
push(ic)
  Info_cell *ic ;
{
  Stack *s ;

  ptr_check(ic, Info_cell, "push", 0) ;

  if((s = new_Stack(ic)) == (Stack *)0)
  {

  /* "Error from new_Stack()" */

    error(A_ERR, "push", PUSH_001) ;
    return 0 ;
  }
  s->prev = stop ;
  stop = s ;
  return 1 ;
}


int
stack_match(p, n)
  const char **p ;
  int n ;
{
  int i ;
  Stack *s = stop ;

  ptr_check(p, char *, "stack_match", 0) ;

  if(stop == (Stack *)0)
  {
    return n == 0 ;
  }
  for(i = n ; i > 0 ; --i, s = s->prev)
  {
    if(s == (Stack *)0)
    {
      break ;
    }
    if(strcmp(s->ident->name, *(p + i - 1)) != 0)
    {
      return 0 ;
    }
  }
  return i == 0 && s == (Stack *)0 ;
}


Stack *
tos()
{
  return stop ;
}
