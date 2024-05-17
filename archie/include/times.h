/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _TIMES_H_
#define	_TIMES_H_
#include "ansi_compat.h"

#include "typedef.h"

extern	       time_t	      cvt_to_inttime PROTO(( char *, int));
extern	       char*	      cvt_from_inttime PROTO(( date_time_t ));
extern	       char*	      cvt_to_usertime PROTO((date_time_t, int));

#ifdef AIX
extern	       int	      timezone_sign();
#endif

#endif
