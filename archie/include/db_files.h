/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _DB_FILES_H
#define _DB_FILES_H

#ifndef DEFAULT_MASTER_DB_DIR
#define	DEFAULT_MASTER_DB_DIR		"db"
#endif


#ifndef	DEFAULT_BIN_DIR
#define	DEFAULT_BIN_DIR		"bin"
#endif

#ifndef DEFAULT_ETC_DIR
#define	DEFAULT_ETC_DIR		"etc"
#endif

#ifndef DEFAULT_INCOMING_DIR
#define	DEFAULT_INCOMING_DIR	"incoming"
#endif

#ifndef DEFAULT_FILES_DB_DIR
#define DEFAULT_FILES_DB_DIR	"anonftp"
#endif

#ifndef DEFAULT_GFILES_DB_DIR
#define DEFAULT_GFILES_DB_DIR	"gopherindex"
#endif

#ifndef DEFAULT_WFILES_DB_DIR
#define DEFAULT_WFILES_DB_DIR	"webindex"
#endif

#ifndef DEFAULT_TMP_DIR
#define DEFAULT_TMP_DIR		"tmp"
#endif

#ifndef DEFAULT_LOCK_DIR
#define DEFAULT_LOCK_DIR		"locks"
#endif

#ifndef DEFAULT_TMP_PREFIX
#define DEFAULT_TMP_PREFIX	"ainput_"
#endif

#ifndef	DB_SUFFIX
#define DB_SUFFIX		"_db"
#endif


#ifndef DEFAULT_HBYADDR_DB
#define DEFAULT_HBYADDR_DB	"hostbyaddr"
#endif

#ifndef DEFAULT_HOST_DB
#define DEFAULT_HOST_DB		"host-db"
#endif

#ifndef DEFAULT_HOSTAUX_DB
#define DEFAULT_HOSTAUX_DB	"hostaux-db"
#endif

#ifndef DEFAULT_DOMAIN_DB
#define DEFAULT_DOMAIN_DB	"domain-db"
#endif

#ifndef DEFAULT_START_DB
#define DEFAULT_START_DB        "start-db"
#endif

#ifndef ARCHIE_HOSTNAME_FILE
#define ARCHIE_HOSTNAME_FILE	"archie.hostname"
#endif


#ifdef SUNOS
#ifndef COMPRESS_PGM
#define	COMPRESS_PGM		"/usr/ucb/compress"
#endif
#ifndef UNCOMPRESS_PGM
#define	UNCOMPRESS_PGM		"/usr/ucb/uncompress"
#endif
#endif


#ifdef AIX
#ifndef COMPRESS_PGM
#define	COMPRESS_PGM		"/usr/ucb/compress"
#endif
#ifndef UNCOMPRESS_PGM
#define	UNCOMPRESS_PGM		"/usr/ucb/uncompress"
#endif
#endif

#ifdef SOLARIS
#ifndef COMPRESS_PGM
#define	COMPRESS_PGM		"/usr/bin/compress"
#endif
#ifndef UNCOMPRESS_PGM
#define	UNCOMPRESS_PGM		"/usr/bin/uncompress"
#endif
#endif


#ifndef CAT_PGM
#define CAT_PGM			"/bin/cat"
#endif

#ifndef PROSPERO_SUBDIR
#define PROSPERO_SUBDIR		"pfs"
#endif

#define	 UPDATE_PREFIX	   "update_"
#define	 RETRIEVE_PREFIX   "retrieve_"
#define	 PARSE_PREFIX	   "parse_"
#define	 FILTER_PREFIX	   "filter_"
#define	 INSERT_PREFIX	   "insert_"
#define	 DELETE_PREFIX	   "delete_"
#define  PARTIAL_PREFIX    "partial_"

#define	 SUFFIX_RETR		 ".retr"
#define	 SUFFIX_UPDATE		 ".update"
#define	 SUFFIX_PARSE		 ".parse"
#define	 SUFFIX_FILTERED	 ".filtered"
#define  SUFFIX_EXCERPT   ".excerpt"
#define  SUFFIX_PARTIAL_UPDATE ".partial_update"
#define  SUFFIX_PARTIAL_EXCERPT ".partial_excerpt"

#define	 TMP_SUFFIX		 "_t"
#define	 INVALID_SUFFIX		 "_invalid"


#endif
