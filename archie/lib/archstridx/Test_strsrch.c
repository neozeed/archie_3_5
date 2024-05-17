#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Test.h"
#include "strsrch.h"


char *prog;


char *strcopy(const char *str)
{
  char *s = malloc(strlen(str) + 1);
  return s ? strcpy(s, str) : s;
}


char *strlower(const char *str)
{
  char *p, *s = strcopy(str);
  if ((p = s)) while ((*p = tolower(*p))) p++;
  return s;
}


char *strcasestr(const char *text, const char *key)
{
  char *k = strlower(key), *t = strlower(text);
  char *r, *s;

  ASSERT(k); ASSERT(t);
  r = (s = strstr(t, k)) ? text + (size_t)(s - t) : 0;
  free(k); free(t);
  return r;
}


void simpleTest(void fn(const char *text, unsigned long tlen, const char *key, unsigned long klen,
                        unsigned long maxhits, unsigned long *nhits, unsigned long start[]),
                char *strmatch(const char *text, const char *key),
                const char *text, const char *key)
{
  const char *s, *t;
  size_t i, n;
  size_t st[100];
  unsigned long nhits, start[100];
    
  /* First, count how many matches strstr() finds. */

  for (n = 0, t = text;
       t <= text + strlen(text) && (s = strmatch(t, key));
       t = s + 1) {
    st[n++] = s - text;
  }

  fn(text, strlen(text), key, strlen(key), 100, &nhits, start);

  ASSERT(n == nhits);
  for (i = 0; i < n; i++) {
    ASSERT(st[i] == start[i]);
  }
}


/*  
 *  Perform all the tests from Test_archBMHSearch(), plus a few more.
 */
void Test__archBMHCaseSearch(void) {
  char *text;

  /*  
   *  Empty text string.
   */

  simpleTest(_archBMHCaseSearch, strcasestr, "", "");
  simpleTest(_archBMHCaseSearch, strcasestr, "", "x");
  simpleTest(_archBMHCaseSearch, strcasestr, "", "xyz");

  /*  
   *  Empty key.
   */

  simpleTest(_archBMHCaseSearch, strcasestr, "x", "");
  simpleTest(_archBMHCaseSearch, strcasestr, "xy", "");
  simpleTest(_archBMHCaseSearch, strcasestr, "xyz", "");

  /*  
   *  Strings occuring once.
   */

  text = "foo bar x\222y foo blah foo";

  simpleTest(_archBMHCaseSearch, strcasestr, text, "x");
  simpleTest(_archBMHCaseSearch, strcasestr, text, "x\222y");
  simpleTest(_archBMHCaseSearch, strcasestr, text, "foo ba");
  simpleTest(_archBMHCaseSearch, strcasestr, text, "h foo");

  /*  
   *  Strings occuring more than once.
   */

  simpleTest(_archBMHCaseSearch, strcasestr, text, "foo");

  /*  
   *  Different case.
   */

  simpleTest(_archBMHCaseSearch, strcasestr, text, "fOo");

  fprintf(stderr, "%s: _archBMHCaseSearch() passed.\n", prog);
}


void Test__archBMHSearch(void) {
  char *text;

  /*  
   *  Empty text string.
   */

  simpleTest(_archBMHSearch, strstr, "", "");
  simpleTest(_archBMHSearch, strstr, "", "x");
  simpleTest(_archBMHSearch, strstr, "", "xyz");

  /*  
   *  Empty key.
   */

  simpleTest(_archBMHSearch, strstr, "x", "");
  simpleTest(_archBMHSearch, strstr, "xy", "");
  simpleTest(_archBMHSearch, strstr, "xyz", "");

  /*  
   *  Strings occuring once.
   */

  text = "foo bar x\222y foo blah foo";

  simpleTest(_archBMHSearch, strstr, text, "x");
  simpleTest(_archBMHSearch, strstr, text, "x\222y");
  simpleTest(_archBMHSearch, strstr, text, "foo ba");
  simpleTest(_archBMHSearch, strstr, text, "h foo");

  /*  
   *  Strings occuring more than once.
   */

  simpleTest(_archBMHSearch, strstr, text, "foo");

  fprintf(stderr, "%s: _archBMHSearch() passed.\n", prog);
}


int main(int ac, char **av)
{
  prog = av[0];
  
  Test__archBMHSearch();
  Test__archBMHCaseSearch();

  return 0;
}
