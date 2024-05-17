/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _ARCHIE_DBM_H_
#define _ARCHIE_DBM_H_

#include "typedef.h"
#include "ansi_compat.h"

extern		status_t	get_dbm_entry PROTO(( void *, int, void *, file_info_t *));
extern		status_t	put_dbm_entry PROTO(( void *, int, void *, int, file_info_t *, int));
extern	        status_t	delete_dbm_entry PROTO((void *, int, file_info_t *));

#endif
