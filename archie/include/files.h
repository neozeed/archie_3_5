/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _FILES_H_
#define	_FILES_H_
#include "ansi_compat.h"

#include "typedef.h"
#ifdef OLD_FILES
#  include "old-host_db.h"
#else
#  include "host_db.h"
#endif

extern		status_t	mmap_file PROTO(( file_info_t *, int));
extern		status_t	mmap_file_private PROTO(( file_info_t *, int));
extern		status_t	munmap_file PROTO((file_info_t *));
extern		status_t	open_file PROTO(( file_info_t *, int));
extern		status_t	close_file PROTO(( file_info_t *));
extern		file_info_t*	create_finfo PROTO((void));
extern		void		destroy_finfo PROTO(( file_info_t *));
extern	  status_t get_file_list PROTO((char *, char ***, char *, int *));
extern    status_t get_input_file PROTO((hostname_t, char *, index_t, char*(*file_funct)(), file_info_t*, hostdb_t*, hostdb_aux_t*, file_info_t*, file_info_t*,file_info_t*));

extern	  char *		tail PROTO((char *));
extern		char		*get_tmp_filename PROTO(( char *));

extern		status_t	unlock_db PROTO((file_info_t *));
extern status_t  get_port PROTO((access_comm_t,char *,int *));


extern status_t archie_fpcopy PROTO((FILE *, FILE *, int));
extern status_t archie_rename PROTO((pathname_t, pathname_t));


#endif
