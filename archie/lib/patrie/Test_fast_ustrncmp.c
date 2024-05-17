#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MIN(a, b) ((a) < (b) ? (a) : (b))
typedef unsigned char uc;


static int simple(const char *s0, const char *s1, size_t n)
{
  int i;
  
  for (i = 0; i < n; i++) {
    if (s0[i] != s1[i]) return (uc)s0[i] < (uc)s1[i] ? -1 : 1;
  }

  return 0;
}


/*  
 *  Assume arguments contain at least `n' characters.
 */
static int ugly4(const char *s0, const char *s1, size_t n)
{
#define C(i) if (s0[i] != s1[i]) return (uc)s0[i] < (uc)s1[i] ? -1 : 1

 top:
  switch (n) {
  case 4:  C(0); C(1); C(2); C(3); return 0;
  case 3:  C(0); C(1); C(2); return 0;
  case 2:  C(0); C(1); return 0;
  case 1:  C(0); return 0;
  case 0:  return 0;

  default:
    {
      size_t i;
      for (i = 0; i < n - 4; i++) {
        C(i);
      }
      s0 += i; s1 += i; n -= 4;
      goto top;
    }
  }

#undef C
}


/*  
 *  Assume arguments contain at least `n' characters.
 */
static int ugly10(const char *s0, const char *s1, size_t n)
{
#define C(i) if (s0[i] != s1[i]) return (uc)s0[i] < (uc)s1[i] ? -1 : 1

 top:
  switch (n) {
  case 10: C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); return 0;
  case 9:  C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); return 0;
  case 8:  C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); return 0;
  case 7:  C(0); C(1); C(2); C(3); C(4); C(5); C(6); return 0;
  case 6:  C(0); C(1); C(2); C(3); C(4); C(5); return 0;
  case 5:  C(0); C(1); C(2); C(3); C(4); return 0;
  case 4:  C(0); C(1); C(2); C(3); return 0;
  case 3:  C(0); C(1); C(2); return 0;
  case 2:  C(0); C(1); return 0;
  case 1:  C(0); return 0;
  case 0:  return 0;

  default:
    {
      size_t i;
      for (i = 0; i < n - 10; i++) {
        C(i);
      }
      s0 += i; s1 += i; n -= 10;
      goto top;
    }
  }

#undef C
}


/*  
 *  Assume arguments contain at least `n' characters.
 */
static int ugly10_(const char *s0, const char *s1, size_t n)
{
#define C(i) if (s0[i] != s1[i]) return (uc)s0[i] < (uc)s1[i] ? -1 : 1

  if (n > 10) {
    size_t i;
    for (i = 0; i < n - 10; i++) {
      C(i);
    }
    s0 += i; s1 += i; n = 10;
  }
  
  switch (n) {
  case 10: C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); return 0;
  case 9:  C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); return 0;
  case 8:  C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); return 0;
  case 7:  C(0); C(1); C(2); C(3); C(4); C(5); C(6); return 0;
  case 6:  C(0); C(1); C(2); C(3); C(4); C(5); return 0;
  case 5:  C(0); C(1); C(2); C(3); C(4); return 0;
  case 4:  C(0); C(1); C(2); C(3); return 0;
  case 3:  C(0); C(1); C(2); return 0;
  case 2:  C(0); C(1); return 0;
  case 1:  C(0); return 0;
  case 0:  return 0;
  }

#undef C
}


static int duff16(const char *s0, const char *s1, size_t n)
{
  if (n == 0) return 0;

  switch (n % 16) {
  case 0: do { if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 14:     if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 13:     if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 12:     if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 11:     if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 10:     if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 9:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 8:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 7:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 6:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 5:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 4:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 3:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 2:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 1:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
             } while (--n > 0);
             return 0;
  }
}


static int duff8(const char *s0, const char *s1, size_t n)
{
  if (n == 0) return 0;

  switch (n % 8) {
  case 0: do { if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 7:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 6:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 5:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 4:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 3:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 2:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
  case 1:      if (*s0++ != *s1++) return (uc)*(s0-1) < (uc)*(s1-1) ? -1 : 1;
             } while (--n > 0);
             return 0;
  }
}


static int duff8_(const char *s0, const char *s1, size_t n)
{
  if (n == 0) return 0;

  switch (n % 8) {
  case 0: do { if (*s0++ != *s1++) return (uc)*(s0-1) - (uc)*(s1-1);
  case 7:      if (*s0++ != *s1++) return (uc)*(s0-1) - (uc)*(s1-1);
  case 6:      if (*s0++ != *s1++) return (uc)*(s0-1) - (uc)*(s1-1);
  case 5:      if (*s0++ != *s1++) return (uc)*(s0-1) - (uc)*(s1-1);
  case 4:      if (*s0++ != *s1++) return (uc)*(s0-1) - (uc)*(s1-1);
  case 3:      if (*s0++ != *s1++) return (uc)*(s0-1) - (uc)*(s1-1);
  case 2:      if (*s0++ != *s1++) return (uc)*(s0-1) - (uc)*(s1-1);
  case 1:      if (*s0++ != *s1++) return (uc)*(s0-1) - (uc)*(s1-1);
             } while (--n > 0);
             return 0;
  }
}


void blob(void)
{
#define SZ 512
  
  int a[SZ];
  int b[SZ];
  int i;

  for (i = 0; i < SZ; i++) {
    a[i] = i; b[i] = SZ - 1 - i;
  }
  
#if 0
  for (i = 0; i < SZ; i++) {
    a[i] = (b[a[i]] + a[b[i]]) / 2;
    b[i] = (a[b[i]] + b[a[i]]) / 2;
  }

  for (i = 0; i < SZ; i++) {
    a[i] = (b[b[i]] + b[b[i]]) / 2;
    b[i] = (a[a[i]] + a[b[i]]) / 2;
  }
#endif

#undef SZ
}


const char *s[] = {
  "",
  "1",
  "12",
  "123",
  "1234",
  "12345",
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


#define RUN(fn, s0, s1, n) res = fn(s0, s1, n)
#define VERIFY(fn, s0, s1, n) if (fn(s0, s1, n) * strncmp(s0, s1, n) < 0) abort()


struct fnTab {
  const char *name;
  int (*fn)(const char *s0, const char *s1, size_t n);
} fntab[] = {
#define FN(name) { #name, name },

  FN(duff16)
  FN(duff8)
  FN(duff8_)
  FN(simple)
  FN(strncmp)
  FN(ugly4)
  FN(ugly10)
  FN(ugly10_)

  { 0, 0 }

#undef FN
};


int main(int ac, char **av)
{
  const char *name;
  int (*fn)(const char *s0, const char *s1, size_t n);
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
    VERIFY(fn, s[i], s[i], i);
    VERIFY(fn, s[i], s[sz - i - 1], MIN(i, sz - i - 1));
  }

  nloops = atoi(av[2]);

  for (loop = 0; loop < nloops; loop++) {
    for (i = 0; i < sz; i++) {
      RUN(fn, s[i], s[sz - i - 1], MIN(i, sz - i - 1));
      blob();
    }
  }

  fprintf(stderr, "\n%s\n", name);

  exit(0);
}
