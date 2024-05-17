/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

char*	 EXITING_000	      =	    "Exiting";
char*	 NULL_POINTER_000     =	    "Invalid NULL pointer";
char*	 NOT_OPEN_MASTER_000  =	    "Can't open master database directory";


/* times.c */


char*	 CVT_TO_USERTIME_001 =  "Can't convert given time %u to string";

char*	 CVT_TO_INTTIME_001  =	"Given string %s not in YYYYMMDDHHMMSS format";

char*	 CVT_FROM_INTTIME_001=	 "Can't convert from %u to YYYYMMDDHHMMSS format";

/* files.c */


char*	 OPEN_FILE_001	     =	 "NULL file info structure";
char*	 OPEN_FILE_002	     =	 "No filename in given structure";
char*	 OPEN_FILE_003	     =	 "Can't open file %s";

char*	 CLOSE_FILE_001	     =	 "NULL file info structure";
char*	 CLOSE_FILE_002	     =   "Can't close given file %s";

char*	 MMAP_FILE_001	     =	 "Given file information pointer is NULL";
char*	 MMAP_FILE_002	     =	 "non-null file information pointer. File: %s";
char*	 MMAP_FILE_003	     =	 "Can't stat mapfile %s";
char*	 MMAP_FILE_004	     =	 "mmap() of file %s failed";


char*	 MUNMAP_FILE_001     =	 "Can't munmap() file %s";
char*	 MUNMAP_FILE_002     =	 "Attempt to munmap non mmaped, or zero length file %s";
#ifdef AIX
char*	 MUNMAP_FILE_003     =   "Can't stat mapfile %s";
char*	 MUNMAP_FILE_004     =   "Error while trying to truncate unmmaped file %s to correct length %d";
#endif


char*	 CREATE_FINFO_001    =	 "Can't allocate new file_info_t structure";

char*	 GET_FILE_LIST_001   =	 "Can't open directory %s";
char*	 GET_FILE_LIST_002   =	 "Can't malloc() space for file list";
char*	 GET_FILE_LIST_003   =	 "Can't realloc() space for file list";
char*	 GET_FILE_LIST_004   =	 "Can't close directory %s";

char*	 GET_INPUT_FILE_001    = "Host %s unknown";
char*	 GET_INPUT_FILE_002    = "Can't find host %s in primary database";
char*	 GET_INPUT_FILE_003    = "Can't find database %s host %s in local databases";


char*	 UNLOCK_DB_001	       = "Can't remove lock file %s !!";


char*	 RENAME_001         =	"Can't open dest file %s, errno = %d";
char*	 RENAME_002         =	"Can't open src file %s, errno = %d";
char*	 RENAME_003         =	"While copying file %s to %s, errno = %d";


/* master.c */


char*	 SET_MASTER_DB_DIR_001 = "Can't access directory %s";
char*	 SET_MASTER_DB_DIR_002 = "%s is not a directory";

/* mydbm.c */

char*	 GET_DBM_ENTRY_001    =	 "NULL key pointer given";
char*	 GET_DBM_ENTRY_002    =	 "Invalid or unopened dbm database given %s";
char*	 GET_DBM_ENTRY_003    =	 "Error while reading dbm database %s";

char*	 PUT_DBM_ENTRY_001    =	 "NULL key or data pointer given";
char*	 PUT_DBM_ENTRY_002    =	 "Unable to insert data into dbm database %s";
char*	 PUT_DBM_ENTRY_003    = "Error while writing dbm database %s";


/* mystrings.c */


char*	 STR_DECOMPOSE_001    =	 "Can't allocate space for string";

char*	 STR_SEP_001	      =	 "Can't allocate space for string";
char*	 STR_SEP_002	      =	 "Can't reallocate space for string";


/* header.c */

char*	 READ_HEADER_001      =	 "Premature end of file";
char*	 READ_HEADER_002      =	 "%s record not found";
char*	 READ_HEADER_003      =	 "%s found but no %s. Aborting";
char*	 READ_HEADER_004      =	 "Duplicate %s found. Ignored";
char*	 READ_HEADER_005      =  "Invalid %s field: %s";
char*	 READ_HEADER_006      =  "Unknown field in header: %s";

char*	 WRITE_ERROR_HEADER_001 = "Can't write error header for %s";

/* archie_dns.c */

char*	 AR_GETHOSTBYNAME_001	 =  "hostdb pointer is NULL";

char*	 AR_GETHOSTBYADDR_001	 =  "hostbyaddr pointer is NULL";

char*	 AR_GHBN_001		 =  "hostname is NULL";
char*	 AR_GHBN_002		 =  "hostdb pointer or contents is NULL";

char*	 AR_GHBA_001		 =  "hostbyaddr pointer or contents is NULL";

char*	 AR_OPEN_DNS_NAME_001	 =  "hostname is NULL";
char*	 AR_OPEN_DNS_NAME_002	 =  "hostdb pointer or contents is NULL";
char*	 AR_OPEN_DNS_NAME_003	 =  "Can't malloc() space for hostentry";

char*	 AR_OPEN_DNS_ADDR_001	 =  "hostbyaddr pointer or contents is NULL";
char*	 AR_OPEN_DNS_ADDR_002	 =  "Can't malloc() space for hostentry";

char*	 CMP_DNS_NAME_001	 = "hostname or DNS entry is NULL";

char*	 CMP_DNS_ADDR_001	 = "DNS entry is NULL";

char*	 GET_DNS_PRIMARY_NAME_001= "DNS entry is NULL";
char*	 GET_DNS_ADDR_001	 = "DNS entry is NULL";

char*	 AR_DNS_CLOSE_001	 = "hostname or DNS entry is NULL";
char*	 AR_DNS_CLOSE_002	 = "Can't free dns entry";

/* archie_inet.c */

char*	 CLICONNECT_001		 = "serverhost is NULL";
char*	 CLICONNECT_002		 = "socket descriptor is NULL";
char*	 CLICONNECT_003		 = "Error from gethostbyname() looking for %s";
char*	 CLICONNECT_004		 = "Can't open socket";
char*	 CLICONNECT_005		 = "Can't connect to server %s";


char*	 GET_NEW_PORT_001	 = "Can't open new socket. Error from socket()";
char*	 GET_NEW_PORT_002	 = "Can't bind() socket %u";
char*	 GET_NEW_PORT_003	 = "All addreses in use. Giving up after %u tries";
char*	 GET_NEW_PORT_004	 = "Can't listen() on socket";

/* archie_xdr.c */

char*	 OPEN_XDR_STREAM_001	 =  "Can't malloc space for xdr structure";

char*	 CLOSE_XDR_STREAM_001	 = "Can't close XDR stream";

/* archie_mail.c */

char*	 WRITE_MAIL_001		 = "Can't open mail file %s";
char*	 WRITE_MAIL_002		 = "Error trying to lock mail file %s";
char*	 WRITE_MAIL_003		 = "Error trying to unlock mail file %s";
char*	 WRITE_MAIL_004		 = "Can't close mail file %s";
char*	 WRITE_MAIL_005		 = "Unknown mail level %d";



