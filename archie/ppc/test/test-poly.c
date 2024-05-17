#include <stdio.h>
#include <stdlib.h>
#include "polygon.h"
#include "all.h"


#define FARX 100.0
#define FARY 100.0


static char *x_getline proto_((FILE *ifp));


#define INCSZ 1024
#define INITSZ 256

/*  
 *  Read a line of text from `ifp', allocating enough space to hold it.
 *  
 *  Start with an initial buffer of size INITSZ, increasing it by INCSZ as
 *  necessary.
 *
 */    

static char *x_getline(ifp)
  FILE *ifp;
{
  char *mem;
  char *p;
  int avail;
  int totsz = INITSZ;

  p = mem = malloc(totsz);
  if ( ! mem)
  {
    efprintf(stderr, "%s: getline: can't malloc() %d bytes: %m.\n",
             prog, totsz);
    return 0;
  }

  mem[0] = '\0';
  avail = totsz;

 read:
  p[avail-2] = '\0';
  if ( ! fgets(p, avail, ifp))
  {
    /* EOF or error */
    
    if ( ! ferror(ifp))
    {
      if (mem[0] != '\0') /* non-empty buffer */
      {
        return mem;
      }
      else /* immediate EOF; nothing was read */
      {
        free(mem);
        return 0;
      }
    }
    else
    {
      efprintf(stderr, "%s: getline: error from fgets(): %m.\n", prog);
      free(mem);
      return 0;
    }
  }

  /* Line read in or buffer too small */

  if (p[avail-2] == '\0' /* read entire line: space left over */ ||
      p[avail-2] == '\n' /* read entire line: buffer filled   */ )
  {
    return mem;
  }
  else /* read partial line: buffer too small */
  {
    char *t;

    t = realloc(mem, totsz + INCSZ);
    if ( ! t)
    {
      efprintf(stderr, "%s: getline: can't realloc() %d bytes: %m.\n",
               prog, totsz + INCSZ);
      free(mem);
      return 0;
    }

    mem = t;
    p = mem + totsz - 1;
    totsz += INCSZ;
    avail = INCSZ + 1;
    goto read;
  }
}


const char *prog;
int debug;


int main(ac, av)
  int ac;
  char **av;
{
  char *line;
  char in[2] = { 'n', 'y' };
  char res;
  const char pstr[2048];
  double farx = FARX, fary = FARY;
  double x, y;
  
  prog = av[0];
  if (scanf("%lf,%lf\n", &farx, &fary) != 2)
  {
    fprintf(stderr, "%s: first line of input doesn't contain `far' point.\n", prog);
    exit(1);
  }

  while ((line = x_getline(stdin)))
  {
    if (sscanf(line, "%lf,%lf %c %[^\n]", &x, &y, &res, pstr) != 4)
    {
      fprintf(stderr, "%s: bad line `%s'.\n", prog, line);
    }
    else if (res != 'y' && res != 'n')
    {
      fprintf(stderr, "%s: bad result character `%c'.\n", prog, res);
    }
    else if (in[in_polygon(x, y, farx, fary, pstr)] == res)
    {
      printf("Okay\n");
    }
    else
    {
      printf("*** FAIL: test point (%f, %f), polygon `%s'.\n",
             x, y, pstr);
    }

    free(line);
  }
  exit(0);
}
