#ifndef AMISC_H
#define AMISC_H

#include "ansi_compat.h"
#include "defs.h"
#include "prosp.h"


extern char *head proto_((const char *path, char *buf, int bufsize));
extern char *logpfx proto_((void));
extern char *now proto_((void));
extern char *nuke_afix_ws proto_((char *s));
extern int getHostPort proto_((const char *hp, char *h, char *p));
extern int immediatePrint proto_((VLINK v, PATTRIB *cat));
extern int there proto_((Where *w, const char *ipstr));
extern int where proto_((Where *w, const char *service));
extern void fprint_argv proto_((FILE *fp, char **av));

#endif
