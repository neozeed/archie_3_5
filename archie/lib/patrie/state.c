#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "patrie.h"
#include "state.h"
#include "utils.h"


#define ENC_BASE 94
#define FIRST_CH '!'     /* ASCII 33 */
#define IS_HEX(c) (strchr("0123456789abcdefABCDEF", c) != 0)
#define MUST_QUOTE_CH(c) ((c) == QUOTE_CH || (c) < 33 || (c) > 126)
#define QUOTE_CH '%'


int _patrieStateEmpty(struct patrie_state *state)
{
  return state->top == -1;
}


/*
 *  bug: this routine should return an int, and pass the popped node through
 *       an argument
 */
struct patrie_node _patrieStatePop(struct patrie_state *state)
{
  ASSERT( ! _patrieStateEmpty(state));

  return state->stack[state->top--];
}


int _patrieStatePush(struct patrie_node node, struct patrie_state *state)
{
  if (state->top == state->nelts - 1) {
    void *new;

    if ( ! _patrieReAlloc(state->stack, (state->nelts + 1) * sizeof *state->stack, &new)) {
      fprintf(stderr, "%s: _patrieStatePush: can't extend stack.\n", prog);
      patrieFreeState(&state);
      return 0;
    }

    state->stack = new;
    state->nelts++;
  }

  state->stack[++state->top] = node;

  return 1;
}


void _patrieStateResetStack(struct patrie_state *state)
{
#if 1
  state->top = -1;
  
#else
  
  if (state->stack) free(state->stack);
  state->stack = 0;

  if (state->key) free(state->key);
  state->key = 0;

  state->top = -1;
  state->nelts = 0;
  state->matchCaseAccSens = 0;
  state->firstMatched = 0;
#endif
}


/*
 *
 *
 *                            External functions
 *
 *
 */


int patrieAllocState(struct patrie_state **state)
{
  struct patrie_state *s;

  if ( ! (s = malloc(sizeof *s))) {
    fprintf(stderr, "%s: patrieAllocState: can't allocate %lu bytes for search state: ",
            prog, (unsigned long)sizeof *s); perror("malloc");
    return 0;
  }

  s->stack = 0;
  s->top = -1;
  s->nelts = 0;
  s->key = 0;
  s->matchCaseAccSens = 0;
  s->firstMatched = 0;

  *state = s;

  return 1;
}


void patrieFreeState(struct patrie_state **state)
{
  struct patrie_state *s = *state;

  if (s->stack) free(s->stack);
  if (s->key) free(s->key);
  free(s);
  *state = 0;
}


/*
 *  We represent the state as compactly as possible by encoding it in base 94
 *  (there being 94 printable characters between ASCII 33 and 126, inclusive).
 *  The space character (ASCII 32) delimits values.
 *  
 *  In order to calculate the amount of memory required to hold this
 *  representation we use the following table; in which the n'th entry gives
 *  the maximum value represented in n base 94 digits.  The `bc' program used
 *  to calculate the values appears below.
 *  
 *  bug: we should calculate the table at run time, so we don't have to guess
 *  at the size of an unsigned long.
 */

#if 0
/*
  Print the number of digits, base b, and the maximum
  value representable with that number of digits.
*/

m = 2^32-1
b = 94 ; v = 0

for (p = 0; v < m; p++) {
  v = b ^ p - 1
  v ; p
}
#endif

#if ULONG_MAX > 4294967295UL
#error table too small for range of unsigned long
#endif

static unsigned long maxrep[] = {
  93,
  8835,
  830583,
  78074895,
  ULONG_MAX
};


/*
 *  Return the amount memory, not including a terminating nul, needed to
 *  encode `n' in base 94.
 *  
 *  bug: if n == ULONG_MAX then we'll return one too many digits, but that's
 *  not a real problem.
 */
static int numberSpace(unsigned long n)
{
  int i;

  for (i = 0; n > maxrep[i]; i++) {
    continue;
  }

  return i + 1;
}


/*
 *  Append the quoted (base 94 encoded) representation to the string in
 *  `*str'.  Update `*str' to point to one character past the end of the
 *  encoded digit.  For simplicity, the number is encoded with the least
 *  significant digit first.
 */
