#ifndef _OUTSTRUCT_H_
#define _OUTSTRUCT_H_
#include "typedef.h"
#ifdef OLD_FILES
#  include "old-host_db.h"
#else
#  include "host_db.h"
#endif

typedef enum {e_size			=     0x1,
	      e_date			=     0x2,
	      e_perms			=     0x4,
	      e_flags			=    0x10,
	      e_source_archie_hostname	=    0x20,
	      e_primary_hostname	=    0x40,
	      e_preferred_hostname	=   0x100,
	      e_primary_ipaddr		=   0x200,
	      e_access_method		=   0x400,
	      e_os_type			=  0x1000,
	      e_timezone		=  0x2000,
	      e_retrieve_time		=  0x4000,
	      e_no_recs			= 0x10000, 
	      e_current_status		= 0x20000,
	      e_LIST			= e_size | e_date
					  | e_perms | e_flags
					  | e_preferred_hostname
					  | e_primary_ipaddr | e_os_type
					  | e_timezone | e_retrieve_time
					  | e_current_status
} e_output_index_t;
	      

typedef struct{

   file_size_t		o_size;
   date_time_t		o_date ;
   perms_t		o_perms;
   flags_t		o_flags;
   hostname_t		o_source_archie_hostname;
   hostname_t		o_primary_hostname;
   hostname_t		o_preferred_hostname;
   access_method_t	o_access_method;
   os_type_t		o_os_type;
   timezone_t		o_timezone;
   date_time_t		o_retrieve_time;
   index_t		o_no_recs;
   current_status_t	o_current_status;
}o_output_t;

#endif
