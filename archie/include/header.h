
/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _HEADER_H_
#define _HEADER_H_
#include "ansi_compat.h"

#include "hinfo.h"
#include "header.h"

typedef char	a_header_field_t[MAX_ASCII_HEADER_LEN];

#define HEADER_END			"@header_end"
#define HEADER_END_I			-1

#define	HEADER_BEGIN			"@header_begin"
#define	HEADER_BEGIN_I			0

#define GENERATED_BY			"generated_by"
#define GENERATED_BY_I			1

#define SOURCE_ARCHIE_HOSTNAME		"source_archie_hostname"
#define SOURCE_ARCHIE_HOSTNAME_I	2
					
#define PRIMARY_HOSTNAME		"primary_hostname"
#define PRIMARY_HOSTNAME_I		3

#define PREFERRED_HOSTNAME		"preferred_hostname"
#define PREFERRED_HOSTNAME_I		4

#define PRIMARY_IPADDR			"primary_ipaddr"
#define PRIMARY_IPADDR_I		5
					
#define ACCESS_METHODS			"access_method"
#define ACCESS_METHODS_I		6
					
#define ACCESS_COMMAND			"access_command"
#define ACCESS_COMMAND_I		7
					
#define OS_TYPE				"os_type"
#define OS_TYPE_I			8
					
#define TIMEZONE			"timezone"
#define TIMEZONE_I			9
					
#define RETRIEVE_TIME			"retrieve_time"
#define RETRIEVE_TIME_I			10
					
#define PARSE_TIME			"parse_time"
#define PARSE_TIME_I			11

#define UPDATE_TIME			"update_time"
#define UPDATE_TIME_I			12
					
#define NO_RECS				"no_recs"
#define NO_RECS_I			13
					
#define CURRENT_STATUS			"current_status"
#define CURRENT_STATUS_I		14

#define	UPDATE_STATUS			"update_status"
#define	UPDATE_STATUS_I			15

#define	ACTION_STATUS			"action_status"
#define	ACTION_STATUS_I			16

#define	FORMAT				"format"
#define	FORMAT_I			17

#define	PROSPERO_HOST			"prospero_host"
#define	PROSPERO_HOST_I			18

#define	HCOMMENT			"comment"
#define HCOMMENT_I			19

#define DATA_NAME			"data_name"
#define DATA_NAME_I		        20

#define SITE_NO_RECS			"site_no_recs"
#define SITE_NO_RECS_I			21

typedef struct {
   int			index;
   a_header_field_t	field;
   int			set;
} ascii_header_fields_t;



typedef struct{
   gen_prog_t		generated_by;		/* System generating header	*/
   hostname_t		source_archie_hostname; /* archie host which generated data	*/
   hostname_t		primary_hostname;	/* Primary DNS hostname of site	*/
   hostname_t		preferred_hostname;	/* Preferred DNS hostname (CNAME) of site	*/
   ip_addr_t		primary_ipaddr;		/* Primary IP address of site	*/
   access_methods_t	access_methods;		/* Access methods used at site	*/
   access_comm_t	access_command;		/* Codename for access script	*/
   os_type_t		os_type;		/* Operating system of host	*/
   date_time_t		retrieve_time;		/* Time of retrieval (GMT) of data	*/
   date_time_t		parse_time;		/* Time of processing (GMT) of data	*/
   date_time_t		update_time;		/* Time of update (GMT) of data	*/   
   index_t		no_recs;		/* Number of records in data if applicable	*/
   index_t		site_no_recs;		/* Number of records in sitefile if applicable	*/
   current_status_t	current_status;		/* Current disposition of site	*/
   update_status_t	update_status;		/* Operation to be performed */
   action_status_t	action_status;		/* Status of attempted update	*/
   timezone_t		timezone;		/* Timezone of host. Signed seconds from GMT */
   format_t		format;			/* Format of file */
   state_t		prospero_host;
   header_flags_t	header_flags;		/* Bitmap of header entries */
   comment_t		comment;		/* Free format comment field */
   access_comm_t	data_name;		/* Name of this data object */
} header_t;

#define	HDR_GENERATED_BY			0x1
#define	HDR_SET_GENERATED_BY(x)			((x) |= HDR_GENERATED_BY)
#define	HDR_UNSET_GENERATED_BY(x)		((x) &= ~HDR_GENERATED_BY)
#define	HDR_GET_GENERATED_BY(x)			((x) & HDR_GENERATED_BY)

#define	HDR_TIMEZONE				0x2
#define	HDR_SET_TIMEZONE(x)			((x) |= HDR_TIMEZONE)
#define	HDR_UNSET_TIMEZONE(x)			((x) &= ~HDR_TIMEZONE)
#define	HDR_GET_TIMEZONE(x)			((x) & HDR_TIMEZONE)

