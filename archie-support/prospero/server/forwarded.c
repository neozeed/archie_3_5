/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#include <stdio.h>
#include <string.h>

#include <ardp.h>
#include <pfs.h>
#include <plog.h>
#include <psrv.h>
#include <perrno.h>
#include <pmachine.h>

#include "dirsrv.h"


int
forwarded(RREQ                req,
          VLINK		fl, 	           /* List of forwarding pointers,
                                              including:  */
          VLINK		fp, 	           /* The current forwarding ptr. */
          char      objectname[]) /* object being forwarde */
{
    int retval;

    if(fp) {
        replyf(req,"FORWARDED %s %'s %s %'s %ld",
                fp->hosttype,fp->host,fp->hsonametype,fp->hsoname,
                fp->version, 0);
        if (fp->dest_exp) {
            char *cp = NULL;
            replyf(req, " DEST-EXP %s\n", 
                   cp = p_timetoasn_stcopyr(fp->dest_exp, cp));
            stfree(cp);
        } else
            reply(req, "\n");
        if (fp->f_magic_no > 0)
            replyf(req, "ID REMOTE %ld\n", fp->f_magic_no,0);
        retval = PSUCCESS;
    } else {
        creplyf(req,"FAILURE SERVER-FAILED the object %'s is marked as \
forwarded, but no forwarding pointer was found for it.\n", objectname);
        retval = PFAILURE;
    }
    vllfree(fl);
    return retval;
}
/* Forwarded directories. */

int
dforwarded(req, client_dir, dir_magic_no, dir)
    RREQ req;
    char client_dir[];
    int dir_magic_no;
    VDIR dir;
{
    VLINK		fl; 	           /* List of forwarding pointers */
    VLINK		fp; 	           /* The current forwarding ptr. */

    fl = dir->f_info->forward; dir->f_info->forward = NULL;
    fp = check_fwd(fl,client_dir,dir_magic_no);

    /* Free what we don't need */
    vdir_freelinks(dir);
    return forwarded(req, fl, fp, client_dir);
}


int
obforwarded(RREQ req, char client_dir[], long dir_magic_no, P_OBJECT dirob)
{
    VLINK		fl; 	           /* List of forwarding pointers */
    VLINK		fp; 	           /* The current forwarding ptr. */

    fl = dirob->forward; dirob->forward = NULL;
    fp = check_fwd(fl,client_dir,dir_magic_no);

    return forwarded(req, fl, fp, client_dir);
}


/* Like dforwarded, but reply with a LINK line (to be followed by a REMCOMP
   line by the caller). */
int
dlinkforwarded(RREQ req, OUTPUT out, char client_dir[], int dir_magic_no, 
               VDIR dir, char *components)
{
    VLINK		fl; 	           /* List of forwarding pointers */
    VLINK		fp; 	           /* The current forwarding ptr. */
    int                 retval; /* value to return */

    fl = dir->f_info->forward; dir->f_info->forward = NULL;
    fp = check_fwd(fl,client_dir,dir_magic_no);

    /* Free what we don't need */
    vdir_freelinks(dir);
    if(fp) {
        fp->name = stcopyr("", fp->name);
        fp->target = stcopyr("DIRECTORY", fp->target);
        fp->linktype = 'L';
        qoprintf(out, "LINK ");
        out_link(out, fp, 0, (TOKEN) NULL);
        retval = PSUCCESS;
    } else {
        creplyf(req,"FAILURE SERVER-FAILED the object %'s is marked as \
forwarded, but no forwarding pointer was found for it.\n", client_dir);
        retval = PFAILURE;
    }
    vllfree(fl);
    return retval;
}

/* Like dforwarded, but reply with a LINK line (to be followed by a REMCOMP
   line by the caller). */
int
oblinkforwarded(RREQ req, OUTPUT out, char client_dir[], int dir_magic_no, 
               P_OBJECT dirob, char *components)
{
    VLINK		fl; 	           /* List of forwarding pointers */
    VLINK		fp; 	           /* The current forwarding ptr. */
    int                 retval; /* value to return */

    fl = dirob->forward; dirob->forward = NULL;
    fp = check_fwd(fl,client_dir,dir_magic_no);

    if(fp) {
        fp->name = stcopyr("", fp->name);
        fp->target = stcopyr("DIRECTORY", fp->target);
        fp->linktype = 'L';
        qoprintf(out, "LINK ");
        out_link(out, fp, 0, (TOKEN) NULL);
        retval = PSUCCESS;
    } else {
        creplyf(req,"FAILURE SERVER-FAILED the object %'s is marked as \
forwarded, but no forwarding pointer was found for it.\n", client_dir);
        retval = PFAILURE;
    }
    vllfree(fl);
    return retval;
}

