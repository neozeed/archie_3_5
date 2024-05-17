/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 *
 * Credits:  Originally written by Clifford Neuman (University of Washington)
 *           Syslog support added by Jonathan Kamens (MIT Project Athena)
 *           Much code mangling by Steven Augart (USC/ISI)
 */

#include <usc-license.h>

#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>

#include <ardp.h>
#include <pfs.h>
#include <pserver.h>
#include <plog.h>
#include <pmachine.h>
/* This definition has to be after pmachine.h so that SCOUNIX gets defined */
#if  defined(AIX) || defined (SCOUNIX)
#include <time.h>
#endif

#define	logfile	P_logfile		/* So as not to conflict with WAIS variable --- bajan */
    
/* this array contains info on the type of entries to be printed */
static int 		logtype_array[NLOGTYPE+1] = INITIAL_LOG_VECTOR;

/* info on the filename and open file */
static char 		*log_name = PSRV_LOGFILE;
FILE 			*logfile;
static int 		is_open = 0; /* Mutexed below */
#if 0
/* not currently used */
static int		syslog_open = 0;
#endif

static char *pr_inet_ntoa();
static FILE *plog_additional_outfile = NULL;

/*VARARGS4*/

/*
 * plog - Add entry to logfile
 *
 * 	  PLOG is used to add entries to the logfile.  Note that
 * 	  it is probably not portable since is makes assumptions
 * 	  about what the compiler will do when it is called with
 * 	  less than the correct number of arguments which is the
 * 	  way it is usually called.
 *
 * 	  PLOG returns a pointer to the logged entry.  If an error
 * 	  occurs, vlog returns immediately, but does not indicate 
 * 	  that the log operating failed.
 *
 *    ARGS: type    - Type of entry (to decide if we should log it)
 *	    req     - Pointer to request info including host address,
 *                    and useride. (NULL if should not be printed)
 *          format  - Format as for qsprintf
 *          remaining arguments -- as for qsprintf
 * RETURNS: Pointer to a string containing the log entry
 *
 *    BUGS: The non-ANSI implementation is not portable.  It should really use
 *          another mechanism to support a variable number of arguments.  
 *          Unfortunately, these mechanisms are not easily available.
 *
 *          Currently, the log file is opened and closed on each
 *	    call.
 */

EXTERN_MUTEX_DECL(PSRV_LOG);

/* Call this if mutexed, but not open */

#define LEAVEOPEN 1