#define	HDR_SOURCE_ARCHIE_HOSTNAME		0x4
#define	HDR_SET_SOURCE_ARCHIE_HOSTNAME(x)	((x) |= HDR_SOURCE_ARCHIE_HOSTNAME)
#define	HDR_UNSET_SOURCE_ARCHIE_HOSTNAME(x)	((x) &= ~HDR_SOURCE_ARCHIE_HOSTNAME)
#define	HDR_GET_SOURCE_ARCHIE_HOSTNAME(x)	((x) & HDR_SOURCE_ARCHIE_HOSTNAME)

#define	HDR_PRIMARY_HOSTNAME			0x8
#define	HDR_SET_PRIMARY_HOSTNAME(x)		((x) |= HDR_PRIMARY_HOSTNAME)
#define	HDR_UNSET_PRIMARY_HOSTNAME(x)		((x) &= ~HDR_PRIMARY_HOSTNAME)
#define	HDR_GET_PRIMARY_HOSTNAME(x)		((x) & HDR_PRIMARY_HOSTNAME)

#define	HDR_PREFERRED_HOSTNAME			0x10
#define	HDR_SET_PREFERRED_HOSTNAME(x)		((x) |= HDR_PREFERRED_HOSTNAME)
#define	HDR_UNSET_PREFERRED_HOSTNAME(x)		((x) &= ~HDR_PREFERRED_HOSTNAME)
#define	HDR_GET_PREFERRED_HOSTNAME(x)		((x) & HDR_PREFERRED_HOSTNAME)

#define	HDR_PRIMARY_IPADDR			0x20
#define	HDR_SET_PRIMARY_IPADDR(x)		((x) |= HDR_PRIMARY_IPADDR)
#define	HDR_UNSET_PRIMARY_IPADDR(x)		((x) &= ~HDR_PRIMARY_IPADDR)
#define	HDR_GET_PRIMARY_IPADDR(x)		((x) & HDR_PRIMARY_IPADDR)

#define	HDR_ACCESS_METHODS			0x40
#define	HDR_SET_ACCESS_METHODS(x)		((x) |= HDR_ACCESS_METHODS)
#define	HDR_UNSET_ACCESS_METHODS(x)		((x) &= ~HDR_ACCESS_METHODS)
#define	HDR_GET_ACCESS_METHODS(x)		((x) & HDR_ACCESS_METHODS)

#define	HDR_ACCESS_COMMAND			0x80
#define	HDR_SET_ACCESS_COMMAND(x)		((x) |= HDR_ACCESS_COMMAND)
#define	HDR_UNSET_ACCESS_COMMAND(x)		((x) &= ~HDR_ACCESS_COMMAND)
#define	HDR_GET_ACCESS_COMMAND(x)		((x) & HDR_ACCESS_COMMAND)

#define	HDR_OS_TYPE				0x100
#define	HDR_SET_OS_TYPE(x)			((x) |= HDR_OS_TYPE)
#define	HDR_UNSET_OS_TYPE(x)			((x) &= ~HDR_OS_TYPE)
#define	HDR_GET_OS_TYPE(x)			((x) & HDR_OS_TYPE)

#define	HDR_RETRIEVE_TIME			0x200
#define	HDR_SET_RETRIEVE_TIME(x)		((x) |= HDR_RETRIEVE_TIME)
#define	HDR_UNSET_RETRIEVE_TIME(x)		((x) &= ~HDR_RETRIEVE_TIME)
#define	HDR_GET_RETRIEVE_TIME(x)		((x) & HDR_RETRIEVE_TIME)

#define	HDR_PARSE_TIME				0x400
#define	HDR_SET_PARSE_TIME(x)			((x) |= HDR_PARSE_TIME)
#define	HDR_UNSET_PARSE_TIME(x)			((x) &= ~HDR_PARSE_TIME)
#define	HDR_GET_PARSE_TIME(x)			((x) & HDR_PARSE_TIME)

#define	HDR_UPDATE_TIME				0x800
#define	HDR_SET_UPDATE_TIME(x)			((x) |= HDR_UPDATE_TIME)
#define	HDR_UNSET_UPDATE_TIME(x)		((x) &= ~HDR_UPDATE_TIME)
#define	HDR_GET_UPDATE_TIME(x)			((x) & HDR_UPDATE_TIME)

#define	HDR_NO_RECS				0x1000
#define	HDR_SET_NO_RECS(x)			((x) |= HDR_NO_RECS)
#define	HDR_UNSET_NO_RECS(x)			((x) &= ~HDR_NO_RECS)
#define	HDR_GET_NO_RECS(x)			((x) & HDR_NO_RECS)

