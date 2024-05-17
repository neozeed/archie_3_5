#ifndef STACK_H
#define STACK_H

#include "queue_def.h"


typedef struct Stack Stack ;

struct Stack
{
  Info_cell *ident ;
  Queue q ;

  Stack *prev ;
} ;

#ifdef __STDC__

extern int empty_stack(void) ;
extern int init_stack(char *root_dir) ;
extern int pop(void) ;
extern int print_stack(void) ;
extern int push(Info_cell *ic) ;
extern int stack_match(const char **p, int n) ;
extern Stack *tos(void) ;

#else

extern int empty_stack(/* void */) ;
extern int init_stack(/* char *root_dir */) ;
extern int pop(/* void */) ;
extern int print_stack(/* void */) ;
extern int push(/* Info_cell *ic */) ;
extern int stack_match(/* const char **p, int n */) ;
extern Stack *tos(/* void */) ;

#endif
#endif
