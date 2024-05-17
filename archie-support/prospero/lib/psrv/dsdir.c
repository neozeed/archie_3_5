/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <sys/param.h>
#include <sys/stat.h>           /* sys/types.h included already */
/*    sys/dir.h on SCO_UNIX doesnt define DIR */
#include <pmachine.h>
#if defined(SCOUNIX) || defined(SOLARIS) || !defined(USE_SYS_DIR_H)
#if defined(PFS_THREADS_SOLARIS) && !defined(_REENTRANT)
#define _REENTRANT              /* needed to get prototype for readdir_r() */
#endif /* defined(PFS_THREADS_SOLARIS) && !defined(_REENTRANT) */
#include <dirent.h>
#else
#include <sys/dir.h>
#endif

#if defined(SOLARIS)		/* seems true on pand05.prod.aol.net */
#define SOLARIS_GCC_BROKEN_LIMITS_H
#endif

#include <stdio.h>
#include <sgtty.h>
#include <string.h>

#include <pserver.h>
#include <ardp.h>
#include <pfs.h>
#include <plog.h>
#include <pprot.h>
#include <perrno.h>
#include <pmachine.h>
#include <pparse.h>
#include <psrv.h>       /* For dsrfinfo etc */
#include <posix_signal.h>	/* Needed on SOLARIS for sigset_t etc */
#include <mitra_macros.h>	/* For L2 */

#define DIR_VNO		     5  /* version # of the directory format. */

extern char	root[];
extern char	shadow[];
extern char	dirshadow[];
extern char	dircont[];
#define TMP_DIRNAME_SUFFIX     ".TMP"
extern char	pfsdat[];
extern char	aftpdir[];

extern char	hostname[];
extern char	hostwport[];

extern char	*acltypes[];


static int indsrdir_v5(INPUT in, char vfs_dirname[], VDIR dir, VLINK ul);
static int reload_native_cache(char native_dirname[], VDIR dir);
static int check_vfs_dir_forwarding(VDIR dir, char name[], long magic);
static size_t canonicalize_prefix(char *native_name, char canon_buf[], 
                                  size_t canon_bufsiz);
size_t nativize_prefix(char *hsoname, char native_buf[], size_t native_bufsiz);

/*
  Policy for caching native links:
  1) If a native directory is converted to NONATIVE (The only case in which
  this happens in the current code is if a native link was
  deleted in it), then it's now entirely virtual, and all native links are
  saved.
  2) If the compilation option PSRV_CACHE_NATIVE is set, then
  all native links and their attributes are cached.  The cache is reloaded
  whenever the native MTIME has changed.  The CACHED attributes on a link will
  also be reloaded whenever an UPDATE protocol message is sent (that mechanism
  is not handled in dsdir).  If no special compilation option is defined, 
  NATIVE-ONLY directories are guaranteed to be not cached unless at least one
  link attribute is set on a link in that directory.
  3) In any case, a entry will be written out for a directory  if any attribute
  is set (via EDIT-LINK-INFO) on an entry in that directory.  NATIVE links with
  LINK attributes on them WILL be written out, but other NATIVE links in the
  directory may or may not be.
  4) For the current implementation, all directories that are not NATIVE-ONLY
  are always written out and NATIVE-ONLY directories with links with non-OBJECT
  and non-CACHED attributes are always written out.
*/  
/* Thoughts on this:
  This will break if we start caching partial attribute reads.  Therefore, the
  solution for THAT is to have a new attribute value type called UNSET.  This
  will always be CACHED and means that the   corresponding OBJECT attribute is
  UNSET and therefore need not be verified.  This can become a part of the
  protocol.  

  We'll also still have the problem of partially-cached reads not containing
  all values.  E.g.: checking of INTRINSIC+ should probably be disabled, no?
  So, CONTENTS is special.  So perhaps a special tag on the cached data saying
  whether it contains all INTRINSIC attributes.  Also, a test whether a
  particular attribute is INTRINSIC+ or just INTRINSIC.

  A quicker solution is not to cache partial attribute reads.

  */



