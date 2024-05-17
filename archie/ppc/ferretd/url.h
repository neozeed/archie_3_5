#ifndef URL_H
#define URL_H

#include "ansi_compat.h"
#include "prosp.h"


enum urlType
{
  URL_FTP
};


extern VLINK url_to_vlink proto_((const char *url));
extern char *UrlFromVlink proto_((VLINK v, enum urlType utype));
extern char *url_dequote proto_((const char *s, void *(*allocfn)(size_t)));
extern char *url_quote proto_((const char *s, void *(*allocfn)(size_t)));
extern char *vlink_to_url proto_((VLINK v));

#endif
