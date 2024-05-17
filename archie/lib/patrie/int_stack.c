#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "int_stack.h"


/*  
 *  `top' is the index of the first unused stack element.
 */

struct int_stack {
  int top;
  int stack[INT_STACK_SZ];
};


void freeIntStack(struct int_stack **stack)
{
  free(*stack);
  *stack = 0;
}


void newIntStack(struct int_stack **stack)
{
  struct int_stack *s;
  
  ASSERT((s = MALLOC(sizeof *s)));
  s->top = 0;
  *stack = s;
}


int isEmptyIntStack(struct int_stack *stack)
{
  return stack->top == 0;
}


int popIntStack(struct int_stack *stack)
{
  ASSERT(stack->top > 0);
  stack->top--;
  return stack->stack[stack->top];
}


void pushIntStack(struct int_stack *stack, int height)
{
  ASSERT(stack->top < INT_STACK_SZ);
  stack->stack[stack->top] = height;
  stack->top++;
}


int topIntStack(struct int_stack *stack)
{
  ASSERT(stack->top > 0 && stack->top < INT_STACK_SZ);
  return stack->stack[stack->top - 1];
}
