#include "search.h"


struct patrie_state {
  int top;
  int nelts;
  struct patrie_node *stack;

  char *key;
  int matchCaseAccSens;
  int firstMatched;
};


extern int _patrieStateEmpty(struct patrie_state *state);
extern int _patrieStatePush(struct patrie_node node, struct patrie_state *state);
extern int patrieAllocState(struct patrie_state **state);
extern struct patrie_node _patrieStatePop(struct patrie_state *state);
extern void _patrieStateResetStack(struct patrie_state *state);
