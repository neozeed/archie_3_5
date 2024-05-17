#ifndef URL_QUOTE_H
#define URL_QUOTE_H

#if defined(AIX)
#include "ppc.h"
#else
#include "ansi_compat.h"
#endif

extern char *dequote_string proto_((const char *s, const unsigned char *safech, void *(*allocfn)(size_t)));
extern char *quote_string proto_((const char *s, const unsigned char *safech, void *(*allocfn)(size_t)));

#endif
