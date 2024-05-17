#ifndef CONTENTS_H
#define CONTENTS_H

#include "ppc.h"


extern PATTRIB sort_tag_attribs proto_((PATTRIB pat));
extern int gopherLinkTo proto_((const char *atval, char **type, char **epath, char **desc));
extern int gopherTypeOf proto_((const char *type));
extern int print_tagged_item proto_((FILE *ofp, const char *tag, const char *val));

#endif
