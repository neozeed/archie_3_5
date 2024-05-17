#ifndef STRVAL_H
#define STRVAL_H

#include "ansi_compat.h"
#include "ptrval.h"


#define STRVAL_END PTRVAL_END
#define StrVal PtrVal


extern int sval proto_((const char *s, const PtrVal *pv));
extern const char *str proto_((int v, const PtrVal *pv));

#endif
