#ifndef PPC_TCL_H
#define PPC_TCL_H

#include <stdio.h>
#include "prosp.h"


extern int tcl_headlines_print proto_((FILE *ofp, const char *dbname, const char *srch_str, int dir_fmt, VLINK v));
extern int tcl_init proto_((const char *startup));
extern int tcl_tagged_print proto_((FILE *ofp, PATTRIB cat));
extern void tcl_reinit_if_needed proto_((void));

#endif
