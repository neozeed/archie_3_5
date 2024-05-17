#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "ptr_stack.h"


struct elt {
  void *data;
  struct elt *down;
};

struct ptr_stack {
  struct elt *top;
};


/*  
 *
 *
 *                           External functions.
 *
 *
 */  


int isEmptyPtrStack(struct ptr_stack *stack)
{
  return stack->top == 0;
}


int pushPtrStack(struct ptr_stack *stack, void *data)
{
  struct elt *p;
  
  ASSERT((p = MALLOC(sizeof *p)));

  p->down = stack->top;
  stack->top = p;
  p->data = data;

  return 1;
}


int sizePtrStack(struct ptr_stack *stack)
{
  int n = 0;
  struct elt *p = stack->top;

  while (p) {
    p = p->down;
    n++;
  }

  return n;
}


void forEachEltPtrStack(struct ptr_stack *stack, void fn(void *data, void *arg), void *arg)
{
  struct elt *e;

  for (e = stack->top; e; e = e->down) {
    fn(e->data, arg);
  }
}


void freePtrStack(struct ptr_stack **stack)
{
  free(*stack);
  *stack = 0;
}


void newPtrStack(struct ptr_stack **stack)
{
  ASSERT((*stack = MALLOC(sizeof **stack)));
  (*stack)->top = 0;
}


/*  
 *  Merge `stack1' into `stack0'.  After the merger `stack1' is empty and can
 *  be deleted.
 *  
 *  `merge' is responsible for free()ing its second argument, if necessary.
 *  (As we move down `stack1' we lose the pointers to the elements.)
 */
void mergePtrStack(struct ptr_stack *stack0, struct ptr_stack *stack1, void *merge(void *, void *, void *), void *userData)
{
  struct elt *curr0, *curr1, *prev0;

  curr0 = stack0->top; prev0 = curr0;
  curr1 = stack1->top;

  while (curr0 && curr1) {
    struct elt *p = curr1;
    
    prev0 = curr0;

    curr0->data = merge(userData, curr0->data, curr1->data);
    curr0 = curr0->down;
    curr1 = curr1->down;
    free(p);
  }

  if (curr1) {
    /*  
     *  Append the remainder of `stack1' to `stack0'.
     */
    if ( ! prev0) {
      /*  
       *  `stack0' is empty.
       */
      stack0->top = curr1;
    } else {
      /*  
       *  We got to the end of `stack0', therefore `prev0' points to last
       *  element.
       */  
      prev0->down = curr1;
    }
  }

  stack1->top = 0;
}


void *popPtrStack(struct ptr_stack *stack)
{
  struct elt *p;
  void *e;

  ASSERT(stack->top != 0);

  p = stack->top;
  e = p->data;
  stack->top = stack->top->down;
  free(p);

  return e;
}


void *topPtrStack(struct ptr_stack *stack)
{
  ASSERT( ! isEmptyPtrStack(stack));
  return stack->top->data;
}


void printPtrStack(struct ptr_stack *stack, const char *name, void print(FILE *, void *))
{
  struct elt *e;

  printf("%s:", name ? name : "Stack");
  for (e = stack->top; e; e = e->down) {
    printf(" "); print(stdout, e->data);
  }
  printf("\n");
}
