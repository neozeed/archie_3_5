#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Test.h"
#include "defs.h"


FILE *openFile(const char *name, const char *mode)
{
  FILE *fp;
  
  /* bug: HACK */
  if (strcmp(name, "-") == 0) {
    if (mode[0] == 'r') return stdin;
    else return stdout;
  }

  if ( ! (fp = fopen(name, mode))) {
    fprintf(stderr, "%s: openFile: can't open `%s' with mode `%s': ", prog, name, mode);
    perror("fopen");
    exit(1);
  }

  return fp;
}


FILE *openTemp(const char *name, const char *mode)
{
  FILE *fp;
  
  fp = openFile(name, mode);
  unlink(name);

  return fp;
}


/*  
 *  Read a string, trashing any trailing newline.
 */
char *getSearchKey(void)
{
  static char key[256];

  if (fgets(key, sizeof key, stdin)) {
    char *nl = strchr(key, '\n');
    if (nl) *nl = '\0';
    return key;
  }
  return 0;
}


/*  
 *  Create a child.  Both parent and child receive input and output file
 *  pointers for communicating with each other.
 */
int fpFork(FILE **rdFP, FILE **wrFP)
{
  FILE *ifp, *ofp;
  int cp[2];
  int pc[2];
  int pid;

  if (pipe(cp) == -1) {
    fprintf(stderr, "%s: pfork: pipe() failed: ", prog); perror("pipe");
    return -1;
  }

  if (pipe(pc) == -1) {
    fprintf(stderr, "%s: pfork: pipe() failed: ", prog); perror("pipe");
    close(cp[0]); close(cp[1]);
    return -1;
  }

  switch (pid = fork()) {
  case -1:
    fprintf(stderr, "%s: pfork: fork() failed: ", prog); perror("fork");
    close(cp[0]); close(cp[1]);
    close(pc[0]); close(pc[1]);
    break;

  case 0:                       /* child */
    close(pc[1]);
    close(cp[0]);
    if ( ! (ifp = fdopen(pc[0], "rb"))) {
      fprintf(stderr, "%s: fpFork: can't open descriptor %d with mode `%s': ",
              prog, pc[0], "rb"); perror("fdopen");
      exit(1);
    }
    if ( ! (ofp = fdopen(cp[1], "wb"))) {
      fprintf(stderr, "%s: fpFork: can't open descriptor %d with mode `%s': ",
              prog, cp[1], "wb"); perror("fdopen");
      exit(1);
    }
    *rdFP = ifp;
    *wrFP = ofp;
    break;

  default:                      /* parent */
    close(cp[1]);
    close(pc[0]);
    if ( ! (ifp = fdopen(cp[0], "rb"))) {
      fprintf(stderr, "%s: fpFork: can't open descriptor %d with mode `%s': ",
              prog, cp[0], "rb"); perror("fdopen");
      close(cp[0]); close(pc[1]);
      return -1;
    }
    if ( ! (ofp = fdopen(pc[1], "wb"))) {
      fprintf(stderr, "%s: fpFork: can't open descriptor %d with mode `%s': ",
              prog, pc[1], "wb"); perror("fdopen");
      fclose(ifp); close(pc[1]);
      return -1;
    }
    *rdFP = ifp;
    *wrFP = ofp;
    break;
  }

  return pid;
}


/*  
 *  Look backward through the file until we find a newline, or hit the
 *  beginning.  Return the string following that point.
 */
void getCurrentString(char *buf, int n, FILE *fp, long *pos)
{
  char *nl;
  int c;
  
  while ((c = getc(fp)) != '\n') {
    if (fseek(fp, -2, SEEK_CUR) == -1) {
      rewind(fp); break;
    }
  }

  /* current character is first of the string */

  *pos = ftell(fp);

  buf[0] = '\0';                /* in case we're at the end of the file */
  fgets(buf, n, fp);
  if ((nl = strchr(buf, '\n'))) *nl = '\0';
}


void printStrings(FILE *textFP, const char *key, unsigned long nhits, unsigned long start[])
{
  char str[1024];
  int i, j;
  long pos;

  for (i = 0; i < nhits; i++) {
    ASSERT(fseek(textFP, (long)start[i], SEEK_SET) != -1);
    getCurrentString(str, sizeof str, textFP, &pos);
    if (strlen(str) > 0) {
      printf("%s\n", str);

      /*  
       *  Underline the substring with carets.
       */
      
      j = 0;
      while (j < start[i] - pos) {
        putchar(' '); j++;
      }
      while (j < start[i] - pos + strlen(key)) {
        putchar('^'); j++;
      }
      while (j < strlen(str)) {
        putchar(' '); j++;
      }
      putchar('\n');
    }
  }

  printf("# ----------------------------------------------------------------------\n");
}


void printStarts(FILE *fp, unsigned long nhits, unsigned long start[])
{
  unsigned long i;
        
  for (i = 0; i < nhits; i++) {
    printf("%ld\n", start[i]);
  }

  printf("# ----------------------------------------------------------------------\n");
}
