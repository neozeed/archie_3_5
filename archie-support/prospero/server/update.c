/* Author: bcn@cs.washington.edu, bcn@isi.edu */
/* Modified by swa@isi.edu */
/*
 * Copyright (c) 1992, 1993, 1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>

#define UPDATE_IN_PROGRESS      /* this is unimplemented still */

#include <string.h>

#include <ardp.h>
#include <pfs.h>
#include <plog.h>
#include <perrno.h>
#include <psrv.h>
#include <pprot.h>              /* for MAXPATHLEN for dirsrv.h */
#include "dirsrv.h"

static int update_name(RREQ req, char *command, char client_dir[],
                           VDIR dir, char *name, int alllinks, 
                           int *item_count); 

int
update(RREQ req, char *command, char *next_word, INPUT in,
       char client_dir[], int dir_magic_no)
{
    VDIR_ST dir_st;
    VDIR dir = &dir_st;
    int retval;
    int item_count = 0;
    long    t_magic_no;

    char *nextname;                   /* Next name to be resolved. */
    char *dummy;                /* Dummy for qsscanf. */
    int tmp;

    vdir_init(dir);
    nextname = "";
    tmp = qsscanf(next_word, "'' COMPONENTS%r %r", &dummy, &nextname);
    if (tmp < 1)
        return error_reply(req, "Bad command format: %'s", command);
    if (in_select(in, &t_magic_no))
        return error_reply(req, "UPDATE: %'s", p_err_string);

#ifdef UPDATE_IN_PROGRESS
    creply(req, "FAILURE SERVER-FAILED UPDATE command not yet fully \
implemented.");
    RETURNPFAILURE;
#endif
    plog(L_DIR_UPDATE, req, "U %s %s", client_dir, nextname,0);

    retval = dsrdir(client_dir,dir_magic_no,dir,NULL,0);
    if(retval == DSRFINFO_FORWARDED) {
        dforwarded(req, client_dir, dir_magic_no, dir);
        return PSUCCESS;
    }
    /* If not a directory, say so */
    if(retval == DSRDIR_NOT_A_DIRECTORY) {
        creply(req,"FAILURE NOT-A-DIRECTORY\n");
        plog(L_DIR_ERR, req, "Invalid directory name: %s", client_dir,0);
        RETURNPFAILURE;
    }

    if (tmp < 2)
        return update_name(req, command, client_dir, dir, 
                           NULL, 1 /* alllinks */, &item_count); 
    else {
        /* A copy of this code is in list.c; if there's a bug here, there's one
           there too. */
        while (nextname) {
            char *son = nextname;          /* Start of Name */
            char *eon;          /* End of Name.  */
            /* Note the double-quotes -- don't unquote! */
            tmp = qsscanf(nextname, "%*'s%r %r", &eon, &nextname); 
            if (son == eon)
                break;    /* No more names -- the line must have had trailing
                             spaces.  */
            if (tmp == 1)       /* No following whitespace; must be the last
                                   name  */
                nextname = NULL;
            else {
                assert(tmp == 2);
                *eon = '\0'; /* Terminate the name. */
            }
            if(retval = update_name(req, command, client_dir, dir, son, 
                                  /* alllinks */ 0, &item_count))
                return retval;  /* Don't process any more names if an error
                                   shows up. */
        }
    }
    retval = 0;
    if(item_count) retval = dswdir(client_dir,dir);

    /* Indicate how many updated */
    if(retval) replyf(req,"FAILED to UPDATE %d links\n",item_count,0);
    else replyf(req,"UPDATED %d links\n",item_count,0);
    vdir_freelinks(dir);
    return PSUCCESS;
}


static int
update_name(RREQ req, char *command, char client_dir[], VDIR dir, 
            char *components, int alllinks, int *item_count)
{
    VLINK clink;
    FILTER cfil;
    int retval;

    /* Here we must check for forwarding of each link and */
    /* update it to reflect the new target                */
    clink = dir->links;
    while(clink) {
        if (clink->linktype == 'N') 
            continue;           /* NATIVE links can't be forwarded. */
        if(alllinks || wcmatch(clink->name,components)) {
            /* Check for forwarding */
            if(retrieve_fp(clink) == PSUCCESS) item_count++;

            /* If filters, check them too */
            cfil = clink->filters;
            while(cfil) {
#ifndef UPDATE_IN_PROGRESS
                if(retrieve_fp(cfil) == PSUCCESS) item_count++;
#endif /* UPDATE_IN_PROGRESS */
                cfil = cfil->next;
            }
        }
        clink = clink->next;
    }

    /* here we must process the union links. */
    clink = dir->ulinks;
    while(clink) {
        if (clink->linktype == 'N') 
            continue;           /* NATIVE links can't be forwarded. */
        if(alllinks || wcmatch(clink->name,components)) {
            /* Check for forwarding */
            if(retrieve_fp(clink) == PSUCCESS) item_count++;

            /* If filters, check them too ***/
            cfil = clink->filters;
            while(cfil) {
                if(cfil->link && (retrieve_fp(cfil->link) == PSUCCESS)) 
                    item_count++;
                cfil = cfil->next;
            }
        }
        clink = clink->next;
    }
    return PSUCCESS;
}