static void quoteNumber(unsigned long n, char **str)
{
  char *s = *str;
  int d;

  do {
    d = n % ENC_BASE;
    n /= ENC_BASE;

    *s++ = FIRST_CH + d;
  } while (n > 0);

  *s = '\0';

  *str = s;
}


/*
 *  Return the amount of memory, not including a terminating nul, needed to
 *  encode the search key.  Characters that are outside the range of ASCII 33
 *  to 126, inclusive, are quoted as a percent followed by two hexadecimal
 *  digits.  The only exception is percent, which is encoded as `%25'.
 */
static int keySpace(const char *key)
{
  int i, n = 0;
  
  for (i = 0; i < strlen(key); i++) {
    if (MUST_QUOTE_CH(key[i])) {
      n += 3;
    } else {
      n++;
    }
  }

  return n;
}


static void quoteKey(const char *key, char **str)
{
  char *s = *str;
  int i;
  
  for (i = 0; i < strlen(key); i++) {
    if (MUST_QUOTE_CH(key[i])) {
      sprintf(s, "%c%02x", QUOTE_CH, (int)(unsigned char)key[i]);
      s += 3;
    } else {
      *s++ = key[i];
    }
  }

  *s = '\0';

  *str = s;
}


/*
 *  Return the amount of memory, not including a terminating nul, needed to
 *  store the components of a `struct patrie_node'.  A space separates each
 *  value.
 */
static int nodeSpace(struct patrie_node *node)
{
  return (numberSpace(node->val)     + 1 +
          numberSpace(node->is_skip) + 1 +
          numberSpace(node->lev)     + 1 +
          numberSpace(node->height)  + 1 +
          numberSpace(node->pos)     + 1 +
          numberSpace(node->inode)   + 1 +
          numberSpace(node->enode)   + 1 +
          numberSpace(node->tnode)   + 1 +
          numberSpace(node->pageNum));
}


static void quoteNode(struct patrie_node *node, char **str)
{
  char *s = *str;

  quoteNumber(node->val, &s);     *s++ = ' ';
  quoteNumber(node->is_skip, &s); *s++ = ' ';
  quoteNumber(node->lev, &s);     *s++ = ' ';
  quoteNumber(node->height, &s);  *s++ = ' ';
  quoteNumber(node->pos, &s);     *s++ = ' ';
  quoteNumber(node->inode, &s);   *s++ = ' ';
  quoteNumber(node->enode, &s);   *s++ = ' ';
  quoteNumber(node->tnode, &s);   *s++ = ' ';
  quoteNumber(node->pageNum, &s);

  *str = s;
}


/*
 *  Return the amount of memory, not including a terminating nul, needed to
 *  store a stack (array) of nodes.  A space separates each node, as well as
 *  the values within a node.
 */
static int stackSpace(struct patrie_state *state)
{
  int i, n = 0;
  
  for (i = 0; i <= state->top; i++) {
    n += nodeSpace(&state->stack[i]);
  }

  return n + state->top;        /* include space separators */
}


static void quoteStack(struct patrie_state *state, char **str)
{
  char *s = *str;
  int i;
  
  for (i = 0; i < state->top; i++) {
    quoteNode(&state->stack[i], &s); *s++ = ' ';
  }
  quoteNode(&state->stack[i], &s);

  *str = s;
}


/*
 *  Return the amount of memory, not including a terminating nul, needed to
 *  store the search state.  A space separates each component of the state.
 */
static int stateSpace(struct patrie_state *state)
{
  int space = 0;

  space += numberSpace(state->top) + 1;
  if (state->top >= 0) {        /* i.e. nelts > 0 */
    space += stackSpace(state);
  }

  space++;

  if (state->key) {
    space += keySpace(state->key);
  }

  space++;

  space += (numberSpace(state->matchCaseAccSens) + 1 +
            numberSpace(state->firstMatched));

  return space;
}


static void quoteState(struct patrie_state *state, char **str)
{
  char *s = *str;

  /*
   *  Warning: if you change the order in which the parts of the state are
   *  quoted you must change the order of the dequoting code, too.
   */
  
  quoteNumber(state->top, &s);          *s++ = ' ';
  if (state->top >= 0) {
    quoteStack(state, &s);
  }
                                        *s++ = ' ';
  if (state->key) {
    quoteKey(state->key, &s);
  }
                                        *s++ = ' ';
  quoteNumber(state->matchCaseAccSens, &s);     *s++ = ' ';
  quoteNumber(state->firstMatched, &s);
}


