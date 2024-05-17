#ifndef QUEUE_H
#define QUEUE_H

#include "queue_def.h"
#include "stack.h"

#ifdef __STDC__

extern int addq(Queue_elt *nqe, Stack *s) ;
extern int dispose_Queue_elt(Queue_elt *qe) ;
extern int empty_queue(const Stack *s) ;
extern int print_queue(const Queue_elt *q) ;
extern Queue_elt *new_Queue_elt(char *s) ;
extern Queue_elt *remq(Stack *s) ;
extern Queue *new_queue_t(void) ;

#else

extern int addq(/* Queue_elt *nqe, Stack *s */) ;
extern int dispose_Queue_elt(/* Queue_elt *qe */) ;
extern int empty_queue(/* const Stack *s */) ;
extern int print_queue(/* const Queue_elt *q */) ;
extern Queue_elt *new_Queue_elt(/* char *s */) ;
extern Queue_elt *remq(/* Stack *s */) ;
extern Queue *new_queue_t(/* void */) ;

#endif
#endif
