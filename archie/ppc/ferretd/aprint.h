#ifndef APRINT_H
#define APRINT_H

#include <stdio.h>
#include "ppc.h"
#include "request.h"


#define MACHINE -100
#define TERSE   -101
#define VERBOSE -102


#define PRT_PARMS proto_((FILE *ofp, Request *req, int print_as_dir))


extern int anon_print PRT_PARMS;
extern int gopher_print PRT_PARMS;
extern int sites_print PRT_PARMS;
extern int wais_print PRT_PARMS;

#endif
