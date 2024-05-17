/* definition of subheader */



#ifndef _SUB_HEADER_H_
#define _SUB_HEADER_H_
#include "ansi_compat.h"
#include "web.h"
#include "hinfo.h"

typedef char	a_sub_header_field_t[MAX_ASCII_HEADER_LEN];

#define SUB_HEADER_END			"@sub_header_end"
#define SUB_HEADER_END_I		-1

#define	SUB_HEADER_BEGIN		"@sub_header_begin"
#define	SUB_HEADER_BEGIN_I		0

#define LOCAL_URL_ENTRY		"local_url"
#define LOCAL_URL_ENTRY_I		1

#define STATE_ENTRY		    "state"
#define STATE_ENTRY_I		  2

#define SIZE_ENTRY		    "size"
#define SIZE_ENTRY_I		  3
					
#define PORT_ENTRY		    "port"
#define PORT_ENTRY_I			4

#define SERVER_ENTRY		  "server"
#define SERVER_ENTRY_I		5

#define TYPE_ENTRY		    "type"
#define TYPE_ENTRY_I		  6

#define RECNO_ENTRY		    "recno"
#define RECNO_ENTRY_I		  7

#define DATE_ENTRY		    "date"
#define DATE_ENTRY_I		  8

#define FORMAT_ENTRY      "format"
#define FORMAT_ENTRY_I    9

#define FSIZE_ENTRY		    "file_size"
#define FSIZE_ENTRY_I		  10

typedef struct{
   hostname_t		local_url; 
   hostname_t		state;
   hostname_t		server;
   hostname_t   type;
   file_size_t	size;
   file_size_t	fsize;
   index_t		recno;
   index_t		port;
   date_time_t date;
   format_t format;
   header_flags_t	sub_header_flags;		/* Bitmap of header entries */
} sub_header_t;

#define	HDR_LOCAL_URL_ENTRY			0x1
#define	HDR_SET_LOCAL_URL_ENTRY(x)		((x) |= HDR_LOCAL_URL_ENTRY)
#define	HDR_UNSET_LOCAL_URL_ENTRY(x)		((x) &= ~HDR_LOCAL_URL_ENTRY)
#define	HDR_GET_LOCAL_URL_ENTRY(x)		((x) & HDR_LOCAL_URL_ENTRY)

#define	HDR_STATE_ENTRY			0x2
#define	HDR_SET_STATE_ENTRY(x)		((x) |= HDR_STATE_ENTRY)
#define	HDR_UNSET_STATE_ENTRY(x)	((x) &= ~HDR_STATE_ENTRY)
#define	HDR_GET_STATE_ENTRY(x)		((x) & HDR_STATE_ENTRY)

#define	HDR_SIZE_ENTRY			0x4
#define	HDR_SET_SIZE_ENTRY(x)		((x) |= HDR_SIZE_ENTRY)
#define	HDR_UNSET_SIZE_ENTRY(x)		((x) &= ~HDR_SIZE_ENTRY)
#define	HDR_GET_SIZE_ENTRY(x)		((x) & HDR_SIZE_ENTRY)

#define	HDR_PORT_ENTRY			0x8
#define	HDR_SET_PORT_ENTRY(x)		((x) |= HDR_PORT_ENTRY)
#define	HDR_UNSET_PORT_ENTRY(x)		((x) &= ~HDR_PORT_ENTRY)
#define	HDR_GET_PORT_ENTRY(x)		((x) & HDR_PORT_ENTRY)

#define	HDR_SERVER_ENTRY			0x10
#define	HDR_SET_SERVER_ENTRY(x)		((x) |= HDR_SERVER_ENTRY)
#define	HDR_UNSET_SERVER_ENTRY(x)		((x) &= ~HDR_SERVER_ENTRY)
#define	HDR_GET_SERVER_ENTRY(x)		((x) & HDR_SERVER_ENTRY)

#define	HDR_TYPE_ENTRY			0x20
#define	HDR_SET_TYPE_ENTRY(x)		((x) |= HDR_TYPE_ENTRY)
#define	HDR_UNSET_TYPE_ENTRY(x)		((x) &= ~HDR_TYPE_ENTRY)
#define	HDR_GET_TYPE_ENTRY(x)		((x) & HDR_TYPE_ENTRY)


#define	HDR_RECNO_ENTRY			0x40
#define	HDR_SET_RECNO_ENTRY(x)		((x) |= HDR_RECNO_ENTRY)
#define	HDR_UNSET_RECNO_ENTRY(x)		((x) &= ~HDR_RECNO_ENTRY)
#define	HDR_GET_RECNO_ENTRY(x)		((x) & HDR_RECNO_ENTRY)

#define	HDR_DATE_ENTRY			0x80
#define	HDR_SET_DATE_ENTRY(x)		((x) |= HDR_DATE_ENTRY)
#define	HDR_UNSET_DATE_ENTRY(x)		((x) &= ~HDR_DATE_ENTRY)
#define	HDR_GET_DATE_ENTRY(x)		((x) & HDR_DATE_ENTRY)

#define	HDR_FORMAT_ENTRY			0x100
#define	HDR_SET_FORMAT_ENTRY(x)		((x) |= HDR_FORMAT_ENTRY)
#define	HDR_UNSET_FORMAT_ENTRY(x)		((x) &= ~HDR_FORMAT_ENTRY)
#define	HDR_GET_FORMAT_ENTRY(x)		((x) & HDR_FORMAT_ENTRY)

#define	HDR_FSIZE_ENTRY			0x200
#define	HDR_SET_FSIZE_ENTRY(x)		((x) |= HDR_FSIZE_ENTRY)
#define	HDR_UNSET_FSIZE_ENTRY(x)		((x) &= ~HDR_FSIZE_ENTRY)
#define	HDR_GET_FSIZE_ENTRY(x)		((x) & HDR_FSIZE_ENTRY)


extern void	init_sub_header PROTO((void));
extern status_t read_sub_header PROTO((FILE *, sub_header_t *, u32 *, int, int));
extern status_t write_sub_header PROTO((FILE *, sub_header_t *, u32 *, int, int));

#endif

