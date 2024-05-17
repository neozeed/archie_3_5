#ifndef PTRVAL_H
#define PTRVAL_H

#include "ansi_compat.h"


#define PTRVAL_END (void *)0


typedef struct
{
  void *ptr;
  int val;
} PtrVal;


extern int val proto_((const void *s, const PtrVal *pv, int (*cmp)(const void *a, const void *b)));
extern const void *ptr proto_((int v, const PtrVal *pv));

#endif
