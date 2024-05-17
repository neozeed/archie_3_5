#ifndef PROG_H_
#define PROG_H_

#include <stdio.h>
#include "prosp.h"
#include "ansi_compat.h"
#include "defs.h"


extern const char *prosp_ident proto_((void));
extern const char *service_str proto_((void));
extern void access_denied proto_((FILE *ifp, FILE *ofp, const Where us));
extern void handle_transaction proto_((FILE *ifp, FILE *ofp, const Where us, const Where them));
extern char *get_gopher_info proto_((VLINK v));

#endif
