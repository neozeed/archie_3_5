/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _MISC_H_
#define _MISC_H_

#include "typedef.h"

#ifdef __STDC__


	  
	  

extern		int		initskip( char *, int, int);
extern		int		strfind( char *, int );
extern		status_t	delete_hostdb_entry( void *, int, file_info_t *);





#else

extern	        int		getopt();
extern		int		atoi();




extern		char *		get_master_db_dir();
extern		char *		set_master_db_dir();
extern		time_t		cvt_to_inttime();
extern		char		*cvt_from_inttime();
extern		int		strrcmp();
extern		status_t	mmap_file();
extern		status_t	munmap_file();
extern		struct in_addr	ipaddr_to_inet();
extern		status_t	open_file();
extern		status_t	close_file();
extern		char*		make_lcase();
extern		int		initskip();
extern		int		strfind();
extern		status_t	get_dbm_entry();
extern		status_t	put_dbm_entry();
extern		file_info_t*	create_finfo();
extern		void		destroy_finfo();
extern		status_t	str_decompose();
extern	        void		insert_char();
extern	        char **		str_sep();
extern		char *		get_archie_home();
extern	        char *		tail();
extern	        char *		cvt_to_usertime();
extern	        void		free_opts();
extern	        status_t	get_file_list();

#endif

#endif