char *
vplog(int type, RREQ req, char *format, va_list ap)
{
    time_t now, systime, svctime, wttime;
    int	 log_username = 0;
    int  notfirstfield = 0;
    char *month_sname();
    char usertxt[MXPLOGENTRY];
    char fieldtxt[MXPLOGENTRY];

    AUTOSTAT_CHARPP(logtxtp);

    CHECK_MEM();

    *logtxtp = vqsprintf_stcopyr(*logtxtp, format, ap);

    /* If we don't log this type of message, don't write to log */
    if (!logtype_array[type])
	return(*logtxtp);

    /* get the time */
#ifndef NDEBUG
    { int retval = 
#endif
	time(&now);
#ifndef NDEBUG
      assert(retval != -1);
    }
#endif


    svctime = systime = wttime = 0;
    if(req) {
	if(req->rcvd_time.tv_sec) 
	    wttime = systime = now - req->rcvd_time.tv_sec;

	if(req->svc_start_time.tv_sec) {
	    svctime = now - req->svc_start_time.tv_sec;
	    wttime = req->svc_start_time.tv_sec - req->rcvd_time.tv_sec; 
	}
    }

    if ((type == L_QUEUE_COMP) && (systime < L_COMP_SYS_THRESHOLD))
	return(*logtxtp);

    if ((type == L_QUEUE_COMP) && (svctime < L_COMP_SVC_THRESHOLD))
	return(*logtxtp);

    *usertxt = '\0';

    if(req && req->peer_addr.s_addr &&(logtype_array[L_FIELDS]&L_FIELDS_HADDR))
	strncat(usertxt, pr_inet_ntoa(req->peer_addr.s_addr),sizeof(usertxt));

    if(type == L_DIR_UPDATE) {
	if(logtype_array[L_FIELDS] & L_FIELDS_USER_U) log_username++;
    }
    else if(type == L_DIR_REQUEST) {
	if(logtype_array[L_FIELDS] & L_FIELDS_USER_R) log_username++;
    }
    else if(logtype_array[L_FIELDS] & L_FIELDS_USER_I) log_username++;

    if(req && req->client_name && *(req->client_name) && log_username) {
	strncat(usertxt, "(",sizeof(usertxt));
	strncat(usertxt, req->client_name,sizeof(usertxt));
 	strncat(usertxt, ")",sizeof(usertxt));
    }

    if(req && req->peer_sw_id && *(req->peer_sw_id) && 
       (logtype_array[L_FIELDS] & L_FIELDS_SW_ID)) {
	strncat(usertxt, "[",sizeof(usertxt));
	strncat(usertxt, req->peer_sw_id,sizeof(usertxt));
 	strncat(usertxt, "]",sizeof(usertxt));
    }

    if(req && (logtype_array[L_FIELDS] & L_FIELDS_PORT)){
	qsprintf(fieldtxt, sizeof fieldtxt, "[udp/%d]", PEER_PORT(req));
	strncat(usertxt, fieldtxt,sizeof(usertxt));
    }

    if(req && req->cid &&(logtype_array[L_FIELDS] & L_FIELDS_CID)) {
	qsprintf(fieldtxt, sizeof fieldtxt, "[cid=%d]", ntohs(req->cid));
	strncat(usertxt, fieldtxt,sizeof(usertxt));
    }

    if(req && (logtype_array[L_FIELDS] & L_FIELDS_STIME) &&
       ((systime>=L_SYSTIME_THRESHOLD) || (svctime>=L_SVCTIME_THRESHOLD) ||
	(wttime>=L_WTTIME_THRESHOLD))) {
	strncat(usertxt, "[",sizeof(usertxt));
	if(wttime >= L_WTTIME_THRESHOLD) {
	    if(notfirstfield++) strncat(usertxt, ",",sizeof(usertxt));
	    qsprintf(fieldtxt, sizeof fieldtxt, "%d:%02dwt", wttime / 60, wttime % 60);
	    strncat(usertxt, fieldtxt,sizeof(usertxt));
	}
	if(svctime >= L_SVCTIME_THRESHOLD) {
	    if(notfirstfield++) strncat(usertxt, ",",sizeof(usertxt));
	    qsprintf(fieldtxt, sizeof fieldtxt,
                     "%d:%02dsvc", svctime / 60, svctime % 60);
	    strncat(usertxt, fieldtxt,sizeof(usertxt));
	}
	if(systime >= L_SYSTIME_THRESHOLD) {
	    if(notfirstfield++) strncat(usertxt, ",",sizeof(usertxt));
	    qsprintf(fieldtxt, sizeof fieldtxt, "%d:%02dsys", systime / 60, systime % 60);
	    strncat(usertxt, fieldtxt,sizeof(usertxt));
	}
	strncat(usertxt, "]",sizeof(usertxt));
    }

#ifdef P_LOGTO_SYSLOG
    if(!syslog_open++) openlog("prospero",LOG_PID|LOG_ODELAY,LOG_PROSPERO);

    if (logtype_array[type] & ~PLOG_TOFILE_ENABLED) {
	syslog(logtype_array[type] & ~PLOG_TOFILE_ENABLED, 
	       "%s%s%s", usertxt, (*usertxt ? " " : ""), *logtxtp);
    }
#endif

    /* If not printing to file return */
    if (! (logtype_array[type] & PLOG_TOFILE_ENABLED)) return(*logtxtp);

#ifndef LEAVEOPEN
    p_th_mutex_lock(p_th_mutexPSRV_LOG);
#endif
    if (!is_open) {
#ifdef LEAVEOPEN
      p_th_mutex_lock(p_th_mutexPSRV_LOG);
#endif
      if (!is_open) {  /* Check still open, now we have mutex*/
	if ((logfile = fopen(log_name,"a")) != NULL) {
	    is_open = 1;
    }
      }
#ifdef LEAVEOPEN
      p_th_mutex_unlock(p_th_mutexPSRV_LOG);
#endif
    }

    if (is_open) {

	/* print the log entry */
        AUTOSTAT_CHARPP(bufp);
#ifndef PFS_THREADS
	struct tm *tm = localtime(&now);
#else
        struct tm tmstruc;
	struct tm *tm = &tmstruc;
	localtime_r(&now, tm);
#endif
        *bufp = qsprintf_stcopyr(*bufp, "%2d-%s-%02d %02d:%02d:%02d %s%s%s\n", 
                                 tm->tm_mday,
                                 month_sname(tm->tm_mon + 1),tm->tm_year,
                                 tm->tm_hour, tm->tm_min, tm->tm_sec,
                                 usertxt, (*usertxt ? " - " : ""), *logtxtp);
#ifndef NDEBUG
    {                           /* variable scope start */
        int retval =
#endif
        fputs(*bufp, logfile);
#ifndef NDEBUG
	    assert(retval == strlen(*bufp));
    }                           /* variable scope end */
#endif
    }
    /* even if the primary logfile couldn't be opened, go ahead and log to
	the manual (additional) logfile. */
    if(plog_additional_outfile) {
#if 0
        /* This is old equivalent code.  Leaving it in here just for historical
           reasons -- no real reason to keep it, unless one is concerned about
           runnuing out of memory. */
        fprintf(plog_additional_outfile, 
                "%s%s%s\n", usertxt, (*usertxt ? " - " : ""), *logtxtp));
#else
        AUTOSTAT_CHARPP(bufp);
        *bufp = qsprintf_stcopyr(*bufp, "%s%s%s\n",
                                 usertxt, (*usertxt ? " - " : ""), *logtxtp);
#ifndef NDEBUG
	{ int retval = 
#endif
              fputs(*bufp, plog_additional_outfile);
#ifndef NDEBUG
	  assert(retval == strlen(*bufp));
	}
#endif
#endif
        zz(fflush(plog_additional_outfile)); /* must be zero */
    }

