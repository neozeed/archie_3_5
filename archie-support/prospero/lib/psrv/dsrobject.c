/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pmachine.h>
#ifndef SOLARIS                 /* if not posix, need MAXPATHLEN still */
#include <sys/param.h>
#endif

#include <pserver.h>		/* For DIRECTORYCACHING and for PLOG
                                   overrides. */ 
#include <pfs.h>
#include <perrno.h>
#include <psrv.h>
#include <plog.h>

#ifdef DSROBJECT_SPEEDUP_IS_EXPERIMENTAL
int dsrobject_speedup = 1;      /* speed up DSROBJECT.  This can be set to 0 to
                                   use the old code.  These changes made by SWA
                                   during the week of 5/9/94 */
#endif


static PATTRIB newamat(void);

#ifdef DIRECTORYCACHING

/* One day */
#define SECONDSPERDAY	(60*60*24)
#define MAXDIRCACHEAGE	(1*SECONDSPERDAY)
int cache_attempt = 0; int cache_can = 0; int cache_yes = 0; 
/* Used to see how many times dsrobject() failed in a retrieval request. */
int dsrobject_fail = 0;

int 
vdir_outofdate(VDIR dir, char *hsoname) 
{
    char native_dirname[MAXPATHLEN];
    char vfs_dirname[MAXPATHLEN];

    nativize_prefix(hsoname, native_dirname, sizeof native_dirname);
    strcpy(vfs_dirname,shadow);  
    strcat(vfs_dirname,native_dirname);
    strcat(vfs_dirname,"/");
    strcat(vfs_dirname,dircont);

    return (stat_age(vfs_dirname) > MAXDIRCACHEAGE);
}
#endif /*DIRECTORYCACHING */
/* Union links are expanded at a higher level.
 * The only message dsrobject() sends to the client is an ardp_rwait().
 * dsrobject() logs NOT-AUTHORIZED failures via plog, but doesn't send messages
 to the client; the caller does that.
 * Checks to make sure hsoname is within the space we're allowed to discuss.
 * Does check to make sure you have r, G, or g rights on a database before it 
 lets you scan that database for information.
 */
 
/* flags honored::
   DRO_VERIFY_DIR
   
*/
/*
  Returns:
  DSRFINFO_FORWARDED
  DIRSRV_NOT_AUTHORIZED
  DIRSRV_NOT_FOUND
  PFAILURE
  PSUCCESS
*/
/* If called with the DRO_VERIFY flag, will set the P_OBJECT_FILE and
   P_OBJECT_DIRECTORY flags in the ob structure, but will not *necessarily* put
   any other information into the ob structure.  Caller is still responsible
   for freeing any links pointed to by the ob structure. */ 

/* If called with the DRO_VERIFY_DIR flag, will return PSUCCESS only if object
   is a directory. Will not necessarily put any other information into the OB
   structure.  Caller is still responsible for freeing any links pointed to by
   the ob structure. */

/* dsrobject(), if it returns anything other than DSRFINFO_FORWARDED or
   PSUCCESS, may leave the ob structure with arbitrary data in it.  The caller
   should not expect the ob structure to remain untouched. */ 

/* At the moment, the only option we look at in dsrobject_list_options is the
   requested_attributes one, and that only to test for the CONTENTS
   attribute. */

/* The database ACL itself may be obtained by using an HSONAME that is just the
   root prefix of the database. */
   
int requested_contents(struct dsrobject_list_options *listopts);
PATTRIB read_contents(char hsoname[]);

