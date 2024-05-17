struct ptr_stack;

extern int isEmptyPtrStack(struct ptr_stack *stack);
extern int pushPtrStack(struct ptr_stack *stack, void *data);
extern int sizePtrStack(struct ptr_stack *stack);
extern void forEachEltPtrStack(struct ptr_stack *stack, void fn(void *data, void *arg), void *arg);
extern void freePtrStack(struct ptr_stack **stack);
extern void *popPtrStack(struct ptr_stack *stack);
extern void printPtrStack(struct ptr_stack *stack, const char *name, void print(FILE *, void *));
extern void *topPtrStack(struct ptr_stack *stack);
extern void newPtrStack(struct ptr_stack **stack);
extern void mergePtrStack(struct ptr_stack *stack0, struct ptr_stack *stack1,
                          void *merge(void *, void *, void *), void *userData);
