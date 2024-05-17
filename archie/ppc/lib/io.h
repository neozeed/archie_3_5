#ifndef IO_H
#define IO_H

#include <stdio.h>
#include "ansi_compat.h"
#include "prosp.h"


extern char *memfgets proto_((FILE *ifp));
extern char *readline proto_((FILE *ifp, int debug));
extern char *timed_readline proto_((FILE *ofp, int debug));
extern int blat_file proto_((FILE *ofp, VLINK v));
extern int fcopy proto_((FILE *ifp, FILE *ofp, int text));
extern int fcopysize proto_((FILE *ifp, FILE *ofp, int size));
extern int file_is_local proto_((VLINK v));
extern int fpsock proto_((int s, FILE **ifp, FILE **ofp));
extern int timed_fread proto_((char *ptr, int size, int nitems, FILE *stream));
extern int timed_getc proto_((FILE *ifp));
extern long contents_size proto_((VLINK v));
extern long local_file_size proto_((VLINK v));

#endif
