/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _ARCHIE_DNS_H_
#define _ARCHIE_DNS_H_

#include "ansi_compat.h"

typedef enum 
{
   DNS_LOCAL_ONLY,
   DNS_LOCAL_FIRST,
   DNS_EXTERN_ONLY
} dns_t;
					
#include "typedef.h"

typedef struct hostent AR_DNS;

extern	AR_DNS*		ar_gethostbyaddr PROTO((ip_addr_t, dns_t, file_info_t *));
extern	AR_DNS*		ar_gethostbyname PROTO((char *, dns_t, file_info_t *));
extern	struct hostent*	ar_ghbn PROTO((hostname_t, file_info_t *));
extern	AR_DNS*		ar_open_dns_name PROTO((hostname_t, dns_t, file_info_t *));
extern	AR_DNS*		ar_open_dns_addr PROTO((ip_addr_t, dns_t, file_info_t *));
extern	status_t	cmp_dns_name PROTO(( hostname_t, AR_DNS *));
extern	status_t	cmp_dns_addr PROTO(( ip_addr_t *, AR_DNS *));
extern	char *		get_dns_primary_name PROTO(( AR_DNS *));
extern	ip_addr_t*	get_dns_addr PROTO((AR_DNS *));
extern	status_t	ar_dns_close PROTO((AR_DNS *));
extern  struct hostent* ar_ghba PROTO((ip_addr_t, file_info_t *));

#endif
