#ifndef _PRWAIS_DO_SEARCH_H_
#define	_PRWAIS_DO_SEARCH_H_

#include "pserver.h"
#include "pfs.h"
#include "psrv.h"
#include "plog.h"
#include "wais_attrib.h"
#include "archie_catalogs.h"

#define	 MAX_KEYWORD_LIST     256

#define	 DEFAULT_MAXDOCS      100
#define	 DEFAULT_MAXHEADERS   200

#define	 INDEX_DISAM	      "E"
#define	 INDEX_PRFX	      "A"

#define	 BUNYIP_SEQ_STR	      "Bunyip-Sequence"

#define	 HASH_TABLE_SIZE 10000
#define  MAX_CHAIN_LEN   5     /* max. number of WAIS doc. pointers in hash table element */

typedef struct{
   char		  keyword_list[1024];
   char		  database[1024];
   long		  Max_Docs;
   long		  Max_Headers;
   int		  expand;
   char		  error_string[256];
   wais_attrib_t  wais_attribs;
} wais_search_req_t;

#define	 MAX_URL  128

typedef char url_t[MAX_URL];

struct wais_doc {
   url_t *url;
   int size;
   void *bytes;
};

typedef struct wais_doc wais_doc_t;

struct htent {
  wais_doc_t *doc[MAX_CHAIN_LEN];
};

typedef struct htent htent_t;

#define	 BUNYIP_SEQUENCE_STR	 "BUNYIP-SEQUENCE"
#define	 SEARCH_GENERAL		 "srch_general"

#define	 MAX_ATTRIBS		 50
   
#define	 PRWAIS_SUCCESS		 0
#define	 PRWAIS_BAD_ARG		 1
#define	 PRWAIS_NO_MATCHES	 2
#define	 PRWAIS_WAIS_ERROR	 3
#define	 PRWAIS_OUT_OF_MEMORY	 4
#define	 PRWAIS_DB_ERROR         5

extern	 url_t		*get_url PROTO((int, char *));
extern   char		*wais_retrieve PROTO((char *,int,char *,char *,char *,char *, char *));
extern	 int		prwais_do_search PROTO((char *,int,P_OBJECT,wais_search_req_t *,catalogs_list_t *, RREQ));
#if 1
extern void contentsAttrFromFreetext PROTO((const char *contents, PATTRIB *resattr));
extern void contentsAttrFromTemplate PROTO((char *contents, catalogs_list_t *cat, int expand, PATTRIB *resattr));
#else
extern	 status_t	breakout_template_to_new_contents_atl PROTO((PATTRIB *,char *, cat_type_t, template_aux_rec *, int, char *, char *));
#endif
extern   status_t       do_attr_value PROTO((char **, catalogs_list_t *, char *));



#endif
