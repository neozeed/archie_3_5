#ifndef _FTP_GETFILE_H_
#define _FTP_GETFILE_H_

#include "header.h"

typedef struct{

   access_methods_t	access_methods;
   os_type_t		os_type;
   char			bin_access[MAX_ACCESS_METHOD];
   char			def_compress_ext[MAX_ACCESS_METHOD];
   char			def_user[MAX_ACCESS_METHOD];
   char			def_pass[MAX_ACCESS_METHOD];
   char			def_acct[MAX_ACCESS_METHOD];
   char			def_ftparg[MAX_ACCESS_METHOD];
   char			def_ftpglob[MAX_ACCESS_METHOD];
   pathname_t		def_indexfile;
} retdefs_t;

#ifndef DEFAULT_RETDEFS
#define DEFAULT_RETDEFS 	"arretdefs.cf"
#endif

#ifndef MAX_RETDEFS
#define MAX_RETDEFS		20
#endif

#ifndef MAX_RETRIEVE_RETRY	
#define	MAX_RETRIEVE_RETRY	2
#endif

#ifndef RETRY_DELAY
#define RETRY_DELAY		5 * 60		/* 5 Minutes */
#endif

#define	 DEFAULT_TIMEOUT	15 * 60	     /* 15 minutes */

#ifndef MIN_LSLR_SIZE
#define MIN_LSLR_SIZE		100	     /* Must be 100 bytes long */
#endif

#define IGNORE_FILE		"*IGNORE*"
#define ATEND_FILE		"*ATEND*"

#define MAX_DEF_FILES		100
#define DEF_INCR		200


extern	status_t	read_retdefs PROTO((file_info_t *, retdefs_t *));
extern	status_t	do_retrieve PROTO((header_t *,file_info_t *, file_info_t *, retdefs_t *, int, int, int, int));
extern	status_t	get_input PROTO((file_info_t *,header_t *,int,int,int));
extern	void		error_header PROTO((va_alist));
#ifndef SOLARIS
extern	void		sig_handle PROTO((int, int, struct sigcontext *, char *));
#else
extern	void		sig_handle PROTO((int));
#endif
extern	status_t	get_files PROTO((file_info_t *, FILE *, FILE *, header_t *, hostname_t, ip_addr_t, int, retdefs_t *, int, char **, int));
extern	status_t	get_list PROTO((file_info_t *, FILE *, FILE *, header_t *, hostname_t, ip_addr_t, int, retdefs_t *, int));
extern	status_t	get_pwd PROTO((file_info_t *, FILE *, FILE *, header_t *, hostname_t, ip_addr_t, retdefs_t *, int, pathname_t));
extern	char*		get_conn_err PROTO((int));
extern	status_t	gather_atend PROTO((char ***,file_info_t *));
extern  int		check_for_lslRZ PROTO((FILE *, FILE *,char **,header_t *, int, retdefs_t *));

#endif
