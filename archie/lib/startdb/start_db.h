/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _START_DB_H
#define _START_DB_H
#include "ansi_compat.h"

#include "typedef.h"
#include "hinfo.h"
#include "header.h"
#include "archie_inet.h"
#include "archie_dns.h"
#include "databases.h"
#include "domain.h"

#ifndef DEFAULT_START_DB_DIR
#define DEFAULT_START_DB_DIR "./start"
#endif

#ifndef DEFAULT_START_TABLE
#define DEFAULT_START_TABLE "./start-table"
#endif

#define END_LIST -1

#define ADD_SITE 1
#define DELETE_SITE 2

#define	DEFAULT_NO_HOSTS	1000
#define	DEFAULT_ADD_HOSTS	500

extern char*	 OPEN_START_DBS_001;
extern char*	 OPEN_START_DBS_002;

extern char*	 SET_START_DB_DIR_001;
extern char*	 SET_START_DB_DIR_002;
extern char*	 SET_START_DB_DIR_003;

typedef s16 host_table_index_t;

#define MAX_LIST_SIZE 65536

extern status_t open_start_dbs PROTO((file_info_t *,file_info_t *,int));
extern status_t close_start_dbs PROTO((file_info_t *,file_info_t *));
extern status_t exist_start_dbs PROTO((file_info_t *, file_info_t *, int *, int*));
extern char* set_start_db_dir PROTO((char *, char *));
extern char* get_start_db_dir PROTO((void));
extern char* start_db_filename PROTO((char *));
extern status_t update_start_dbs PROTO((file_info_t *,int,host_table_index_t,int));
extern status_t get_index_start_dbs PROTO((file_info_t *,int,host_table_index_t *,int *));
extern status_t count_index_start_dbs PROTO((file_info_t *,int,int *));
extern status_t read_host_table PROTO((void));
extern status_t write_host_table PROTO((void));
extern status_t host_table_find PROTO((ip_addr_t *,hostname_t,int *,host_table_index_t *));
extern status_t host_table_add PROTO((ip_addr_t,hostname_t,int,host_table_index_t *));
extern status_t host_table_del PROTO((host_table_index_t));
#ifdef __STDC__
/* #warning ***** standard C ******/
#endif
extern int host_table_cmp PROTO((host_table_index_t,host_table_index_t));
extern int domain_priority PROTO((hostname_t));

#endif


