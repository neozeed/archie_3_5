/*
  parameter.c
  Copyright (c) 1993, the University of Southern California
  For copying and distribution information, read the file <usc-copyr.h>
*/

#include <usc-copyr.h>

#include <ardp.h>
#include <pserver.h>	/* For #define PSRV_P_PASSWORD */
#include <pfs.h>
#include <plog.h>
#include <psrv.h>
#ifdef PSRV_P_PASSWORD
#include <ppasswd.h>
#endif
#include "dirsrv.h"
#include <perrno.h>

/* Query: PARAMETER [MANDATORY] { GET | SET } <paramname> [<value-token>] */
/* Reply: PARAMETER VALUE <paramname> <value-token> 
          PARAMETER NOTSET <paramname>
          WARNING NOT-FOUND PARAMETER <paramname>
          FAILURE NOT-FOUND PARAMETER <paramname>
*/          
#ifdef PFS_THREADS
p_th_mutex p_th_mutexP_PARAMETER_MOTD;
#endif

int
parameter(RREQ req, char *command, char *next_word)
{
    int tmp;
    AUTOSTAT_CHARPP(t_tmpp);
    char *t_text;
    char *cp;
    int mandatory = 0;          /* 1 if MANDATORY */
    int set = -1;                /* 1 for set; 0 for get. */
    static char *p_motd = NULL; /* message of the day.  MUTEXED with
                                   p_th_mutexP_PARAMETER_MOTD.   Init in
                                   dirsrv.c */

    tmp = qsscanf(next_word, "%&'s %r", &*t_tmpp,  &next_word);
    if (tmp < 2)
        goto malformed;
    if (strequal(*t_tmpp, "MANDATORY")) {
        mandatory = 1;
        tmp = qsscanf(next_word, "%&'s %r", *t_tmpp, &next_word);
        if (tmp < 2)
            goto malformed;
    }
    if (strequal(*t_tmpp, "SET")) set = 1;
    else if (strequal (*t_tmpp, "GET")) set = 0;
    else goto malformed;

    t_text = NULL;
    tmp = qsscanf(next_word, "%'&s %'&s %r", &*t_tmpp, &t_text,
		  &next_word);
    if (tmp < 1)
	goto malformed;
    if (strequal(*t_tmpp, "PASSWORD")) {
	if (tmp != 3)
	    goto malformed;
    }
    else if (tmp > 2)
        goto malformed;

#ifndef NDEBUG
    if (tmp >= 2) assert(t_text != NULL);
    else assert(t_text == NULL);
#endif

    /* *t_tmpp now should contain a variable name.  if text was provided, t_text
       contains it. */
    if (set) plog(L_DIR_UPDATE, req, "%s", command);
    else plog(L_DIR_REQUEST, req, "%s", command);
    
    /* Make sure ACLs are initialized properly */
    if (!maint_acl) srv_check_acl((ACL) NULL, (ACL) NULL, req, "X", SCA_MISC,NULL,NULL);
    if (strequal(*t_tmpp, "MOTD")) {
        if (set) {
            if (!srv_check_acl(maint_acl,NULL,req,"U",SCA_MISC,NULL,NULL)) {
                plog(L_AUTH_ERR, req, "Unauthorized: %s", command);
                if (mandatory) {
                    creplyf(req, "FAILURE NOT-AUTHORIZED %'s\n", command);
                    RETURNPFAILURE;
                } else {
                    replyf(req, "WARNING NOT-AUTHORIZED %'s\n", command);
                    return PSUCCESS;
                }
            }
            p_th_mutex_lock(p_th_mutexP_PARAMETER_MOTD);
            stfree(p_motd);
            p_motd = t_text;
            t_text = NULL;
            p_th_mutex_unlock(p_th_mutexP_PARAMETER_MOTD);
            reply(req, "SUCCESS\n");
        } else {
            /* Ok to examine the value once; looking at a variable's value is
               atomic and need not be mutexed. */
            if (p_motd)
                replyf(req, "PARAMETER VALUE MOTD %'s\n", p_motd);
            else
                reply(req, "PARAMETER NOTSET MOTD\n");
        }
    } else if (strequal(*t_tmpp, "TERMINATE")) {
        if (set) {
            if (!t_text || !strequal(t_text, "NOW")) {
                stfree(t_text); t_text = NULL;
                if (mandatory) {
                    creply(req, "FAILURE BAD-VALUE Must be NOW\n");
                    RETURNPFAILURE;
                } else {
                    reply(req, "WARNING BAD-VALUE Must be NOW\n");
                    return PSUCCESS;
                }
            }
            stfree(t_text); t_text = NULL;
            if (!srv_check_acl(maint_acl,NULL,req,"T",SCA_MISC,NULL,NULL)) {
                plog(L_AUTH_ERR, req, "Unauthorized: %s", command);
                if (mandatory) {
                    creplyf(req, "FAILURE NOT-AUTHORIZED %'s\n", command);
                    RETURNPFAILURE;
                } else {
                    replyf(req, "WARNING NOT-AUTHORIZED %'s\n", command);
                    return PSUCCESS;
                }
            }
            plog(L_STATUS,req,"Server killed (terminate message received)",0);
            creply(req,"SUCCESS\n");
            log_server_stats();
            exit(0);
        } else {
            reply(req, "PARAMETER NOTSET TERMINATE\n");
        }
    } else if (strequal(*t_tmpp, "RESTART")) {
        if (set) {
            if (!t_text || !strequal(t_text, "NOW")) {
                stfree(t_text); t_text = NULL;
                if (mandatory) {
                    creply(req, "FAILURE BAD-VALUE Must be NOW\n");
                    RETURNPFAILURE;
                } else {
                    reply(req, "WARNING BAD-VALUE Must be NOW\n");
                    return PSUCCESS;
                }
            }
            stfree(t_text); t_text = NULL;
            if (!srv_check_acl(maint_acl,NULL,req,"S",SCA_MISC,NULL,NULL)) {
                plog(L_AUTH_ERR, req, "Unauthorized: %s", command);
                if (mandatory) {
                    creplyf(req, "FAILURE NOT-AUTHORIZED %'s\n", command);
                    RETURNPFAILURE;
                } else {
                    replyf(req, "WARNING NOT-AUTHORIZED %'s\n", command);
                    return PSUCCESS;
                }
            }
            plog(L_STATUS,req,"Server restarted (restart message received)",0);
            creply(req,"SUCCESS\n");
            restart_server(0, (char *) NULL);
        } else {
            reply(req, "PARAMETER NOTSET RESTART\n");
        }
    }
#ifdef PSRV_P_PASSWORD
    else if (strequal(*t_tmpp, "PASSWORD")) {
	int authenticated = 0;
	PAUTH auth;

	if (!t_text || !next_word)
	    goto malformed;

	/* Check if principal has been authenticated by old password */
	for (auth = req->auth_info; auth; auth = auth->next) {
	    if (auth->ainfo_type == PFSA_P_PASSWORD &&
		strequal(t_text, auth->principals->token)) {
		authenticated = 1;
		break;
	    }
	}

	if (set) {
	    if (/* User authenticated by old password */
		authenticated ||
		/* 'P' gives right to create and modify password entry */
		srv_check_acl(maint_acl,NULL,req,"P",SCA_MISC,NULL,NULL) ||
		 /* 'p' gives only modify password right */
		(srv_check_acl(maint_acl,NULL,req,"p",SCA_MISC,NULL,NULL) &&
		 get_ppw_entry(t_text))) {
		char *passwd = NULL;
		qsscanf(next_word, "%'&s", &passwd); /* Unquote */
						    /* password */
		if (set_passwd(t_text, passwd)) {
		    /* Should never occur */
		    stfree(t_text); t_text = NULL;
		    creplyf(req, "UNKNOWN FAILURE\n");
		    RETURNPFAILURE;
		}
		stfree(passwd);
		reply(req, "SUCCESS\n");
	    }
	    else {
		stfree(t_text); t_text = NULL;
		plog(L_AUTH_ERR, req, "Unauthorized: PARAMETER SET PASSWORD");
		if (mandatory) {
		    creplyf(req, "FAILURE NOT-AUTHORIZED SET PASSWORD\n");
		    RETURNPFAILURE;
		} else {
		    replyf(req, "WARNING NOT-AUTHORIZED SET PASSWORD\n");
		    return PSUCCESS;
		}
	    }
	}
	else {
	    reply(req, "PARAMETER NOTSET PASSWORD\n");
	}
    }
#endif
    else /* not found */ {
        stfree(t_text); t_text = NULL;
        if (mandatory) /* set or get */ {
            creplyf(req, "FAILURE NOT-FOUND PARAMETER %s\n", *t_tmpp); 
            RETURNPFAILURE;
        } else                    /* set or get */
            replyf(req, "WARNING NOT-FOUND PARAMETER %s\n", *t_tmpp);
    }
    stfree(t_text); t_text = NULL;
    return PSUCCESS;

 malformed:
    stfree(t_text); t_text = NULL;
    return error_reply(req, "Malformed commmand: %'s", command);
}
