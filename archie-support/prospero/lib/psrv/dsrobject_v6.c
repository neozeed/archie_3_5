/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 */

#include <usc-copyr.h>
#include <pfs.h>
#include <perrno.h>
#include <psrv.h>
#include <plog.h>

static PATTRIB newamat(void);

/* Union links are expanded at a higher level.
 * The only message dsrobject() sends to the client is an ardp_rwait().
 * dsrobject() logs NOT-AUTHORIZED failures via plog, but doesn't send messages
 to the client; the caller does that.
 * Checks to make sure hsoname is within the space we're allowed to discuss.
 * Does check to make sure you have r, G, or g rights on a database before it 
 lets you scan that database for information.
 */
 
/* flags:
   DRO_VERIFY
*/
/*
  Returns:
  DSRFINFO_FORWARDED
  DIRSRV_NOT_AUTHORIZED
  PFAILURE
  PSUCCESS
*/
/* If called with the DRO_VERIFY flag, will set the P_OBJECT_FILE and
   P_OBJECT_DIRECTORY flags in the ob structure, but will not *necessarily* put
   any other information into the ob structure.  Caller is still responsible
   for freeing any links pointed to by the ob structure. */ 
/* dsrobject(), if it returns anything other than DSRFINFO_FORWARDED or
   PSUCCESS, may leave the ob structure with arbitrary data in it. 
   The caller should not expect the ob structure to remain 
*/

/* The database ACL itself may be obtained by using an HSONAME that is just the
   root prefix of the database. */
int
dsrobject(RREQ req, char hsoname_type[], char hsoname[], long version, 
          long magic_no,  
          int flags, struct dsrobject_list_options *listopts, P_OBJECT ob)
{
    VDIR_ST dir_st;
    register VDIR dir = &dir_st;
    register PFILE fi = NULL;            /*  Only set if it needs to be. */
    int     retval;             /* Integer return value from subfunctions */

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

    if (*hsoname != '/') {	/* Database or special prefix in use */
        register int i;
	for (i = 0; i < db_num_ents; i++) {
	    if (strnequal(hsoname, db_prefixes[i].db_prefix,
                          strlen(db_prefixes[i].db_prefix))) {
		if (db_prefixes[i].db_acl
		    && !srv_check_acl(db_prefixes[i].db_acl, NULL, req, "r",
                                      SCA_LINKDIR,db_prefixes[i].db_prefix,NULL)
		    && !srv_check_acl(db_prefixes[i].db_acl, NULL, req, "g",
                                      SCA_LINKDIR,db_prefixes[i].db_prefix,NULL)
		    && !srv_check_acl(db_prefixes[i].db_acl, NULL, req, "G",
                                      SCA_LINKDIR,db_prefixes[i].db_prefix,NULL)) {
                    plog(L_AUTH_ERR, req, 
                         "Unauthorized database request: %s %s",
                         hsoname, listopts? *(listopts->thiscompp) : "");
                    vdir_freelinks(dir);
                    return DIRSRV_NOT_AUTHORIZED;
                }
                /* This could take a while, tell client not to retry */
		ardp_rwait(req, 180, 0, 0);
		retval = db_prefixes[i].db_function(req, hsoname, 
                    listopts? listopts->thiscompp : NULL,
                    listopts? listopts->remcompp : NULL, dir, 
                    (flags & DRO_VERIFY) ? DSDB_VERIFY : 0,
                    "#INTERESTING" /* requested attributes */, 
                    listopts ? listopts->filters : NULL);
		goto dbquery_done;
	    }
	}
        /* Here we have a prefix that is not a database, */
        /* but not a normal filename either */
        /* If normal file names need not begin with /    */
        /* fall through and try a normal query           */
    }
    /* Local directories can have associated finfo too. */
    retval = dsrdir(hsoname, magic_no, dir, NULL, DSRD_ATTRIBUTES);
    if (retval == PSUCCESS) ob->flags |= P_OBJECT_DIRECTORY;
    if (retval == PSUCCESS || retval == DIRSRV_NOT_DIRECTORY) {
        fi = pfalloc();
        retval = dsrfinfo(hsoname,magic_no,fi);
        if (retval == DSRFINFO_FORWARDED) {
            ob->inc_native = VDIN_MUNGED;
            ob->forward = fi->forward; fi->forward = NULL;
            pffree(fi);
            vdir_freelinks(dir);
            return retval;
        }
        if (retval < 0) {       /* dsrfinfo returns <0 to mean directory */
            ob->flags |= P_OBJECT_DIRECTORY;
            retval = PSUCCESS;
        } else if (retval == PSUCCESS) {
            ob->flags |= P_OBJECT_FILE;
        }
    }
dbquery_done:
    if (retval == DSRFINFO_FORWARDED) {
        /* Only get here if dsrdir() or dsdb() returned DSRFINFO_FORWARDED */
        ob->inc_native = VDIN_MUNGED;
        ob->forward = dir->f_info->forward; dir->f_info->forward = NULL;
        vdir_freelinks(dir);
        return retval;
    }
    if (retval) {
        vdir_freelinks(dir);
        if (fi) pffree(fi);
        return retval;
    }
    /* Now for some manly calisthenics! 
       Merge f_info and directory info. */
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
    if (ob->flags & P_OBJECT_FILE) {
        if (fi->f_magic_no) ob->magic_no = fi->f_magic_no;
        if (fi->oacl) {
            CONCATENATE_LISTS(ob->acl, fi->oacl); fi->oacl = NULL;
        }
        ob->exp = fi->exp;
        ob->ttl = fi->ttl;
        ob->last_ref = fi->last_ref;
        ob->forward = fi->forward; fi->forward = NULL;
        ob->backlinks = fi->backlinks; fi->backlinks = NULL;
        ob->attributes = fi->attributes; fi->attributes = NULL;
    }
    vdir_freelinks(dir);
    if (fi) pffree(fi); fi = NULL;
    return retval;
}

/* Return a list of OBJECT INTRINSIC ACCESS-METHOD attributes for the local
   real file FILENAME.  These attributes should be usable by the client coming
   from the address client_addr.    One day this function will no longer be
   necessary when we have the Prospero access method working.
   Allocates pattribs and puts them into RETVAL. 

   Note that FILENAME has already been expanded from any HSONAME.  
*/

void
get_access_method(char filename[], long client_addr, PATTRIB *retval)
{
    PATTRIB at;                 /* temporary working attribute. */
    TOKEN           nfs_am;     /* Access method for NFS (if any) */

    *retval = NULL;              /* return list starts empty. */

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
