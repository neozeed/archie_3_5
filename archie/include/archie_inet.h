/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _ARCH_INET_H_
#define _ARCH_INET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>

#include "typedef.h"
#include "ansi_compat.h"

typedef enum{
   CON_HOST_UNKNOWN = 3,
   CON_NULL_SERVERHOST,
   CON_NULL_SOCKET,
   CON_SOCKETFAILED,
   CON_USER_UNKNOWN,
   CON_TIMEOUT = ETIMEDOUT,
   CON_UNREACHABLE = EHOSTUNREACH,
   CON_NETUNREACHABLE = ENETUNREACH,
   CON_REFUSED = ECONNREFUSED,
   CON_CONNECTFAILED /*, */
} con_status_t;
   
#define	DEFAULT_PORT	4711
#define	SOCK_QUEUE	1
#define	DEFAULT_TRIES	50
   

extern	int		find_sitefile_in_db PROTO((hostname_t, file_info_t *, file_info_t *));
extern	con_status_t	cliconnect PROTO((hostname_t, int, int *));
extern  status_t	get_new_port PROTO((int *, int *));
extern	struct in_addr	ipaddr_to_inet PROTO((ip_addr_t ));
extern  ip_addr_t	inet_to_ipaddr PROTO((struct in_addr *inaddr));
extern	char		*get_conn_err PROTO((int));


#endif
