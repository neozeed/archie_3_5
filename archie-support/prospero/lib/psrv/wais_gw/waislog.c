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

#define waislog_c
#if !defined(IN_RMG) && !defined(PFS_THREADS)
#include "waislog.h"
#include "futil.h"

void waisLogInternal(long priority,long pid,long lineNum,long code,
		     char* format,va_list ap);

long log_line = 0;
long wais_log_level = 10;
char* logFile = NULL;
boolean expertUser = false;

/*----------------------------------------------------------------------*/

void
waislog(long priority,long code,char* format,...)

{
  va_list ap; 
  va_start(ap,format);
  initMyPID();
  waisLogInternal(priority,myPID,log_line++,code,format,ap);
  va_end(ap); 
}

/*----------------------------------------------------------------------*/

void
waisLogDetailed(long priority,long pid,long lineNum,long code,
		char* format,...)
{
  va_list ap; 
  va_start(ap,format);
  waisLogInternal(priority,pid,lineNum,code,format,ap);
  va_end(ap); 
}

/*----------------------------------------------------------------------*/

void
waisLogInternal(long priority,long pid,long lineNum,long code,
		char* format,va_list ap)
{
  if (priority <= wais_log_level) 
   { 
     FILE* log = NULL;

     if (logToStderr(logFile))
       log = stderr;
     else if (logToStdout(logFile))
       log = stdout;
     else
       log = locked_fopen(logFile,"a");
     
     if (log) 
      {
	fprintf(log,"%d: %d: %s: %d: ",pid,lineNum,
		printable_time(),code);

	vfprintf(log,format,ap); 
	fprintf(log,"\n");
	fflush(log);
	
	if (logToStderr(logFile) == false && logToStdout(logFile) == false)
	  locked_fclose(log,logFile,FALSE);

      }
   }
}
#endif /* !defined(IN_RMG) && !defined(PFS_THREADS)*/
/*-------------------------------------------------------------------------- */
