/*  
 *  A brute force sistring sorter.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define ARG_IS(str)  (strcmp(av[0], str) == 0)
#define NEXTDBL(dbl) do{if(av++,--ac<= 0||sscanf(av[0],"%lf",&dbl)!=1)usage();}while(0)
#define NEXTINT(i)   do{if(av++,--ac<=0||sscanf(av[0],"%d",&i)!=1)usage();}while(0)
#define NEXTSTR(str) do{if(av++,--ac<=0)usage();else str=av[0];}while(0)


char *prog;
/*  
 *  Element `i' holds the position of the first set bit in the number `i'.
 *  Bit positions start at 0, which is the most significant bit.
 */
unsigned int diffTab[256];


void initDiffTab(void)
{
  int i;
  
  for (i = 0; i < 256; i++) {
    if      ((i >> 1) == 0) diffTab[i] = 7;
    else if ((i >> 2) == 0) diffTab[i] = 6;
    else if ((i >> 3) == 0) diffTab[i] = 5;
    else if ((i >> 4) == 0) diffTab[i] = 4;
    else if ((i >> 5) == 0) diffTab[i] = 3;
    else if ((i >> 6) == 0) diffTab[i] = 2;
    else if ((i >> 7) == 0) diffTab[i] = 1;
    else if ((i >> 8) == 0) diffTab[i] = 0;
  }
}


/*  
 *  Return the first bit position at which `s0' differs from `s1'.  Bit
 *  positions start at 0, which is the most significant bit of a byte.
 *  
 *  We assume that the two sistrings _are_ different.
 */
int diffBit(const char *s0, const char *s1)
{
  int diff, n;

  for (n = 0; *s0 == *s1; s0++, s1++) {
    n++;
  }

  diff = n * 8 + diffTab[(unsigned char)(*s0 ^ *s1)];
  return diff;
}


void usage(void)
{
  fprintf(stderr, "Usage: %s [-help] [-unique] <text-file> <sorted-file> <infix-trie>\n", prog);
  fprintf(stderr, "\n");
  fprintf(stderr, "\t-help\t# print this message\n");
  fprintf(stderr, "\t-unique\t# number of bytes of text stored with the starts\n");
  fprintf(stderr, "\n");
  exit(1);
}


int main(int ac, char **av)
{
  FILE *sfp;                    /* input: starts of sorted sistrings */
  FILE *tfp;                    /* output: infix trie */
  char *text;
  int uniqueLen = 10;
  long val;
  long buf0[128], buf1[128];
  long *off0 = buf0, *off1 = buf1;
  size_t reclen;
  
  prog = av[0];
  
  while (av++, --ac) {
    if      (ARG_IS("-help"))   usage();
    else if (ARG_IS("-unique")) NEXTINT(uniqueLen);
    else                        break;
  }

  if (ac != 3) usage();

  reclen = sizeof(long) + uniqueLen;
  if (reclen > sizeof buf0) {
    fprintf(stderr, "%s: input records too long!  Recompile.\n", prog);
    exit(1);
  }

  if ( ! (sfp = fopen(av[1], "rb"))) {
    fprintf(stderr, "%s: can't open sorted file `%s': ", prog, av[1]); perror("fopen");
    exit(1);
  }

  if ( ! (tfp = fopen(av[2], "wb"))) {
    fprintf(stderr, "%s: can't open infix trie file `%s': ", prog, av[2]); perror("fopen");
    fclose(sfp);
    exit(1);
  }

  /*  
   *  Load the text file into memory.
   */
  
  {
    FILE *fp;
    struct stat sbuf;
    
    if ( ! (fp = fopen(av[0], "r"))) {
      fprintf(stderr, "%s: can't open `%s': ", prog, av[0]); perror("fopen");
      fclose(sfp); fclose(tfp);
      exit(1);
    }

    if (fstat(fileno(fp), &sbuf) == -1) {
      fprintf(stderr, "%s: can't stat `%s': ", prog, av[0]); perror("fstat");
      fclose(sfp); fclose(tfp); fclose(fp);
      exit(1);
    }
    
    if ( ! (text = malloc(sbuf.st_size + 1))) {
      fprintf(stderr, "%s: malloc() failed to allocate %lu bytes: ",
              prog, (unsigned long)(sbuf.st_size + 1)); perror("malloc");
      fclose(sfp); fclose(tfp); fclose(fp);
      exit(1);
    }

    if (fread(text, 1, sbuf.st_size, fp) != sbuf.st_size) {
      fprintf(stderr, "%s: error reading %lu bytes: ", prog, (unsigned long)sbuf.st_size);
      perror("fread"); fclose(sfp); fclose(tfp); fclose(fp);
      exit(1);
    }

    fclose(fp);

    text[sbuf.st_size] = '\0';
  }
  
  /*  
   *  For each pair of input records print out their starts and the first bit
   *  position in which they differ.
   *  
   *  Bit positions start at 0, which is the most significant bit of a byte.
   */

  initDiffTab();
  
  val = 0;
  fwrite(&val, sizeof val, 1, tfp);
  fread(off0, reclen, 1, sfp);
  while (fread(off1, reclen, 1, sfp)) {
    long *tmp;
    
    val = -(*off0 + 1);
    fwrite(&val, sizeof val, 1, tfp);
    val = diffBit(text + *off0, text + *off1);
    fwrite(&val, sizeof val, 1, tfp);

    tmp = off0; off0 = off1; off1 = tmp;
  }
  val = -(*off0 + 1);
  fwrite(&val, sizeof val, 1, tfp);
  val = 0;
  fwrite(&val, sizeof val, 1, tfp);
  
  free(text);
  fclose(sfp); fclose(tfp);
  
  exit(0);
}
