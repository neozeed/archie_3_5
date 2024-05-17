#ifndef _GINDEXDB_OPS_H_
#define _GINDEXDB_OPS_H_

#include "ansi_compat.h"

#define	GOPHERINDEX_DB_NAME	"gopherindex"


#ifndef DEFAULT_GSTRINGS_IDX
#define DEFAULT_GSTRINGS_IDX	"strings-idx"
#endif

#ifndef DEFAULT_GSTRINGS_LIST
#define DEFAULT_GSTRINGS_LIST	"strings-list"
#endif

#ifndef DEFAULT_GSTRINGS_HASH
#define DEFAULT_GSTRINGS_HASH	"strings-hash"
#endif

#ifndef DEFAULT_HOST_HASH
#define DEFAULT_HOST_HASH	"host-hash"
#endif

#ifndef DEFAULT_HOST_LIST
#define DEFAULT_HOST_LIST	"host-list"
#endif

#ifndef DEFAULT_SEL
#define DEFAULT_SEL		 "sel-list"
#endif


#ifndef DEFAULT_SEL_HASH
#define DEFAULT_SEL_HASH	 "sel-hash"
#endif

#define	 DEFAULT_GOPHER_PORT  70
#define	 DEFAULT_GOPHER_STRING	 "\r\n"
#define	 CRLF		      "\r\n"


extern	char		*set_gfiles_db_dir PROTO(( char *));
extern	char		*get_gfiles_db_dir PROTO((void));
extern	char		*gfiles_db_filename PROTO((char *));
extern	status_t	open_gfiles_db PROTO((file_info_t *, file_info_t *, file_info_t *, file_info_t *, file_info_t *, file_info_t *, file_info_t *, int));
extern	status_t	close_gfiles_db PROTO((file_info_t *, file_info_t *, file_info_t *, file_info_t *, file_info_t *, file_info_t *, file_info_t *));
extern	long		add_host_to_hash PROTO((char *,int *, file_info_t *,file_info_t *));

#endif
