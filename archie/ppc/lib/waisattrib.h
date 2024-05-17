#ifndef WAISATTRIB_H
#define WAISATTRIB_H

#include "ansi_compat.h"
#include "prosp.h"


extern PATTRIB nextAttrWithTag proto_((const char *tagname, PATTRIB cat));
extern const char *getTagValue proto_((const char *name, PATTRIB pat));
extern int link_to proto_((const char *val, const char *prec[], char **contype, char **encpath, char **descrip));

#endif
