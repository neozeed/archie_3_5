/*
 * Copyright (c) 1991, 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <usc-copyr.h>.
 */

#include <usc-copyr.h>

#include <ardp.h>
#include <pserver.h>            /* needed for def. of SERVER_DONT_FLAG_V1 */
#include <pfs.h>
#include <plog.h>
#include <pprot.h>
#include <pparse.h>
#include <psrv.h>      /* For replyf */
#include "dirsrv.h"

/* Return the # of the client version, if it is one that this version of dirsrv
   supports.  If it is one that this version cannot support, or if there's an
   error, return -1. */
int
version(req, command, next_word)
    RREQ req;
    char *command;
    char *next_word;
{
    int tmp;
    int client_version;
    char t_sw_id[40];
    char *cp;                   /* throw-away temp. ptr. */

#ifndef SERVER_DONT_FLAG_V1
    /* Save the last data encountered in order to avoid logging old software
       versions quite so many times. */
    static char last_oldvers_client_sw_id[40] = "";
    static long last_oldvers_client_host = 0L;
#endif
    
    t_sw_id[0] = '\0';
    tmp = qsscanf(next_word,"%d %'!!s %r",&client_version, 
                  t_sw_id, sizeof t_sw_id, &cp);

    /* Save sw_id before checking tmp so we know how generated */
    if(*t_sw_id) req->peer_sw_id = stcopyr(t_sw_id,req->peer_sw_id);

    if (tmp < 0) {
        interr_buffer_full();
        return -1;
    }
    else if (tmp == 0) {
        replyf(req,"VERSION %d %s\n", MAX_VERSION,PFS_SW_ID);
        return MAX_VERSION;
    } else if (tmp == 3) {
        error_reply(req, "VERSION command takes at most 2 arguments,\
but we received: %'s", command);
        return -1;
    }
    assert(tmp == 1  || tmp == 2);   /* The client must want us to use
                                        a certain version of the
                                        protocol. */  
    if(client_version == MAX_VERSION) return client_version;
    if((client_version < MAX_VERSION) && 
       (client_version >= MIN_VERSION)) {
#ifndef SERVER_DONT_FLAG_V1
        if ((last_oldvers_client_host != req->peer_addr.s_addr)
            || !strequal(last_oldvers_client_sw_id,t_sw_id)) {
            /* Attempt to reduce the # of unnecessary repeated messages logged.
               */ 
            plog(L_DIR_PWARN, req, "Old version in use: %d %s%s",
                 client_version,
                 (t_sw_id[0] ? ", by a client with software ID " : ""),
                 t_sw_id, 0);
            qsprintf(last_oldvers_client_sw_id, 
                     sizeof last_oldvers_client_sw_id, "%s", t_sw_id);
            last_oldvers_client_host = req->peer_addr.s_addr;
        }
#endif /* SERVER_DONT_FLAG_V1 */
        return client_version;
    }
    if(MAX_VERSION == MIN_VERSION)
        creplyf(req,"VERSION-NOT-SUPPORTED TRY %d",MAX_VERSION);
    else creplyf(req,"VERSION-NOT-SUPPORTED TRY %d-%d",
		 MIN_VERSION,MAX_VERSION);
    plog(L_DIR_PERR, req,
         "Unimplemented version in use: %d %s%s",
             client_version,
             (t_sw_id[0] ? ", by a client with software ID " : ""),
             t_sw_id, 0);
    return -1;
}