/*
 * New policy: the PFSDAT directory must be NONATIVE.  This is enforced.
 * Fixes some old bugs.
 */

/*
 * dsrdir - Read the contents of a directory from disk
 *
 *   DSRDIR is used by the directory server to read the contents of
 *   a directory off disk.  It takes the host specific part of the system
 *   level name of the directory as an argument, and a directory structure
 *   which it fills in.
 *   If the directory is forwarded, it returns DSRFINFO_FORWARDED. 
 *   On error, it returns an error code and logs the circumstances to plog().
 *   It may also return a warning code.
 *
 *   dsrdir() is frequently called to expand union links.  Therefore,
 *   it must be careful about caching. */
int
dsrdir(hsoname,magic,dir,ul,flags)
    char	*hsoname;          /* hsoname */
    int		magic;          /* magic # specified in DIRECTORY command */
    VDIR	dir;    /* dir to fill in */
    VLINK	ul;     /* Pointer to the vlink currently being expanded */
	                /* used for location to insert new union links   */
    int		flags;  /* DSRD_ATTRIBUTES = get attributes for files    */
                        /* Or DSRD_VERIFY_DIR -- see psrv.h */
{
    FILE    *vfs_dir;
    char    vfs_dirname[MAXPATHLEN]; /* name of .directory#contents file */
    char    native_dirname[MAXPATHLEN]; /* name of physical directory
                                           corresponding to 'hsoname'.  Only
                                           differs from 'hsoname' if "AFTP" or
                                           other special prefix in use.  */
    int	    tmp;
    int     vfs_dir_problem = 0;   /* 1 if failure while reading VFS dir. */
    time_t  native_dir_mtime;    /* mtime of native dir. */
    int		return_value = DSRDIR_NOT_A_DIRECTORY;
    int     pfsdatlen = strlen(pfsdat);


    nativize_prefix(hsoname, native_dirname, sizeof native_dirname);
    strcpy(vfs_dirname,shadow);  
    strcat(vfs_dirname,native_dirname);
    strcat(vfs_dirname,"/");

    strcat(vfs_dirname,dircont);

    /* vfs_dirname is now the name of a file which, if it exists, means we are
       guaranteed that the virtual directory exists. */

    /* native_dirname is the name of a local UNIX directory which, if it
       exists, will be read to produce a virtual directory. */

    /* Special case code for common case of verifying */
    if (flags & DSRD_VERIFY_DIR) {
        if (is_file(vfs_dirname) || is_dir(native_dirname))
            return PSUCCESS;    /* VERIFIED the directory exists */
        else
            return PFAILURE;
    }
    dir->version = -1;
    dir->magic_no = 0;
    dir->inc_native = VDIN_INCLREAL;    /* Include native if can't read */
    dir->f_info = NULL;
    dir->dacl = NULL;

    /* XXX This should no longer be necessary. */
    if(ul == NULL) {               /* Don't clear links if expanding */
        dir->links = NULL;
        dir->ulinks = NULL;
    }

    /* Check whether cacheing will be ok. */
    /* Special code in dump_links to handle ULINK_PLACEHOLDER */
    if (!dir->links && 
        (!dir->ulinks || 
         !dir->ulinks->next && dir->ulinks->expanded == ULINK_PLACEHOLDER))
        dir->flags |= VDIR_FIRST_READ;
    else
        dir->flags &= ~VDIR_FIRST_READ;
        

    /* NOTE: A temporary inefficient directory format is  */
    /* in use.   It will be changed.                        */

    /* Read the contents of the VFS directory */
    if((vfs_dir = locked_fopen(vfs_dirname,"r")) != NULL) {
        INPUT_ST in_st;
        INPUT in = &in_st;
        if(tmp = wholefiletoin(vfs_dir, in)) {
	    locked_fclose_A(vfs_dir, vfs_dirname, TRUE);
	    plog(L_DIR_ERR,NOREQ,"%s",p_err_string);
            return tmp;
	}
        if (ferror(vfs_dir)) vfs_dir_problem++;
        if (locked_fclose_A(vfs_dir, vfs_dirname, TRUE)) vfs_dir_problem++;
	VLDEBUGBEGIN;
        return_value = indsrdir_v5(in, vfs_dirname, dir, ul);
	VLDEBUGDIR(dir);
	VLDEBUGEND;
        /* What it means when indsrdir_v5 returns an error is not fully
           clearly defined. */
	if (return_value) vfs_dir_problem++;
    } else     /* Note that this directory is entirely native */
        (dir->inc_native = VDIN_ONLYREAL);

    /* (Re)load the contents of the native directory into the current cached
       listing if cache is out of date or not present. */ 
    /* This comment is from MITRA and I disavow it.  --swa, 5/17/94 */
    /* mitra: I havent corrected this behaviour for VDIN_PSEUDO yet 
	behaviour if their is a real directory is unpredictable */
    if(dir->inc_native != VDIN_NONATIVE && 
       (native_dir_mtime = mtime(native_dirname)) > dir->native_mtime) {
        /* Report an error if we couldn't read the native directory. */
        return_value = reload_native_cache(native_dirname, dir);
        if (!return_value && !vfs_dir_problem
            && (dir->flags & VDIR_FIRST_READ)) {
            /* XXX Need to set a don't-ever-write flag if there are problems so
               that we know we can't support updates to this directory. */
            /* if we successfully reloaded the cache */
            /* update mod time.  Use a value of mtime guaranteed to be before
               reload_native_cache() started reading. */
            dir->native_mtime = native_dir_mtime; 
#ifndef PSRV_CACHE_NATIVE        /* don't bother writing out changes if not
                                   cacheing.  */
            if (dir->inc_native != VDIN_ONLYREAL)
#endif
                /* Write out the changes.  Failure to write out the updated
                   cache is not an error, but does yield a warning. */
                if (tmp = dswdir(hsoname, dir)) {
                    p_warn_string = qsprintf_stcopyr(p_warn_string,
                             "Failed to update native cache for directory %'s:\
 Prospero error %d", hsoname, tmp);
                }
        }
    }
    /* Handle directory forwarding. */
    if((return_value != PSUCCESS) || (dir->magic_no != magic)) 
        if (check_vfs_dir_forwarding(dir, hsoname, magic) 
            == DSRFINFO_FORWARDED)
            return DSRFINFO_FORWARDED;
    /* Directories in the pfsdat area must be NONATIVE */
    if (strnequal(hsoname, pfsdat, pfsdatlen)  
        && (hsoname[pfsdatlen] == '/' || hsoname[pfsdatlen] == '\0'))
        dir->inc_native = VDIN_NONATIVE;
    return(return_value);
}


