#include <stdio.h>
#include <stdlib.h> /* malloc? */
#include "defines.h"
#include "parse.h"
#include "stack.h"
#include "info_cell.h"
#include "queue.h"
#include "error.h"
#include "lang_parsers.h"

/*
  Add an element to the tail of the queue.
*/

int
addq(nqe, s)
  Queue_elt *nqe ;
  Stack *s ;
{
  ptr_check(nqe, Queue_elt, "addq", 0) ;
  ptr_check(s, Stack, "addq", 0) ;

  nqe->next = s->q.tail ;
  nqe->prev = (Queue_elt *)0 ;
  if(s->q.tail != (Queue_elt *)0)
  {
    s->q.tail->prev = nqe ;
  }
  s->q.tail = nqe ;
  if(s->q.head == (Queue_elt *)0)
  {
    s->q.head = nqe ;
  }
  return 1 ;
}

int
dispose_Queue_elt(qe)
  Queue_elt *qe ;
{
  ptr_check(qe, Queue_elt, "dispose_Queue_elt", 0) ;

  if(qe->ident != (Info_cell *)0)
  {
    dispose_Info_cell(qe->ident) ;
  }
  free(qe) ;
  return 1 ;
}


/*
  's' may be null if we have popped the stack until it is empty.
*/

int
empty_queue(s)
  const Stack *s ;
{
  if(s == (Stack *)0)
  {
    return 0 ;
  }
  return s->q.head == (Queue_elt *)0 ;
}


Queue_elt *
new_Queue_elt(s)
  char *s ;
{
  Queue_elt *qe = (Queue_elt *)0 ;

  ptr_check(s, char, "new_Queue_elt", qe) ;

  if((qe = (Queue_elt *)malloc((unsigned)sizeof(Queue_elt))) == (Queue_elt *)0)
  {

    /* "Error allocating %d bytes for new queue element" */

    error(A_ERR, "new_Queue_elt", NEW_QUEUE_ELT_001, sizeof(Queue_elt)) ;
  }
  else
  {
    if((qe->ident = new_Info_cell(s)) != (Info_cell *)0)
    {
      qe->next = qe->prev = (Queue_elt *)0 ;
    }
    else
    {

     /* "Error from init_Info_cell()" */

      error(A_ERR, "new_Queue_elt", NEW_QUEUE_ELT_002) ;
      free(qe) ;
      qe = (Queue_elt *)0 ;
    }
  }

  return qe ;
}


int
print_queue(q)
  const Queue_elt *q ;
{
  Queue_elt *qe = q ;

  fprintf(stderr, "-|") ;
  while(qe != (Queue_elt *)0)
  {
    fprintf(stderr, "%s|", qe->ident->name) ;
    qe = qe->next ;
  }
  fprintf(stderr, "-\n") ;
  return 1 ;
}


Queue_elt *
remq(s)
  Stack *s ;
{
  Queue_elt *qe = (Queue_elt *)0 ;

  ptr_check(s, Stack, "remq", qe) ;

  if(s->q.head == (Queue_elt *)0)
  {
    return qe ;
  }
  qe = s->q.head ;
  s->q.head = qe->prev ;
  if(s->q.head != (Queue_elt *)0)
  {
    qe->prev->next = (Queue_elt *)0 ;
  }
  else
  {
    s->q.tail = (Queue_elt *)0 ;
  }
  qe->prev = (Queue_elt *)0 ;
  return qe ;
}
