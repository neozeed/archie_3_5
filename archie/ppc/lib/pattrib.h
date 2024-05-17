#ifndef PATTRIB_H
#define PATTRIB_H

#include "ansi_compat.h"
#include "prosp.h"


extern char *nextAttrStr proto_((const char *name, PATTRIB attr));
extern PATTRIB get_contents proto_((VLINK v));
extern PATTRIB get_contents_from_server proto_((VLINK v));
extern PATTRIB nextAttr proto_((const char *name, PATTRIB at));
extern PATTRIB vlinkAttr proto_((const char *name, VLINK v));
extern TOKEN appendAttrTokens proto_((PATTRIB at, const char *name));
extern char *vlinkAttrStr proto_((const char *name, VLINK v));
#ifdef DEBUG
extern void aPrint proto_((PATTRIB at));
#endif
#endif
