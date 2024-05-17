/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef USE_SYS_DIR_H
#include <sys/dir.h>
#else
#include <dirent.h>
#endif
#include <stdio.h>
#include <sgtty.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

#define FILE_VNO            5   /* version # of the file format. */

#include <ardp.h>
#include <pfs.h>
#include <plog.h>
#include <pprot.h>
#include <perrno.h>
#include <pmachine.h>
#include <pparse.h>
#include <psrv.h>	/* For nativize_prefix etc */

extern char	shadow[];
extern char	dirshadow[];
extern char	dircont[];
extern char	aftpdir[];

PATTRIB	atalloc();
VLINK check_fwd();

/* Temporary hack.   This will go away when the interface to dsrfinfo() and
   dsrobject() changes.   Note that the attributes returned depend upon the
   client address; this will have to be resolved when we try caching this kind
   of directory service information.  */
PERFILE_STATIC_LONG_DEF(client_addr);        /* set client address.   Initially
                                                0. */
#define client_addr p_th_arclient_addr[p__th_self_num()]

void
set_client_addr(long client_addr_arg)
{
    client_addr = client_addr_arg;
}

/*
 * dsrfinfo - Read file info from disk
 *
 *   DSRFINFO is used by the directory server to read file information from
 *   disk.  It takes the host specific part of the system level name of the 
 *   file about which information is to be read, and a pointer to the file
 *   structure which it fills in.  It returns 0 or -1 on success, 0 if a file
 *   and -1 if a directory.  On errors with some corrupt lines in the shadow
 *   file, it logs the bad data format to the Prospero logfile, and attempts to
 *   continue.   On other errors, it returns PFAILURE.   If unable to find the
 *   object, it returns DSRFINFO_NOT_A_FILE.  If it finds useful forwarding 
 *   information, it will return DSRFINFO_FORWARDED.
 */

int
dsrfinfo(char *nm,long magic,PFILE fi)
{
    struct requested_attributes req_obj_ats_st;

    ZERO(&req_obj_ats_st);
    req_obj_ats_st.all = 1;
    return dsrfinfo_with_attribs(nm, magic, fi, &req_obj_ats_st);
}

/*
 * For now, we have turned off the magic # matching features.  Forwarding is
 * no longer magic number based, pending a determination on the role of magic
 * numbers in object creation.  See /nfs/pfs/notes/forwarding.
 */
int
dsrfinfo_with_attribs(char *nm,long magic,PFILE fi, 
                      struct requested_attributes *req_obj_ats)
{
    FILE 		*pfs_fi = NULL;
    char                *pfs_fi_name = NULL;
    char		name[MAXPATHLEN];
    char		shadow_fname[MAXPATHLEN];
    char		dirshadow_fname[MAXPATHLEN];

    VLINK		cur_fp; /* current forwarding poiner */
    int		tmp;
    char		*ls;  /* Last slash */

    struct passwd   *ownpw;
    struct group    *owngr;
    struct tm	*mt;
    char   		mtime_str[20];
    char   		mode_str[15];

    PATTRIB	at;
    PATTRIB	ap; /* Temp attribute pointer */

    struct stat	file_stat_dat;
    struct stat	*file_stat = &file_stat_dat;

    /* This may be reset to -1 once we determine it's a directory.  If we go up
       the directory tree looking for forwarding information, it will be reset
       to DSRFINFO_FORWARDED.  */
    int		retval = PSUCCESS;
    INPUT_ST        in_st;
    INPUT           in = &in_st;

    /* Expand special file names */
    if((*nm != '/') && *aftpdir && (strncmp(nm,"AFTP",4) == 0)) {
	strcpy(name,aftpdir);
	strcat(name,nm+4);
    }
    else strcpy(name,nm);

    /* Need to canonicalize the file or directory name (strip out any 
       symbolic links. */
    /* XXX Find the real name of the file and use it */
    /*		    rnl = readlink(filename,rname,MAXPATHLEN); */
    /* 		    if(rnl >= 0) *(rname+rnl) = '\0';                 */

    fi->version = -1;       /* sentinel value in case not found. */
    fi->f_magic_no = 0;
    /* No need to initialize rest of fi, its already initialized */
startover:

    /* Determine name of shadow file */
    strcpy(shadow_fname,shadow);
    nativize_prefix(name, shadow_fname + strlen(shadow),
			sizeof shadow_fname - strlen(shadow));
    qsprintf(dirshadow_fname,sizeof dirshadow_fname,"%s/%s",shadow_fname,dirshadow);

