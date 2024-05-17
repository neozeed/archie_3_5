#ifndef _CONTROL_H_
#define _CONTROL_H_

#include <signal.h>
#include "host_db.h"

#define	 NONE_MAN	0
#define	 RETRIEVE_MAN	1
#define	 PARSE_MAN	2
#define	 UPDATE_MAN	3
#define  PARTIAL_MAN 4

#define	 CNTL_DONT_WAIT	1
#define	 CNTL_WAIT	2

#define	 DEFAULT_MAX_COUNT	 30


#define	 DEFAULT_TIMEOUT	 10 * 60  /* 10 Minutes */
#define  DEFAULT_SLEEP_PERIOD 5     /* 5 seconds */

#define	 MAX_FAIL		  2

#ifndef POST_PROCESS_PGM
#define POST_PROCESS_PGM	 "archie_postprocess"
#endif

#define	PROCESS_STOP_FILE	 "process.stop"

typedef struct{
   access_methods_t dbname;
   hostname_t	    primary_hostname;
   pathname_t	    input;
   pathname_t	    output;
   pathname_t	    proc_prog;
   int		    pid;
} retlist_t;

extern	 status_t	preprocess_file PROTO((char *, char **, retlist_t *, int, int));
extern	 status_t	cntl_function PROTO((retlist_t *, int, int, int, int, int, int, int));
extern	 host_status_t	cntl_check_hostdb PROTO((file_info_t *,file_info_t *,file_info_t *,header_t *, hostdb_t *, int,int));
extern	 status_t	write_hostdb_failure PROTO((file_info_t *,file_info_t *,header_t *,int,int));


#endif