int
dsrobject(RREQ req, char hsoname_type[], char hsoname[], long version, 
          long magic_no,  
          int flags, struct dsrobject_list_options *listopts, P_OBJECT ob)
{
    VDIR_ST dir_st;
    register VDIR dir = &dir_st;
    register PFILE fi = NULL;            /*  Only set if it needs to be. */
    int     retval;             /* Integer return value from subfunctions */
    int     dirretval = PFAILURE; 	/* Save initial dsrdir result*/
#ifdef DIRECTORYCACHING
    int	    cancache = TRUE;
#endif

#ifdef DSROBJECT_SPEEDUP_IS_EXPERIMENTAL
    if (!dsrobject_speedup)
        flags &= ~DRO_VERIFY;   /* if not speedup, turn off DRO_VERIFY flag. */
#endif
    VLDEBUGBEGIN;
    vdir_init(dir);             /* empty directory. */
    set_client_addr(req->peer_addr.s_addr); /* temporary hack. */

    /* Check whether HSONAME-TYPE and HSONAME are valid. */
    if (!strequal(hsoname_type, "ASCII")) {
	plog(L_DIR_ERR, req, "dsrobject(): got invalid hsoname-type: %s",
             hsoname_type, 0);
	vdir_freelinks(dir);
	RETURNPFAILURE;
    }
    if (check_handle(hsoname) == FALSE) {
	plog(L_AUTH_ERR, req, "Got an HSONAME outside the part of the \
filesystem that this server is authorized to publish information about: %s", 
             hsoname);
    /* Free the directory links */
	vdir_freelinks(dir);
	return DIRSRV_NOT_AUTHORIZED;
    }

    if ( *hsoname != '/') {	
	/* Database or special prefix in use */
        register int i;
#ifdef DIRECTORYCACHING         /* mitracode */
	cache_attempt++;
        /* SWA bug fix: this assumes listopts is always set, which it need not
           be. */ 
	/* mitra: Can not cache if specifying a component (other than"*") */
        /* swa added: Can not cache if #ALL attributes not set.  This prevents
           us from blowing it badly. */

	if (!listopts 
            || (listopts->thiscompp && strcmp(*(listopts->thiscompp),"*"))
            || !listopts->req_link_ats.all)
            cancache = FALSE;
	if (cancache) {
            cache_can++;
            VLDEBUGBEGIN;
            if (!(dirretval 
                  = dsrdir(hsoname, magic_no, dir, NULL, DSRD_ATTRIBUTES)))
                cache_yes++;
            VLDEBUGDIR(dir);
            VLDEBUGEND;
	}
	   
#endif
	if (dirretval != PSUCCESS 
#ifdef DIRECTORYCACHING
	    || vdir_outofdate(dir,hsoname) 
#endif
	    || !(dir->links) ) {
            for (i = 0; i < db_num_ents; i++) {
                if (strnequal(hsoname, db_prefixes[i].prefix,
                              strlen(db_prefixes[i].prefix))
                    && strequal(db_prefixes[i].hsoname_type, hsoname_type)) {
                    ACL dbacl;
                    
                    VLDEBUGBEGIN;
                    get_named_acl(db_prefixes[i].named_acl, &dbacl);
                    VLDEBUGEND;
                    if (dbacl
                        && !srv_check_acl(dbacl, NULL, req, "r",
                                          SCA_LINKDIR,db_prefixes[i].prefix,NULL)
                        && !srv_check_acl(dbacl, NULL, req, "g",
                                          SCA_LINKDIR,db_prefixes[i].prefix,NULL)
                        && !srv_check_acl(dbacl, NULL, req, "G",
                                          SCA_LINKDIR,db_prefixes[i].prefix,NULL)) {
                        plog(L_AUTH_ERR, req, 
                             "Unauthorized database request: %s %s",
                             hsoname, listopts? *(listopts->thiscompp) : "");
                        vdir_freelinks(dir);
                        return DIRSRV_NOT_AUTHORIZED;
                    }
                    /* This could take a while, tell client not to retry.
                     This should be customized on a per-database basis. */
                    ardp_rwait(req, 180, 0, 0);
                    VLDEBUGBEGIN;
                    retval= db_prefixes[i].read_function(req, hsoname, 
                                                         version, magic_no, flags, listopts, ob);
                    VLDEBUGOB(ob);
                    VLDEBUGEND;
#ifdef DIRECTORYCACHING
                    if (retval == PSUCCESS && ob->links) {
                        if (cancache) {
                            VLDEBUGBEGIN;
                            dswobject(hsoname_type,hsoname,ob);
                            VLDEBUGEND;
                        }
                        vdir_freelinks(dir);
			return(retval);
                    } else if (dirretval == PSUCCESS) { 
                        /* Must just have been old*/
                        /*drop through and reread dir and file*/
			break;
                    } else { /* Neither old nor new version available */
			/* Or directory is really empty */
                        vdir_freelinks(dir);
                        return(retval);
                    }
#else
                    vdir_freelinks(dir);
                    return retval;
#endif /*DIRECTORYCACHING*/
                }
            } /*for*/
        } /*PSUCCESS*/
        /* Here we have a prefix that is not a database, */
        /* but not a normal filename either */
        /* If normal file names need not begin with /    */
        /* fall through and try a normal query           */
    }
    /* Local directories can have associated finfo too. */
    if (dirretval == PSUCCESS) { /* No need to reread if did already */
	retval = dirretval;
    } else {
      VLDEBUGBEGIN;
        retval = dsrdir(hsoname, magic_no, dir, NULL, 
                        /* Set DSRD_VERIFY_DIR iff DRO_VERIFY_DIR is set. */
                        ((flags & DRO_VERIFY_DIR) ? DSRD_VERIFY_DIR : 0)
                        /* Note that dsrdir() curently always returns all
                           object attributes.  This costs us some potentially
                           unnecessary MALLOC()s. */
                        | ((listopts && listopts->requested_attrs) ?
                           DSRD_ATTRIBUTES : 0));
      VLDEBUGDIR(dir);
      VLDEBUGEND;
    }
    if (retval == PSUCCESS) ob->flags |= P_OBJECT_DIRECTORY;
    if (retval == PSUCCESS || retval == DSRDIR_NOT_A_DIRECTORY ||
        retval == DIRSRV_NOT_DIRECTORY || retval == DIRSRV_NOT_FOUND) {
        int dsrfinfo_retval;

#ifdef SERVER_DO_NOT_SUPPORT_FORWARDING
        /* Don't bother with the dsrfinfo() unless a specific attribute or set
           of attributes was requested, or unless a directory was not found. */
        if (listopts->req_obj_ats.all  
            || listopts->req_obj_ats.interesting
            || listopts->req_obj_ats.specific) {
            fi = pfalloc();
            dsrfinfo_retval = dsrfinfo_with_attribs(hsoname, magic_no, fi, 
                                                    &listopts->req_obj_ats);
        } else {
            dsrfinfo_retval = PFAILURE;
        }
#else
	VLDEBUGBEGIN;
        dsrfinfo_retval = dsrfinfo(hsoname,magic_no,fi);
	VLDEBUGFI(fi);
	VLDEBUGEND;
        if (dsrfinfo_retval == DSRFINFO_FORWARDED) {
            ob->inc_native = VDIN_MUNGED;
            ob->forward = fi->forward; fi->forward = NULL;
            pffree(fi);
            vdir_freelinks(dir);
            return dsrfinfo_retval;
        }
#endif
        if (dsrfinfo_retval < 0) {       /* dsrfinfo returns <0 to mean
                                            directory */ 
            ob->flags |= P_OBJECT_DIRECTORY;
            retval = PSUCCESS;
        } else if (dsrfinfo_retval == PSUCCESS) {
            ob->flags |= P_OBJECT_FILE;
            if (requested_contents(listopts)) {
                PATTRIB at = read_contents(hsoname);
                /* Append it to fi->attributes since ob->attributes will get
                   set from that. */
                if (at) APPEND_ITEM(at, fi->attributes);
            }
            retval = PSUCCESS;
        }
    }

#ifndef SERVER_DO_NOT_SUPPORT_FORWARDING
    if (retval == DSRFINFO_FORWARDED) {
        /* Only get here if dsrdir() or dsdb() returned DSRFINFO_FORWARDED */
        ob->inc_native = VDIN_MUNGED;
        ob->forward = dir->f_info->forward; dir->f_info->forward = NULL;
        vdir_freelinks(dir);
        return retval;
    }
#endif
    if (retval) {
        vdir_freelinks(dir);
        if (fi) pffree(fi);
        if (retval == DIRSRV_NOT_DIRECTORY || retval == DSRDIR_NOT_A_DIRECTORY
            || retval == DSRFINFO_NOT_A_FILE)
            return DIRSRV_NOT_FOUND;
        return retval;
    }
    /* Now for some manly calisthenics! 
       Merge f_info and directory info. */
    /* Note this is the model for the VERSION5 version dswobject
       so, if you change this, probably need to change that - Mitra */
    /* Only potential conflict here: ACL.  Handle it by appending the two
       ACLs if both are set. (DIRECTORY first; why not).  If one is unset,
       use the default (DEFAULT SYSTEM for DIRECTORY, CONTAINER for OBJECT).
       If directory and object both have magic numbers set use one from
       directory. 
       */
    if (ob->flags & P_OBJECT_DIRECTORY) {
        ob->version = dir->version;            /* always 0 */
        ob->inc_native = dir->inc_native;
        ob->magic_no = dir->magic_no;
        ob->acl = dir->dacl; dir->dacl = NULL;
        assert(!dir->f_info);   /* should have already been handled */
        ob->links = dir->links; dir->links = NULL;
        ob->ulinks = dir->ulinks; dir->ulinks = NULL;
        ob->native_mtime = dir->native_mtime;
    } else {
        ob->inc_native = VDIN_NOTDIR;
    }
    /* If FINFO present (will not always need to be present), merge it in. */
    if (fi) {
        /* Magic # on directory, if present, supersedes magic # on finfo. */
        if (!ob->magic_no) ob->magic_no = fi->f_magic_no;
        /* Append object ACLs. */
        if (fi->oacl) {
            CONCATENATE_LISTS(ob->acl, fi->oacl); fi->oacl = NULL;
        }
        ob->exp = fi->exp;
        ob->ttl = fi->ttl;
        ob->last_ref = fi->last_ref;
        ob->forward = fi->forward; fi->forward = NULL;
        ob->backlinks = fi->backlinks; fi->backlinks = NULL;
        ob->attributes = fi->attributes; fi->attributes = NULL;
        /* Done merging in attributes from FINFO structure. */
    }

    /* Clean up and return. */
    vdir_freelinks(dir);
    if (fi) pffree(fi); fi = NULL;
    VLDEBUGOB(ob);
    VLDEBUGEND;
    return retval;
}

