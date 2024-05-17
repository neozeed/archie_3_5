/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _AR_SEARCH_H_
#define _AR_SEARCH_H_

#include "typedef.h"
#include "domain.h"

#ifndef  PROSP_SERVER
#include "pfs.h"
#endif

#include "ar_attrib.h"

#if 0
#define	DEF_MAX_PCOMPS		15
#define	DEF_INCR_PCOMPS		10
#else
#define	MAX_PCOMPS		128
#endif

#define MATCH_CACHE_SIZE     15


/* Error codes returned by prarch routines */
#define PRARCH_SUCCESS		0	/* Successful completion       */
#define PRARCH_BAD_ARG		1	/* Bad argument                */
#define PRARCH_OUT_OF_MEMORY	2	/* Can't allocate enough space */
#define PRARCH_BAD_REGEX	3	/* Bad regular expression      */
#define PRARCH_SITE_NOT_FOUND	4	/* Site not found  */
#define PRARCH_CANT_OPEN_FILE	5	/* Can't open site file */
#define PRARCH_DB_ERROR		6	/* Generic database error      */
#define	PRARCH_BAD_DOMAINDB	7	/* Loop in domain database     */
#define	PRARCH_BAD_MMAP		8	/* Can't mmap() file */
#define PRARCH_NO_HOSTNAME      9       /* Can't get local hostname    */
#define	PRARCH_NOT_DIRECTORY   10       /* Given pathname is not a directory */
#define	PRARCH_DIR_NOT_FOUND   11       /* Given pathname not found */
#define	PRARCH_INTERN_ERR      12       /* Internal system error */
#define	PRARCH_TOO_MANY	       13	/* Too many hosts returned */


#define	MAX_NO_COMPONENTS	8

#define DEFAULT_MAXHITS	      100   /* default max total number of hits    */
#define	DEFAULT_MAXMATCH      100   /* default max unique strings	  */
#define	DEFAULT_MAXHITPM       20   /* default max hits per unique string */

#define	MAX_MAXHITS	    10000   /* max total number of hits */
#define	MAX_MAXMATCH	     1000
#define	MAX_MAXHITPM	      100

/* Search methods */

typedef enum {
       S_FULL_REGEX,		/* Full regular expression	*/
       S_SUB_CASE_STR,		/* Substring, case sensitive	*/
       S_SUB_NCASE_STR,		/* Substring, case insensitive	*/
       S_EXACT,			/* Exact match			*/
       S_E_FULL_REGEX,		/* If no exact, regex		*/
       S_E_SUB_CASE_STR,	/* If no exact, S_SUB_CASE_STR	*/
       S_E_SUB_NCASE_STR,	/* If no exact, S_SUB_NCASE_STR	*/
       S_ZUB_NCASE,		/* No attrib, substr case insen */
       S_E_ZUB_NCASE,		/* No attrib, if no exact substr case insen */
       S_SUB_KASE,		/* No attrib, substring case sensitive */
       S_E_SUB_KASE,		/* No attrib, if no exact substr case sen */
       S_X_REGEX,		/* No attrib, Full regex */
       S_E_X_REGEX,		/* No attrib, if no exact, full regex */
       S_NOATTRIB_EXACT		/* No attrib, exact */
       
} search_sel_t;


typedef char	search_string_t[MAX_PATH_LEN];

typedef struct{
      search_string_t		search_str;	/* string to search */
      search_sel_t		orig_type;	/* original search method */
      search_sel_t		curr_type;      /* search method used */
      int			orig_offset;	/* original offset */
      int			curr_offset;	/* returned offset */
      int			maxhits;	/* maximum total links returned */
      int			maxmatch;	/* max unique strings */
      int			maxhitpm;	/* max entries per unique string */
      struct token		*domains;	/* domain list */
      struct token		*comp_restrict; /* pathname component restriction */
      char			*error_string;  /* error string returned if any */
      attrib_list_t		attrib_list;	/* attribute list see ar_attrib.h */
      int			no_matches;	/* Number of matches returned */
} search_req_t;
      

typedef index_t search_result_t	;

extern	int		parchie_search_files_db PROTO((file_info_t *,file_info_t *, file_info_t *, file_info_t *, search_req_t *,P_OBJECT));
extern	status_t	check_comp_restrict PROTO(( char **, char **));
extern	status_t	ar_exact_match PROTO((search_req_t *, search_result_t *, file_info_t *));
extern	status_t	ar_regex_match PROTO((search_req_t *, search_result_t *, file_info_t *, file_info_t *));
extern	status_t	ar_sub_match PROTO((search_req_t *,index_t *,file_info_t *,file_info_t *));
extern	int		prep PROTO((unsigned char *,int, int));
extern	int		prarch_host_dir PROTO((hostname_t, attrib_list_t,char *,P_OBJECT, file_info_t *, file_info_t *, file_info_t *));
extern	status_t	cvt_token_to_internal PROTO((struct token *, char *, int));

extern	int		parchie_search_gindex_db PROTO((file_info_t *,file_info_t *,file_info_t *,file_info_t *,file_info_t *, file_info_t *, file_info_t *, file_info_t *, file_info_t *, search_req_t *,P_OBJECT,int));

#endif