    /* NOTE: A temporary inefficient shadow file format is*/
    /* in use.  For this reason, the code supporting it   */
    /* is also interim code, and does not do any checking */
    /* to make sure that the file actually follows        */
    /* the format.  Thus, a munged file will result       */
    /* in unpredictable results.			      */

    /* Read the contents of the shadow file */
    /* First find out if it is a directory  */
    if(stat(shadow_fname,file_stat) == 0) {
        if(file_stat->st_mode & S_IFDIR)
            pfs_fi = locked_fopen(pfs_fi_name = dirshadow_fname,"r");
        else pfs_fi = locked_fopen(pfs_fi_name = shadow_fname,"r");
    }

    if(pfs_fi != NULL) {
#define RETURN(rv) { retval = (rv); goto cleanup ; }
        char *line, *next_word;
        tmp = wholefiletoin(pfs_fi, in);
        locked_fclose_A(pfs_fi, pfs_fi_name, TRUE);
        if(tmp) {
            plog(L_DIR_ERR,NOREQ,"%s",p_err_string);
            return tmp;
	}
        while(in_nextline(in)) {
            if (in_line(in, &line, &next_word)) {
		/* If we cannot parse it, do not use it. */
                plog(L_DATA_FRM_ERR,NOREQ,
                     "Unreadable file info format in %s.", shadow_fname);
                RETURN(PFAILURE);
	    }

	    CHECK_PTRinBUFF(line,next_word);
            switch(*line) {
            case 'V': /* Version Number */
                tmp = sscanf(line,"VERSION %d",&(fi->version));
                if(tmp != 1) {
                    plog(L_DATA_FRM_ERR,NOREQ,"Bad file info format %s: %s",
                         shadow_fname,line,0);
                }
                if (fi->version != FILE_VNO) {
                    plog(L_DATA_FRM_ERR, NOREQ, "Bad file info format %s: \
unknown version %d, expected %d", shadow_fname, fi->version, FILE_VNO);
                    RETURN(PFAILURE);
                }
                break;
            case 'M': /* Magic Number */
                tmp = sscanf(line,"MAGIC-NUMBER %ld",&(fi->f_magic_no));
                if(tmp != 1) {
                    fi->f_magic_no = 0;
                    plog(L_DATA_FRM_ERR,NOREQ,"Bad file info format %s: %s",
                         shadow_fname,line);
                }
                break;
            case 'E': /* Expiration Date */
                break;
            case 'T': /* Time to live */
                break;
            case 'L': /* Time of last reference */
                break;
            case 'F': /* Forwarding Pointer */ 
                /* A forwarding pointer has its NAME member set to the old
                   hosname of the object (now being forwarded).  the NAME
                   member may terminate in a *, indicating a wildcarded match.
                   The HOST is the new host of the object and the HSONAME is
                   its new HSONAME.  A trailing * in the HSONAME is replaced
                   by whatever matched the trailing * in the NAME member 
                   of the object being forwarded.  Yes, this means that it is
                   difficult to forward an object whose real HSONAME ends in
                   '*'.  We do not currently create such objects, although we
                   might.  */
                if (strnequal(line, "FORWARD", 7)
                    && in_link(in, line, next_word, 0, &cur_fp, 
                               (TOKEN *) NULL) == PSUCCESS) { 
                    APPEND_ITEM(cur_fp, fi->forward);
                } else {
                    plog(L_DATA_FRM_ERR,NOREQ,"Bad file info format %s: %s",
                         shadow_fname,line);
                }
                break;

            case 'B': /* Back Link */
                break;
            case 'A': /* Attribute or ACL*/ 
		if (strnequal(line, "ACL", 3)) {
		    if(in_ge1_acl(in, line, next_word, &fi->oacl)) {
			plog(L_DATA_FRM_ERR,NOREQ,"Bad file info format %s: %s",
			     shadow_fname,line);
			RETURN(PFAILURE);
		    }
		}
		else if (!strnequal(line, "ATTRIBUTE", 9) 
			 || in_ge1_atrs(in, line, next_word, &fi->attributes))
                    plog(L_DATA_FRM_ERR,NOREQ,"Bad file info format %s: %s",
                         shadow_fname,line);
                break;
	    /*default will skip on to next line - prob reasonable*/
            }
        }


#ifdef SERVER_DO_NOT_SUPPORT_FORWARDING
#if 0                           /* This code is inconsistent; was murdered from
                                   older working code */
        /* Explicitly setting the magic # of an object to a negative number
           indicates that 'this object is not really here; it's just
           a placeholder for forwarding pointers'.  */
        /* Comparing the magic #s is a fast operation; faster than scanning
           though a list of forwarding pointers.  Therefore, if the magic #s
           match, all is well. */
        /* Check_above sets the return value to DSRFINFO_FORWARDED to indicate
           that we're searching up the UNIX directory hierarchy for a matching
           FORWARD message */
        /* Determine if the file has been forwarded, and if so return, */
        /* indicating that fact                                        */
        /* A file is assumed to have been forwarded if (a) an explicit magic #
           was requested and it doesn't match the current magic # and matching
           forwarding data was found here for it
        if (((fi->f_magic_no && magic && (fi->f_magic_no != magic)) ||
             (fi->f_magic_no < 0)) 
            && check_fwd(fi->forward,nm,magic))
            return(DSRFINFO_FORWARDED);
        /* Otherwise, this isn't the object that we thought it was (or it
           has 'I am not really here' set on it, but no forwarding
           informatin found. */
        else
            return DSRFINFO_NOT_A_FILE;
#else

        /* Test: is this object marked indicating that it's the ghost of a
           forwarded object? */
        if (fi->f_magic_no < 0) retval = DSRFINFO_FORWARDED;
        if (retval == DSRFINFO_FORWARDED) {
            if (check_fwd(fi->forward, nm, magic)) /* if not locally forwarded.
                                                      */ 
                return DSRFINFO_FORWARDED;
            else
                return DSRFINFO_NOT_A_FILE;
        }
#endif                          
#endif /* SERVER_DO_NOT_SUPPORT_FORWARDING */
            
    }

