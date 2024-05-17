#include <stdio.h>
#include <string.h>


int main(void)
{
  char *a = "\222";
  char *b = "\122";

  printf("chars are %s.\n", a[0] < 0 ? "signed" : "unsigned");
  printf("strcmp() compares chars as %s.\n",
         strcmp(a, b) < 0 ? "signed" : "unsigned");
  printf("strncmp() compares chars as %s.\n",
         strncmp(a, b, 1) < 0 ? "signed" : "unsigned");
  printf("strcasecmp() compares chars as %s.\n",
         strcasecmp(a, b) < 0 ? "signed" : "unsigned");
  printf("strncasecmp() compares chars as %s.\n",
         strncasecmp(a, b, 1) < 0 ? "signed" : "unsigned");

  return 0;
}
