#include <stdio.h>
#include "defs.h"
#include "error.h"
#include "prosp.h"
#include "parchie.h"
#include "protos.h"
#include "srch.h"
#include "all.h"


#define MAXHITS 100


VLINK archie_search(ahost, sreq)
  const char *ahost;
  sReq *sreq;
{
  struct aquery arq;

  aq_init(&arq);
  arq.string = sreq->srchs;
  arq.host = (char *)ahost;
  arq.maxhits = MAXHITS;
  arq.offset = 0;
  arq.query_type = AQ_CI_SUBSTR;

  switch (sreq->type)
  {
  case T_ANON:
    arq.cmp_proc = aq_defcmplink;
    break;

  case T_WAIS:
    arq.dbs = sreq->dbs;
    arq.flags |= AQ_NOSORT;
    arq.query_type = AQ_GENERIC;
    arq.maxmatch = arq.maxhitpm = arq.maxhits; /* bug? */
    break;

  default:
    break;
  }

  if (aq_query(&arq, arq.flags) > 0)
  {
    perrmesg((char *)0, 0, (char *)0);
    return (VLINK)0;
  }

  return arq.results;
}