    /* Fill in attributes from the real file, if it exists and if we are not
       just running up the hierarchy for forwarding information, and if we
       actually want any of these attributes. */

    if((retval == PSUCCESS) && was_attribute_requested("CONTENTS", req_obj_ats))
	return(retval);

    if ((retval == PSUCCESS) 
        && (   was_attribute_requested("SIZE", req_obj_ats)
           || was_attribute_requested("NATIVE-OWNER", req_obj_ats)
           || was_attribute_requested("NATIVE-GROUP", req_obj_ats)
           || was_attribute_requested("LAST-MODIFIED", req_obj_ats)
           || was_attribute_requested("UNIX-MODES", req_obj_ats))
        && (stat(name,file_stat) == 0)) {
        if (was_attribute_requested("SIZE", req_obj_ats)) {
            /*       off_t   st_size;	  /* total size	of file	    */
            at = atalloc();
            at->aname = stcopy("SIZE");
            at->avtype = ATR_SEQUENCE;
            at->nature = ATR_NATURE_INTRINSIC;
            at->value.sequence = tkalloc(NULL);
            at->value.sequence->token = 
                qsprintf_stcopyr((char *) NULL, "%d bytes", file_stat->st_size);
            APPEND_ITEM(at, fi->attributes);
        }

#ifndef PFS_THREADS
        if (was_attribute_requested("NATIVE-OWNER", req_obj_ats)) {
            /*       short   st_uid;	  /* user-id of	owner       */
            ownpw = getpwuid(file_stat->st_uid); /* not MT safe */
            if(ownpw) {
                at = atalloc();
                at->aname = stcopy("NATIVE-OWNER");
                at->avtype = ATR_SEQUENCE;
                at->nature = ATR_NATURE_INTRINSIC;
                at->value.sequence = tkalloc(ownpw->pw_name);
                APPEND_ITEM(at, fi->attributes);
            }
        }
#endif


#ifndef PFS_THREADS
        if (was_attribute_requested("NATIVE-GROUP", req_obj_ats)) {
            /*       short   st_gid;	  /* group-id of owner      */
            owngr = getgrgid(file_stat->st_gid); /* not MT safe */
            if(owngr) {
                at = atalloc();
                at->aname = stcopy("NATIVE-GROUP");
                at->avtype = ATR_SEQUENCE;
                at->nature = ATR_NATURE_INTRINSIC;
                at->value.sequence = tkalloc(owngr->gr_name);
                APPEND_ITEM(at, fi->attributes);
            }
        }
#endif

        if (was_attribute_requested("LAST-MODIFIED", req_obj_ats)) {
            /*       time_t  st_atime;    /* file last access time  */

            /*       time_t  st_mtime;    /* file last	modify time */
            if(file_stat->st_mtime) {
                at = atalloc();
                at->aname = stcopy("LAST-MODIFIED");
                at->avtype = ATR_SEQUENCE;
                at->nature = ATR_NATURE_INTRINSIC;
                at->value.sequence = tkalloc(NULL);
                at->value.sequence->token = 
                    p_timetoasn_stcopyr(file_stat->st_mtime, 
                                        at->value.sequence->token);
                APPEND_ITEM(at, fi->attributes);
            }
        }

        if (was_attribute_requested("UNIX-MODES", req_obj_ats)) {
            /*       u_short st_mode;	  /* protection	            */
            at = atalloc();
            at->aname = stcopy("UNIX-MODES");
            at->avtype = ATR_SEQUENCE;
            at->nature = ATR_NATURE_INTRINSIC;

            strcpy(mode_str,"----------");
            if((file_stat->st_mode & S_IFLNK) == S_IFLNK) mode_str[0] = 'l';
            if(file_stat->st_mode & S_IREAD) mode_str[1] = 'r';
            if(file_stat->st_mode & S_IWRITE) mode_str[2] = 'w';
            if(file_stat->st_mode & S_IEXEC) mode_str[3] = 'x';
            if(file_stat->st_mode & S_ISUID) mode_str[3] = 's';
            if(file_stat->st_mode & (S_IREAD>>3)) mode_str[4] = 'r';
            if(file_stat->st_mode & (S_IWRITE>>3)) mode_str[5] = 'w';
            if(file_stat->st_mode & (S_IEXEC>>3)) mode_str[6] = 'x';
            if(file_stat->st_mode & S_ISGID) mode_str[6] = 's';
            if(file_stat->st_mode & (S_IREAD>>6)) mode_str[7] = 'r';
            if(file_stat->st_mode & (S_IWRITE>>6)) mode_str[8] = 'w';
            if(file_stat->st_mode & (S_IEXEC>>6)) mode_str[9] = 'x';
            if(file_stat->st_mode & S_IFDIR) {
                mode_str[0] = 'd';
                if(retval == PSUCCESS) retval = -1;
            }
            at->value.sequence = tkalloc(mode_str);
            APPEND_ITEM(at, fi->attributes);
        }
    }
    /* If no native file AND no PFS file info, then look for forwarding */
    else if(fi->version < 0) goto check_above;

