/*
 *  Test the routines to convert from and to the string representation of the
 *  search state.
 *  
 *  Create states containing random, but valid, values and whose stacks vary
 *  randomly in the number of elements.
 */

#include <stddef.h>
#include <stdio.h>
#include <signal.h>
#include "state.c"


#define MAX_KEY_LEN 50
#define MIN_KEY_LEN 1
#define MAX_STACK_SZ 1000
#define MIN_STACK_SZ 0


char *prog;


static int randomRange(int min, int max)
{
  int r = abs(max - min);
  return r ? min + (rand() % r) : min;
}


static char *randomString(int minch, int maxch, int nch)
{
  char *s;
  int i;

  ASSERT(s = malloc((size_t)nch + 1));
  for (i = 0; i < nch; i++) {
    s[i] = randomRange(minch, maxch);
  }
  s[i] = '\0';

  return s;
}


static void randomNode(struct patrie_node *nd)
{
  nd->val = randomRange(-10000, 10000);
  nd->is_skip = randomRange(0, 1);
  nd->lev = randomRange(0, 500);
  nd->height = randomRange(0, 1000);
  nd->pos = randomRange(0, 1000);
  nd->inode = randomRange(0, 500);
  nd->enode = randomRange(0, 500);
  nd->tnode = randomRange(0, 500);
  nd->pageNum = randomRange(0, 10000);
}


static struct patrie_node *randomStack(int stackSize)
{
  int i;
  struct patrie_node *nd;

  ASSERT(nd = malloc(stackSize * sizeof *nd));
  for (i = 0; i < stackSize; i++) {
    randomNode(nd + i);
  }

  return nd;
}


static struct patrie_state *randomState(int stackSize)
{
  struct patrie_state *st;

  ASSERT(patrieAllocState(&st));
  st->top = stackSize - 1;
  st->nelts = stackSize;
  if (st->nelts > 0) {
    ASSERT(st->stack = randomStack(st->nelts));
    ASSERT(st->key = randomString('\001', '\377', randomRange(MIN_KEY_LEN, MAX_KEY_LEN)));
    st->caseSens = randomRange(0, 1);
    st->firstMatched = randomRange(0, 1);
  }

  return st;
}


static int equivNode(struct patrie_node *a, struct patrie_node *b)
{
  return (a->val == b->val &&
          a->is_skip == b->is_skip &&
          a->lev == b->lev &&
          a->height == b->height &&
          a->pos == b->pos &&
          a->inode == b->inode &&
          a->enode == b->enode &&
          a->tnode == b->tnode &&
          a->pageNum == b->pageNum);
}


static int equivStack(struct patrie_node *a, struct patrie_node *b, int nelts)
{
  int i;
  
  for (i = 0; i < nelts; i++) {
    if ( ! equivNode(a + i, b + i)) return 0;
  }

  return 1;
}


static int equivState(struct patrie_state *a, struct patrie_state *b)
{
  if (a->top != b->top ||
      a->nelts != b->nelts ||
      a->caseSens != b->caseSens ||
      a->firstMatched != b->firstMatched) {
    return 0;
  }

  if (a->nelts > 0) {
    return equivStack(a->stack, b->stack, a->nelts) && strcmp(a->key, b->key) == 0;
  }

  return 1;
}


volatile /* sig_atomic_t */ int done = 0;


static void sigint(int signo)
{
  done = 1;
}


int main(int ac, char **av)
{
  char *s;
  struct patrie_state *nst, *st;
  unsigned long n = 0;
  
  prog = av[0];
  
  signal(SIGINT, sigint);

  while ( ! done) {
    st = randomState(randomRange(MIN_STACK_SZ, MAX_STACK_SZ));
    ASSERT(s = patrieGetStateString(st));
    ASSERT(patrieSetStateFromString(s, &nst));
    ASSERT(equivState(st, nst));

    patrieFreeState(&st);
    patrieFreeState(&nst);
    free(s);

    n++;
  }

  fprintf(stderr, "%s: tested %lu random states.\n", prog, n);

  exit(0);
}
