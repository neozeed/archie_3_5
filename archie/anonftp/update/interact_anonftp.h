#ifndef _INTERACT_ANONFTP_H_
#define	_INTERACT_ANONFTP_H_
#include <limits.h>
#include "defines.h"
#include "typedef.h"
#include "header.h"
#include "host_db.h"
#include "archstridx.h"
#include "patrie.h"

#ifdef __STDC__

extern	 status_t archQuery PROTO((struct arch_stridx_handle *, char *, file_info_t *, file_info_t *));
extern   status_t getResultFromStart PROTO(( index_t, struct arch_stridx_handle *, ip_addr_t, int  ));
#else

extern	 status_t archQuery PROTO(());
extern   status_t getResultFromStart PROTO(());

#endif

#endif
