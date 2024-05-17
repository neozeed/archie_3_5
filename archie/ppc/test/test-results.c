#include "ppc.h"
#include "results.h"
#include "all.h"


#define OFN(n) \
int ofn##n(FILE *ofp, VLINK v, TOKEN t) \
{                                       \
  printf("ofn" #n);                     \
  while (t && t->token)                 \
  {                                     \
    printf(" `%s'", t->token);          \
    t = t->next;                        \
  }                                     \
  printf("\n");                         \
}

#define REG(n) registerOutputFunction("OUTPUT" #n, ofn##n)


OFN(0)
OFN(1)
OFN(2)
OFN(3)
OFN(4)

  
const char *prog;
int debug = 0;


int main(ac, av)
  int ac;
  char **av;
{
  struct Result res;
  prog = av[0];

  REG(0);
  REG(1);
  REG(2);
#if 0
  REG(3);
  REG(4);
#endif
  REG(2); /* repeat 2 */

  res.res = 0;
  res.commands = 0;
  
  {
    TOKEN l = res.commands;

    l = tkappend("OUTPUT0", l); tkappend(0, l);
    tkappend("OUTPUT1", l); tkappend("a1", l); tkappend(0, l);
    tkappend("OUTPUT2", l); tkappend("a1", l); tkappend("a2", l); tkappend(0, l);
#if 0
    tkappend("OUTPUT3", l); tkappend(0, l);
    tkappend("OUTPUT4", l); tkappend(0, l);
#endif
    res.commands = l;
  }
  
  printContents();
  displayResults(stdout, &res);
  cancelOutputFunctions();
  
  return 0;
}
