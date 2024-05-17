/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 */

#include <usc-copyr.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <pmachine.h>
#include <pfs.h>
#include <perrno.h>
#include <psrv.h>
#include <plog.h>

#if (OBJECT_VNO == 5) 
/* Note this is modeled on the "manly calisthenics" in dsrobject, if this 
   is wrong, then that probably is and vica versa */
void 
ob2dir(P_OBJECT ob, VDIR dir) 
{

  assert(dir);

    if (ob->flags & P_OBJECT_DIRECTORY) {
        dir->version = ob->version;            /* always 0 */
        dir->inc_native = ob->inc_native;
        dir->magic_no = ob->magic_no;
        dir->dacl = aclcopy(ob->acl);  
        dir->links = vlcopy(ob->links,TRUE);
        dir->ulinks = vlcopy(ob->ulinks,TRUE);
        dir->native_mtime = ob->native_mtime;
    }
    if (TRUE) { /* (ob->flags & P_OBJECT_FILE) { */
  	/* I'm not sure this is the best way, but since we have f_info field*/
  	register PFILE fi = dir->f_info;
  	if (!dir->f_info) { fi = pfalloc(); dir->f_info = fi; }
        if (ob->magic_no) fi->f_magic_no = ob->magic_no;
	/* acl copied above */
        fi->exp = ob->exp;
        fi->ttl = ob->ttl;
        fi->last_ref = ob->last_ref;
        fi->forward = vlcopy(ob->forward,TRUE);
        fi->backlinks = vlcopy(ob->backlinks,TRUE);
        fi->attributes = atlcopy(ob->attributes);
    }
}

void
dswobject(char hsonametype[], char hsoname[], P_OBJECT ob)
{
	VDIR_ST	dir_st;
	register VDIR dir = &dir_st;
	int	tmp;

	vdir_init(dir);
	ob2dir(ob,dir);
        if (tmp = dswdir(hsoname, dir)) {
            p_warn_string = qsprintf_stcopyr(p_warn_string, 
                "Failed to update cache for directory %'s: Prospero error %d", 
			hsoname, tmp);
        }
	if (dir->f_info) {
          if (tmp = dswfinfo(hsoname, dir->f_info)) {
            p_warn_string = qsprintf_stcopyr(p_warn_string,
                "Failed to update finfo for %'s: Prospero error %d", 
			hsoname, tmp);
	  }
        }
	vdir_freelinks(dir);
}
#endif /*VERSION5*/

#if (OBJECT_VNO == 6)
/* XXX This code is untested and does not work at the moment.  
   I am currently working on it.   -swa@ISI.EDU, April 13, 1994. */
static int write_data(FILE * outf, P_OBJECT ob);
static FILE *open_shadow_outfile_A(char nm[], char *shadow_fname,
                                     char **objshadownamep);
static size_t canonicalize_prefix(char *native_name, char canon_buf[], size_t canon_bufsiz);
static size_t nativize_prefix(char *hsoname, char native_buf[], size_t native_bufsiz);
extern int dswobject(char hsonametype[], char hsoname[], P_OBJECT ob)
{
    static int write_data(FILE * outf, P_OBJECT ob);
    char *objshadowname;
    int     retval;
    FILE *outf;
    char shadow_fname[MAXPATHLEN];

    assert(strequal(hsonametype, "ASCII"));
    filelock_obtain(objshadowname,FALSE); /* Obtain write lock on rel dir */
    if (!(outf = 
	  open_shadow_outfile_A(hsoname, &shadow_fname, &objshadowname)))
      { filelock_release(objshadowname, FALSE);
	return(perrno);
    }
      
    retval = write_data(outf, ob);
    retval = locked_fclose_and_rename(outf, shadow_fname, objshadowname,retval);
    return(retval);
}

