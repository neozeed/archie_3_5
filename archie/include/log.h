/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _ALOG_H_
#define _ALOG_H_
#include "ansi_compat.h"

extern	status_t	open_alog PROTO(( char *, log_level_t ));
extern	void		alog PROTO(());

#endif
