#include <stdio.h>
#include "amisc.h"
#include "all.h"

#define do_strrspn(s1, s2, n) \
do \
{ \
  char s[128]; \
  strcpy(s, s1); \
  printf("strrspn(\"%s\", \"%s\") %s.\n", s1, s2, \
         strrspn(s, s2) == n ? "worked" : "failed"); \
} \
while (0)
  
#define do_ws(s) \
do \
{ \
  char x[128]; \
  strcpy(x, s); \
  printf("`%s' --> `%s'.\n", s, nuke_afix_ws(x)); \
} \
while (0)
  

char *prog;

main(ac, av)
  int ac;
  char **av;
{
  prog = av[0];

  do_strrspn("", "", 0);
  do_strrspn("", "1", 0);
  do_strrspn("", "11", 0);
  do_strrspn("1", "", 0);
  do_strrspn("11", "", 0);
  do_strrspn("1", "2", 0);
  do_strrspn("11", "2", 0);
  do_strrspn("11", "22", 0);
  do_strrspn("101010", "12", 0);
  do_strrspn("101020", "01", 1);
  do_strrspn("101020", "10", 1);
  do_strrspn("101010", "01", 6);
  do_strrspn("101010", "10", 6);
  do_strrspn("10101000", "21304", 8);
  do_strrspn("10101000", "2304", 3);

  do_ws("");
  do_ws("x");
  do_ws("xx");
  do_ws("x x");
  do_ws("x  x");
  do_ws(" xx");
  do_ws("xx ");
  do_ws("   x");
  do_ws("x   ");
  do_ws("   xx   ");
  do_ws("   xx  xx   ");

  return 0;
}
