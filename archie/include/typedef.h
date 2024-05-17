/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _TYPEDEF_H
#define _TYPEDEF_H
#include <fcntl.h>
#include <stdio.h>
#ifndef SEEN_NDBM
#define SEEN_NDBM
/*
 *  This is a hack to work around multiple inclusions of ndbm.h on AIX.
 */
#include <ndbm.h>
#endif
#include "./defines.h"

typedef unsigned long u32 ;
#define XDR_U_LONG		xdr_u_long

#ifdef __STDC__
typedef signed long s32 ;
#else
typedef long s32 ;
#endif

#define XDR_S_LONG		xdr_long

#define XDR_U_INT		xdr_u_int

typedef unsigned short u16;
#define XDR_U_SHORT		xdr_u_short

typedef short s16;
#define XDR_S_SHORT		xdr_short

typedef u32 date_time_t ;
#define xdr_date_time_t	XDR_U_LONG

typedef u32 file_size_t ;
#define xdr_file_size_t	XDR_U_LONG

#if defined(SOLARIS)
/* warning Resolving conflict for index_t  */
#define index_t aindex_t
#endif

typedef s32 index_t ;
#define xdr_index_t	XDR_S_LONG

typedef u32 ip_addr_t ;
#define xdr_ip_addr_t	XDR_U_LONG

typedef u16 strlen_t ;
#define xdr_strlen_t	XDR_U_SHORT

typedef u16 perms_t ;
#define xdr_perms_t	XDR_U_SHORT

typedef u16 flags_t;
#define xdr_flags_t	XDR_U_SHORT

#define xdr_ftype_t	XDR_S_LONG
#define	xdr_port_t	XDR_S_LONG

typedef char hostname_t[MAX_HOSTNAME_LEN]; 
/*typedef char hostname_t[2048]; */
typedef  char comment_t[MAX_PATH_LEN];

#if (!defined(FALSE) && !(defined(TRUE)))
typedef enum{FALSE=0,TRUE=1} bool_t;
#endif

typedef struct {
  char filename[MAX_PATH_LEN];
  union{
    FILE *fp;
    DBM *dbm;
  } fp_or_dbm;
  char *ptr;
  u32  size;
  s32 offset;
#ifdef AIX
  int write;
#endif	
} file_info_t;


typedef enum {A_OK = 0, ERROR = 1} status_t;

typedef s32		timezone_t;	/* Signed seconds from UTC */

typedef	 char		pathname_t[MAX_PATH_LEN];



#endif
