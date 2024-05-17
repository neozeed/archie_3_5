#include <stdio.h>
#include "patrie.h"

/* #define BSZ 5*/

#include "text.c"


FILE *fp;
char f[] = "0123456789\nabcdef\n";
struct patrie_config *cf;


void check(long offset, const char *stop)
{
  char buf[sizeof f], *s;
  size_t len;
  
  if ( ! (s = patrieGetSubstring(cf, offset, stop))) {
    fprintf(stderr, "%s: check: patrieGetSubstring() failed on offset %ld, stop `%s'.\n",
            prog, offset, stop);
    abort();
  }

  len = strcspn(f + offset, stop);
  strncpy(buf, f + offset, len);
  buf[len] = '\0';

  if (strcmp(buf, s) != 0) {
    fprintf(stderr, "%s: check: check failed!\n", prog);
    abort();
  }

  free(s);
}


char *prog;


int main(int ac, char **av)
{
  int i;

  prog = av[0];

  if ( ! (fp = tmpfile())) {
    fprintf(stderr, "%s: can't create temporary file: ", prog); perror("tmpfile");
    exit(1);
  }
  
  patrieAlloc(&cf);
  patrieSetTextFP(cf, fp);

  if (fwrite(f, 1, sizeof f - 1, fp) != sizeof f - 1) {
    fprintf(stderr, "%s: error writing to temporary file: ", prog); perror("fwrite");
    exit(1);
  }

  fflush(fp);

  for (i = 0; i < strlen(f); i++) {
    check(i, "");
    check(i, "0");
    check(i, "01");
    check(i, "489");
    check(i, "\n");
    check(i, "fed");
  }

  exit(0);
}
