#ifndef _WINDEXDB_OPS_H_
#define _WINDEXDB_OPS_H_

#include "ansi_compat.h"

#define	WEBINDEX_DB_NAME	"webindex"

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

#define	 DEFAULT_WEB_PORT  80
#define	 DEFAULT_WEB_STRING	 "/"
#define	 CRLF		      "\r\n"


extern	char		*set_wfiles_db_dir PROTO(( char *));
extern	char		*get_wfiles_db_dir PROTO(( void ));
extern	char		*wfiles_db_filename PROTO(( char *, int ));
extern	status_t	open_wfiles_db PROTO(( file_info_t *, file_info_t *, file_info_t *, file_info_t *, int));
extern	status_t	close_wfiles_db PROTO(( file_info_t *, file_info_t *, file_info_t *, file_info_t *));
extern	long		add_host_to_hash PROTO((char *,int *, file_info_t *,file_info_t *));

#endif