#ifndef LEAVEOPEN
    if (is_open) {
      zz(fclose(logfile));
      is_open = 0;
    }
    p_th_mutex_unlock(p_th_mutexPSRV_LOG);
#else
    zz(fflush(logfile));
#endif /*LEAVEOPEN*/
    return(*logtxtp);

}

char *
plog(int type,                  /* Type of log entry	    */
     RREQ req,                  /* Optional info on request */
     char *format, ...)         /* format string            */
{
    char *retval;
    va_list ap;

    va_start(ap, format);
    retval = vplog(type, req, format, ap);
    va_end(ap);
    return retval;
}


/*
 * set_logfile - Change the name of the logfile
 *
 *		  SET_LOGFILE changes the name of the file to which
 * 		  messages are logged.  If set_logfile is not called,
 * 		  the logfile defaults to PFSLOG.
 *
 *         ARGS:  filename - Name of new logfile
 *      Returns:  success always
 */
/* Does not need to be mutexed; set only upon initialization */
int
set_logfile(char *filename)
{
    assert(P_IS_THIS_THREAD_MASTER());
    log_name = filename;
    if(is_open) (void) fclose(logfile);
    is_open = 0;
    return(PSUCCESS);
}

static char *
pr_inet_ntoa(long a)
{
    AUTOSTAT_CHARPP(astringp);

#if BYTE_ORDER == BIG_ENDIAN
    *astringp = qsprintf_stcopyr(*astringp,"%d.%d.%d.%d",(a >> 24) & 0xff,
                                 (a >> 16) & 0xff,(a >> 8) & 0xff, a & 0xff);
#else
    *astringp = qsprintf_stcopyr(*astringp,"%d.%d.%d.%d", a & 0xff,
                                 (a >> 8) & 0xff,(a >> 16) & 0xff, 
                                 (a >> 24) & 0xff); 
#endif
    return(*astringp);
}
		
void
close_plog(void)
{
    if(is_open) fclose(logfile);
    is_open = 0;

#ifdef P_LOGTO_SYSLOG
    if(syslog_open) closelog();
    syslog_open = 0;
#endif
}


void
plog_manual(FILE *outf)
{
    plog_additional_outfile = outf;
}
