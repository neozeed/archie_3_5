#ifndef OS_INDEP_H
#define OS_INDEP_H

#include "ansi_compat.h"


typedef void Sigfunc proto_((int));


extern Sigfunc *ppc_signal proto_((int sig, Sigfunc *fn));
extern int max_open_files proto_((void));

#endif
