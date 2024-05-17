/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#ifndef PLOG_H_INCLUDED
#define PLOG_H_INCLUDED

/*
 * IMPORTANT: This file defines the configuration for the
 * information to be logged for each type of Prospero query.  As
 * distributed, Prospero is configured to log lots of information,
 * including the names of clients and the specific commands issued.  This
 * information should be considered sensitive and you must protect the
 * logfile.  Additionally, you should configure the server to log only
 * the information you actually need.  This can be accomplished by
 * copying the definition of INITIAL_LOG_VECTOR to <pserver.h> and editing it.
 */

/* This file will be preceded by <pserver.h>, which can contain definitions
   overriding some of the ones in the following file.  You should edit that
   file in preference to this one, so that when you upgrade to a later release
   of Prospero you will be able to use the stock version of plog.h and just
   change <pserver.h>.
 */
#include <stddef.h>
#include <stdarg.h>

#define PLOG_TOFILE_ENABLED  0x80000000
#define PLOG_TOFILE_DISABLED 0x00000000

/* 
 * If log messages are to be written to the prospero log file,
 * PLOG_TOFILE must be set to PLOG_TOFILE_ENABLED.  To disable logging
 * to the log file, it's value should be PLOG_TOFILE_DISABLED.  Note that
 * it is acceptable to log to both syslog and the log file.
 *
 * This can most conveniently be set by #defining PSRV_LOGFILE in the 
 * "include/pserver.h" configuration file.
 */
#ifndef PSRV_LOGFILE
#define PLOG_TOFILE PLOG_TOFILE_DISABLED
#define PSRV_LOGFILE  "pfs.nolog"  
#else
#define PLOG_TOFILE PLOG_TOFILE_ENABLED
#endif

/*
 * This following definitions define the types of log messages logged by 
 * plog.  The action to be taken for each event can be selectively
 * defined by setting the corresponding entry in INITIAL_LOG_VECTOR.
 */

#define MXPLOGENTRY	1000	/* Maximum length of single entry   */

#define	NLOGTYPE	25	/* Maximum number of log msg types  */

#define L_FIELDS	  0	/* Fields to include in messages    */
#define L_STATUS	  1	/* Startup, termination, etc        */
#define L_FAILURE	  2	/* Failure condition                */
#define L_STATS		  3	/* Statistics on server usage       */
#define L_NET_ERR	  4	/* Unexpected error in network code */
#define L_NET_RDPERR      5     /* Reliable datagram protocol error */
#define L_NET_INFO	  6	/* Info on network activity	    */
#define L_QUEUE_INFO      7     /* Info on queue managment          */
#define L_QUEUE_COMP	  8     /* Requested service completed      */
#define L_DIR_PERR	  9	/* PFS Directory protocol errors    */
#define L_DIR_PWARN      10	/* PFS Directory protocol warning   */
#define L_DIR_PINFO	 11	/* PFS Directory protocol info	    */
#define L_DIR_ERR	 12	/* PFS Request error		    */
#define L_DIR_WARN	 13	/* PFS Request warning		    */
#define L_DIR_REQUEST	 14	/* PFS information request	    */
#define L_DIR_UPDATE     15     /* PFS information update           */
#define L_AUTH_ERR       16     /* Unauthorized operation attempted */
#define L_DATA_FRM_ERR	 17     /* PFS directory format error       */
#define L_DB_ERROR	 18     /* Error in database operation      */
#define L_DB_INFO	 19     /* Error in database operation      */
#define L_ACCOUNT	 20     /* Accounting info. record          */
#define L_ERR_UNK  NLOGTYPE	/* Unknown error type		    */

/* Fields to include in log messages (L_FIELDS_HNAME not yet implemented) */
#define L_FIELDS_USER_R  0x01	/* Include user ID in request log message */
#define L_FIELDS_USER_U  0x02   /* Include user ID in update log messages */
#define L_FIELDS_USER_I  0x04   /* Include user ID in informational msgs  */
#define L_FIELDS_HADDR   0x08	/* Include host address in log messages   */
#define L_FIELDS_HNAME   0x10	/* Include host name in log messages      */
#define L_FIELDS_SW_ID   0x20	/* Include host name in log messages      */
#define L_FIELDS_CID     0x40	/* Include connection ID in log messages  */
#define L_FIELDS_PORT    0x80   /* Include UDP port number in log msgs    */
#define L_FIELDS_STIME  0x100   /* Include cuurent system time of rreq    */

#define L_FIELDS_USER	(L_FIELDS_USER_R|L_FIELDS_USER_U|L_FIELDS_USER_I)

/*  
 * P_LOGTO_SYSLOG should be defined if log messages are to be sent to syslog.
 * If you are logging to syslog, you will probably be better off if you 
 * assign one of the local facility names (e.g., LOG_LOCAL1) to Prospero and
 * define LOG_PROSPERO (below) accordingly.  By default, Prospero does a lot
 * of logging.  If you choose to use LOG_DAEMON (which is the default value
 * of LOG_PROSPERO), you might want to turn off logging of certain events 
 * to reduce clutter in your system wide log files.
 *
 * This can also be conveniently  set by #defining P_LOGTO_SYSLOG in the 
 * "include/pserver.h" configuration file, and possibly also LOG_PROSPERO
 */
/*
 * #define P_LOGTO_SYSLOG
 */

#ifdef P_LOGTO_SYSLOG 
#include <syslog.h>
#define LOG_PROSPERO LOG_DAEMON
#else 
#define LOG_PROSPERO 0
#endif 

#ifndef LOG_INFO
#define LOG_CRIT	2
#define LOG_ERR		3
#define LOG_WARNING	4
#define LOG_NOTICE	5
#define LOG_INFO	6
#endif  LOG_INFO


