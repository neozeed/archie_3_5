/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include "error.h"
#include "misc.h"
#include "times.h"
#include "master.h"
#include "files.h"

#include "protos.h"

static file_info_t log;
static Etype alog_level;
static pathname_t prog_name;


/*
 * Common interface to print error messages, warnings, etc.  It either
 * writes to stderr or to a log file depending on if the log was opened or
 * not.
 */


/*
 * Get the name of the current log file
 */

char *get_archie_logname()

{
   static char logname[MAX_PATH_LEN];

#if 0
   if(log.filename[0] == '\0')
      sprintf(log.filename, "%s/%s/%s", get_archie_home(), DEFAULT_LOG_DIR, DEFAULT_LOG_FILE);
#endif

   strcpy(logname, log.filename);
   return(logname);

}

/*
 * error: Central error notification routine.
 *
 * called as error(error_type,* subroutine_name, format_string, arg [,arg...]);
 *
 *
 */
   
/*  
 *  These routines seem like fairly old versions, there may be bugs.
 *  
 *  - wheelan -- Sat Apr 9 01:10:47 EDT 1994
 */  
#ifdef __STDC__
void error(  Etype etype , ...)
{
  extern int errno;
  extern int sys_nerr;
  extern char *sys_errlist[];
#ifndef __STDC__
  extern int getpid();
#endif
  extern int vfprintf();

  char *estr ;
  char format[ 512 ] ;          /* use malloc? */
  char *sub_name ;

  FILE *fp ;
  int errno_copy ;
  va_list al ;

  errno_copy = errno;
    
  va_start(al,etype);
/*  etype = va_arg(al, Etype);  */
  sub_name = va_arg(al, char *); 

  switch(etype)
  {
	case A_FATAL:   fp = stderr; estr = "FATAL";               break;
	case A_ERR:     fp = stderr; estr = "ERROR";               break;
  case A_INFO:    fp = stderr; estr = "INFO";                break;
  case A_INTERR:  fp = stderr; estr = "INTERNAL ERROR";      break;
  case A_SYSERR:  fp = stderr; estr = "SYSTEM CALL ERROR";   break;
  case A_SYSWARN: fp = stderr; estr = "SYSTEM CALL WARNING"; break;
  case A_WARN:    fp = stdout; estr = "WARNING";             break;
  case A_SILENT:  fp = stdout; estr = "";                    break;
  default:                      /* hee, hee */
    fprintf(stderr, "# INTERNAL ERROR: error: unknown error type, '%d'.\n",
            (int)etype);
    return;
  }

  /* Set up output string */

  if (log.fp_or_dbm.fp != (FILE *)NULL)
  {
    fp = log.fp_or_dbm.fp;
    sprintf(format, "%s (%d) %s %s: %s: %s", prog_name, getpid(),
            cvt_to_usertime(time((time_t *) NULL),1), estr, sub_name, va_arg(al, char *)) ;
  }
  else
  {
    sprintf(format, "# %s: %s: %s", estr, sub_name, va_arg(al, char *));
  }

  if (etype == A_SYSERR)
  {
    if (errno_copy >= sys_nerr)
    {
	    strcat(format, ": unlisted error"); /* should give the number */
    }
    else
    {
	    strcat(format, ": " );
	    strcat(format, sys_errlist[errno_copy]);
    }
  }

  strcat(format, "\n");

  /* Lock the log file for exclusive access */

#if 0
  lockf(fileno(fp), F_LOCK, 0);
#endif

  vfprintf(fp, format, al);
  fflush(fp);

  /* Unlock it */
#if 0
  lockf(fileno(fp), F_ULOCK, 0);
#endif
  va_end(al);
}
#else
void error(va_alist)
  va_dcl
{
  extern int errno;
  extern int sys_nerr;
  extern char *sys_errlist[];
#ifndef __STDC__
  extern int getpid();
#endif
  extern int vfprintf();

  char *estr ;
  char format[ 512 ] ;          /* use malloc? */
  char *sub_name ;
  Etype etype ;
  FILE *fp ;
  int errno_copy ;
  va_list al ;

  errno_copy = errno;
    
  va_start(al);
  etype = va_arg(al, Etype);
  sub_name = va_arg(al, char *);

  switch(etype)
  {
	case A_FATAL:   fp = stderr; estr = "FATAL";               break;
	case A_ERR:     fp = stderr; estr = "ERROR";               break;
  case A_INFO:    fp = stderr; estr = "INFO";                break;
  case A_INTERR:  fp = stderr; estr = "INTERNAL ERROR";      break;
  case A_SYSERR:  fp = stderr; estr = "SYSTEM CALL ERROR";   break;
  case A_SYSWARN: fp = stderr; estr = "SYSTEM CALL WARNING"; break;
  case A_WARN:    fp = stdout; estr = "WARNING";             break;
  case A_SILENT:  fp = stdout; estr = "";                    break;
  default:                      /* hee, hee */
    fprintf(stderr, "# INTERNAL ERROR: error: unknown error type, '%d'.\n",
            (int)etype);
    return;
  }

  /* Set up output string */

  if (log.fp_or_dbm.fp != (FILE *)NULL)
  {
    fp = log.fp_or_dbm.fp;
    sprintf(format, "%s (%d) %s %s: %s: %s", prog_name, getpid(),
            cvt_to_usertime(time((time_t *) NULL),1), estr, sub_name, va_arg(al, char *)) ;
  }
  else
  {
    sprintf(format, "# %s: %s: %s", estr, sub_name, va_arg(al, char *));
  }

  if (etype == A_SYSERR)
  {
    if (errno_copy >= sys_nerr)
    {
	    strcat(format, ": unlisted error"); /* should give the number */
    }
    else
    {
	    strcat(format, ": " );
	    strcat(format, sys_errlist[errno_copy]);
    }
  }

  strcat(format, "\n");

  /* Lock the log file for exclusive access */

#if 0
  lockf(fileno(fp), F_LOCK, 0);
#endif

  vfprintf(fp, format, al);
  fflush(fp);

  /* Unlock it */
#if 0
  lockf(fileno(fp), F_ULOCK, 0);
#endif
  va_end(al);
}
#endif