/*
 *  Set `*val' to the integer encoded by the two hexadecimal digits at `s'.
 */
static int getHex(const char *s, int *val)
{
  char *c0, *c1;
  char dig[] = "0123456789abcdefABCDEF";
  int hex[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 10, 11, 12, 13, 14, 15};

  if ((c0 = strchr(dig, s[0])) && (c1 = strchr(dig, s[1]))) {
    *val = hex[c0 - dig] * 16 + hex[c1 - dig];
    return 1;
  } else {
    fprintf(stderr, "%s: getHex: invalid hexadecimal pair `%c%c'.\n", prog, s[0], s[1]);
    return 0;
  }
}


/*
 *  We allocate the memory for the key.
 */
static int dequoteKey(const char *str, char **key, const char **newstr)
{
  char *k, *p;
  const char *s;
  size_t slen;                  /* length of the encoded key */

  for (s = str; *s && *s != ' '; s++) {
    continue;
  }
  slen = s - str;

  if (slen == 0) {
    fprintf(stderr, "%s: dequoteKey: no characters in key.\n", prog);
    return 0;
  }
  
  if ( ! (k = malloc(slen + 1))) {
    fprintf(stderr, "%s: dequoteKey: can't allocate %lu bytes for key: ",
            prog, (unsigned long)(slen + 1)); perror("malloc");
    return 0;
  }

  for (s = str, p = k; *s && *s != ' ';) {
    if (*s != QUOTE_CH) {
      *p++ = *s++;
    } else {
      int h;
    
      if ( ! getHex(++s, &h)) {
        free(k);
        return 0;
      }
      *p++ = h; s += 2;
    }
  }
  *p = '\0';

  *key = k;
  *newstr = s;

  return 1;
}


/*
 *  Convert a base 94 representation of a number to an unsigned long.
 */
static int dequoteULong(const char *str, unsigned long *num, const char **newstr)
{
  const char *s = str;
  unsigned long mult = 1, n = 0;

  while (*s && *s != ' ') {
    n += (*s++ - FIRST_CH) * mult;
    mult *= ENC_BASE;
  }

  if (s == str) {
    fprintf(stderr, "%s: dequoteULong: no digits in string.\n", prog);
    return 0;
  }

  *num = n;
  *newstr = s;

  return 1;
}


/*
 *  Convert a base 94 representation of a number to an signed int.  (We can
 *  use `dequoteULong' because we expect all the numbers to be positive.)
 */
static int dequoteSInt(const char *str, int *num, const char **newstr)
{
  unsigned long n;

  if ( ! dequoteULong(str, &n, newstr)) {
    fprintf(stderr, "%s: dequoteSInt: error from dequoteULong().\n", prog);
    return 0;
  }

  *num = n;

  return 1;
}


static int dequoteNode(const char *str, struct patrie_node *nd, const char **newstr)
{
#define TRY(x, e)                                               \
  do {                                                          \
    if ( ! (x)) {                                               \
      if (e) fprintf(stderr, "%s: dequoteNode: %s", prog, e);   \
      return 0;                                                 \
    }                                                           \
  } while (0)

  const char *s = str;

  TRY(dequoteULong(s, &nd->val, &s), "error getting `val' field from string.\n");
  TRY(*s++ == ' ', "space missing after `val' field.\n");

  TRY(dequoteSInt(s, &nd->is_skip, &s), "error getting `is_skip' from string.\n");
  TRY(*s++ == ' ', "space missing after `is_skip' field.\n");

  TRY(dequoteSInt(s, &nd->lev, &s), "error getting `lev' from string.\n");
  TRY(*s++ == ' ', "space missing after `lev' field.\n");

  TRY(dequoteSInt(s, &nd->height, &s), "error getting `height' from string.\n");
  TRY(*s++ == ' ', "space missing after `height' field.\n");

  TRY(dequoteSInt(s, &nd->pos, &s), "error getting `pos' from string.\n");
  TRY(*s++ == ' ', "space missing after `pos' field.\n");

  TRY(dequoteSInt(s, &nd->inode, &s), "error getting `inode' from string.\n");
  TRY(*s++ == ' ', "space missing after `inode' field.\n");

  TRY(dequoteSInt(s, &nd->enode, &s), "error getting `enode' from string.\n");
  TRY(*s++ == ' ', "space missing after `enode' field.\n");

  TRY(dequoteSInt(s, &nd->tnode, &s), "error getting `tnode' from string.\n");
  TRY(*s++ == ' ', "space missing after `tnode' field.\n");

  TRY(dequoteSInt(s, &nd->pageNum, &s), "error getting `pageNum' from string.\n");

  *newstr = s;

  return 1;

#undef TRY
}


