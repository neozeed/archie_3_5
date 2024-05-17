#ifndef PRINT_RESULTS_H
#define PRINT_RESULTS_H

#include <stdio.h>
#include "ppc.h"
#include "request.h"


extern void print_results proto_((FILE *ofp, Request *req));
extern void print_vllength proto_((FILE *ofp, Request *req));

#endif
