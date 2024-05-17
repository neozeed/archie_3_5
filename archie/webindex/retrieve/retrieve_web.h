#ifndef _RETRIEVE_GOPHER_H_
#define _RETRIEVE_GOPHER_H_

#include "menu.h"

#define	 DEFAULT_TIMEOUT      5 * 60   /* 5 minutes */
#define  DEFAULT_SLEEP_PERIOD 5        /* 5 seconds */

#define	 DATA_END	      ".\r\n"

#define	 HASH_TSIZE	      10000

#ifndef MAX_RETRIEVE_RETRY	
#define	MAX_RETRIEVE_RETRY	2
#endif

#ifndef RETRY_DELAY
#define RETRY_DELAY		5 * 60		/* 5 Minutes */
#endif

#define	 GOPHERPLUS_INIT_STR  "\t!"
#define	 GOPHERPLUS_ACK	      "+"
#define	 GOPHERPLUS_NACK      "-"
#define	 GOPHERPLUS_VERONICA  "+VERONICA"
#define	 GOPHERPLUS_TREEWALK  "treewalk"
#define	 GOPHERPLUS_YES	      "yes"
#define	 GOPHERPLUS_RECURSE   "Directory/recursive:"
#define	 GOPHERPLUS_DO_RECURSE   "\t+Directory/recursive"
#define	 GOPHERPLUS_OK1	      "-1"
#define	 GOPHERPLUS_OK2	      "-2"

#define	 SYS_ERROR	      -2
#define	 GOPHERPLUS_ERROR     -1
#define	 DO_INDEX	       0
#define	 DONT_INDEX	       1
#define	 NOT_GOPHERPLUS	       2
#define	 GOPHERPLUS_MANUAL     3

extern	 char*		get_gline PROTO((char *,int,FILE *,int));
extern	 status_t	open_gconnection PROTO((header_t *,file_info_t *,file_info_t *,int,int,FILE **,FILE **));
extern	 status_t	do_retrieve PROTO((header_t *,file_info_t *, int, int, int));
extern	 int		do_gopherplus PROTO((FILE *,FILE *, int));
extern	 int		tree_recurse PROTO((menu_t *, char *,file_info_t *, int));
extern	 void		sig_handle PROTO((int));


#endif


