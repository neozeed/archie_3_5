#ifndef _DELETE_H_
#define _DELETE_H_

#include <limits.h>
#include "typedef.h"
#include "host_db.h"

#define	 MAX_INTERNAL_LINKNO	 5000


extern status_t	  setup_delete PROTO((file_info_t *,file_info_t *, int, ip_addr_t, int));

#endif
