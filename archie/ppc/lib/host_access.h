#ifndef HOST_ACCESS_H
#define HOST_ACCESS_H

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include "ansi_compat.h"


extern int load_host_acl_file proto_((const char *aclfile));
extern int load_host_acl_fp proto_((FILE *afp));
extern int is_host_allowed proto_((struct in_addr host));


#endif
