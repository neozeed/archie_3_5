/* psrv.h */

/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h> 
 */

#include <usc-license.h>

/* Cranked out by swa@isi.edu */
#ifndef PSRV_H
#define PSRV_H

#ifndef PPARSE_H_INCLUDED       /* needed for INPUT */
#include <pparse.h>
#endif

#if 0 /* This is ridiculous; I don't need it right now. */
/* Return values for the optquery() function. */
#define OPT_UNSET   0           /* Option not set. */
#define OPT_ARGS    1           /* Option set with arguments */
#define OPT_NOARGS  2           /* Option set with no arguments. */

struct opt {
    char *name;
    int used;                   /* reference if used. */
    struct opt *args;          /* The parsed argument string, set by the
                                  application. */ 
    char *argstring;            /* The unparsed arguments.  Not necessarily
                                   useful once the args have been generated. */
    struct opts *next;
};
typedef struct opt *OPT;
typedef struct opt OPT_ST;

extern OPT optalloc(char *name);
extern void optlfree(OPT);
extern OPT optparse(char * optionstr, char separator);
extern int optquery(OPT opts, char * optname);
extern OPT optargs(OPT opts, char * optname); /* Returns a list of tokens that
                                                 are the option's arguments. */
extern int optnargs(OPT opts, char *);    /* # of arguments to the option.  0
                                             if none specified. */
extern OPT optremaining(OPT opts);    /* Any options which haven't been queried
                                         yet. Check for leftovers! */
#endif /* 0 */

/* Temporary hack to get this going.  Don't want to waste the time to do more.
   The only option that currently takes arguments is "attributes".  And sending
   too much stuff back is never wrong, only slightly inefficient.  So we will
   Not Worry About It.
   */ 
#define optdecl char *optset_options
#define optstart(optstr) (optset_options = optstr)
#define opttest(optname) (sindex(optset_options, optname))

/* Flags for srv_change_acl */
#define SCA_LINK 0
#define SCA_DIRECTORY 1
#define SCA_LINKDIR 2
#define SCA_OBJECT 3
#define SCA_MISC 4
#define SCA_CONTAINER 4

#define reply(req, mesg) ardp_breply((req), ARDP_R_INCOMPLETE, (mesg), 0)
#define creply(req, mesg) ardp_breply((req), ARDP_R_COMPLETE, (mesg), 0)
extern int vcreplyf(RREQ req, const char *format, va_list ap);
extern int vreplyf(RREQ req, const char *format, va_list ap);
extern int creplyf(RREQ req, const char *format, ...);
extern int replyf(RREQ req, const char *format, ...);
int error_reply(RREQ req, char formatstring[], ...);


extern int dsrfinfo(char *nm, long magic, PFILE fi);
extern int dswfinfo(char *nm, PFILE fi);




/* This structure defines how you request attributes, whether they be
   associated with the object or with the link.  */
struct requested_attributes {
    TOKEN specific;             /* This is a list of TOKENs.  They are the
                                   names of specific attributes that have been
                                   requested by name. */
    /* These are BITfields that specify various classes or sets of attributes
       that you are looking for. */
    int all:1;
    int interesting:1;
    /* Others to be implemented later on an as-needed basis.  'all' is a
       surrogate for them right now. */
    /* int object:1; */
    /* int field:1; */
};

/* This is called by dsrfinfo(), internally. */
extern int
dsrfinfo_with_attribs(char *nm,long magic,PFILE fi, 
                      struct requested_attributes *req_obj_ats);

/* Special options given to dsrobject() when it's called with the LIST or
   GET-OBJECT-INFO protocol messages on a directory. */ 
struct dsrobject_list_options {
    char **thiscompp;           /* This option only valid for LIST ..
                                   COMPONENTS  */
                                /* if '*' and no remcomp, then exactly
                                      equivalent to empty. */
    TOKEN *remcompp;            /* This option only valid for LIST ..
                                   COMPONENTS  */ 
                                /* If last one '*', exactly equivalent to being
                                   empty. */
    struct requested_attributes 
        req_link_ats;           /* Requested attributes associated with the
                                   links contained by this object (i.e., LINK,
                                   CACHED, REPLACEMENT, or ADDITIONAL, and
                                   OBJECT iff very convenient). */
    struct requested_attributes 
        req_obj_ats;            /* Requested attributes associated with this
                                   object itself.   (only OBJECT precedence).
                                   */
    FILTER filters;             /* This option currently only valid for LIST
                                   ... COMPONENTS.  OBJECT filters, when
                                   implemented, will need this option for
                                   GET-OBJECT-INFO. */ 
/* Get rid of the following definition as soon as all the databases are
   updated.  It is set by dirsrv() and looked at only by some older databases.
   (The older databases will be unable to compile if they are using this.  So
   you'll know.) */ 
#define DSROBJECT_LIST_OPTIONS_REQUESTED_ATTRS_BACK_COMPATIBILITY
#ifdef DSROBJECT_LIST_OPTIONS_REQUESTED_ATTRS_BACK_COMPATIBILITY
    const char *requested_attrs; /* this is now nearly obsolete.  It will be
                                    obsolete as soon as all the databases are
                                    updated.   It is an unholy combination of
                                    req_link_ats and req_obj_ats.  It is either
                                    NULL or consists of a concatenated series
                                    of attribute names, separated by the '+'
                                    character.  Every attribute name is
                                    preceded and followed by a '+' (in other
                                    words, the string starts and
                                    ends with a '+' character.) */
    
#endif /* DSROBJECT_LIST_OPTIONS_REQUESTED_ATTRS_BACK_COMPATIBILITY */
};


