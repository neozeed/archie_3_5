#ifndef _BOOLEANOPS_H_
#define	_BOOLEANOPS_H_
#include "search.h"

#ifdef __STDC__

extern void tuplecpy PROTO((site_tuple_t *,site_tuple_t *));
extern void unique_tuple PROTO((site_tuple_t **,int *));
extern void unique PROTO((index_t **,int *));
extern void unique_hindex PROTO((host_table_index_t **,int *));
extern void getLongestAndWord PROTO((bool_list_t, int *));
extern status_t archSiteIndexOper PROTO((host_table_index_t **, int ,
                                         host_table_index_t **, int ,
                                         bool_ops_t,
                                         host_table_index_t **, int *));
extern status_t archTupleOper PROTO((site_tuple_t **, int,
                                     site_tuple_t **, int,
                                     bool_ops_t,
                                     site_tuple_t **, int *));
extern status_t archAnonftpOper PROTO((index_t **,int *,
                                       bool_ops_t,char *,
                                       struct arch_stridx_handle *,
                                       int));
extern status_t archTupleConcat PROTO((site_tuple_t **, int,
                                     site_tuple_t **, int,
                                     site_tuple_t **, int *));
extern status_t archAnonftpOperOn2Lists PROTO((index_t **, int,
                                       index_t **, int,
                                       bool_ops_t,
                                       index_t **, int *));
extern status_t orSitesForStarts PROTO((index_t *, int,
                                        file_info_t *, int, int *,
                                        host_table_index_t **, domain_t *, int));
extern status_t archAnonftpOperCont PROTO((index_t *, int, int *, int *,
                                           bool_ops_t , char *,
                                           struct arch_stridx_handle *,
                                           int , index_t **, int *));
extern status_t archStartIndexConcat PROTO((index_t **, int, index_t **, int,
                                            index_t **,  int *));


#else

extern void tuplecpy PROTO(());
extern void unique_tuple PROTO(());
extern void unique PROTO(());
extern void unique_hindex PROTO(());
extern void getLongestAndWord PROTO(());
extern status_t archSiteIndexOper PROTO(());
extern status_t archTupleOper PROTO(());
extern status_t archAnonftpOper PROTO(());
extern status_t archTupleConcat PROTO(());
extern status_t archAnonftpOperOn2Lists PROTO(());
extern status_t orSitesForStarts PROTO(());
extern status_t archAnonftpOperCont PROTO(());
extern status_t archStartIndexConcat PROTO(());
#endif
#endif
