#ifndef VLKINK_H
#define VLKINK_H

#include "ansi_compat.h"
#include "prosp.h"


extern VLINK file_in_vdir proto_((VLINK parent, const char *name));
extern VLINK get_req_child_vlink proto_((char *req));
extern VLINK get_req_vlink proto_((char *req));
extern VLINK get_root_menu proto_((void));
extern VLINK mkvlink proto_((const char *host, const char *hsoname, int tch));
extern int vcd proto_((const char *dir));

#endif
