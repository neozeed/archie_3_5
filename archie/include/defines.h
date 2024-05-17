/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _DEFINES_H_
#define _DEFINES_H_

#include <sys/types.h>
#include <sys/stat.h>

#ifndef	ARCHIE_USER
#define	ARCHIE_USER	"archie"
#endif

#ifndef	P_USER_ID
#define	P_USER_ID	"pfs"
#endif

#ifndef SITE_NAME_LEN
#define	SITE_NAME_LEN	  64
#endif

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN	  256
#endif

#ifndef MAX_COMMAND_LEN
#define MAX_COMMAND_LEN	   512
#endif

/* This is YYYYMMDDHHMMSS */

#ifndef EXTERN_DATE_LEN
#define EXTERN_DATE_LEN	   14
#endif

#ifndef MAX_HOSTNAME_LEN
#define MAX_HOSTNAME_LEN	256
#endif

#ifndef DEFAULT_FILE_PERMS
#define DEFAULT_FILE_PERMS	S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH
#endif

#ifndef MAX_ASCII_HEADER_LEN
#define MAX_ASCII_HEADER_LEN	256
#endif

#ifndef PATH_SEPARATORS
#define PATH_SEPARATORS		"/"
#endif

#ifndef COMMENT_CHAR
#define COMMENT_CHAR		'#'
#endif

#ifndef	CONTINUATION_LINE
#define CONTINUATION_LINE	"\\\n"
#endif

#ifndef DEFAULT_NO_FILES
#define	DEFAULT_NO_FILES	100
#endif

#ifndef NO_FILES_INC
#define NO_FILES_INC	        100
#endif

#define NET_DELIM_STR	":"
#define NET_DELIM_CHAR	':'

#define NET_SEPARATOR_CHAR ','
#define NET_SEPARATOR_STR  ","

#define	 MAX_NO_PARAMS		 30


#ifdef __STDC__
#   ifdef TELNET_CLIENT

#   define ptr_check(ptr, type, func, ret_val) \
    do \
    { \
      if((ptr) == (type *)0) \
      { \
        fprintf(stderr, "%s: %s: parameter '" #ptr "' is a null pointer.\n", prog, func) ; \
        return ret_val ; \
      } \
    } while(0)

#   else

#   define ptr_check(var, type, fnam, rval) \
    do \
    {  \
      if(var == (type *)0) \
      { \
        error(A_INTERR, fnam, "argument " #var " to " fnam " is a null pointer") ; \
        return rval ; \
      } \
    } \
    while(0)

#endif
#else

#define ptr_check(var, type, fnam, rval) \
  do \
  {  \
    if(var == (type *)0) \
    { \
      error(A_INTERR, fnam, "an argument is a null pointer") ; \
      return rval ; \
    } \
  } \
  while(0)

#endif

#endif
