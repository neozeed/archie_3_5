/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef	_HINFO_H_
#define	_HINFO_H_

#define	MAX_ACCESS_METHOD	128
typedef char access_methods_t[MAX_ACCESS_METHOD];


#define MAX_ACCESS_COMM		64
typedef char access_comm_t[MAX_ACCESS_COMM];


#define OS_TYPE_UNIX_BSD	"unix_bsd"
#define OS_TYPE_VMS_STD		"vms_std"
#define OS_TYPE_UNIX_AIX	"unix_aix"
#define OS_TYPE_NOVELL	        "novell"

typedef enum{UNIX_BSD=1,
	     VMS_STD=2,
	     UNIX_AIX=3,
	     NOVELL=4
} os_type_t;


#define GEN_PROG_PARSER		"parser"
#define	GEN_PROG_RETRIEVE	"retrieve"
#define	GEN_PROG_SERVER		"server"
#define	GEN_PROG_ADMIN		"admin"
#define	GEN_PROG_INSERT		"insert"
#define	GEN_PROG_CONTROL	"control"

typedef	enum{PARSER=1,
	     RETRIEVE=2,
	     SERVER=3,
	     ADMIN=4,
	     INSERT=5,
	     CONTROL=6
	     
	     
} gen_prog_t;



#define CURRENT_STATUS_ACTIVE		"active"
#define CURRENT_STATUS_NOT_SUPPORTED	"not_supported"
#define CURRENT_STATUS_DEL_BY_ADMIN	"del_by_admin"
#define CURRENT_STATUS_DEL_BY_ARCHIE	"del_by_archie"
#define CURRENT_STATUS_INACTIVE		"inactive"
#define	CURRENT_STATUS_DELETED	        "deleted"
#define	CURRENT_STATUS_DISABLED	        "disabled"

typedef enum{ACTIVE=1,		/* On active update			*/
	     NOT_SUPPORTED=2,	/* Can't parse OS listings		*/
	     DEL_BY_ADMIN=3,	/* Deleted by request of site admin	*/
	     DEL_BY_ARCHIE=4,	/* Host dead/can't connect etc.		*/
	     INACTIVE=5,	/* Temporarily out of order		*/
	     DELETED=6,		/* Data files removed			*/
	     DISABLED=7		/* Disabled by administrator */
} current_status_t;



#define	ACTION_STATUS_NEW	"new"
#define	ACTION_STATUS_UPDATE	"update"
#define	ACTION_STATUS_DELETE	"delete"

typedef enum{NEW = 1,		/* New record */
	     UPDATE=2,		/* Update existing record */
	     DELETE=3		/* Delete appropriate record */
} action_status_t;


#define	UPDATE_STATUS_SUCCEED	"succeed"
#define	UPDATE_STATUS_FAIL	"fail"

typedef enum{FAIL = 1,
	     SUCCEED = 2
} update_status_t;


#define	FORMAT_RAW		"raw"
#define	FORMAT_FXDR		"fxdr"
#define	FORMAT_COMPRESS_LZ	"compress_lz"
#define	FORMAT_COMPRESS_GZIP	"compress_gzip"
#define	FORMAT_XDR_COMPRESS_LZ	"xdr_compress_lz"
#define	FORMAT_XDR_GZIP		"xdr_compress_gzip"

typedef enum{FRAW = 1,
	     FXDR = 2,
	     FCOMPRESS_LZ = 3,
	     FXDR_COMPRESS_LZ = 4,
	     FXDR_GZIP = 5,
       FCOMPRESS_GZIP = 6
} format_t;

typedef enum{NO = 1,
	     YES = 2
}state_t;

#define	PROSPERO_YES		"yes"
#define	PROSPERO_NO		"no"

typedef u32 header_flags_t;



#endif
