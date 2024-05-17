/* Copyright (c) 1992, 1993 by the University of Southern California
 * For copying and distribution information, please see the file
 * <usc-copyr.h>
 */

#include <usc-copyr.h>
#include <pfs.h>
#include <pparse.h>
#include <psrv.h>
#include <stdio.h>
#include <perrno.h>
#include <plog.h>
#ifndef MAXPATHLEN
#include <sys/param.h>
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
#endif
#include <sys/stat.h>

extern char *rindex();

extern char	shadow[];
extern char	dirshadow[];
extern char	dircont[];
extern char	pfsdat[];
extern char	aftpdir[];

#define FILE_VNO            5   /* version # of the file format.   Appears in
                                   this file and in dsrfinfo.c */

/* Forward definitions */
static FILE *open_shadow_outfile_A(char nm[], char **shadow_fnamep);
static int write_data(FILE * outf, PFILE fi);

/* Logs errors to plog() and returns an error code. */
int 
dswfinfo(char *nm, PFILE fi)
{
    int     retval;
    FILE *outf;
    AUTOSTAT_CHARPP(shadow_fnamep);

    assert(fi);	/* write_data cant handle null fi*/
    if (outf = open_shadow_outfile_A(nm,shadow_fnamep)) {
        retval = write_data(outf, fi);
    } else {
        return perrno;
    }
    locked_fclose_A(outf, *shadow_fnamep, FALSE);
    return retval;
}
    

static FILE *
open_shadow_outfile_A(char nm[], char **shadow_fnamep)
{
    char        real_fname[MAXPATHLEN];     /* Filename after expansion  */
    char        shadow_dirname[MAXPATHLEN]; /* directory we create */
    char        *sp;                        /* pointer to slash          */
    FILE        *outf;
    
    struct stat	file_stat_dat;
    struct stat	*file_stat = &file_stat_dat;

    nativize_prefix(nm, real_fname, sizeof(real_fname));
    
    /* Create the directory to contain the shadow file, if it doesn't already
       exist. */  
    /* Is the shadow file a directory? */
    if ((stat(real_fname, file_stat) == 0)  || (*nm != '/')) {
        if ((file_stat->st_mode & S_IFDIR) || (*nm != '/')) {
            *shadow_fnamep = qsprintf_stcopyr(*shadow_fnamep,
                     "%s%s/%s", shadow, real_fname, dirshadow);
        } else {
            *shadow_fnamep = qsprintf_stcopyr(*shadow_fnamep, 
                     "%s%s", shadow, real_fname);
        }
    } else {
       plog(L_DATA_FRM_ERR, NOREQ, "Output data file doesnt already exist: %s", 
             real_fname);
        perrno = PFAILURE;
	return NULL;
    }
    strcpy(shadow_dirname, *shadow_fnamep);
    assert(sp = rindex(shadow_dirname, '/')); /* must be true. */
    *sp = '\0';
    mkdirs(shadow_dirname);
    if ((outf = locked_fopen(*shadow_fnamep, "w")) == NULL) {
        plog(L_DATA_FRM_ERR, NOREQ, "Couldn't open the output data file %s", 
             *shadow_fnamep);
        perrno = PFAILURE;
    }
    return outf;
}


static int
write_data(FILE * outf, PFILE fi)
{
    OUTPUT_ST out_st;
    OUTPUT out = &out_st;
    PATTRIB at;
    VLINK cur_fp;

    assert(fi);

    filetoout(outf, out);
    qoprintf(out, "VERSION %d\n", FILE_VNO);
    qoprintf(out, "MAGIC-NUMBER %d\n", fi->f_magic_no);
    /* Any forwarding pointers. */
    for (cur_fp = fi->forward; cur_fp; cur_fp = cur_fp->next) {
        qoprintf(out, "FORWARD ");
        out_link(out, cur_fp, 0, (TOKEN) NULL);
    }
    /* print out the object ACL if present */
    out_acl(out, fi->oacl);
    for (at = fi->attributes; at; at = at->next) {
        if (at->nature != ATR_NATURE_INTRINSIC) /* INTRINSIC attributes are 
                                              written specially  */
            out_atr(out, at, 0);
    }
    return PSUCCESS;
}
