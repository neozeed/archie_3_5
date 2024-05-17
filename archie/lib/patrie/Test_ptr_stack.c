#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "ptr_stack.h"


char *prog;


/*  
 *  We are responsible for freeing `elt1'.
 */
void *merge(void *junk, void *elt0, void *elt1)
{
  int *i0 = elt0, *i1 = elt1;

  i0[0] += i1[0];
  i0[1] += i1[1];

  free(elt1);

  return elt0;
}


/*  
 *  `p' really pointes to an array of two ints.
 */
void print(FILE *fp, void *p)
{
  int *i = p;

  fprintf(fp, "[%d %d]", i[0], i[1]);
}


struct ptr_stack *make_stack(int nelts, const char *name, int echo)
{
  int i;
  struct ptr_stack *st;

  newPtrStack(&st);

  if (echo) {
    printPtrStack(st, name, print);
  }

  for (i = 0; i < nelts; i++) {
    int *p;

    ASSERT((p = malloc(2 * sizeof(int))));
    p[0] = i + 1; p[1] = -i - 1;
    
    pushPtrStack(st, p);

    if (echo) {
      printf("\tPush: [%d %d]\n", p[0], p[1]);
      printPtrStack(st, name, print);
    }
  }

  return st;
}


void testMerge(const char *title, int sz0, int sz1)
{
  struct ptr_stack *st0, *st1;

  /*  
   *  Test:
   *  
   *  mergePtrStack()
   */

  printf("%s\n\n", title);
  
  st0 = make_stack(sz0, 0, 0); printPtrStack(st0, "\tst0", print);
  st1 = make_stack(sz1, 0, 0); printPtrStack(st1, "\tst1", print);
  
  mergePtrStack(st0, st1, merge, 0);

  ASSERT(isEmptyPtrStack(st1));

  printf("\n");
  printPtrStack(st0, "\tst0", print);
  printPtrStack(st1, "\tst1", print);

  /*  
   *  Clean up `st0'.
   */
  
  while ( ! isEmptyPtrStack(st0)) {
    int *p;
    
    ASSERT((p = popPtrStack(st0)) != 0);
    free(p);
  }

  freePtrStack(&st0);
  freePtrStack(&st1);
}


void testPushPopEtc(void)
{
  int size = 0;
  struct ptr_stack *st0;

  /*  
   *  Test:
   *  
   *  freePtrStack()
   *  newPtrStack()
   *  isEmptyPtrStack()
   *  popPtrStack()
   *  printPtrStack()
   *  pushPtrStack()
   *  sizePtrStack()
   */   
  
  printf("Test pushing and popping...\n\n");

  size = 10;
  st0 = make_stack(size, "\tst0", 1);
  ASSERT(sizePtrStack(st0) == size);

  while ( ! isEmptyPtrStack(st0)) {
    int *p;
    
    ASSERT((p = popPtrStack(st0)) != 0);
    size--;
    ASSERT(sizePtrStack(st0) == size);
    free(p);
    printf("\tPop: [%d %d]\n", p[0], p[1]);
    printPtrStack(st0, "\tst0", print);
  }

  freePtrStack(&st0);
}


int main(int ac, char **av)
{
  prog = av[0];

  testPushPopEtc();

  testMerge("\nTest the merger of stacks of size 0.",       0, 0);
  testMerge("\nTest the merger of stacks of size 0 and 1.", 0, 1);
  testMerge("\nTest the merger of stacks of size 0 and 2.", 0, 2);
  testMerge("\nTest the merger of stacks of size 1 and 0.", 1, 0);
  testMerge("\nTest the merger of stacks of size 2 and 0.", 2, 0);
  testMerge("\nTest the merger of stacks of size 1 and 1.", 1, 1);
  testMerge("\nTest the merger of stacks of size 1 and 2.", 1, 2);
  testMerge("\nTest the merger of stacks of size 2 and 1.", 2, 1);
  testMerge("\nTest the merger of stacks of size 2 and 2.", 2, 2);
  testMerge("\nTest the merger of stacks of size 3 and 7.", 3, 7);
  testMerge("\nTest the merger of stacks of size 6 and 2.", 6, 2);
  
  exit(0);
}