/* Returns PSUCCESS if no forwarding, DSRFINFO_FORWARDING if forwarding
   occurred.   This is only called if the directory's magic number has a
   mismatch. */ 
static int
check_vfs_dir_forwarding(VDIR dir, char hsoname[], long magic)
{
    PFILE		dfi = pfalloc();
    int tmp = dsrfinfo(hsoname,magic,dfi);
    if(tmp == DSRFINFO_FORWARDED) {
        dir->f_info = dfi;
        return(DSRFINFO_FORWARDED);
    }
    /* This bit of code may not be necessary.  Anyway, this stuff is
       getting replaced soon. --swa@isi.edu */
    else if((dir->magic_no < 0) && dfi->forward) {
        dir->f_info = dfi;
        return(DSRFINFO_FORWARDED);
    }
    pffree(dfi);
    return PSUCCESS;
}


/* Returns PFAILURE if it attempts to read a malformed directory; 
   PSUCCESS if all went well. */
static
int
indsrdir_v5(INPUT in, char vfs_dirname[], VDIR dir, VLINK ul)
{
    char *command, *next_word;
    char include_native[30];
    char *cp;                   /* dummy pointer */
    VLINK cur_link = NULL;      /* set this when we see a link.  */
    int     seen_version = 0;   /* seen directory format version #?  */

    while(in_nextline(in)) {
        char t_timestring[30];

        if(in_line(in, &command, &next_word)) {
            plog(L_DATA_FRM_ERR, NOREQ, "Couldn't read line from %s: %s",
                         vfs_dirname, p_err_string);
            RETURNPFAILURE;
        }

        if (qsscanf(command, "VERSION %d %r", &(dir->version), &cp) == 1) {
            if (dir->version != DIR_VNO) {
                plog(L_DATA_FRM_ERR, NOREQ, 
                     "Bad directory info format: %s: Encountered VERSION %d; \
dirsrv can only interpret version %d", vfs_dirname, dir->version, DIR_VNO);
                RETURNPFAILURE;
            }
            seen_version++;
            continue;
        }
        if (!seen_version) {
            plog(L_DATA_FRM_ERR, NOREQ, 
                 "Bad directory info format: %s: VERSION line not found; must \
appear at the start of the file.", vfs_dirname);
            RETURNPFAILURE;
        }
        if (qsscanf(command, "MAGIC-NUMBER %d %r", &(dir->magic_no), &cp) == 1)
            continue;
        if (qsscanf(command, "INCLUDE-NATIVE %!!s %r", 
                    include_native, sizeof include_native, &cp) == 1) {
            if(strequal(include_native, "INCLNATIVE"))
                dir->inc_native = VDIN_INCLNATIVE;
            else if (strequal(include_native, "INCLREAL"))
                dir->inc_native = VDIN_INCLREAL;
            else if (strequal(include_native, "NONATIVE"))
                dir->inc_native = VDIN_NONATIVE;
            else if (strequal(include_native, "PSEUDO"))
                dir->inc_native = VDIN_PSEUDO;
            else {
                plog(L_DATA_FRM_ERR, NOREQ, 
                     "Bad directory info format %s: %s", vfs_dirname, command);
                RETURNPFAILURE;
            }
            continue;
        }
        if (qsscanf(command, "NATIVE-MTIME %!!s %r", 
                    t_timestring, sizeof t_timestring, &cp) == 1) {
            dir->native_mtime = asntotime(t_timestring);
            continue;
        }
        if (qsscanf(command, "ACL %r", &cp) == 1) {
            if (cur_link) {
                if(in_ge1_acl(in, command, next_word, &cur_link->acl)) {
                    plog(L_DATA_FRM_ERR, NOREQ, 
                         "Bad directory info format %s: %s",
                         vfs_dirname, p_err_string);
                    RETURNPFAILURE;
                }
            } else {
                if(in_ge1_acl(in, command, next_word, &dir->dacl)) {
                    plog(L_DATA_FRM_ERR, NOREQ, 
                         "Bad directory info format %s: %s",
                         vfs_dirname, p_err_string);
                    RETURNPFAILURE;
                }
            }
            continue;
        }
        /* in_link() will automatically call in_atrs. */
        if (qsscanf(command, "LINK %r", &next_word) == 1) {
            if(in_link(in, command, next_word, 0, &cur_link, 
                    (TOKEN *) NULL)) {
                plog(L_DATA_FRM_ERR, NOREQ, 
                     "Bad directory info format %s: %s",
                     vfs_dirname, p_err_string);
                RETURNPFAILURE;
            }
            /* if union link without UL variable set, then vl_insert will
               insert it at the end of the list of union links. */
                
            /* This call to ul_insert() makes sure that, *if*
               list() is in the process of expanding union links, any new
               union links are inserted in the right position (i.e., right
               after the link we're currently expanding).   This makes sure
               that nested union links are expanded in the proper order,
               whereas calling vl_insert() would instead mean that the
               new union link CUR_LINK was stuck at the end of the
               directory's list of union links; this would be wrong. 
               Also, ul_insert() checks for conflicts if given an insertion
               positon. */
            if(ul && cur_link->linktype == 'U') {
                int tmp = ul_insert(cur_link,dir,ul);
                if(!tmp) ul = cur_link; /* A subsequent identical link in this
                                           directory won't supersede this one.
                                           */ 
            } else {
                /* vl_insert(cur_link, dir, VLI_ALLOW_CONF); */
                /* this is an experiment in efficiency. */
                vl_insert(cur_link, dir, VLI_NOSORT);                 
            }
	    /* By this point cur_link should be inserted so dont free */
            /* If it's a native link, then set the flags appropriately. */
             if (cur_link->linktype == 'n') {
                cur_link->linktype = 'I';
                cur_link->flags |= VLINK_NATIVE;
            } else if (cur_link->linktype == 'N') {
                cur_link->linktype = 'L';
                cur_link->flags |= VLINK_NATIVE;
            }
            continue;
        }
        /* Gee, nothing seems to match.  Guess it must be an error. */
        plog(L_DATA_FRM_ERR, NOREQ, "Bad directory info format %s: %s",
             vfs_dirname, command);
        RETURNPFAILURE;
    }
    return PSUCCESS;
}