/* If L_FILEDS_STIME selected, then... */
/* This can also be conveniently  set by #defining L_WTTIME_THRESHOLD in the 
 * "include/pserver.h" configuration file.
 */
#ifndef L_WTTIME_THRESHOLD
#define L_WTTIME_THRESHOLD   1  /* log waiting time if >= seconds */
#endif
/* This can also be conveniently  set by #defining L_SYSTIME_THRESHOLD in the 
 * "include/pserver.h" configuration file.
 */
#ifndef L_SYSTIME_THRESHOLD
#define L_SYSTIME_THRESHOLD  1  /* log systime if >= seconds */
#endif
/* This can also be conveniently redefined by #defining L_SVCTIME_THRESHOLD in
 * the"include/pserver.h" configuration file.
 */
#ifndef L_SVCTIME_THRESHOLD
#define L_SVCTIME_THRESHOLD  1  /* log svctime if >= seconds */
#endif

/* If L_QUEUE_COMP selected, then the following conditions are ANDed */
/* This can also be conveniently redefined by #defining L_COMP_SVC_THRESHOLD
 * in the"include/pserver.h" configuration file.
 */
#ifndef L_COMP_SVC_THRESHOLD
#define L_COMP_SVC_THRESHOLD 30 /* Log L_QUEUE_COMP only if svctime exceeds */
#endif
/* This can also be conveniently redefined by #defining L_COMP_SYS_THRESHOLD
 * in the"include/pserver.h" configuration file.
 */
#ifndef L_COMP_SYS_THRESHOLD
#define L_COMP_SYS_THRESHOLD  0 /* Log L_QUEUE_COMP only if systime exceeds */
#endif

/*
 * INITIAL_LOG_VECTOR defines the actions to be taken for log messages
 * of particular types.  If INITIAL_LOG_VECTOR is defined in psite.h,
 * that definition will override the definition that appears here.
 *
 * Event 0 in the vector is a bit vector identifying the information
 * to be included in each log message (see L_FIELDS above).  All other
 * events in the vector specify the (OR'd together) syslog facility and
 * priority to be used for the log message if P_LOGTO_SYSLOG is defined.
 * The facility and priority are also OR'd with PLOG_TOFILE to indicate
 * that the message should be written to the Prospero logfile
 * (conditional on PLOG_TOFILE being enabled).  If the entry for an
 * event is 0, then no logging will occur.
 *
 * NOTE: The syslog event and priority of 0 is being overloaded.  
 *       You cannot specify the combination of the LOG_EMERG priority
 *       and the LOG_KERN facility.  
 */
/* This can also be conveniently redefined by #defining INITIAL_LOG_VECTOR
 * in the"include/pserver.h" configuration file.
 */
#ifndef INITIAL_LOG_VECTOR
#define INITIAL_LOG_VECTOR { \
    L_FIELDS_USER|L_FIELDS_HADDR|L_FIELDS_SW_ID,  /* L_FIELDS       */       \
    LOG_PROSPERO|LOG_NOTICE|PLOG_TOFILE,	  /* L_STATUS       */       \
    LOG_PROSPERO|LOG_CRIT|PLOG_TOFILE,	  	  /* L_FAILURE      */       \
    PLOG_TOFILE,			  	  /* L_STATS        */       \
    LOG_PROSPERO|LOG_ERR|PLOG_TOFILE,	  	  /* L_NET_ERR      */       \
    LOG_PROSPERO|LOG_ERR|PLOG_TOFILE,	  	  /* L_NET_RDPERR   */       \
    0,					  	  /* L_NET_INFO     */       \
    LOG_PROSPERO|LOG_INFO|PLOG_TOFILE,	  	  /* L_QUEUE_INFO   */       \
    0,					  	  /* L_QUEUE_COMP   */       \
    LOG_PROSPERO|LOG_ERR|PLOG_TOFILE,	  	  /* L_DIR_PERR     */       \
    LOG_PROSPERO|LOG_WARNING|PLOG_TOFILE, 	  /* L_DIR_PWARN    */       \
    LOG_PROSPERO|LOG_INFO|PLOG_TOFILE,	  	  /* L_DIR_PINFO    */       \
    LOG_PROSPERO|LOG_ERR|PLOG_TOFILE,	  	  /* L_DIR_ERR      */       \
    LOG_PROSPERO|LOG_WARNING|PLOG_TOFILE, 	  /* L_DIR_WARN     */       \
    LOG_PROSPERO|LOG_INFO|PLOG_TOFILE,	  	  /* L_DIR_REQUEST  */       \
    LOG_PROSPERO|LOG_INFO|PLOG_TOFILE,	  	  /* L_DIR_UPDATE   */       \
    LOG_PROSPERO|LOG_WARNING|PLOG_TOFILE, 	  /* L_AUTH_ERR     */       \
    LOG_PROSPERO|LOG_ERR|PLOG_TOFILE,	  	  /* L_DATA_FRM_ERR */       \
    LOG_PROSPERO|LOG_ERR|PLOG_TOFILE,		  /* L_DB_ERROR     */       \
    LOG_PROSPERO|LOG_INFO|PLOG_TOFILE,	  	  /* L_DB_INFO      */       \
    LOG_PROSPERO|LOG_INFO|PLOG_TOFILE	  	  /* L_ACCOUNT      */       \
}

#endif  /* INITIAL_LOG_VECTOR */

extern char *vplog(int, RREQ, char *, va_list);
extern char *plog(int, RREQ, char *, ...);
extern void plog_manual(FILE *outf);          /* call if plogging manually. */
#endif /* PLOG_H_INCLUDED */
