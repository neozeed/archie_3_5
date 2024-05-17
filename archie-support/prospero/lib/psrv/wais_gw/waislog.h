/*****************************************************************************
* 	        (c) Copyright 1992 Wide Area Information Servers, Inc        *
*	   	  of California.   All rights reserved.   	             *
*									     *
*  This notice is intended as a precaution against inadvertent publication   *
*  and does not constitute an admission or acknowledgement that publication  *
*  has occurred or constitute a waiver of confidentiality.		     *
*									     *
*  Wide Area Information Server software is the proprietary and              *
*  confidential property of Wide Area Information Servers, Inc.              *
*****************************************************************************/

#ifndef waislog_h
#define waislog_h

#include <stdarg.h>

#include "cutil.h"

/*-------------------------------------------------------------------------- */
/*-------------------------------------------------------------------------- */

#define WLOG_HIGH	(1L)
#define WLOG_MEDIUM 	(5L)
#define WLOG_LOW	(9L)

#define WLOG_CONNECT	(1L)
#define WLOG_CLOSE	(2L)
#define WLOG_SEARCH	(3L)
#define WLOG_RESULTS	(4L)
#define WLOG_RETRIEVE	(5L)
#define WLOG_INDEX	(6L)
#define WLOG_PARSE	(7L)
#define WLOG_INFO	(100L)
#define WLOG_ERROR	(-1L)
#define WLOG_WARNING	(-2L)

#define logToStderr(log) (log == NULL || log[0] == '\0')
#define logToStdout(log) (log[0] == '-' && log[1] == '\0')
#define dbg (expertUser && (logToStderr(logFile) || logToStdout(logFile))) 

void waislog(long priority, long code, char* format,...);
void waisLogDetailed(long priority,long pid,long lineNum,long code,
		     char* format,...);

#ifndef waislog_c
extern long log_line;
extern wais_log_level;
extern char* logFile;
extern boolean expertUser;
#endif /* ndef waislog_c */

/*-------------------------------------------------------------------------- */

#endif /* ndef waislog_h */



