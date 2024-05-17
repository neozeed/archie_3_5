#include <string.h>
#include "strval.h"
#include "all.h"


const char *str(v, pv)
  int v;
  const PtrVal *pv;
{
  return (const char *)ptr(v, pv);
}


int sval(p, pv)
  const char *p;
  const PtrVal *pv;
{
  return val((const void *)p, pv,
             (int (*) proto_((const void *a, const void *b)))strcmp);
}
