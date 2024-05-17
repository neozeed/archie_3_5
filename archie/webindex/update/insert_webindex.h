#ifndef _INCLUDE_H_
#define _INCLUDE_H_
#include <limits.h>
#include "defines.h"
#include "typedef.h"
#include "header.h"
#include "host_db.h"
#include "archstridx.h"
#include "patrie.h"

typedef struct{
    index_t	new_record_no;
    index_t     strings_idx;
    int     status;
    index_t     parent_idx;
/*    site_entry_ptr_t site_rec; */
    char        *instring;
} outlist__t;

typedef outlist__t * outlist_t;

struct tmp_list{
    index_t recno;
    struct tmp_list *next;
};

typedef struct tmp_list tmp_list_t;


#define	STRING_NOT_FOUND (int) 0
#define	STRING_FOUND (int) 1
#define PRNT_STRING  (int) -1
#define ACTIVE_STRING "A"

extern	status_t	setup_output_file PROTO((file_info_t *, header_t *, char *));
extern	status_t	reopen_output_file PROTO((file_info_t *));
extern	int		outlist_compare PROTO((outlist_t, outlist_t));
extern	status_t	setup_hash_table PROTO((int));
extern	status_t	add_tmp_string PROTO((char *, index_t));
extern	status_t	setup_insert PROTO((struct arch_stridx_handle *, char *, full_site_entry_t *, int , int * , int ));
extern	status_t	inactive_or_new PROTO((full_site_entry_t *,int,ip_addr_t,outlist_t,file_info_t *));
extern	status_t	do_internal PROTO((full_site_entry_t *,int,outlist_t,ip_addr_t,file_info_t *));
extern	status_t	make_links  PROTO((full_site_entry_t *,int,outlist_t,ip_addr_t,file_info_t *,  file_info_t *, file_info_t *, int));

#endif