/*
 * dswdir - Write directory contents to disk
 * 
 * On success, returns PSUCCESS.  On error, returns an error code but does
 * not set p_err_string.
 * 
 * The contents of the 
 * Any OBJECT attributes to be written will be automatically downgraded to
 * CACHED. 
 * Any INTRINSIC namespace attributes will not be written.
 */
int
dswdir(char *name,VDIR dir)
{
    char		vfs_dirname[MAXPATHLEN];
    char		tmp_vfs_dirname[MAXPATHLEN];
    FILE 		*vfs_dir;
    int               retval;


    /* if a VFS directory, then create it if necessary */
    if(strnequal(pfsdat,name,strlen(pfsdat)))
        if(mkdirs(name)) RETURNPFAILURE;

    /* Determine name of shadow */
    strcpy(vfs_dirname,shadow);
#if 1
    nativize_prefix(name, vfs_dirname + strlen(shadow), 
                    sizeof vfs_dirname - strlen(shadow));
#else
    if((*name != '/') && *aftpdir && (strncmp(name,"AFTP",4) == 0)) {
	/* Special file name */
	strcat(vfs_dirname,aftpdir);
	strcat(vfs_dirname,name+4);
    }
    else strcat(vfs_dirname,name);
#endif

    /* Create the shadow directory if necessary */
    if(mkdirs(vfs_dirname)) RETURNPFAILURE;

    /* Determine name of directory contents */
    strcat(vfs_dirname,"/");
    strcat(vfs_dirname,dircont);


    /* NOTE: A temporary inefficient directory format is  */
    /* in use.  For this reason, the code supporting it   */
    /* is also interim code, and does not do any checking */
    /* to make sure that the directory actually follows   */
    /* the format.  This, a munged directory will result  */
    /* in unpredictable results.			  */

    /* A name to write a temporary directory to.  We write to a temporary
       file and then rename it after it's been successfully written.  We do
       this so that we're guaranteed that if the disk fills up we don't lose
       any data. */
    if(qsprintf(tmp_vfs_dirname, sizeof tmp_vfs_dirname, "%s%s", 
                vfs_dirname, TMP_DIRNAME_SUFFIX) > sizeof tmp_vfs_dirname)
        RETURNPFAILURE;


    /* Write the contents of the VFS directory */
    filelock_obtain(vfs_dirname,FALSE); /* Make sure can write original */
    if((vfs_dir = locked_fopen(tmp_vfs_dirname,"w")) == NULL) {
      filelock_release(vfs_dirname,FALSE); 
      return(PFAILURE);
    }
        fdswdir_v5(vfs_dir, dir);
    retval = locked_fclose_and_rename(vfs_dir, tmp_vfs_dirname,
				     vfs_dirname,FALSE);
	return(retval);
	
}

