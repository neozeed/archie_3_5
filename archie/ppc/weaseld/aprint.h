#ifndef APRINT_H
#define APRINT_H

#include <stdio.h>
#include "ppc.h"


#define MACHINE -100
#define TERSE   -101
#define VERBOSE -102


#if 0
extern char *wais_attr_name proto_((VLINK item));
extern char *wais_hdln proto_((VLINK item));
#endif
void anon_print proto_((FILE *ofp, VLINK v, int print_as_dir, const Where here));
void gopher_print proto_((FILE *ofp, VLINK v, int print_as_dir, const Where here));
void print_sites proto_((FILE *ofp, VLINK v, int print_as_dir, const Where here));
void print_wais proto_((FILE *ofp, VLINK v, int print_as_dir, const Where here));

#endif
