#ifndef EPATH_H
#define EPATH_H

#include "ansi_compat.h"
#include "prosp.h"


extern VLINK epath_to_vlink proto_((const char *epath));
extern char *vlink_to_epath proto_((VLINK v));

#endif
