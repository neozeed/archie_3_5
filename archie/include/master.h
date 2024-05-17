/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _MASTER_H_
#define _MASTER_H_
#include "ansi_compat.h"

extern		char *		set_master_db_dir PROTO((char *));
extern		char *		get_master_db_dir PROTO((void));
extern		char *		get_archie_home PROTO((void));
extern	        char *		get_archie_hostname PROTO((char *, int));
extern		char *		master_db_filename PROTO((char *));

#endif