/*
 * open_alog: Open the log file
 */


status_t open_alog(a_logfile, log_level, program_name)
   char *a_logfile;	/* log file name */
   Etype log_level;	/* Minimal level at which to log */
   char *program_name;	/* Name of current program */

{
   if(log.fp_or_dbm.fp != (FILE *) NULL){
      error(A_ERR,"open_alog","Attempt to open log that is already open already open");
      return(ERROR);
   }

   alog_level = log_level;

   if(a_logfile == (char *) NULL)
      sprintf(log.filename, "%s/%s/%s", get_archie_home(), DEFAULT_LOG_DIR, DEFAULT_LOG_FILE);
   else
      strcpy(log.filename, a_logfile);

   if((log.fp_or_dbm.fp = fopen( log.filename, "a")) == (FILE *) NULL){
      error(A_ERR,"open_alog", "Can't open log file %s", log.filename);
      return(ERROR);
   }

   if(fcntl(fileno(log.fp_or_dbm.fp), F_SETFL, O_NONBLOCK | O_APPEND) == -1){
      error(A_ERR,"open_alog", "Can't set file options on log file %s", log.filename);
      return(ERROR);
   }

   strcpy(prog_name, program_name);

   /* Redirect stderr to the log file */

   if ( freopen(log.filename, "a", stderr) == NULL ) {
      error(A_ERR,"open_alog", "Can't redirect stderr to the log file %s", log.filename);
      return(ERROR);
   }

   setbuf(stderr,NULL); /* Make stderr unbuffered */
   
   return(A_OK);
}


/*
 * close_alog: close log file
 */

status_t close_alog()

{
   if(log.fp_or_dbm.fp == (FILE *) NULL){
      error(A_INTERR,"close_alog","Attempt to close log file that is not open");
      return(ERROR);
   }

   if(close_file(&log) == ERROR){
      error(A_SYSERR,"close_alog","Error trying to close log %s", log.filename);
      return(ERROR);
   }

   memset(&log, '\0', sizeof(file_info_t));

   return(A_OK);
}

