#ifndef PSEARCH_H
#define PSEARCH_H

#include <stdio.h>
#include "ansi_compat.h"
#include "local_attrs.h"
#include "prosp.h"


enum SearchType
{
  SRCH_,
  SRCH_ANONFTP,
  SRCH_DOMAINS,
  SRCH_ERROR,                   /* special: if we can't determine a search type */
  SRCH_GOPHER,
  SRCH_SITELIST,
  SRCH_SITES,
  SRCH_WAIS,
  SRCH_WHATIS,
  SRCH_UNKNOWN                  /* special: ? */
};


extern VLINK search proto_((VLINK v, const char *srch, char **dbname, enum SearchType *stype));
extern const char *nth_token_str proto_((TOKEN t, int n));
extern int display_as_dir proto_((VLINK v));

#endif
