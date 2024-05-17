#ifndef FILE_TYPE_H
#define FILE_TYPE_H

#include "ppc.h"


extern int match_content_type proto_((const char **ctype, const char **stype, const char **enc));
extern void clear_acceptance proto_((void));
extern void client_accepts proto_((const char *type));
extern void content_type_of proto_((VLINK v, const char **ctype, const char **stype, const char **enc));

#endif
