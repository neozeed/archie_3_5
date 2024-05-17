/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _ARCHIE_XDR_H_
#define _ARCHIE_XDR_H_

#include "ansi_compat.h"

extern	  XDR*		open_xdr_stream PROTO(( FILE *, int));
extern	  void		close_xdr_stream PROTO(( XDR *));

#endif
