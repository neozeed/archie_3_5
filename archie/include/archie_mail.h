/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#ifndef _ARCHIE_MAIL_H_
#define _ARCHIE_MAIL_H_

#include "ansi_compat.h"

typedef enum{
   MAIL_HOST_ADD = 1,
   MAIL_HOST_DELETE = 2,
   MAIL_HOST_FAIL = 3,
   MAIL_HOST_SUCCESS = 4,
   MAIL_PARSE_FAIL = 5,
   MAIL_RETR_FAIL = 6
   
} mail_t;

#define MAIL_HOST_ADD_FILE	 "mail.add"
#define	MAIL_HOST_DELETE_FILE	 "mail.delete"
#define	MAIL_HOST_FAIL_FILE	 "mail.fail"
#define	MAIL_HOST_SUCCESS_FILE	 "mail.success"
#define	MAIL_PARSE_FAIL_FILE	 "mail.parse"
#define	MAIL_RETR_FAIL_FILE	 "mail.retr"

#define	MAIL_RESULTS_FILE	 "mail.results"

extern void write_mail();

#endif
