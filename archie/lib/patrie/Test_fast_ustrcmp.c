#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef unsigned char uc;


#define CMP(i) \
  do { \
    if (s0[i] != s1[i]) return (uc)s0[i] < (uc)s1[i] ? -1 : 1; \
    else if (s0[i] == '\0')  return 0; \
  } while (0)
#define INC(i) do { s0 += i; s1 += i; } while (0)


static int fast_ustrcmp1(const char *s0, const char *s1)
{
  while (1) {
    CMP(0);
    INC(1);
  }
}


static int fast_ustrcmp2(const char *s0, const char *s1)
{
  while (1) {
    CMP(0);
    CMP(1);
    INC(2);
  }
}


static int fast_ustrcmp3(const char *s0, const char *s1)
{
  while (1) {
    CMP(0);
    CMP(1);
    CMP(2);
    INC(3);
  }
}


static int fast_ustrcmp4(const char *s0, const char *s1)
{
  while (1) {
    CMP(0);
    CMP(1);
    CMP(2);
    CMP(3);
    INC(4);
  }
}


static int fast_ustrcmp5(const char *s0, const char *s1)
{
  while (1) {
    CMP(0);
    CMP(1);
    CMP(2);
    CMP(3);
    CMP(4);
    INC(5);
  }
}


static int fast_ustrcmp6(const char *s0, const char *s1)
{
  while (1) {
    CMP(0);
    CMP(1);
    CMP(2);
    CMP(3);
    CMP(4);
    CMP(5);
    INC(6);
  }
}


static int fast_ustrcmp7(const char *s0, const char *s1)
{
  while (1) {
    CMP(0);
    CMP(1);
    CMP(2);
    CMP(3);
    CMP(4);
    CMP(5);
    CMP(6);
    INC(7);
  }
}


static int fast_ustrcmp8(const char *s0, const char *s1)
{
  while (1) {
    CMP(0);
    CMP(1);
    CMP(2);
    CMP(3);
    CMP(4);
    CMP(5);
    CMP(6);
    CMP(7);
    INC(8);
  }
}


#if 0
static int fast_ustrncmp(const char *s0, const char *s1, size_t n)
{
}
#endif


const char *s[] = {
  "",
  "1",
  "12",
  "123",
  "1234",
  "12345"
  "123456",
  "1234567",
  "12345678",
  "123456789",
  "1234567890",
  "12345678901",
  "123456789012",
  "1234567890123",
  "12345678901234",
  "123456789012345",
  "1234567890123456",
  "12345678901234567",
  "123456789012345679",
  "1234567890123456790",
  "12345678901234567901",
  "123456789012345679012",
  "1234567890123456790123",
  "12345678901234567901234",
  "123456789012345679012345",
  "1234567890123456790123456",
  "12345678901234567901234567",
  "123456789012345679012345678",
  "1234567890123456790123456789",
  "12345678901234567901234567890",
};


#define RUN(fn, s0, s1) res = fn(s0, s1)
#define VERIFY(fn, s0, s1) if (fn(s0, s1) * strcmp(s0, s1) < 0) abort()


struct fnTab {
  const char *name;
  int (*fn)(const char *s0, const char *s1);
} fntab[] = {
#define FN(name) { #name, name },

  FN(fast_ustrcmp1)
  FN(fast_ustrcmp2)
  FN(fast_ustrcmp3)
  FN(fast_ustrcmp4)
  FN(fast_ustrcmp5)
  FN(fast_ustrcmp6)
  FN(fast_ustrcmp7)
  FN(fast_ustrcmp8)
  FN(strcmp)

  { 0, 0 }

#undef FN
};


int main(int ac, char **av)
{
  const char *name;
  int (*fn)(const char *s0, const char *s1);
  int i, loop, nloops, res;
  size_t sz = sizeof s / sizeof s[0];
  
  if (ac != 3) {
    fprintf(stderr, "Usage: %s <fn-name> <#-loops>\n", av[0]);
    exit(1);
  }

  for (i = 0; fntab[i].name; i++) {
    if (strcmp(fntab[i].name, av[1]) == 0) {
      name = fntab[i].name; fn = fntab[i].fn;
      break;
    }
  }

  if ( ! fntab[i].name) {
    fprintf(stderr, "Invalid function name.  Known names are:\n\n\t");
    for (i = 0; fntab[i].name; i++) {
      fprintf(stderr, " %s", fntab[i].name);
    }
    fprintf(stderr, "\n");
    exit(1);
  }

  for (i = 0; i < sz; i++) {
    VERIFY(fn, s[i], s[i]);
    VERIFY(fn, s[i], s[sz - i - 1]);
  }

  nloops = atoi(av[2]);

  for (loop = 0; loop < nloops; loop++) {
    for (i = 0; i < sz; i++) {
      RUN(fn, s[i], s[sz - i - 1]);
    }
  }

  fprintf(stderr, "\n%s\n", name);

  exit(0);
}
