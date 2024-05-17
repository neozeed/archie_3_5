/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _DATABASES_H_
#define _DATABASES_H_


#ifndef MAX_DATABASE_LEN
#define MAX_DATABASE_LEN		32
#endif

#ifndef MAX_NO_DATABASES
#define MAX_NO_DATABASES		16
#endif

#define	DATABASES_ALL			"*"

typedef char	database_name_t[MAX_DATABASE_LEN];


#endif
