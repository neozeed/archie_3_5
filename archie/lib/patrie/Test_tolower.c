#include <ctype.h>
#include <stdio.h>
#include <string.h>


#define TOLOWER(c) ((c) <= 'Z') && ((c) >= 'A' ? (c) | 0x20 : (c))


static void checkMacro(void)
{
  int i;
  
  for (i = -256; i < 256; i++) {
    if (TOLOWER(i) != tolower(i)) {
      fprintf(stderr, "Macro and function differ on `%c' (%d decimal).\n", i, i);
      exit(1);
    }
  }

  fprintf(stderr, "Macro and function agree on all values.\n");
  exit(0);
}


static void printValues(void)
{
  int i;
  
  for (i = -256; i < 256; i++) {
    printf("%d\n", tolower((unsigned char)i));
  }

  exit(0);
}


int main(int ac, char **av)
{
  if (ac == 1) checkMacro();
  else printValues();

  return 0;                     /* keep gcc quiet */
}