/* Return a list of OBJECT INTRINSIC ACCESS-METHOD attributes for the local
   real file FILENAME.  These attributes should be usable by the client coming
   from the address client_addr.    One day this function will no longer be
   necessary when we have the Prospero access method working.
   Allocates pattribs and puts them into RETVAL. 

   Note that FILENAME has already been expanded from any HSONAME.  
*/

/* Called by dsrfinfo at the moment. */
void
get_access_method(char filename[], long client_addr, PATTRIB *retval)
{
    PATTRIB at;                 /* temporary working attribute. */
    TOKEN           nfs_am;     /* Access method for NFS (if any) */

    *retval = NULL;              /* return list starts empty. */

    /* Check for PROSPERO-CONTENTS access method */
    /* Always true for any local file. */
    at = newamat();
    at->value.sequence = tkappend("PROSPERO-CONTENTS", at->value.sequence);
    at->value.sequence = tkappend("", at->value.sequence);
    at->value.sequence = tkappend("", at->value.sequence);
    at->value.sequence = tkappend("", at->value.sequence);
    at->value.sequence = tkappend("", at->value.sequence);
    APPEND_ITEM(at, *retval);

    
    /* Check for LOCAL access method. */
    if (myaddress() == client_addr ||
        /* Check for loopback net.  */
#if BYTE_ORDER == BIG_ENDIAN
        (client_addr & 0xff000000) == (127 << 24)
#else
        (client_addr & 0x000000ff) == 127
#endif
        ) {
        /* This may not return information for multi-homed hosts.
           But it will never return incorrect information.  */
        at = newamat();
        at->value.sequence = tkappend("LOCAL", at->value.sequence);
        at->value.sequence = tkappend("", at->value.sequence);
        at->value.sequence = tkappend("", at->value.sequence);
        at->value.sequence = tkappend("", at->value.sequence);
        at->value.sequence = tkappend("", at->value.sequence);
        APPEND_ITEM(at, *retval);
    } else {
#ifdef SHARED_PREFIXES
        char *cp;      /* this memory doesn't need to be freed. */
        if (cp = check_localpath(filename, client_addr)){
            at = newamat();
            at->value.sequence = tkappend("LOCAL", at->value.sequence);
            at->value.sequence = tkappend("", at->value.sequence);
            at->value.sequence = tkappend("", at->value.sequence);
            at->value.sequence = tkappend("", at->value.sequence);
            at->value.sequence = tkappend(cp, at->value.sequence);
            APPEND_ITEM(at, *retval);
        }
#endif            
    }

    /* Check for NFS access method. */
#ifdef NFS_EXPORT
    if(nfs_am = check_nfs(filename,client_addr)) {
        at = newamat();
        at->value.sequence = nfs_am;
        APPEND_ITEM(at, *retval);
    }
#endif NFS_EXPORT

    /* Check for AFS access method.  Note that the hostname is irrelevant. */
    if(*afsdir && strnequal(filename, afsdir, strlen(afsdir))) {
        char *suffix = filename + strlen(afsdir);
        at = newamat();
        at->value.sequence = tkappend("AFS", at->value.sequence);
        at->value.sequence = tkappend("", at->value.sequence);
        at->value.sequence = tkappend("", at->value.sequence);
        at->value.sequence = tkappend("ASCII", at->value.sequence);
        at->value.sequence = tkappend(suffix, at->value.sequence);
        APPEND_ITEM(at, *retval);

#ifdef AFS_AFTP_GATEWAY         /* provide additional access method for sites
                                   that don't run AFS. */
        at = newamat();
        at->value.sequence = tkappend("AFTP", at->value.sequence);
        at->value.sequence = tkappend("INTERNET-D", at->value.sequence);
        at->value.sequence = tkappend(hostname, at->value.sequence);
        at->value.sequence = tkappend("ASCII", at->value.sequence);
        at->value.sequence = 
            tkappend(qsprintf_stcopyr((char *) NULL,
                                      "%s%s", AFS_AFTP_GATEWAY, suffix), 
                     at->value.sequence);
        at->value.sequence = tkappend("BINARY", at->value.sequence);
        APPEND_ITEM(at, *retval);
#endif
    }

    /* Check for AFTP access method. */
    if(*aftpdir && strnequal(filename,aftpdir, strlen(aftpdir))) {
        char *suffix = filename + strlen(aftpdir);

        at = newamat();
        at->value.sequence = tkappend("AFTP", at->value.sequence);
        at->value.sequence = tkappend("INTERNET-D", at->value.sequence);
        at->value.sequence = tkappend(hostname, at->value.sequence);
        at->value.sequence = tkappend("ASCII", at->value.sequence);
        at->value.sequence = tkappend(suffix, at->value.sequence);
        at->value.sequence = tkappend("BINARY", at->value.sequence);
        APPEND_ITEM(at, *retval);
    }
}


