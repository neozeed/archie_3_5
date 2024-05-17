/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>

#include <stdio.h>
#include <string.h>

#include <pfs.h>
#include <perrno.h>
#include <psrv.h>
#include <plog.h>

extern char security[];         /* full pathname of security directory.  */

#define NAMED_ACL_SECURITY_SUBDIR "named" /* subdirectory of security for named
                                             ACLs.  */ 
/* Get a named ACL with a name from the database, or write it.  */

/*   Returns: DIRSRV_NOT_FOUND,  PFAILURE, or PSUCCESS */

static void set_cached_named_acl(char *aclname, ACL wacl);
static int get_cached_named_acl(char *aclname, ACL *waclp);

extern int 
get_named_acl(char *t_name, ACL *waclp)
{
    AUTOSTAT_CHARPP(filenamep);
    INPUT_ST in_st;
    INPUT in = &in_st;
    int tmp;
    FILE *fp;

    if(get_cached_named_acl(t_name, waclp) == PSUCCESS)
        return PSUCCESS;
    *waclp = NULL;               /* start off empty */
    *filenamep = qsprintf_stcopyr(*filenamep, "%s/%s/%s", security, 
                                NAMED_ACL_SECURITY_SUBDIR, t_name);
    fp = locked_fopen(*filenamep, "r");
    if (!fp) return DIRSRV_NOT_FOUND;
    if (wholefiletoin(fp, in)) {
	plog(L_DIR_ERR,NOREQ,"%s",p_err_string);
        locked_fclose_A(fp,*filenamep,TRUE);
        RETURNPFAILURE;
    }
    tmp = in_acl(in, waclp);
    if(tmp)
        plog(L_DATA_FRM_ERR, NOREQ, "Bad NAMED ACL info format %s: %s",
             *filenamep, p_err_string);
    if (locked_fclose_A(fp,*filenamep, TRUE) && !tmp) {
        plog(L_DATA_FRM_ERR, NOREQ, "Error closing NAMED ACL file %s", 
             *filenamep);
        RETURNPFAILURE;
    }
    if (tmp) RETURNPFAILURE;
    return PSUCCESS;
}


/*   Returns: PFAILURE, or PSUCCESS */
extern int 
set_named_acl(char *t_name, ACL wacl)
{
    AUTOSTAT_CHARPP(filenamep);
    AUTOSTAT_CHARPP(tmpfilenamep);
    FILE *fp;
    OUTPUT_ST out_st;
    OUTPUT out = &out_st;

    set_cached_named_acl(t_name, wacl);
    *filenamep = qsprintf_stcopyr(*filenamep, "%s/%s/%s", security, 
                                NAMED_ACL_SECURITY_SUBDIR, t_name);
    *tmpfilenamep = qsprintf_stcopyr(*tmpfilenamep, "%s.TMP", *filenamep);
    filelock_obtain(*filenamep,FALSE); /* Obtain write lock on rel dir */
    fp = locked_fopen(*tmpfilenamep, "w");
    if (!fp) {
        char *sp;
        /* couldn't open file for writing.  Maybe a subdirectory needs to be
           created?  This could happen if there's a slash in the named acl or
           if the security directory wasn't created yet. */
        sp = strrchr(*tmpfilenamep, '/');
        if (sp) {
            *sp = '\0';
            mkdirs(*tmpfilenamep);
            *sp = '/';
            fp = locked_fopen(*tmpfilenamep, "w");
        }
        if (!fp) {
	  filelock_release(*filenamep,FALSE);
	  return(PFAILURE);
	}
    }
    filetoout(fp, out);
    out_acl(out, wacl);
    return(locked_fclose_and_rename(fp, *tmpfilenamep,*filenamep,FALSE));
}


static void
set_cached_named_acl(char *aclname, ACL wacl)
{
}

static int
get_cached_named_acl(char *aclname, ACL *waclp)
{
    RETURNPFAILURE;            /* does nothing for now. */
}
