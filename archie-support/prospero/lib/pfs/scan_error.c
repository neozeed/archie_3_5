/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <stdio.h>
#include <string.h>
#include <pfs.h>
#include <perrno.h>

char *p_motd = NULL; /* motd is read here. */

static void fix_server_might_append_null_to_packet(RREQ req);

int
scan_error(char *erst, RREQ req)
{
    p_clear_errors();

    if (strequal(erst, "SUCCESS"))
        return PSUCCESS;

#ifdef CLIENTS_REQUEST_VERSION_FROM_SERVER
    if (qsscanf(erst, "VERSION %*d %&s", &req->peer_sw_id) == 1) {
#ifdef SERVER_MIGHT_APPEND_NULL_TO_PACKET
        if (req->peer_sw_id && (strnequal(req->peer_sw_id, "A50", 3) ||
                           strnequal(req->peer_sw_id, "A51", 3) ||
                           strnequal(req->peer_sw_id, "B51", 3) ||
                           strnequal(req->peer_sw_id, "A52", 3)))
            fix_server_might_append_null_to_packet(req);
#endif
        return PSUCCESS;
    }
#endif
    if(strncmp(erst,"VERSION-NOT-SUPPORTED",21) == 0) {
        qsscanf(erst,"%'&[^\n]",&p_err_string);
        return perrno = DIRSRV_BAD_VERS;
    }

    if(strncmp(erst,"WARNING ",8) == 0) {
        erst += 8;
        *p_warn_string = '\0';
        qsscanf(erst,"%~%*[^\n \t\r]%~%'&[^\n\r]",
                &p_warn_string);
        /* Return values for warnings are negative */
        if(strncmp(erst,"OUT-OF-DATE",11) == 0) {
            pwarn = PWARN_OUT_OF_DATE;
            return(PSUCCESS);
        }
        if(strncmp(erst,"MESSAGE",7) == 0) {
            pwarn = PWARN_MSG_FROM_SERVER;
            return(PSUCCESS);
        }
        pwarn = PWARNING;
        qsscanf(erst,"%'&[^\n]",&p_warn_string);
        return(PSUCCESS);
    }
    else if(strncmp(erst,"ERROR",5) == 0) {
        if(*(erst+5)) 
            qsscanf(erst+6,"%'&[^\n\r]",&p_err_string);
        perrno = DIRSRV_ERROR;
        return(perrno);
    }
    else if (strnequal(erst, "PARAMETER", 9)) {
        qsscanf(erst, "PARAMETER%~VALUE%*( \t)MOTD%*( \t)%'&s", 
                &p_motd);
        /* It doesn't matter whether we successfully read in a value
           for p_motd, since we return SUCCESS anyway, and since
           p_motd is initialized to the empty string. */
        return PSUCCESS;
    }
    /* FAILURE on a line by itself. */
    else if (strequal(erst, "FAILURE")) return perrno = PFAILURE;
    /* The rest start with "FAILURE " */
    else if(!strnequal(erst,"FAILURE ", 8)) {
        /* Unrecognized Protocol message - Give warning, but return PSUCCESS */
        if(pwarn == 0) {
            *p_warn_string = '\0';
            pwarn = PWARN_UNRECOGNIZED_RESP;
            qsscanf(erst,"%'&[^\n\r]", &p_warn_string);
        }
        return(PSUCCESS);
    }
    erst += 8;

    qsscanf(erst,"%*[^\n \t\r]%*[ \t]%&'[^\n\r]", &p_err_string);

    /* Still to add               */
    /* DIRSRV_AUTHENT_REQ     242 */
    /* DIRSRV_BAD_VERS        245 */

    if(strncmp(erst,"NOT-FOUND",9) == 0) 
        perrno = DIRSRV_NOT_FOUND;
    else if(strncmp(erst,"NOT-FOUND",9) == 0) 
        perrno = DIRSRV_NOT_FOUND;
    else if(strncmp(erst,"NOT-AUTHORIZED",13) == 0) 
        perrno = DIRSRV_NOT_AUTHORIZED;
    else if(strncmp(erst,"ALREADY-EXISTS",14) == 0) 
        perrno = DIRSRV_ALREADY_EXISTS;
    else if(strncmp(erst,"NAME-CONFLICT",13) == 0) 
        perrno = DIRSRV_NAME_CONFLICT;
    else if(strncmp(erst,"TOO-MANY",8) == 0) 
        perrno = DIRSRV_TOO_MANY;
    else if(strncmp(erst,"SERVER-FAILED",13) == 0) 
        perrno = DIRSRV_SERVER_FAILED;
    else if(strncmp(erst,"UNIMPLEMENTED",13) == 0) {
        perrno = DIRSRV_UNIMPLEMENTED;
    } else if(strncmp(erst,"NOT-A-DIRECTORY",15) == 0) 
        perrno = DIRSRV_NOT_DIRECTORY;
    else perrno = PFAILURE;

    return perrno;
}


#ifdef SERVER_MIGHT_APPEND_NULL_TO_PACKET
/* This allows clients to fix output they get from the 5.0, 5.1, and 5.2
   servers. */
static
void
fix_server_might_append_null_to_packet(RREQ req)
{
    PTEXT pt = req->rcvd;
    for ( ; pt; pt = pt->next) {
        /* If the packet has a text area with at least one character in it */
        if ((pt->start + pt->length - pt->text > 0) 
            /* and that character is ASCII NUL */
            && (pt->start[pt->length - 1] == '\0')) {
            --pt->length;       /* get rid of it (shorten the packet by 1)  */
        }
    }
}
#endif
