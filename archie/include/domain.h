/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _DOMAIN_H_
#define _DOMAIN_H_

#include "defines.h"
#include "ansi_compat.h"


#ifndef MAX_DOMAIN_LEN
#define MAX_DOMAIN_LEN		128
#endif

#ifndef MAX_DOMAIN_DESC
#define MAX_DOMAIN_DESC		256
#endif

typedef	char	domain_desc_t[MAX_DOMAIN_DESC];
typedef char	domain_t[MAX_DOMAIN_LEN];

typedef struct{
   domain_t	  domain_name;
   domain_t	  domain_def;
   domain_desc_t  domain_desc;
} domain_struct;

#define cvt_to_domainlist(x)		(x)


#ifndef MAX_NO_DOMAINS
#define MAX_NO_DOMAINS		257
#endif

#ifndef DEFAULT_ARDOMAINS
#define DEFAULT_ARDOMAINS	"ardomains.cf"
#endif


#define	DOMAIN_ALL	"*"


#define	MAX_DOMAIN_DEPTH     20


extern char *check_domains_addr PROTO(( char *, domain_t *, ip_addr_t ));
extern int find_in_domains PROTO((char *, domain_t *, int));
extern status_t	compile_domains PROTO(( char *, domain_t *, file_info_t *, int *));
extern status_t domain_construct PROTO(( char *, domain_t *, file_info_t *, int *));
extern status_t sort_domains PROTO((domain_struct domain_set[]));

#endif
