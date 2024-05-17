/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef	_DB_OPS_H_
#define	_DB_OPS_H_
#include "ansi_compat.h"

#define	ANONFTP_DB_NAME	"anonftp"
#define WEBINDEX_DB_NAME "webindex"

#define	ANONFTP_DEFAULT_PORT	"21"
#define	WEBINDEX_DEFAULT_PORT "80"

#ifndef SOLARIS
#define INIT_STRING_SIZE   getpagesize()
#else
#define INIT_STRING_SIZE   sysconf(_SC_PAGESIZE)
#endif

#ifdef OLD_FILES
#  include "old-host_db.h"
#else
#  include "host_db.h"
#endif

#define	 ANONFTP_LOCKFILE  "anonftp_lock"
#define	 WEBINDEX_LOCKFILE  "webindex_lock"

#ifndef DEFAULT_STRINGS_IDX
#define DEFAULT_STRINGS_IDX	"strings-idx"
#endif

#ifndef DEFAULT_STRINGS_LIST
#define DEFAULT_STRINGS_LIST	"strings-list"
#endif

#ifndef DEFAULT_STRINGS_HASH
#define DEFAULT_STRINGS_HASH	"strings-hash"
#endif

#ifndef DEFAULT_ARUPDATE_CONFIG
#define	DEFAULT_ARUPDATE_CONFIG	"arupdate.cf"
#endif

#ifndef DEFAULT_RETRIEVE_CONFIG
#define	DEFAULT_RETRIEVE_CONFIG	"arretrieve.cf"
#endif


extern	char		*set_files_db_dir PROTO(( char *));
extern	char		*get_files_db_dir PROTO((void));
extern	char		*files_db_filename PROTO((char *filename, int port));
extern	status_t	open_files_db PROTO((file_info_t *, file_info_t *, file_info_t *, int));
extern	status_t	close_files_db PROTO((file_info_t *, file_info_t *, file_info_t *));
extern	char		*unix_perms_itoa PROTO((int, int, int));



#endif
