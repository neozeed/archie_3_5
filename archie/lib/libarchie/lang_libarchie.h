/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#ifndef _LANG_LIBARCHIE_H_
#define _LANG_LIBARCHIE_H_

extern	 char*	 CVT_TO_USERTIME_001;

extern	 char*	 CVT_TO_INTTIME_001 ;

extern	 char*	 CVT_FROM_INTTIME_001;

/* files.c */

extern	 char*	 OPEN_FILE_001;
extern	 char*	 OPEN_FILE_002;
extern	 char*	 OPEN_FILE_003;

extern	 char*	 CLOSE_FILE_001;
extern	 char*	 CLOSE_FILE_002;

extern	 char*	 MMAP_FILE_001;
extern	 char*	 MMAP_FILE_002;
extern	 char*	 MMAP_FILE_003;
extern	 char*	 MMAP_FILE_004;


extern	 char*	 MUNMAP_FILE_001;
extern	 char*	 MUNMAP_FILE_002;
#ifdef AIX
extern	 char*	 MUNMAP_FILE_003;
extern	 char*	 MUNMAP_FILE_004;
#endif

extern	 char*	 CREATE_FINFO_001;

extern	 char*	 GET_FILE_LIST_001;
extern	 char*	 GET_FILE_LIST_002;
extern	 char*	 GET_FILE_LIST_003;
extern	 char*	 GET_FILE_LIST_004;


extern	 char*	 GET_INPUT_FILE_001    ;
extern	 char*	 GET_INPUT_FILE_002    ;
extern	 char*	 GET_INPUT_FILE_003    ;

extern	 char*	 UNLOCK_DB_001	      ;

extern   char*   RENAME_001 ;
extern   char*   RENAME_002 ;
extern   char*   RENAME_003 ;

/* master.c */


extern	 char*	 SET_MASTER_DB_DIR_001;
extern	 char*	 SET_MASTER_DB_DIR_002;

/* mydbm.c */

extern	 char*	 GET_DBM_ENTRY_001;
extern	 char*	 GET_DBM_ENTRY_002;
extern	 char*	 GET_DBM_ENTRY_003;

extern	 char*	 PUT_DBM_ENTRY_001;
extern	 char*	 PUT_DBM_ENTRY_002;
extern	 char*	 PUT_DBM_ENTRY_003;


/* mystrings.c */


extern	 char*	 STR_DECOMPOSE_001;

extern	 char*	 STR_SEP_001;
extern	 char*	 STR_SEP_002;


/* header.c */

extern	 char*	 READ_HEADER_001;
extern	 char*	 READ_HEADER_002;
extern	 char*	 READ_HEADER_003;
extern	 char*	 READ_HEADER_004;
extern	 char*	 READ_HEADER_005;
extern	 char*	 READ_HEADER_006;

extern	 char*	 WRITE_ERROR_HEADER_001;


/* archie_dns.c */

extern	char*	 AR_GETHOSTBYNAME_001;

extern	char*	 AR_GETHOSTBYADDR_001;

extern	char*	 AR_GHBN_001;
extern	char*	 AR_GHBN_002;

extern	char*	 AR_GHBA_001;

extern	char*	 AR_OPEN_DNS_NAME_001;
extern	char*	 AR_OPEN_DNS_NAME_002;
extern	char*	 AR_OPEN_DNS_NAME_003;

extern	char*	 AR_OPEN_DNS_ADDR_001;
extern	char*	 AR_OPEN_DNS_ADDR_002;

extern	char*	 CMP_DNS_NAME_001;

extern	char*	 CMP_DNS_ADDR_001;

extern	char*	 GET_DNS_PRIMARY_NAME_001;
extern	char*	 GET_DNS_ADDR_001;

extern	char*	 AR_DNS_CLOSE_001;
extern	char*	 AR_DNS_CLOSE_002;

/* archie_inet.c */

extern	char*	 CLICONNECT_001;
extern	char*	 CLICONNECT_002;
extern	char*	 CLICONNECT_003;
extern	char*	 CLICONNECT_004;
extern	char*	 CLICONNECT_005;


extern	char*	 GET_NEW_PORT_001;
extern	char*	 GET_NEW_PORT_002;
extern	char*	 GET_NEW_PORT_003;
extern	char*	 GET_NEW_PORT_004;

/* archie_xdr.c */

extern	char*	 OPEN_XDR_STREAM_001;

extern	char*	CLOSE_XDR_STREAM_001;

/* archie_mail.c */

extern	 char*	 WRITE_MAIL_001		 ;
extern	 char*	 WRITE_MAIL_002		 ;
extern	 char*	 WRITE_MAIL_003		 ;
extern	 char*	 WRITE_MAIL_004		 ;
extern	 char*	 WRITE_MAIL_005		 ;


#endif