#define	HDR_CURRENT_STATUS			0x2000
#define	HDR_SET_CURRENT_STATUS(x)		((x) |= HDR_CURRENT_STATUS)
#define	HDR_UNSET_CURRENT_STATUS(x)		((x) &= ~HDR_CURRENT_STATUS)
#define	HDR_GET_CURRENT_STATUS(x)		((x) & HDR_CURRENT_STATUS)

#define	HDR_UPDATE_STATUS			0x4000
#define	HDR_SET_UPDATE_STATUS(x)		((x) |= HDR_UPDATE_STATUS)
#define	HDR_UNSET_UPDATE_STATUS(x)		((x) &= ~HDR_UPDATE_STATUS)
#define	HDR_GET_UPDATE_STATUS(x)		((x) & HDR_UPDATE_STATUS)

#define	HDR_ACTION_STATUS			0x8000
#define	HDR_SET_ACTION_STATUS(x)		((x) |= HDR_ACTION_STATUS)
#define	HDR_UNSET_ACTION_STATUS(x)		((x) &= ~HDR_ACTION_STATUS)
#define	HDR_GET_ACTION_STATUS(x)		((x) & HDR_ACTION_STATUS)

#define	HDR_FORMAT				0x10000
#define	HDR_SET_FORMAT(x)			((x) |= HDR_FORMAT)
#define	HDR_UNSET_FORMAT(x)			((x) &= ~HDR_FORMAT)
#define	HDR_GET_FORMAT(x)			((x) & HDR_FORMAT)

#define	HDR_PROSPERO_HOST			0x20000
#define	HDR_SET_PROSPERO_HOST(x)		((x) |= HDR_PROSPERO_HOST)
#define	HDR_UNSET_PROSPERO_HOST(x)		((x) &= ~HDR_PROSPERO_HOST)
#define	HDR_GET_PROSPERO_HOST(x)		((x) & HDR_PROSPERO_HOST)

#define	HDR_HCOMMENT				0x40000
#define	HDR_SET_HCOMMENT(x)			((x) |= HDR_HCOMMENT)
#define	HDR_UNSET_HCOMMENT(x)			((x) &= ~HDR_HCOMMENT)
#define	HDR_GET_HCOMMENT(x)			((x) & HDR_HCOMMENT)

#define	HDR_DATA_NAME				0x80000
#define	HDR_SET_DATA_NAME(x)			((x) |= HDR_DATA_NAME)
#define	HDR_UNSET_DATA_NAME(x)			((x) &= ~HDR_DATA_NAME)
#define	HDR_GET_DATA_NAME(x)			((x) & HDR_DATA_NAME)

/* added this to accomodate the new site database (Nov95)*/

#define	HDR_SITE_NO_RECS			0x100000
#define	HDR_SET_SITE_NO_RECS(x)			((x) |= HDR_SITE_NO_RECS)
#define	HDR_UNSET_SITE_NO_RECS(x)		((x) &= ~HDR_SITE_NO_RECS)
#define	HDR_GET_SITE_NO_RECS(x)			((x) & HDR_SITE_NO_RECS)


#define	HDR_HOSTDB_ONLY(x)	((x) = HDR_TIMEZONE  | HDR_PRIMARY_HOSTNAME | HDR_PREFERRED_HOSTNAME | HDR_ACCESS_METHODS | HDR_OS_TYPE | HDR_PROSPERO_HOST)
#define	HDR_HOSTAUX_ONLY(x)	((x) = HDR_GENERATED_BY | HDR_ACCESS_COMMAND | HDR_RETRIEVE_TIME | HDR_PARSE_TIME | HDR_SITE_NO_RECS | HDR_NO_RECS | HDR_FORMAT | HDR_CURRENT_STATUS | HDR_SOURCE_ARCHIE_HOSTNAME | HDR_HCOMMENT  | HDR_DATA_NAME)

#define	HDR_HOSTDB_ALSO(x)	((x) |= HDR_TIMEZONE  | HDR_PRIMARY_HOSTNAME |  HDR_ACCESS_METHODS | HDR_OS_TYPE | HDR_PROSPERO_HOST)
#define	HDR_HOSTAUX_ALSO(x)	((x) |= HDR_GENERATED_BY | HDR_PREFERRED_HOSTNAME |HDR_ACCESS_COMMAND | HDR_RETRIEVE_TIME | HDR_PARSE_TIME | HDR_SITE_NO_RECS | HDR_NO_RECS | HDR_FORMAT | HDR_CURRENT_STATUS | HDR_SOURCE_ARCHIE_HOSTNAME | HDR_HCOMMENT | HDR_RETRIEVE_TIME )


extern void	init_header PROTO((void));
extern status_t read_header PROTO((FILE *, header_t *, u32 *, int, int));
extern status_t write_header PROTO((FILE *, header_t *, u32 *, int, int));
extern void     write_error_header PROTO((file_info_t *, header_t *));
extern void     do_error_header();

#endif
