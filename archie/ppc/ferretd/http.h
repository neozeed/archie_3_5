#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>
#include "ppc.h"
#include "request.h"


extern void handle_request proto_((Request *req, FILE *ifp, FILE *ofp));
extern void http_get proto_((Request *req));
extern void http_menu proto_((Request *req, VLINK v));

#endif
