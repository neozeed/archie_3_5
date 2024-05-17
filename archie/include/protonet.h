/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _PROTONET_H_
#define _PROTONET_H_

#include "typedef.h"


#ifndef ARCHIE_PORT
#define	ARCHIE_PORT	2710
#endif

/* Type used for transmitting and receiving commands over the network */

#define	CR	'\015'
#define LF	'\012'


#define	MAX_PARAMS	10

typedef	char	net_scommand_t[MAX_COMMAND_LEN];
typedef	u32	net_icommand_t;

typedef struct{
	net_scommand_t  scommand;	/* string */
	net_icommand_t	icommand;       /* Correspoding command integer */
} command_t;


#define	C_ERROR		0
#define	C_QUIT		1
#define C_LISTSITES	2
#define C_SENDHEADER	3
#define	C_UPDATELIST	4
#define C_TUPLELIST	5
#define	C_SITEFILE	6
#define	C_SENDSITE	7
#define	C_SITELIST	8
#define	C_HEADER	9
#define	C_DUMPCONFIG   10
#define C_ENDDUMP      11
#define	C_VERSION      12
#define C_AUTH_ERR     13
#define C_SENDEXCERPT  14
#define	C_UNKNOWN      15     /* MUST be the last one */

typedef struct{
	net_icommand_t command;
	net_scommand_t params[MAX_PARAMS];
} command_v_t;



#define DEFAULT_DB_SERVER_PREFIX	"net_"

#endif
