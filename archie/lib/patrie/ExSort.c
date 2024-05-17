/*  
 *  Examine a sorted text file.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Test.h"
#include "utils.h"


#define MAXLINELEN 256
#define AROUND 5
#define WIDTH 15


char *prog;
int around = AROUND, width = WIDTH;


void help(void)
{
  char *h[] = {
    "a <n>", "display <n> strings around the target",
    "h", "print this help message",
    "q", "quit",
    "r", "repeat last command",
    "s <start>", "print strings around <start>",
    "t <text>", "print strings around <text>",
    "w <n>", "display the first <n> characters of starts",
    0, 0
  };
  int i;
  
  printf("Commands:\n\n");
  for (i = 0; h[i]; i += 2) {
    printf("\t%-15s # %s\n", h[i], h[i+1]);
  }
}


void stringPrint(const char *text, size_t off)
{
  int i;
  
  printf("%-8ld `", (long)off);
  for (i = 0; i < width && text[off+i]; i++) {
    fputs(chstr(text[off+i]), stdout);
  }
  printf("'");
}


void stringsPrint(const char *text, size_t nstarts, long *start, int idx)
{
  int i;
  
  for (i = idx - around; i <= idx + around; i++) {
    if (i >= 0 && i < nstarts) {
      if (i == idx) printf(" * "); else printf("   ");
      stringPrint(text, start[i]); printf("\n");
    }
  }
}


void startFind(const char *text, size_t nstarts, long *start, long st)
{
  int i;
  
  if (st < 0) st = -st - 1;

  for (i = 0; i < nstarts; i++) {
    if (start[i] == st) break;
  }

  if (i == nstarts) {
    printf("*** start %ld not found ***\n", st);
  } else {
    stringsPrint(text, nstarts, start, i);
  }
}


void strFind(size_t textsz, char *text, size_t nstarts, long *start, const char *str)
{
  char *p, s[256];
  void *bmstate;

  bdequote(str, sizeof s, s);
  if (bmpreproc(s, &bmstate)) {
    if ( ! (p = bmstrstr(s, textsz, text, bmstate))) {
      fprintf(stderr, "*** string `%s' not found ***\n", str);
    } else {
      startFind(text, nstarts, start, p - text);
    }
    free(bmstate);
  }
}


void usage(void)
{
  fprintf(stderr, "Usage: %s [-help] <text-file> <infix-trie>\n", prog);
  fprintf(stderr, "\n");
  fprintf(stderr, "\t-help\t# print this message\n");
  fprintf(stderr, "\n");
  exit(1);
}


int main(int ac, char **av)
{
  FILE *ifp, *tfp;
  char *text;
  int i;
  long *start;
  long val;
  off_t ilen, tlen;
  size_t nstarts;
  
  prog = av[0];

  while (av++, --ac) {
    if (ARG_IS("-help")) usage();
    else                 break;
  }

  if (ac != 2) usage();

  tfp = openFile(av[0], "rb");
  ifp = openFile(av[1], "rb");

  ilen = _patrieFpSize(ifp);
  nstarts = (ilen / sizeof(long) - 2) / 2 + 1;

  if ( ! (start = malloc(nstarts * sizeof(long)))) {
    fprintf(stderr, "%s: can't allocate %lu bytes for starts: ",
            prog, (unsigned long)(nstarts * sizeof(long)));
    perror("malloc");
    exit(1);
  }

  tlen = _patrieFpSize(tfp);
  
  if ( ! (text = malloc(tlen + 1))) {
    fprintf(stderr, "%s: can't allocate %ld bytes for text: ", prog, tlen + 1);
    perror("malloc");
    exit(1);
  }
  
  for (i = 0; fread(&val, sizeof(long), 1, ifp) > 0;) {
    if (val < 0) start[i++] = -val - 1;
  }
  fclose(ifp);

  fread(text, 1, tlen, tfp); text[tlen] = '\0';
  fclose(tfp);

  {
    char oline[MAXLINELEN];
    
    oline[0] = '\0';
    
    while (1) {
      char *nl;
      char line[MAXLINELEN], str[MAXLINELEN];
      long st;

      printf("> "); fflush(stdout);
      if ( ! fgets(line, sizeof line, stdin)) exit(0);
      if ((nl = strchr(line, '\n'))) *nl = '\0';

    rerun:
      if (sscanf(line, " a %d", &around) == 1) {
        /* do nothing more */
      } else if (sscanf(line, " %[h]", str) == 1) {
        help();
      } else if (sscanf(line, " %[q]", str) == 1) {
        exit(0);
      } else if (sscanf(line, " %[r]", str) == 1) {
        strcpy(line, oline); goto rerun;
      } else if (sscanf(line, " s %ld", &st) == 1) {
        startFind(text, nstarts, start, st);
      } else if (sscanf(line, " t \"%[^\"]\"", str) == 1 ||
                 sscanf(line, " t %s", str) == 1) {
        strFind(tlen, text, nstarts, start, str);
      } else if (sscanf(line, " w %d", &width) == 1) {
        /* do nothing more */
      } else if (strspn(line, " \t") == strlen(line)) {
        continue;
      } else {
        printf("Unrecognized command `%s'.\n", line);
        continue;
      }

      strcpy(oline, line);
    }
  }

  exit(0);
}