/* Allocate a new ACCESS-METHOD attribute. */
static 
PATTRIB
newamat(void)
{
    PATTRIB retval = atalloc();
    retval->aname = stcopyr("ACCESS-METHOD", retval->aname);
    retval->precedence = ATR_PREC_OBJECT;
    retval->nature = ATR_NATURE_INTRINSIC;
    retval->avtype = ATR_SEQUENCE;
    return retval;
}


/* Was the CONTENTS attribute requested on the object itself?  CONTENTS is an
   INTRINSIC- attribute.  Return non-zero (C language "true") if true, zero (C
   language "false") if false. */
int 
requested_contents(struct dsrobject_list_options *listopts)
{
    return listopts 
        && was_attribute_requested("CONTENTS", &listopts->req_obj_ats);
}


/* Return the CONTENTS attribute. */
PATTRIB 
read_contents(char hsoname[])
{
    char *filename;
    PATTRIB retval;
    int fd;                     /* file descriptor to read from. */
    struct stat file_stat_st;
    struct stat * file_stat = &file_stat_st;
    TOKEN tk;                   /* working token */

    /* Code modified from dsrfinfo() */
    /* Expand special file names */
    if((*hsoname != '/') && *aftpdir && strnequal(hsoname,"AFTP",4))
        filename = qsprintf_stcopyr("%s%s", aftpdir, hsoname + 4);
    else 
        filename = stcopy(hsoname);

    fd = open(filename, 0);     /* open for reading */
    if (fd < 0) {
        stfree(filename);
        return NULL;
    }
    if (stat(filename,file_stat) != 0) {
        close(fd);
        stfree(filename);
        return NULL;
    }

    retval = atalloc();
    retval->aname = stcopyr("CONTENTS", retval->aname);
    retval->precedence = ATR_PREC_OBJECT;
    retval->nature = ATR_NATURE_INTRINSIC;
    retval->avtype = ATR_SEQUENCE;
    /* Build a two-element sequence, DATA and a byte stream of the data. */
    retval->value.sequence = tkappend("DATA", retval->value.sequence);

    tk = tkalloc(NULL);
    /* allocate one extra space for the trailing null. */
    tk->token = stalloc(file_stat->st_size + 1);
    /* tk->token is the buffer we fill in.  It remains a valid reference even
       after the APPEND_ITEM.   We call APPEND_ITEM() first so that it's easy
       to abort in case of an error (one less freeing operation to call). */
    APPEND_ITEM(tk, retval->value.sequence);

    /* Read whole contents and close the file.  If either fails, don't set the
       CONTENTS attribute. */
    /* This might fail IF the file size changes between the stat() and the
       read.  Oh well.  */
    if(read(fd, tk->token, file_stat->st_size) != file_stat->st_size  
       || close(fd)) {
        atfree(retval);
        close(fd);
        return NULL;
    }
    p_bst_set_buffer_length_nullterm(tk->token, file_stat->st_size);
    /* Automatically null terminates it for us too. */
    return retval;
}
