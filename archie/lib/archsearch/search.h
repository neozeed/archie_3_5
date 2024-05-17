#ifndef _ARCHSEARCH_H_
#define	_ARCHSEARCH_H_
#include <limits.h>
#include "defines.h"
#include "archie_catalogs.h"
#include "db_ops.h"
#include "start_db.h"
#include "webindexdb_ops.h"
#include "typedef.h"
#include "header.h"
#include "host_db.h"
#include "archstridx.h"
#include "patrie.h"
#include "site_file.h"
#include "core_entry.h"
#include "excerpt.h"

typedef struct{
  flags_t flags;
  struct{
    struct{
      perms_t perms;
      file_size_t size;
      date_time_t date;
    } file;
    struct{
      double weight;
    } kwrd;
  } type;
} details_t;

typedef enum{
  INDX = 3,
  UNINDX = 2,
  NOT_INDX = 1,
} url_type_t;

struct query_result__t {
  url_type_t type;
  char qrystr[MAX_PATH_LEN];
  char *str;
  details_t details;
  excerpt_t excerpt;
  ip_addr_t ipaddr;
};

typedef struct query_result__t  query_result_t;

typedef struct{
  int stop;
  index_t string;
  index_t site_stop;
  index_t site_prnt;
  index_t site_file;
} start_return_t;

typedef enum{
  AND_OP = 3,
  OR_OP = 2,
  NOT_OP = 1,
  NULL_OP = 0,
} bool_ops_t;

typedef struct{
  char **lwords;
  int lnum;
} bool_list_t;

typedef struct{
  char *string;
  bool_list_t *and_list;
  int orn;
  bool_list_t *not_list;
} bool_query_t;

typedef struct{
  host_table_index_t site;
  index_t child;
  index_t file;
} site_tuple_t;

typedef struct{
  char *string;
  int now;
  bool_ops_t *ops;
  char **words;
} query_t;

typedef struct {
    char *name;
    char *val;
} entry;


typedef struct{
  int and_tpl;            /* and/not tuple */
  int site_in_list;       /* site in list */
  int acc_res;            /*
                           *  for the result_tuples alredy found for the
                           *  above how many are accepted to be printed out.
                           *  This will let us know where to start from.
                           */
} boolean_return_t;

#define EXCERPT_SUFFIX ".excerpt"

#ifdef __STDC__

extern	 status_t strOnlyQuery PROTO((struct arch_stridx_handle *, char *,
                                      int, int, int, int));
extern	 status_t archQuery PROTO((struct arch_stridx_handle *, char *, int,
                                   int, int, int , int, char **, char *,int,
                                   int, domain_t *, int, int, file_info_t *,
                                   int,query_result_t **, index_t ***, int *,
                                   start_return_t *,char*(*func_name)(), int,
                                   int));
extern	 status_t archQueryMore PROTO((struct arch_stridx_handle *, int, int,
                                       int, char **, char *,int, int, domain_t *,
                                       int, int, file_info_t *, int,
                                       query_result_t **, index_t ***, int *,
                                       start_return_t *, char*(*func_name)(),
                                       int, int));
extern	 status_t archWebBooleanQueryMore PROTO((struct arch_stridx_handle *,
                                                 bool_query_t, int, int, int,
                                                 int,int,char **, char *,int,
                                                 int, domain_t *, int, int,
                                                 file_info_t *, int,
                                                 query_result_t **,index_t ***,
                                                 int *, boolean_return_t *,
                                                 char*(*func_name)(),
                                                 int, int));
extern	 status_t archBooleanQueryMore PROTO((struct arch_stridx_handle *,
                                              bool_query_t, int, int, int,
                                              int,int,char **, char *,int,
                                              int, domain_t *, int, int,
                                              file_info_t *, int,
                                              query_result_t **,index_t ***,
                                              int *, boolean_return_t *,
                                              char*(*func_name)(),
                                              int, int));
extern   status_t getResultFromStart PROTO(( index_t, struct arch_stridx_handle *,
                                            start_return_t *, int, ip_addr_t,
                                            int, int *, int, char **, char *,
                                            int, int,query_result_t * ));
extern   status_t getStringsFromStart PROTO(( index_t, struct arch_stridx_handle *,
                                             start_return_t *, int, ip_addr_t,
                                             int, int *, int, char **, char *,
                                             int, int,query_result_t *,index_t **));
extern   status_t search_site_index PROTO((file_info_t *, index_t, int **,
                                           int *));
extern   void handle_entry PROTO((file_info_t *,  full_site_entry_t *, int ,
                                  int,  query_result_t *));
extern   void handle_key_prnt_entry PROTO((file_info_t *,  full_site_entry_t *,
                                           int , int,  query_result_t *));
extern   status_t handle_chain PROTO((file_info_t *,  struct arch_stridx_handle *,
                                      index_t , index_t,  int,  ip_addr_t,
                                      int,  int,  query_result_t *));
extern   void handle_partial_entry PROTO((file_info_t *, full_site_entry_t *,
                                          int, query_result_t *));
extern   int check_correct_path PROTO((char *, char **, char *, int , int));

#else

extern	 status_t strOnlyQuery PROTO(());
extern	 status_t archQuery PROTO(());
extern	 status_t archQueryMore PROTO(());
extern	 status_t archWebBooleanQueryMore PROTO(());
extern	 status_t archBooleanQueryMore PROTO(());
extern   status_t getResultFromStart PROTO(());
extern   status_t search_site_index PROTO(());
extern   void handle_entry PROTO(());
extern   status_t handle_chain PROTO(());
extern   void handle_partial_entry PROTO(());
extern   int check_correct_path PROTO(());

#endif

#endif
