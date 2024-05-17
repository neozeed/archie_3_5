#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>


static jmp_buf env;


#define INIT       unsigned char *p = instring;
#define GETC()     (*p++)
#define PEEKC()    (*p)
#define UNGETC(c)  (--p)
#define RETURN(c)  return (c);
#define ERROR(c)   longjmp(env, (c))

#include <regexp.h>


static char *prog;


static void regerr(char *re, int err)
{
  fprintf(stderr, "%s: regerr: `%s' caused error %d.\n", prog, re, err);
}


#if 1

#define TSZ (64 * 1024)


struct inbuf {
  char txt[TSZ];
  size_t boff;                  /* offset of first byte in buffer */
  size_t noff;                  /* offset of next byte to be read */
  char *end;                    /* pointer to last valid byte, plus one */
  char *nl;                     /* pointer to last newline in buffer */
};


void initBuf(struct inbuf *b)
{
  b->end = b->nl = b->txt + sizeof b->txt;
  b->boff = b->noff = 0;
}


int fillBuf(int fd, struct inbuf *b)
{
  char *dst = b->txt;
  char *src = b->nl + 1;
  int n;
  
  /*
   *  If there are any trailing bytes, from the previous call, shift them to
   *  the front of the buffer.
   */
  
  while (src < b->end) {
    *dst++ = *src++;
  }
    
  if ((n = read(fd, dst, b->end - dst)) < 1) {
    if (n == -1) {
      fprintf(stderr, "%s: fillBuf: error filling text buffer: ",
              prog); perror("read");
    }
    return 0;
  }

  b->end = dst + n;

  b->boff = b->noff;
  b->noff += n;

  /*
   *  Set b->nl to point to the last newline in the buffer.  We don't want to
   *  try to match the regular expression against the last string, as it
   *  probably is not complete.
   *  
   *  Put a sentinel at b->txt[0] to ensure we don't run past the beginning of
   *  the buffer.
   */

  {
    char c = b->txt[0];

    b->txt[0] = '\n';
    for (b->nl = b->end - 1; *b->nl != '\n'; b->nl--) {
      continue;
    }
    if (b->nl == b->txt && c != '\n') {
      fprintf(stderr, "%s: fillBuf: no newline in buffer!\n", prog);
      abort();
    }
    b->txt[0] = c;
  }

  return 1;
}


static void searchRE(char *re, FILE *fp)
{
  int err;
  
  if ((err = setjmp(env)) != 0) {
    regerr(re, err);
  } else {
    char cre[512];
    int fd = fileno(fp);
    struct inbuf b;

    rewind(fp);

    compile(re, cre, cre + sizeof cre, '\0');

    initBuf(&b);
    while (fillBuf(fd, &b)) {
      char *p = b.txt;
      
      while (p < b.nl) {
        char *nl = p;

        while (*nl != '\n') {
          nl++;
        }
        *nl = '\0';
        
        if (step(p, cre)) {
          printf("\t%lu\t%s\n", (unsigned long)(b.boff + (p - b.txt)), p);
        }

        p = nl + 1;
      }
    }
  }
}

#else

static void searchRE(char *re, FILE *fp)
{
  int err;
  
  if ((err = setjmp(env)) != 0) {
    regerr(re, err);
  } else {
    char cre[512], line[512];
    
    compile(re, cre, cre + sizeof cre, '\0');

    rewind(fp);
    line[sizeof line - 2] = '\0';

    while (fgets(line, sizeof line, fp)) {
      if (line[sizeof line - 2] != '\0') {
        fprintf(stderr, "%s: searchRE: line too long -- skipping it.\n", prog);
      } else if (step(line, cre)) {
        size_t off = ftell(fp) - strlen(line);
        printf("\t%7lu\t%s", (unsigned long)off, line);
      }

      line[sizeof line - 2] = '\0';
    }
  }
}
#endif


static void usage(void)
{
  fprintf(stderr, "Usage: %s <text-file>\n", prog);
  exit(1);
}


int main(int ac, char **av)
{
  FILE *fp;
  char re[128];
  
  prog = av[0];
  
  if (ac != 2) usage();

  if ( ! (fp = fopen(av[1], "r"))) {
    fprintf(stderr, "%s: can't open `%s' for reading: ",
            prog, av[1]); perror("fopen");
    exit(1);
  }

  printf("Key: ");
  while (fgets(re, sizeof re, stdin)) {
    char *nl = strchr(re, '\n');
    if (nl) *nl = '\0';

    if (re[0] != '\0') {
      searchRE(re, fp);
    }

    printf("Key: ");
  }

  exit(0);
}
