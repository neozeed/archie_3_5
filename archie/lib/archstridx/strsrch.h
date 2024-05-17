#include <stddef.h>

/*
 *  used to pass to _archBMHSearch functions.  determines what type of
 *    comparison functions to use
 */
struct funcs {
  int (*lowerCase)(int);
  int (*upperCase)(int);
  int (*noAccent)(int);
  int (*withAccent)(int);
  int (*toComp)(int *,int *);
};

/*
 *  got that from libpatrie.a
 */

extern int NoAccent(const char c);
extern int _patrieUStrCiAiCmp(const char *s0, const char *s1);
extern int _patrieUStrCiAsCmp(const char *s0, const char *s1);
extern int _patrieUStrCsAiCmp(const char *s0, const char *s1);
extern int _patrieUStrCsAsCmp(const char *s0, const char *s1);

extern void _archBMHCaseSearch(const char *text,
                               unsigned long tlen,
                               const char *key,
                               unsigned long klen,
                               unsigned long maxhits,
                               unsigned long *nhits,
                               unsigned long start[]);
extern void _archBMHSearch(const char *text,
                           unsigned long tlen,
                           const char *key,
                           unsigned long klen,
                           unsigned long maxhits,
                           unsigned long *nhits,
                           unsigned long start[]);
extern void _archBMHGeneralSearch(const char *text,
                                  unsigned long tlen,
                                  const char *key,
                                  unsigned long klen,
                                  unsigned long maxhits,
                                  unsigned long *nhits,
                                  unsigned long start[],
                                  struct funcs funs);

