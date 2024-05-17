#ifndef RMEM_H
#define RMEM_H

#include "ansi_compat.h"
#include "misc_ansi_defs.h"


#define last_index(mem) last_index_((const char *)mem)
#define rappend(mem, data, type) \
  (type *)rappend_((char *)mem, (char *)data, sizeof (type))
#define rfree(mem) rfree_((const char *)mem)
#define rmalloc(n, type) (type *)rmalloc_(n, sizeof (type))


#ifdef DEBUG
#define MAGIC 0x66666666
#define set_magic(r) (r->m1 = r->m2 = r->m3 = r->m4 = MAGIC)
#define has_magic(r) (r->m1 == MAGIC && r->m2 == MAGIC && r->m3 == MAGIC && r->m4 == MAGIC)
#define try_magic(r) \
  do { if ( ! has_magic(r)) { fprintf(stderr, "Bad magic!\n"); abort(); } } while (0)

typedef struct
{
  unsigned long m1;
  char *mem;
  unsigned long m2;
  unsigned elts;
  unsigned long m3;
  unsigned index;
  unsigned long m4;
} Rmem;

#else

#define set_magic(r)
#define try_magic(r)

typedef struct
{
  char *mem;
  unsigned elts;
  unsigned index;
} Rmem;
#endif

extern int last_index_ PROTO((const char *m));
extern char *rappend_ PROTO((char *m, char *data, unsigned size));
extern char *rmalloc_ PROTO((unsigned nitems, unsigned size));
extern void rfree_ PROTO((const char *r));

#endif
