#ifndef SRCH_H
#define SRCH_H

#include "prosp.h"


#define T_ANON -1000
#define T_WAIS -1001


typedef struct
{
  int type;
  char *dbs;
  char *srchs;
} sReq;


extern VLINK archie_search proto_((const char *ahost, sReq *sreq));

#endif