#ifdef DSROBJECT_SPEEDUP_IS_EXPERIMENTAL
#define dsrobject_list_options_init(dlo) do {    \
    ZERO(dlo);                                  \
    if (!dsrobject_speedup)                         \
        (dlo)->all = 1;   /* emulate old inefficient behavior */ \
} while(0)
#else
#define dsrobject_list_options_init(dlo) \
    ZERO(dlo);
#endif

/* Used by get_obj_info() and by list(). */
extern void 
p__parse_requested_attrs(const char *requested_atlist_unparsed, 
                         struct requested_attributes * parsed);
extern int
was_attribute_requested(char *atname, struct requested_attributes *parsed);

extern int dsrobject(RREQ req, char hsoname_type[], char hsoname[], 
                     long version, long magic_no, int flags, 
                     struct dsrobject_list_options *listopts, P_OBJECT ob);
#define DRO_NULLOPT (struct dsrobject_list_options *) 0

/* Flags for DSROBJECT */

#define DRO_ACCESS_METHOD_ONLY 0x1 /* only want access-method attribute. */

#define DRO_VERIFY             0x2 /* Verify that the object exists.
                                      This is implemented for
                                      ARCHIE and GOPHER-GW; otherwise, still a
                                      full cost call. */

#define DRO_VERIFY_DIR         0x4 /* Verify that the object exists and that it
                                      is a directory.  This is implemented for
                                      the local PFSDAT directories.  If this is
                                      given to a call that is eventually passed
                                      to a call to a DSDB function, then
                                      whether it is noticed depends upon the
                                      individual DSDB function; might still be
                                      a full-cost call. */

/* --- */
/* From lib/psrv/dsrdir.c */
/* DSRDIR will die soon, superseded by dsrobject().  --swa */
extern int dsrdir(char *name,int magic,VDIR dir,VLINK ul,int flags);
/* Flags for dsrdir() */
#define DSRD_ATTRIBUTES		       0x1 /* Fill in attributes for links */
#define DSRD_VERIFY_DIR                0x2 /* Verify that the directory exists
                                              and is a directory.   Returns
                                              PSUCCESS upon SUCCESS, PFAILURE
                                              upon FAILURE.  This flag implies
                                              that the DIR option to dsrdir()
                                              could be set to NULL with no ill
                                              consequences.  In fact, dsrdir()
                                              does just that, as a safety
                                              check.  */

/* ---- */

/* The following is unimplemented. */
extern void dswobject(char hsonametype[], char hsoname[], P_OBJECT ob);

extern void set_client_addr(long addr); /* short-term hack */

extern size_t nativize_prefix(char *hsoname, char native_buf[], 
			size_t native_bufsiz);

extern int dswdir(char *name, VDIR dir);
extern int fdswdir_v5(FILE *vfs_dir, VDIR dir);
TOKEN   check_nfs(char filename[], long client_addr);
char *check_localpath(char filename[], long client_addr);
extern int srv_check_acl(ACL pacl,ACL sacl,RREQ req,char *op,int flags,char *objid,char *itemid);

extern int change_acl(ACL *aclp, ACL ae, RREQ req, int flags, ACL diracl);
extern ACL get_container_acl(char *path);

extern ACL maint_acl;

extern int delete_matching_at(PATTRIB key, PATTRIB *headp, 
                              int (*equal)(PATTRIB, PATTRIB));
extern int delete_matching_fl(FILTER key, FILTER *headp);


#ifdef PSRV_ARCHIE              /* This is still old format. */
extern int arch_dsdb(RREQ    req,        /* Request pointer (unused)            */
              char    *hsoname,   /* Name of the directory               */
              long version,       /* Version #; currently ignored        */
              long magic_no,      /* Magic #; currently ignored          */
              int flags,          /* Currently only recognize DRO_VERIFY */
              struct dsrobject_list_options *listopts, /* options (use *remcompp
                                                          and *thiscompp)*/
              P_OBJECT    ob);     /* Object to be filled in */
