#include <malloc.h>
#ifdef AIX
#include <stdlib.h>
#endif
#include "lang.h"
#include "rmem.h"

#include "protos.h"
#ifdef DEBUG
static char *foo;
#endif
#ifdef STATS
long rmem_allocated = 0;
long rmem_freed = 0;
#endif


#define RMEM(m) ((Rmem *)(m) - 1)
#define CRMEM(m) ((const Rmem *)(m) - 1)


int last_index_(m)
  const char *m;
{
  const Rmem *r = CRMEM(m);

  try_magic(r);
  return r->index - 1;
}


char *rappend_(m, data, size)
  char *m;
  char *data;
  unsigned size;
{
  Rmem *r = RMEM(m);
  char *ret;

  try_magic(r);
  if (r->index < r->elts)
  {
    memcpy(r->mem + r->index++ * size, data, (unsigned)size);
    try_magic(r);
    ret = r->mem;
  }
  else
  {
    int new_elts = 2 * (r->elts ? r->elts : 1) ;
    Rmem *new_r = (Rmem *)realloc((char *)r, sizeof (Rmem) + new_elts * size);

    if ( ! new_r)
    {
      ret = (char *)0;
    }
    else
    {
      try_magic(new_r);
#ifdef STATS
      rmem_allocated += new_elts - r->elts;
#endif
      new_r->mem = (char *)(new_r + 1);
      memcpy(new_r->mem + new_r->index++ * size, data, (unsigned)size);
      new_r->elts = new_elts;
      try_magic(new_r);
      ret = new_r->mem;
    }
  }

  return ret;
}


void rfree_(m)
  const char *m;
{
  const Rmem *r = CRMEM(m);

  try_magic(r);
#ifdef STATS
  rmem_freed += r->elts;
#endif
#ifdef DEBUG
  fprintf(stderr, curr_lang[237], (char *)r);
#endif
  free(r);
}


char *rmalloc_(nitems, size)
  unsigned nitems;
  unsigned size;
{
  Rmem *r;
  char *ret;

  if ( ! (r = (Rmem *)malloc(sizeof (Rmem) + nitems * size)))
  {
    ret = (char *)0;
  }
  else
  {
#ifdef DEBUG
    fprintf(stderr, curr_lang[238], (char *)r);
    if ((char *)r == foo)
    {
      fprintf(stderr, curr_lang[239], foo);
      abort();
    }
    else
    {
      foo = (char *)r;
      fprintf(stderr, curr_lang[240], foo);
    }
#endif
#ifdef STATS
    rmem_allocated += nitems;
#endif
    r->elts = nitems;
    r->index = 0;
    r->mem = (char *)(r + 1);
    set_magic(r);
    ret = r->mem;
  }

  return ret;
}
