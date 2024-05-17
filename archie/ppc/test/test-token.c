#include <stdio.h>
#include <string.h>
#include "ppc.h"

const char *tstr10[] =
{
  "str0", "2", "xxxxxx3", "", "str5", "str6", "str7", "str8", "str9", 0
};

const char *tstr3[] =
{
  "x0", "", "x2", 0
};


int main(int ac, char **av)
{
  TOKEN t;
  char **tarr;
  int i;

  /*  
   *
   *  NOW TESTING: tkmatches().
   *
   */  

  /* TEST: null pointer -- should return one 0 element. */
  t = 0;
  if ( ! (tarr = tkarray(0))) abort();
  if (tarr[0]) abort();
  free(tarr);

  /* TEST: list of one element */
  t = 0;
  t = tkappend(tstr10[0], t);
  tarr = tkarray(t);
  if (strcmp(tarr[0], tstr10[0]) != 0) abort();
  if (tarr[1]) abort();
  tklfree(t);
  free(tarr);

  /* TEST: list of length > 1 */
  for (i = 0, t = 0; tstr10[i]; i++)
  {
    t = tkappend(tstr10[i], t);
  }
  tarr = tkarray(t);
  for (i = 0; tarr[i]; i++)
  {
    if (strcmp(tarr[i], tstr10[i]) != 0) abort();
  }
  tklfree(t);
  free(tarr);

  /*  
   *
   *  NOW TESTING: tkmatches().
   *
   */

  /* TEST: null pointer -- should return 0. */
  if (tkmatches((TOKEN)0, (char *)0)) abort();
  if (tkmatches((TOKEN)0, "foo", (char *)0)) abort();

  /* TEST: list of one element */
  t = tkappend(tstr10[0], (TOKEN)0);
  if ( ! tkmatches(t, tstr10[0], (char *)0)) abort();
  if (tkmatches(t, tstr10[0], tstr10[1], (char *)0)) abort();
  if ( ! tkmatches(t, (char *)0)) abort();
  tklfree(t);

  /* TEST: list of length > 1 */
  for (i = 0, t = 0; tstr3[i]; i++)
  {
    t = tkappend(tstr3[i], t);
  }
  if ( ! tkmatches(t, tstr3[0], (char *)0)) abort();
  if ( ! tkmatches(t, tstr3[0], tstr3[1], (char *)0)) abort();
  if ( ! tkmatches(t, tstr3[0], tstr3[1], tstr3[2], (char *)0)) abort();

  if (tkmatches(t, "12345", (char *)0)) abort();
  if (tkmatches(t, tstr3[0], "12345", tstr3[2], (char *)0)) abort();
  tklfree(t);
  free(tarr);

  return 0;
}