#endif
#ifdef PSRV_GOPHER              /*  Currently unused.  Probably never used. */
#error this is unimplemented
int gopher_dsdb(RREQ req, char name[], char **componentsp, TOKEN *rcompp, 
                VDIR dir, int options, const char *rattrib, FILTER filters);
#endif
#ifdef PSRV_GOPHER_GW
int gopher_gw_dsdb(RREQ req, char hsoname[], long version, long magic_no, 
                   int flags, struct dsrobject_list_options *listopts, 
                   P_OBJECT ob);
void gopher_gw_init_mutexes(void);
#ifndef NDEBUG
void gopher_gw_diagnose_mutexes(void);
#endif /*NDEBUG*/
/* Memory allocators used inside GOPHER_GW library. */
extern int glink_count, glink_max;
#endif
#ifdef PSRV_WAIS_GW
int wais_gw_dsdb(RREQ req, char hsoname[], long version, long magic_no, 
                   int flags, struct dsrobject_list_options *listopts, 
                   P_OBJECT ob);
void wais_gw_init_mutexes(void);
/* Memory allocators used inside WAIS library. */
extern int waismsgbuff_count, waismsgbuff_max;
extern int ietftype_count, ietftype_max;
extern int waissource_count, waissource_max;
#ifndef NDEBUG
void wais_gw_diagnose_mutexes(void);
#endif /*NDEBUG*/
#endif
#ifdef PSRV_KERBEROS
extern int 
check_krb_auth(char *auth, struct sockaddr_in client, char **ret_client_name);
#endif

extern char     shadow[];
extern char	pfsdat[];
extern char	dirshadow[];
extern char	dircont[];
extern char     security[];


extern char     root[];

extern char	aftpdir[];

extern char	afsdir[];
extern char	*db_prefix;     /* For old v1 support. */
extern char	hostname[];    /* Server's host name */
extern char	hostwport[]; /* Host name w/ port if non-standard
                                             */ 
extern char	*logfile_arg;


struct db_entry {
    char        *hsoname_type; /* hsoname type for database; should always
                                     be ASCII */
    char	*prefix;       /* Prefix for database */
    /* Hsonametype is already known; doesn't need to be passed. */
    int		(*read_function)(RREQ req, char hsoname[], long version,
                               long magic_no, int flags,
                               struct dsrobject_list_options *listopts,
                               P_OBJECT ob);
    int         (*write_function)(char hsoname[], P_OBJECT ob);
    char        *named_acl;  /* name of the NAMED ACL that the database
                                   reads as a pre-filter.  If no such ACL or if
                                   NULL, unused.  Normally same as db_prefix.
                                   */ 
};

extern struct db_entry 	db_prefixes[];
extern int		db_num_ents;

#define OBJECT_VNO            5   /* version # of the object format.  */
                                  /* Used internally by dswobject() and
                                     dsrobject(). */

extern int get_named_acl(char *t_name, ACL *wacl);
extern int set_named_acl(char *t_name, ACL wacl);

/* lib/psrv/magic.c */
extern long generate_magic(VLINK vl);
extern int magic_no_in_list(long magic, VLINK links);

/* lib/psrv/dsrobject.c */
extern int requested_contents(struct dsrobject_list_options *listopts);

/* lib/psrv/psrv_mutexes.c */

extern void psrv_init_mutexes(void);
#ifndef NDEBUG
extern void psrv_diagnose_mutexes(void);
#endif
extern void p_init_shared_prefixes(void);
extern void p_srv_check_acl_initialize_defaults(void);
#ifdef PFS_THREADS
p_th_mutex p_th_mutexPSRV_CHECK_ACL_INET_DEF_LOCAL;
p_th_mutex p_th_mutexPSRV_CHECK_NFS_MYADDR;
p_th_mutex p_th_mutexPSRV_LOG;
#endif


extern void get_access_method(char filename[], long caddr, PATTRIB *retval);
extern int set_logfile(char *filename);
extern int check_handle(char *handle);
extern void srv_add_client_to_acl(char *rights, RREQ r, ACL *a, int f);
extern int check_prvport(RREQ req);
extern void close_plog(void);
extern retrieve_fp(VLINK l);

extern int stat_object(P_OBJECT ob); /* not yet defined. */



#ifdef DIRECTORYCACHING
extern int dsrobject_fail;      /* How many times did DSROBJECT fail?  Used
                                   only in a DIRECTORYCACHING context. */
#endif /* DIRECTORYCACHING */
#endif /*PSRV_H*/

 
