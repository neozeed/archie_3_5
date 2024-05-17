/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef SRCH_H
#define SRCH_H
#include "ansi_compat.h"

#include "ar_search.h"

#ifndef	CHARTYPE
#define	CHARTYPE	unsigned char
#endif
#define	MAXPAT	256

#define	TABTYPE	unsigned char
#ifndef	TABTYPE
#define	TABTYPE	long
#endif
typedef TABTYPE Tab;

extern int ci_exec PROTO((CHARTYPE *base, int n, search_result_t *sres, int max_hits) );
extern int cs_exec PROTO((CHARTYPE *base, int n, search_result_t *sres, int max_hits) );

#endif