/* XXX This routine will have to be changed. XXX BAD XXX */
static FILE *
open_shadow_outfile_A(char nm[], char *shadow_fname, char **objshadowname)
{
    char        real_fname[MAXPATHLEN];     /* Filename after expansion  */
    char	shadow_dirname[MAXPATHLEN]; /* directory we create.      */
    char        *sp;                        /* pointer to slash          */
    FILE        *outf;
    int         retval;         /* return value from subfunctions */
    
    struct stat	file_stat_dat;
    struct stat	*file_stat = &file_stat_dat;

    /* If special file name, change to real file name */
    nativize_prefix(nm, real_fname, sizeof real_fname);
    /* Create the directory to contain the shadow file, if it doesn't already
       exist. */  
    /* Is the shadow file a directory? */
    if ((retval = is_dir(real_fname)) == 0) {
    if (stat(real_fname, file_stat) == 0) {
        if(file_stat->st_mode & S_IFDIR)  {
            qsprintf(shadow_fname, sizeof shadow_fname, 
                     "%s%s/%s", shadow, real_fname, dirshadow);
        } else {
            qsprintf(shadow_fname, sizeof shadow_fname,
                     "%s%s", shadow, real_fname);
        }
    }
    strcpy(shadow_dirname, shadow_fname);
    assert(sp = rindex(shadow_dirname, '/')); /* must be true. */
    *sp = '\0';
    mkdirs(shadow_dirname);
    if ((outf = locked_fopen(shadow_fname, "w")) == NULL) {
        plog(L_DATA_FRM_ERR, NOREQ, "Couldn't open the output data file %s", 
             shadow_fname);
        perrno = PFAILURE;
    }
    return outf;
}


static void dump_links(OUTPUT out, VLINK cur_link);

static int
write_data(FILE * outf, P_OBJECT ob)
{
    OUTPUT_ST out_st;
    OUTPUT out = &out_st;
    PATTRIB at;
    VLINK cur_fp;

    filetoout(outf, out);
    qoprintf(out, "VERSION %d\n", OBJECT_VNO);
    if (ob->version) qoprintf("OBJECT-VERSION %d\n", ob->version);
    qoprintf(out, "MAGIC-NUMBER %d\n", ob->f_magic_no);
    assert(ob->inc_native != VDIN_UNINITIALIZED);
    if (ob->inc_native != VDIN_NOTDIR) {
        qoprintf(out, "INCLUDE-NATIVE ");
        if(dir->inc_native == VDIN_INCLREAL || dir->inc_native == VDIN_ONLYREAL) 
            qoprintf(out,"INCLREAL\n");
        else if(dir->inc_native == VDIN_INCLNATIVE)
            qoprintf(out,"INCLNATIVE\n");
        else if (dir->inc_native == VDIN_NONATIVE)
            qoprintf(out,"NONATIVE\n");
        else
            internal_error("unknown value of dir->inc_native");
    }
    /* print out the ACL if present */
    out_acl(out, ob->acl);
    if (ob->exp) {
        char *cp = NULL;
        qoprintf(out, "EXP %s\n", cp = p_timetoasn_stcopyr(ob->exp, cp));
        stfree(cp);
    }
    if (ob->ttl) qoprintf(out, "TTL %ld\n", ob->ttl);
    if (ob->last_ref) {
        char *cp = NULL;
        qoprintf(out, "LAST-REF %s\n", 
                 cp = p_timetoasn_stcopyr(ob->last_ref, cp));
        stfree(cp);
    }
    /* Any forwarding pointers. */
    for (cur_fp = ob->forward; cur_fp; cur_fp = cur_fp->next) {
        qoprintf(out, "FORWARD ");
        out_link(out, cur_fp, 0, (TOKEN) NULL);
    }
    /* Back links */
    for (cur_fp = ob->backlinks; cur_fp; cur_fp = cur_fp->next) {
        qoprintf(out, "BACKLINK ");
        out_link(out, cur_fp, 0, (TOKEN) NULL);
    }
    /* Object attributes */
    for (at = ob->attributes; at; at = at->next) {
        if (at->nature != ATR_NATURE_INTRINSIC) /* INTRINSIC attributes are 
                                                   written specially  */
            out_atr(out, at, 0);
    }
#if 0                           /*  need to see if this is still necessary. */
    /* If directory was, shouldn't be any links which are NATIVE */
    for (vl = dir->links; vl; vl = vl->next) {
        vl->flags &= ~VLINK_NATIVE;
    }
#endif
    dump_links(out, dir->links);
    dump_links(out, dir->ulinks);
    if (ob->native_mtime && ob->inc_native != VDIN_NONATIVE) {
        char *cp = NULL;
        qoprintf(out, "NATIVE-MTIME %s\n", 
                 cp = p_timetoasn_stcopyr(dir->native_mtime, cp));
        stfree(cp);
    }
    return PSUCCESS;
}


static void
dump_links(OUTPUT out, VLINK cur_link)
{
    for (; cur_link; cur_link = cur_link->next) {
        PATTRIB ca;
        FILTER cur_fil;

        /* Special case for ULINK_PLACEHOLDER. */
        if (cur_link->expanded == ULINK_PLACEHOLDER) continue;
        /* don't output native links to directory unless we just converted
           it to NONATIVE, or unless caching is enabled for the native
           directory, or unless they have attributes beyond the OBJECT ones. */
        if (cur_link->flags & VLINK_NATIVE)  {
            cur_link->linktype = cur_link->linktype == 'I' ? 'n' : 'N';
        }
        qoprintf(out, "LINK ");
        out_link(out, cur_link, 0, (TOKEN) NULL);
        for (ca = cur_link->lattrib; ca; ca = ca->next) {
            if (ca->nature != ATR_NATURE_INTRINSIC) {
                if (ca->precedence == ATR_PREC_OBJECT) 
                    ca->precedence = ATR_PREC_CACHED;
                out_atr(out, ca, 0);
            }
        }
        for (cur_fil = cur_link->filters; cur_fil; cur_fil = cur_fil->next) {
            qoprintf(out, "ATTRIBUTE LINK FIELD FILTER FILTER ");
            out_filter(out, cur_fil, 0);
        }
        out_acl(out, cur_link->acl);
        if (cur_link->flags & VLINK_NATIVE)  {
            cur_link->linktype = cur_link->linktype == 'N' ? 'L' : 'I';
        }
    }
}


/* Stat and get a modification time.  Returns 0 upon failure. */
static time_t
mtime(char native_dirname[])
{
    struct stat st_buf;

    if(stat(native_dirname, &st_buf) == 0)
        return st_buf.st_mtime;
    else
        return 0;
}

/* Is it a directory? 1 = yes, 0 = no, -1 = failure? */
static int
is_dir(char native_filename[])
{
    struct stat st_buf;

    if(stat(native_filename, &st_buf) == 0)
        return S_ISDIR(st_buf.st_mode) ? 1 : 0;
    else
        return -1;
}


/* Turns a name of the form /usr/ftp/... (or whatever) into AFTP/... (or
   whatever).  Returns the # of characters it needed to write out the new
   canonical version of the name.  This is the canonical form of the HSONAME.
   */ 
static size_t
canonicalize_prefix(char *native_name, char canon_buf[], size_t canon_bufsiz)
{
    int aftpdirsiz = strlen(aftpdir);
    if (*aftpdir && strnequal(native_name, aftpdir, aftpdirsiz) 
        && (native_name[aftpdirsiz] == '/' || 
            native_name[aftpdirsiz] == '\0')) {
        return qsprintf(canon_buf, canon_bufsiz, "AFTP%s", 
                        native_name + aftpdirsiz);
    }
    return qsprintf(canon_buf, canon_bufsiz, "%s", native_name);
}


static size_t
nativize_prefix(char *hsoname, char native_buf[], size_t native_bufsiz)
{
    int aftpdirsiz = strlen(aftpdir);
    if (*aftpdir && strnequal(hsoname, "AFTP", 4)
        && (hsoname[4] == '/' || hsoname[4] == '\0')) {
        return qsprintf(native_buf, native_bufsiz, "%s%s", 
                        aftpdir, hsoname + 4);
    }
    return qsprintf(native_buf, native_bufsiz, "%s", hsoname);
}
#endif                          /* 0 */
