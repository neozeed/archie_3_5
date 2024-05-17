#ifndef _PARCHIE_SEARCH_FILES_DB_H_
#define _PARCHIE_SEARCH_FILES_DB_H_

#include "pfs.h"
/*
 *  #ifdef OLD_FILES # include "old-site_file.h" #else # include "site_file.h"
 *  #endif
 */
#include "site_file.h"
#include "patrie.h"
#include "archstridx.h"

extern	VLINK		atoplink PROTO((full_site_entry_t *, int, hostname_t, file_info_t *, char *, int));
extern	char**          find_ancestors PROTO((void *, full_site_entry_t *, file_info_t *));
extern	char**          new_find_ancestors PROTO((void *, full_site_entry_t *, struct arch_stridx_handle *));
#endif
