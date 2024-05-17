#include "ptrval.h"
#include "all.h"


const void *ptr(v, pv)
  int v;
  const PtrVal *pv;
{
  while (pv->ptr)
  {
    if (pv->val == v) return pv->ptr;
    pv++;
  }
  return pv->ptr;
}


int val(p, pv, cmp)
  const void *p;
  const PtrVal *pv;
  int (*cmp) proto_((const void *a, const void *b));
{
  while (pv->ptr)
  {
    if (cmp(p, pv->ptr) == 0) return pv->val;
    pv++;
  }
  return pv->val;
}