    /* Stick on the INTRINSIC ACCESS-METHOD attributes. */
    if (retval == 0             /* if it's a file */
        && was_attribute_requested("ACCESS-METHOD", req_obj_ats)) {
        PATTRIB at1 = NULL;
        PATTRIB nextat;
        get_access_method(name, client_addr, &at1);
        for ( ; at1; at1 = nextat) {
            nextat = at1->next;
	    EXTRACT_HEAD_ITEM(at1);
            APPEND_ITEM(at1, fi->attributes);
        }
    }
    return(retval);

    /* Check above looks for forwarding pointers in directories   */
    /* above the named file.  It is reached when we have been passed
       an hsoname for a nonexistent object.
       We might be passed an hsoname for a nonexistent object either 
       (a) if the object has been forwarded or (b) the object has not
       yet been created.  So we look for forwarding information or
       for a real superior object.  Once we make a hit (or hit the root of the
       tree), we return either DSRFINFO_FORWARDED or DSRFINFO_NOT_A_FILE.
       */
    /* Look up, in case the object has moved, but a forwarding address does not
       exist in the corresponding file info, but might exist further up. */

 check_above:

    retval = DSRFINFO_NOT_A_FILE; /* If the forwarded test succeeds, will
                                   return DSRFINFO_FORWARDED instead. */

    ls = strrchr(name,'/');
    /* If we have used up all components, return failure */
    if((ls == 0) || (ls == name)) return(DSRFINFO_NOT_A_FILE);

    *ls = '\0';

    goto startover;

cleanup:                        /* used for abnormal exit */
    /* input file is closed in code above. */
    return retval;
}



/* check_fwd takes a list of forwarding pointers, checks to see */
/* if any apply to the object with the specified magic number   */
/* and returns the appropiate link, or none if not forwarded    */
/* If the match is a widarded match, the returned link will be  */
/* modified so that the matched wildcard is replaced by the     */
/* text that matched it                                         */
/*                                                              */
/* BUGS only a trailing * wildcard is allowed                   */
VLINK 
check_fwd(fl,name,magic)
    VLINK	fl;
    char	*name;
    int		magic;
{
    char	*sp; /* Star pointer */

    while(fl) {
        if(((magic == 0) || (fl->f_magic_no == 0) ||
            (fl->f_magic_no == magic)) && (wcmatch(name,fl->name))) {

            sp = strchr(fl->hsoname,'*');
            if(sp) *sp = '\0';

            sp = strchr(fl->name,'*');
            if(sp) {
                int n;

                sp = name + (sp - fl->name);
                fl->name = stcopyr(name,fl->name);
                fl->hsoname = qsprintf_stcopyr(fl->hsoname,
                                          "%s%s",fl->hsoname,sp);
            }

            return(fl);
        }
        fl = fl->next;
    }
    return(NULL);
}    
    


