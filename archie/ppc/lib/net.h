#ifndef NET_H
#define NET_H

#include <netinet/in.h>
#include "defs.h"


extern const char *sockAddr proto_((int s));
extern int getPeer proto_((int s, struct sockaddr_in *saddr));
extern int passivesock proto_((const char *service, const char *protocol, int qlen));
extern int passiveTCP proto_((const char *service, int qlen));
extern int portnumTCP proto_((const char *service));
extern int sockPortNum proto_((int s));
extern struct in_addr net_addr proto_((const char *s));
extern struct in_addr network proto_((struct in_addr in));
extern unsigned short nportnum proto_((const char *service, const char *protocol));

#endif
