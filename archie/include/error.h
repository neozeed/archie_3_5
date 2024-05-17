/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _ERROR_H
#define _ERROR_H
#include "ansi_compat.h"
 
#include "typedef.h"


#ifdef DEBUGGING
#define derror(x) error x
#else
#define derror(x)
#endif

typedef enum 
{
    A_INFO = 1000,
    A_WARN,
    A_SYSWARN,
    A_INTERR,
    A_ERR,
    A_SYSERR,
    A_FATAL,
    A_SILENT
}Etype;


#ifndef	DEFAULT_LOG_DIR
#define	DEFAULT_LOG_DIR 	"logs"
#endif

#ifndef	DEFAULT_LOG_FILE
#define	DEFAULT_LOG_FILE 	"archie.log"
#endif


extern	status_t	open_alog PROTO(( char *, Etype, char * ));
extern	status_t	close_alog PROTO((void));
#ifdef __STDC__
extern	void		error (Etype, ...);
#else
extern	void		error PROTO(());
#endif
/* extern	void		debug PROTO(()); */
extern	char *		get_archie_logname PROTO((void));

#endif