static void dump_links(OUTPUT out, VLINK cur_link);

/* Actually returns no useful return value.  We check for failures using
   ferror(). */
int fdswdir_v5(FILE *vfs_dir, VDIR dir)
{
    OUTPUT_ST out_st;
    OUTPUT out = &out_st;
    VLINK vl;

    filetoout(vfs_dir, out);
    qoprintf(out, "VERSION %d\n", DIR_VNO);
    if (dir->magic_no)
        qoprintf(out, "MAGIC-NUMBER %d\n", dir->magic_no);
    qoprintf(out, "INCLUDE-NATIVE ");
    if(dir->inc_native == VDIN_INCLREAL || dir->inc_native == VDIN_ONLYREAL) {
        qoprintf(out,"INCLREAL\n");
    } else if(dir->inc_native == VDIN_INCLNATIVE) {
        qoprintf(out,"INCLNATIVE\n");
    } else if (dir->inc_native == VDIN_NONATIVE) {
        qoprintf(out,"NONATIVE\n");
        /* If directory is not native, shouldn't be any links which are NATIVE
           */ 
        for (vl = dir->links; vl; vl = vl->next) {
            vl->flags &= ~VLINK_NATIVE;
        }
    } else if (dir->inc_native == VDIN_PSEUDO) {
        qoprintf(out,"PSEUDO\n");
    } else
        internal_error("unknown value of dir->inc_native");
    if (dir->native_mtime && dir->inc_native != VDIN_NONATIVE) {
        char *cp = NULL;
        qoprintf(out, "NATIVE-MTIME %s\n", 
                 cp = p_timetoasn_stcopyr(dir->native_mtime, cp));
        stfree(cp);
    }
    /* print out the directory ACL. */
    out_acl(out, dir->dacl);
    dump_links(out, dir->links);
    dump_links(out, dir->ulinks);
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


static VLINK read_native_dir(char native_dirname[], int inc_native_flag, 
                             int read_attributes, int read_basetype);
static void merge_native(VLINK links, VDIR dir);

/* PSUCCESS if all went well; anything else if problems. */
static int
reload_native_cache(char native_dirname[], VDIR dir)
{
    VLINK links = read_native_dir(native_dirname, dir->inc_native, 1, 1);
    if (!links & perrno) return perrno;
    merge_native(links, dir);   /* can't fail */
    return PSUCCESS;            /* done! */
}


static void replace_cached(PATTRIB *nativeatlp, PATTRIB *vdiratlp);

/* Merge the native directory listing with the links read in from disk (if
   ANY!).  Free the unused LINKS. */
static void 
merge_native(VLINK links, VDIR dir)
{
    register VLINK curl, nxtl; /* do a for loop with CURrent and NeXT link
                                  references since we may delete the current
                                  link. */
    /* Optimize a common case.  This will be the case for every directory that
       is pure VDIN_ONLYREAL */
    if (!dir->links) {
        dir->links = links;
        return;
    }
    /* Each native link we confirm in DIR is tagged as CONFIRMED. */
    for (curl = links, nxtl = curl ? curl->next : NULL;
         curl; 
         curl = nxtl, nxtl = curl ? curl->next : NULL) {
        VLINK vl;
        /* Search in the existing directory for a VLINK that matches CURL */
        for (vl = dir->links; vl; vl = vl->next) {
            if (!(vl->flags & VLINK_CONFIRMED) /* only do ones from this
                                                  directory  */
                && (vl->flags & VLINK_NATIVE) && vl_equal(curl, vl)) {
                /* match found.  Update any CACHED attributes on the link in
                   the directory from those in the native link.  Note that
                   vl_equal() ignores the L/I LINKTYPE distinction. */  
                vl->flags |= VLINK_CONFIRMED;
                replace_cached(&curl->lattrib, &vl->lattrib);
                EXTRACT_ITEM(curl, links);
                vlfree(curl);
                goto next_native;       /* on to the next native link. */
            }
        }
        /* No match found for CURL.  Plop it on the list. */
        EXTRACT_ITEM(curl, links); /* so we don't munge pointers in the list
                                      links.  (This was a PERNICIOUS bug). */
        APPEND_ITEM(curl, dir->links);
        curl->flags |= VLINK_CONFIRMED | VLINK_NATIVE;
    next_native:
    }    
    /* Remove any NATIVE links that are not CONFIRMED */
    /* We do not again use VLINK_CONFIRMED.  Don't bother turning it off on the
       links, since that would just be more expensive. */
    for (curl = dir->links, nxtl = L2(curl,next);
         curl; 
         curl = nxtl, nxtl = L2(curl,next)) {
        if ((curl->flags & VLINK_NATIVE) && !(curl->flags & VLINK_CONFIRMED)) {
            EXTRACT_ITEM(curl, dir->links);
            vlfree(curl);
        }
    }
}

/* Replace all the CACHED and OBJECT attributes on VLATL with all OBJECT
   attributes from the list on NATIVEATLP.  Make sure that *nativeatlp does not
   point to memory that we don't want to be freed.  */ 
static void
replace_cached(PATTRIB *nativeatlp, PATTRIB *vlatlp)
{
    PATTRIB ca,  ca_temp;
    /* Flush all CACHED attributes on VLATL */
    for (ca = *vlatlp; ca; ca = ca_temp) {
        ca_temp = ca->next;	
        if (ca->precedence == ATR_PREC_CACHED
	    || ca->precedence == ATR_PREC_OBJECT) {
            EXTRACT_ITEM(ca, *vlatlp);
            atfree(ca);
        }
    }
    /* Put the OBJECT attributes on the VLATL. They will be sent out as
       OBJECT attributes.  Code in dswdir() converts them to CACHED attributes
       upon write. */
    /* Future efficiency speedup: Replace this loop with the CONCATENATE_LISTS
       macro. */ 
    CONCATENATE_LISTS(*vlatlp, *nativeatlp);
    /* *nativeatlp will automatically be set to NULL by the CONCATENATE_LISTS
       macro. */ 
}

#include <limits.h>             /* for _POSIX_PATH_MAX for readdir_result. */

#ifndef SOLARIS_GCC_BROKEN_LIMITS_H
#include <limits.h>             /* for _POSIX_PATH_MAX for readdir_result. */
#else
#ifndef _POSIX_PATH_MAX
#undef _LIMITS_H
#include "/usr/include/limits.h"
/* XXX why on earth is this not defining _POSIX_PATH_MAX?  At any rate,
   pick a really large number that will certainly work well enough for now. */
#ifndef _POSIX_PATH_MAX
#define _POSIX_PATH_MAX 2048
#endif
#endif /* not _POSIX_PATH_MAX */
#endif /*  SOLARIS_GCC_BROKEN_LIMITS_H */

/* Return a linked list of files with the VLINK_NATIVE flag set on them. */
/* The flags are both always set for now.  They will be useful later if various
   degrees of #FAST are specified. */
/* Error reporting: return NULL and set perrno. */
static VLINK 
read_native_dir(char native_dirname[], 
                int inc_native_flag,
                int read_attributes, /* implies read_basetype. */
                int read_basetype) /* make sure basetype is correct in 
                                       listing */
{
    DIR			*dirp;
    struct dirent	*dp;
#ifdef PFS_THREADS
    struct {
        struct dirent a;
        char b[_POSIX_PATH_MAX];
    } readdir_result_st;
    struct dirent *readdir_result = (struct dirent *) &readdir_result_st;
#endif
    char		*slash;
    char		vl_hsoname[MAXPATHLEN];
    VLINK               retval = NULL; /* list of links to return. */
    VLINK               cur_link;
    int                 tmp;

    if(is_dir(native_dirname) != 1) {
        perrno = DSRDIR_NOT_A_DIRECTORY;
        return NULL;
    }
    if((dirp = opendir(native_dirname)) == NULL) {
        perrno = PFAILURE;
	p_err_string = qsprintf_stcopyr(p_err_string,"%s",unixerrstr());
        return NULL;
    }

#ifdef PFS_THREADS
    for (dp = readdir_r(dirp, readdir_result); dp != NULL; 
         dp = readdir_r(dirp, readdir_result)) {
#else                           /* PFS_THREADS */
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
#endif                          /* PFS_THREADS */
        canonicalize_prefix(native_dirname, vl_hsoname, sizeof vl_hsoname);

        /* We must special case unix concept of . and .. */
        /* if we want to eliminate useless parts of path */
        /* If the filename is "..", then                 */
        if(strequal(dp->d_name,"..")) {
#ifdef NODOTDOT
            continue;
#else
            if(inc_native_flag == VDIN_INCLREAL ||
               inc_native_flag == VDIN_ONLYREAL) continue;
            slash = rindex(vl_hsoname,'/');
            if(slash) *slash = '\0';
            else strcpy(vl_hsoname,"/");
            if(!(*vl_hsoname)) strcpy(vl_hsoname,"/");
#endif
        }
        /* Else if ".", do nothing.  If not "." add to   */
        /* vl_hsoname					 */
        else if(!strequal(dp->d_name,".")) {
            if(*(vl_hsoname + strlen(vl_hsoname) - 1)!='/')
                strcat(vl_hsoname,"/");
            strcat(vl_hsoname,dp->d_name);
        }
        /* If ".", must still decide if we include it */
        else if(inc_native_flag == VDIN_INCLREAL ||
                inc_native_flag == VDIN_ONLYREAL) {
            continue;
        }

        cur_link = vlalloc();

        cur_link->name = stcopy(dp->d_name);
        cur_link->hsoname = stcopy(vl_hsoname);
        cur_link->host = stcopy(hostwport);
        cur_link->flags = VLINK_NATIVE;
#define INVISIBLE_DOT_FILES
#ifdef INVISIBLE_DOT_FILES
        /* Native . files are initially invisible.  We do special things to
           make sure that linktype is not important in comparing two links
           during a cache reload, since this could be reset after the link
           is loaded. */
        if (*cur_link->name == '.') 
            cur_link->linktype = 'I';
        else 
            cur_link->linktype = 'L';
#else
        cur_link->linktype = 'L';
#endif
        if (read_attributes) {
            PFILE		fi = pfalloc();
            tmp = dsrfinfo(cur_link->hsoname, cur_link->f_magic_no,fi);
            if(tmp == -1) 
                cur_link->target = stcopyr("DIRECTORY",cur_link->target);
            /* 5/16/94: is this the attribute deletion bug? */
            if(tmp <= 0) {
                cur_link->lattrib = fi->attributes;
		fi->attributes = NULL;
	    }
	    pffree(fi);
        }  else if (read_basetype) {
            char                native_filename_buf[MAXPATHLEN];
            char *native_filename;
            /* This code saves us the hassle of a trip through nativize_prefix
               if it is not necessary. */
            if (cur_link->hsoname[0] != '/') {
                nativize_prefix(cur_link->hsoname, native_filename, 
                                sizeof native_filename);
                native_filename = native_filename_buf;
            } else {
                native_filename = cur_link->hsoname;
            }
            if(tmp = is_dir(native_filename)) {
                if (tmp == 1) {
                    cur_link->target = 
                        stcopyr("DIRECTORY",cur_link->target);
                } else if (tmp == -1) {
                    /* Maybe we should report an error to plog?? */
                    vlfree(cur_link);
                    continue;
                }
            }
        }
        APPEND_ITEM(cur_link, retval);
    }
    closedir(dirp);
    perrno = PSUCCESS;
    return retval;
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


size_t
nativize_prefix(char *hsoname, char native_buf[], size_t native_bufsiz)
{
    int aftpdirsiz = strlen(aftpdir);
    if (*aftpdir && strnequal(hsoname, "AFTP", 4)
        && (hsoname[4] == '/' || hsoname[4] == '\0')) {
        return qsprintf(native_buf, native_bufsiz, "%s%s", 
                        aftpdir, hsoname + 4);
    }
#ifdef DIRECTORYCACHING
    /* MITRAISM -- this is used exclusively for the DIRECTORYCACHING stuff.
    /* This should handle all other prefixes, without breaking any 
	other code, which wont call this if it has a prefix */
    if (*hsoname != '/') 
	return qsprintf(native_buf, native_bufsiz, "/%s", hsoname);
#endif
    return qsprintf(native_buf, native_bufsiz, "%s", hsoname);
}
