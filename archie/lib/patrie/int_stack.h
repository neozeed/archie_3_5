#define INT_STACK_SZ 10000


struct int_stack;


extern int isEmptyIntStack(struct int_stack *stack);
extern int popIntStack(struct int_stack *stack);
extern int topIntStack(struct int_stack *stack);
extern void freeIntStack(struct int_stack **stack);
extern void newIntStack(struct int_stack **stack);
extern void pushIntStack(struct int_stack *stack, int height);
