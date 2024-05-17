#include <stdio.h>
#include "misc.h"
#include "str.h"
#include "all.h"


void print(src, splchs, n, arr)
  const char *src;
  const char *splchs;
  int n;
  const char **arr;
{
  int i;

  printf("Source: `%s'        Split chars: `%s'\n", src, splchs);
  for (i = 0; i < n; i++) {
    printf("        `%s'\n", arr[i]);
  }
}
  

const char *prog;
int debug;


int main(ac, av)
  int ac;
  char **av;
{
#if 1
  char **argv;
  int argc;

#define try(a, b) \
  do{ strsplit(a,b,&argc,&argv); print(a,b,argc,argv); free((char *)argv); }while(0)

  try("", "");
  try("", "0");
  try("", "01234");

  try("0", "");
  try("01", "");
  try("01234", "");

  try("0/12/34", "abc");
  try("0/12/34", "/");
  try("0/12/34", "+-/=");

  try("/123/45/", "+-/=");
  try("/12//45/", "+-/=");
  try("/12///45/", "+-/=");
  try("//12", "+-/=");
  try("45//", "+-/=");

  try("/a+/bc=", "+-/=");

#undef try
  
#else
  
  char *rest;
  char *s[64];
  int r;
  
  prog = av[0];
  splitstr = av[1]; av++, --ac;
  while (av++, --ac)
  {
    r = strnsplit(splitstr, av[0], s, 3, &rest);
    if (r == 0)
    {
      printf("%s: strnsplit() returned 0 on `%s' (rest is `%s').\n",
             prog, av[0], rest);
    }
    else
    {
      int i;
      
      printf("%s: %d substring(s):", prog, r);
      for (i = 0; i < r; i++)
      {
        printf(" `%s'", s[i]);
      }
      printf("; rest is `%s'.\n", rest);
    }
  }
#endif
  
  return 0;
}