static int dequoteStack(const char *str, int nelts, struct patrie_node *stack, const char **newstr)
{
  const char *s = str;
  int i;

  for (i = 0; i < nelts - 1; i++) {
    if ( ! dequoteNode(s, stack + i, &s)) {
      fprintf(stderr, "%s: dequoteStack: error getting node from search state.\n", prog);
      return 0;
    }
    if (*s++ != ' ') {
      fprintf(stderr, "%s: dequoteStack: space missing after node.\n", prog);
      return 0;
    }
  }

  if ( ! dequoteNode(s, stack + i, &s)) {
    fprintf(stderr, "%s: dequoteStack: error getting node from search state.\n", prog);
    return 0;
  }

  *newstr = s;

  return 1;
}


int patrieDequoteKey(const char *qkey, char **key, char **newstr)
{
  return dequoteKey(qkey, key, newstr);
}


int patrieQuoteKey(const char *key, char **qkey)
{
  char *k;
  int ksz;

  ksz = keySpace(key);
  if ( ! (k = malloc(ksz + 1))) {
    fprintf(stderr, "%s: patrieQuoteKey: can't allocate memory for quoted key: ", prog);
    perror("malloc");
    return 0;
  }

  quoteKey(key, &k);

  *qkey = k;

  return 1;
}


/*
 *  Return a string representation of the state.  Only printable ASCII
 *  characters are used.
 */
char *patrieGetStateString(struct patrie_state *state)
{
  char *str;
  size_t mem;

  mem = stateSpace(state);
  if ( ! (str = malloc(mem + 1))) {
    fprintf(stderr, "%s: patrieGetStateString: can't allocate %lu bytes for state: ",
            prog, (unsigned long)mem); perror("malloc");
    return 0;
  }

  quoteState(state, &str);

  str[mem] = '\0';

  return str;
}


int patrieSetStateFromString(const char *str, struct patrie_state **state)
{
#define TRY(x, e)                                                               \
  do {                                                                          \
    errno = 0;                                                                  \
    if ( ! (x)) {                                                               \
      if (*e) fprintf(stderr, "%s: patrieSetStateFromString: %s", prog, e);     \
      if (errno != 0) perror("");                                               \
      if (st) patrieFreeState(&st);                                             \
      return 0;                                                                 \
    }                                                                           \
  } while (0)
                
  const char *s = str;
  struct patrie_state *st = 0;
  
  TRY(patrieAllocState(&st), "can't allocate search state structure.\n");
    
  TRY(dequoteSInt(s, &st->top, &s), "");
  TRY(*s++ == ' ', "missing space after `top' field.\n");

  st->nelts = st->top + 1;
  if (st->nelts > 0) {
    TRY(st->stack = malloc(st->nelts * sizeof st->stack[0]),
        "can't allocate search stack: ");

    TRY(dequoteStack(s, st->nelts, st->stack, &s), "");
  }
  TRY(*s++ == ' ', "missing space after stack values.\n");

  if (*s != ' ') {
    TRY(dequoteKey(s, &st->key, &s), "");
  }
  TRY(*s++ == ' ', "missing space after search string.\n");

  TRY(dequoteSInt(s, &st->matchCaseAccSens, &s), "");
  TRY(*s++ == ' ', "missing space after case sensitivity flag.\n");

  TRY(dequoteSInt(s, &st->firstMatched, &s), "");
  TRY(*s++ == '\0', "missing nul terminator after first matched flag.\n");
  
  *state = st;

  return 1;

#undef TRY
}
