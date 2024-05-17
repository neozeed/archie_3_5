/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#ifndef PFS_H_INCLUDED
#define PFS_H_INCLUDED

#include <pfs_utils.h>	/* Some things moved here, so ardp can include them*/
#include <ardp.h>	/* Types defined in ARDP.H are used in the prototypes
			   of functions defined in this file. */

#include <pmachine.h>	/* Must be after ardp.h */

#if defined(SOLARIS)
#define VDIR PVDIR
#endif

/*
 * PFS_RELEASE identifies the Prospero release.  PFS_SW_ID identifies 
 * the particular software and version of the software being used.  
 * Applications using Prospero must request a string to identify their
 * release.  Software IDs may be obtained from info-prospero@isi.edu.
 */
#define               PFS_RELEASE     "PreAlpha.5.3 17 May 94, patchlevel b"
#define               PFS_SW_ID       "P53.17May94b"


/* General Definitions */

#define		MAX_VPATH	1024	 /* Max length of virtual pathname   */

/* Definition of structure containing information on virtual link            */
/* Some members of this structure were recently renamed. The  #defined       */
/* aliases exist only for backwards compatability                            */
/* Uncomment the aliases if you need this backwards compatability.           */
#if 0
#define    type		target	    /* compat (will go away)                 */
#define    nametype	hsonametype /* compat (will go away)                 */
#define    filename     hsoname     /* compat                                */
#endif

struct vlink {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    int                 consistency;
#endif
    int			dontfree;   /* Flag: don't free this link            */
    int                 flags;      /* Special server flags; *vl5 */
    char		*name;	    /* Component of pathname: NAME-COMPONENT */
    char		linktype;   /* L = Link, U = Union, N= Native, 
                                       I = Invisible */
    int			expanded;   /* Has a union link been expanded? *vl4  */
    char		*target;    /* Type of the target of the link *vl1   */
    struct filter       *filters;   /* Filters to be applied when link used  */
    struct vlink	*replicas;  /* Replicas *vl2                         */
    char		*hosttype;  /* Type of hostname		             */
    char		*host;	    /* Files physical location	             */
    char		*hsonametype;/* Type of hsoname                      */
    char		*hsoname;   /* Host specific object name             */
    long		version;    /* Version # of dest: OBJECT-VERSION     */
    long		f_magic_no; /* File's magic number.  *vl3            */
    struct pattrib      *oid;       /* Head of a list of object identifiers  */
    struct acl		*acl;       /* ACL for link		             */
    long		dest_exp;   /* Expiration for dest of link. 0=unset  */
    struct pattrib	*lattrib;   /* Attributes associated w/ link         */
    struct pfile	*f_info;    /* Client uses 2 assoc info w/ refed obj */
    union {
        int flg;		/* Flag or number */
        void *ptr;              /* Generic Pointer */
    } app;
    struct vlink	*previous;  /* Previous elt in linked list           */
    struct vlink	*next;	    /* Next element in linked list           */
};

typedef struct vlink *VLINK;
typedef struct vlink VLINK_ST;

/* *vl1 Target of link:  For links in a directory that don't point   */
/*      to objects, could be SYMBOLIC or EXTERNAL. For a forwarding  */
/*      pointer it is FP.  Links to objects will have a target field */
/*      indicating the object's BASE-TYPE attribute.  When stored on */
/*      a vlink structure, it may have exactly one of the following  */
/*      forms: "OBJECT", "FILE", "DIRECTORY", "DIRECTORY+FILE".      */
/*      There is no guarantee that the type of the target has not    */
/*      changed.  Thus, this field is the cached value of the        */
/*      object's base type.                                          */

/* *vl2 Note that vlink->replicas is not really a list of replicas   */
/*      of the object.  Instead, it is a list of the objects         */
/*      returned during name resolution that share the same name as  */
/*      the current object.  Such an object should only be           */
/*      considered a replica if it also shares the same non-zero     */
/*      object identifiers.                                          */

/* *vl3 For f_magic_no, zero means unset.  Also, This is the value   */
/*      of the ID REMOTE type.  It must be updated at the same time  */
/*      that the appropriate attribute hanging off of the OID member */
/*      is. This speeds up checks.   Do not write code that depends  */
/*      on this member being present; it will go away Very Soon Now. */

/* *vl4 The EXPANDED member of the VLINK structure is normally set to either
        FALSE or TRUE or FAILED (all three of which are defined at the end of
        PFS.H). In addition, dsrdir(), ul_insert(), and list_name() work
        together to test and set the following two possible values for it.
        These values are only used on the server. */

/*      This virtual link points to the initial directory given dsrdir() */
#define ULINK_PLACEHOLDER 2 

/*      This virtual link should not be expanded because the directory it
        refers to has already been expanded.  However, it should be returned in
        a listing if the LEXPAND option was not specified to the LIST command.
        */ 
#define ULINK_DONT_EXPAND 3

/* *vl5 The FLAGS member of the VLINK structure can have the following values.
        These are used only on the server. */
#define VLINK_CONFIRMED    0x1 /* Used internally in dsdir.c on the server */
#define VLINK_NATIVE       0x2 /* Native link (cached).  Should be stored in
                                  the database with type N or n. */

/* This flag is used to check whether the link has already been freed.    */
/* definition for virtual directory structure */
struct vdir {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    int                 consistency;
#endif
    int		   version;     /* Version no of directory instance (curr 0) */
    int            flags;       /* Flags *vd1 */
    int		   inc_native;	/* Include the native directory    *vd2      */
    long	   magic_no;	/* Magic number of this directory            */
    struct acl	   *dacl;       /* Default acl for links in dir              */
    struct pfile   *f_info;	/* Directory file info                       */
    struct vlink   *links;	/* The directory entries	             */
    struct vlink   *ulinks;	/* The entries for union links               */
    struct dqstate *dqs;	/* State needed by pending query.  Free when
                                   unused. */
    int		   status;	/* Status of query                           */
    time_t         native_mtime; /* last time native directory modified.  
                                   Only used on the server. */
    struct pattrib *attributes;  /* used only on the client, and even then only
                                    if attributes were returned by a LIST
                                    operation. */ 
    union {
        int flg;		/* Flag or number */
        void *ptr;              /* Generic Pointer */
    } app;
};

typedef struct vdir *VDIR;
typedef struct vdir VDIR_ST;

/* Initialize directory */
/* lib/psrv/dsrobject.c assumes that a directory that has been the target of
   vdir_init() will not need to have vdir_freelinks() applied to it unless it
   has since been passed to a function that might add members to it. --swa,
   5/14/94. */
#ifdef ALLOCATOR_CONSISTENCY_CHECK
#define vdir_init(dir) do { \
                        dir->consistency = INUSE_PATTERN;           \
			dir->version = 0;     dir->inc_native = 0;  \
                        dir->flags = 0;                             \
                        dir->magic_no = 0;    dir->dacl = NULL;     \
                        dir->f_info = NULL;   dir->links = NULL;    \
    			dir->ulinks = NULL;   dir->dqs = NULL;      \
			dir->status = DQ_INACTIVE; dir->native_mtime = 0; \
                        dir->attributes = NULL;         } while (0)
#else
#define vdir_init(dir) do { \
			dir->version = 0;     dir->inc_native = 0;  \
                        dir->flags = 0;                             \
                        dir->magic_no = 0;    dir->dacl = NULL;     \
                        dir->f_info = NULL;   dir->links = NULL;    \
    			dir->ulinks = NULL;   dir->dqs = NULL;      \
			dir->status = DQ_INACTIVE; dir->native_mtime = 0; \
                        dir->attributes = NULL;         } while (0)
#endif

#define vdir_copy(d1,d2) do { \
                         d2->consistency = d1->consistency; \
    			 d2->version = d1->version; \
                         d2->flags = d1->flags; \
                         d2->inc_native = d1->inc_native; \
                         d2->magic_no = d1->magic_no; \
                         d2->dacl = d1->dacl; \
    			 d2->f_info = d1->f_info; \
                         d2->links = d1->links; \
                         d2->ulinks = d1->ulinks; \
			 d2->status = d1->status; \
			 dir->dqs = NULL; \
                         d2->native_mtime = d1->native_mtime; \
                         } while (0)
                         
/* Double freeing is OK here. */
#ifdef ALLOCATOR_CONSISTENCY_CHECK
#define vdir_freelinks(dir) do {                        \
        assert(dir->consistency == FREE_PATTERN         \
               ||dir->consistency == INUSE_PATTERN);    \
        dir->consistency = FREE_PATTERN;                \
        dir->flags = 0;                                 \
        aclfree(dir->dacl); dir->dacl = NULL;           \
        pflfree(dir->f_info); dir->f_info = NULL;       \
        vllfree(dir->links); dir->links = NULL;         \
        vllfree(dir->ulinks); dir->ulinks = NULL;       \
        atlfree(dir->attributes); dir->attributes = NULL; \
     } while(0)
#else
#define vdir_freelinks(dir) do {                        \
        dir->flags = 0;                                 \
        aclfree(dir->dacl); dir->dacl = NULL;           \
        pflfree(dir->f_info); dir->f_info = NULL;       \
        vllfree(dir->links); dir->links = NULL;         \
        vllfree(dir->ulinks); dir->ulinks = NULL;       \
        atlfree(dir->attributes); dir->attributes = NULL; \
     } while(0)
#endif

struct p_object {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    int                 consistency;
#endif
    int             version;    /* Version of this instance of the object
                                   (curr. always 0) */
    int            flags;       /* Flags *ob1 */
    int		   inc_native;	/* Include the native directory    *vd2      */
    long	   magic_no;	/* Magic number of this object               */
    struct acl	   *acl;        /*  Acl this object.  If a directory, default
                                    ACL for links in dir                     */
    time_t		exp;		  /* Expiration date/timeout     *ob3*/
    long		ttl;		  /* Time to live after reference*ob3*/
    time_t		last_ref;	  /* Time of last reference     *ob3 */
    struct vlink	*forward;	  /* List of forwarding pointers *ob4*/
    struct vlink	*backlinks;	  /* Partial list of back links  *ob3*/
    struct pattrib	*attributes;	  /* List of object attributes       */
    struct vlink   *links;	/* The directory entries	             */
    struct vlink   *ulinks;	/* The entries for union links               */
    time_t         native_mtime; /* last time native object modified.  *ob5 */
    char           *shadow_file; /* Name of the shadow file to write
                                    this back to.  Used only on the server. */
    int		   status;	/* Status of directory query.  Client only. */
    struct dqstate *dqs;	/* State needed by pending directory query.
                                   Client only.  Not freed by obfree(). */
    struct stat *statbuf;       /* Buffer allocated for stat()s.  Currently we
                                   use stalloc() to stuff it with memory.  This
                                   isn't a great way of doing things but it's
                                   expedient. */
    union {
        int flg;		/* Flag or number */
        void *ptr;              /* Generic Pointer */
    } app;
    struct p_object *previous;  /* Previous element in linked list. */
    struct p_object *next;      /* Next element in linked list. */
};
typedef struct p_object P_OBJECT_ST;
typedef struct p_object *P_OBJECT;
/* *ob1 / *vd1 The FLAGS member of the VDIR and P_OBJECT structure can have
   the following values. */ 
/*  This is only set and tested by vl_insert(). */
#define VDIR_UNSORTED             0x1
/* This is only set and tested by dsrdir() on the server. */
/* Means the directory did not contain any links (except, possibly, for a
   placeholder union link) when it was passed to dsrdir(). */
#define VDIR_FIRST_READ                0x2
/* *ob1: This is only used for the p_object format: */
#define P_OBJECT_FILE                   0x4     /* This object has an
                                                   associated FILE  */
#define P_OBJECT_DIRECTORY           0x8     /* This object is a DIRECTORY  */


/* *vd2 Values of ->inc_native in vdir and p_object structure            */
/* This is irrelevant for P_OBJECT_FILE, except for the VDIN_PSEUDO and
   VDIN_MUNGED.  Defult will be VDIN_NOTDIR. */
/* Note: VDIN_NONATIVE has the same value as FALSE     */
/*       VDIN_INCLNATIVE has the same value as TRUE    */
#define VDIN_INCLREAL	-1   /* Include native files, but not . and ..       */
#define VDIN_NONATIVE	 0   /* Do not include files from native directory   */
#define VDIN_INCLNATIVE	 1   /* Include files from native directory          */
#define VDIN_ONLYREAL    2   /* All entries are from native dir (no . and ..)*/
#define VDIN_PSEUDO      3   /* Directory or object is from a database.      */
#define VDIN_MUNGED      4   /* Object must not be written back; union links
                                have been expanded, filters applied, or a
                                restricted set of attributes returned. */
#define VDIN_UNINITIALIZED 5    /* Uninitialized. */
#define VDIN_NOTDIR      6  /* Not a native directory; just a file or object.
                               */ 

/* *ob3 This member is not yet implemented  and must be zero.               */
/* *ob4 The list of forwarding pointers member is used only on the server.  
    It will always be NULL on the client. */

/* *ob5 This is used only on the server.  It is used to handle caching of 
    native directory information if the object is a directory. */



/* Definition of structure containing filter information             */
struct filter {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    int                 consistency;
#endif
    char 		*name;  /* For PREDEFINED filters only  *f1  */
    struct vlink 	*link;  /* For LOADABLE filters only    *f1  */
    short		type;   /* dir, hierarchy, obj, upd     *f2  */
    short   execution_location; /* FIL_SERVER or FIL_CLIENT     *f3  */
    short 	   pre_or_post; /* FIL_PRE or FIL_POST          *f4  */
    short		applied;/* Flag - set means has been applied */
    struct token	*args;  /* Filter arguments                  */
    char                 *errmesg; /* error message */
    union {
        int flg;		/* Flag or number */
        void *ptr;              /* Generic Pointer */
    } app;
    struct filter	*next;  /* Next filter in linked list        */
    struct filter    *previous; /* Previous filter in linked list    */
};
typedef struct filter *FILTER;
typedef struct filter FILTER_ST;

/* *f1: Exactly one of the following two will be set.  This also lets us */
/*      distinguish between the two basic types of filters, PREDEFINED   */
/*      and LOADABLE.                                                    */ 

/* *f2: Values for the type field */
#define FIL_DIRECTORY       1
#define FIL_HIERARCHY       2
#define FIL_OBJECT          3
#define FIL_UPDATE          4    

/* *f3: Values for the execution_location field */
#define FIL_SERVER          1
#define FIL_CLIENT          2

/* *f4: Values for the pre_or_post field */
#define FIL_PRE             1
#define FIL_POST            2
#define FIL_ALREADY         3   /* already expanded! */
#define FIL_FAILED          4   /* Failed to expand it; error message can go
                                   into fil->errmesg.  */
/* This is the structure for attribute values.  */
union avalue {
    struct filter       *filter;   /* A filter                            */
    struct vlink	*link;	   /* A link		                  */
    struct token        *sequence; /* Most general type (subsumes string) */
};

/* definition of structure containing attribute information */
struct pattrib {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    int                 consistency;
#endif
    char		precedence;  /* Precedence for link attribute *a1  */
    char                nature;      /* FIELD, APPLICATION or INTRINSIC *a2 */
    char		avtype;	     /* Type of attribute value (sm int) *a3 */
    char		*aname;	     /* Name of the attribute              */
    union avalue	value;	     /* Attribute Value                    */
    union {
        int flg;		/* Flag or number */
        void *ptr;              /* Generic Pointer */
    } app;
    struct pattrib	*previous;   /* Previous element in linked list    */
    struct pattrib	*next;	     /* Next element in linked list        */
};

typedef struct pattrib *PATTRIB;
typedef struct pattrib PATTRIB_ST;

/* *a1: values for the precedence field. */
#define         ATR_PREC_UNKNOWN 'X'   /* Default setting. */
#define 	ATR_PREC_OBJECT  'O'   /* Authoritative answer for object */
#define 	ATR_PREC_LINK    'L'   /* Authoritative answer for link   */
#define 	ATR_PREC_CACHED  'C'   /* Object info cached w/ link      */
#define 	ATR_PREC_REPLACE 'R'   /* From link (replaces O)          */
#define 	ATR_PREC_ADD     'A'   /* From link (additional value)    */

/* *a2: values for the nature field. */
#define         ATR_NATURE_UNKNOWN        '\0'
#define         ATR_NATURE_FIELD          'F'
#define         ATR_NATURE_APPLICATION    'A'
#define         ATR_NATURE_INTRINSIC      'I'

/* *a3: Values for the AVTYPE field. */
#define         ATR_UNKNOWN         0  /* error return value and init value */
#define         ATR_FILTER          1
#define         ATR_LINK            2
#define         ATR_SEQUENCE        3


/* Definition of structure containing information on a specific file */
/* **** Incomplete ****                                              */
struct pfile {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    int                 consistency;
#endif
    int			version;	  /* Version of local finfo format   */
    long		f_magic_no;	  /* Magic number of current file    */
    struct acl		*oacl;		  /* Object ACL                      */
    long		exp;		  /* Expiration date of timeout      */
    long		ttl;		  /* Time to live after reference    */
    long		last_ref;	  /* Time of last reference          */
    struct vlink	*forward;	  /* List of forwarding pointers     */
    struct vlink	*backlinks;	  /* Partial list of back links      */
    struct pattrib	*attributes;	  /* List of file attributes         */
    struct pfile	*previous;        /* Previous element in linked list */
    struct pfile	*next;		  /* Next element in linked list     */
};

typedef struct pfile *PFILE;
typedef struct pfile PFILE_ST;

/* Definition of structure contining an access control list entry */
struct acl {
    int			acetype;     /* Access Contol Entry type            */
    char		*atype;      /* Authent type if acetyep=ACL_AUTHENT */
    char		*rights;     /* Rights                              */
    struct token        *principals; /* Authorized principals               */
    struct restrict     *restrictions; /* Restrictions on use               */
    union {
        int flg;		/* Flag or number */
        void *ptr;              /* Generic Pointer */
    } app;
    struct acl		*previous;     /* Previous elt in linked list       */
    struct acl		*next;	       /* Next element in linked list       */
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    int                 consistency;
#endif
};
typedef struct acl *ACL;
typedef struct acl ACL_ST;

/* Values for acetype field */
#define ACL_UNINITIALIZED       0         /* Should never be displayed */
#define ACL_NONE		1         /* Nobody authorized by ths entry */
#define ACL_DEFAULT		2         /* System default                 */
#define ACL_SYSTEM		3         /* System administrator           */
#define ACL_OWNER               4         /* Directory owner                */
#define ACL_DIRECTORY           5         /* Same as directory              */
#define ACL_CONTAINER           6         /* Enclosing directory            */
#define ACL_ANY                 7         /* Any user                       */
#define ACL_AUTHENT             8         /* Authenticated principal        */
#define ACL_LGROUP              9         /* Local group; NAMED ACLs.       */
#define ACL_GROUP               10         /* External group; unimplemented */
#define ACL_ASRTHOST            11        /* Check host and asserted userid */
#define ACL_TRSTHOST            12        /* ASRTHOST from privileged port  */
#define ACL_IETF_AAC            13        /* Reserved IETF-AAC acl format   */
#define ACL_SUBSCRIPTION	14	  /* LGROUP w/alt failure semantics */
#define ACL_DEFCONT             15       /* Used only internally by the server.
                                            Treated as DEFAULT or CONTAINER,
                                            depending on whether CONTAINER
                                            exists. */


/* Note that any named LGROUP or SUBSCRIPTION entries, plus the special */
/* names SYSTEMS, DEFAULT, and MAINTENANCE are treated as "named" ACLs  */

/* Definition of structure contining access restrictions */
/* for future extensions                                 */
struct restrict {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    int                 consistency;
#endif
/* Are these really pointers to acl - or should they be struct restrict -Mitra*/
    struct acl		*previous;        /* Previous elt in linked list    */
    struct acl		*next;		  /* Next element in linked list    */
};

/* Definition of "linked list of strings" structure       */
/* Used for ACL principalnames, for values of attributes, */
/* and other uses.                                        */
struct token {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    int                 consistency;
#endif
    char *token;            /* string value of the token    */
    struct token *next;     /* next element in linked list  */
    struct token *previous; /* previous element in list     */
};

typedef struct token *TOKEN;
typedef struct token TOKEN_ST;

/* Authorization/authentication information */
struct pfs_auth_info {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    int                 consistency;
#endif
    int		ainfo_type;     /* Type of auth info                 *ai1 */
    char	*authenticator; /* For storing auth data                  */
    TOKEN	principals;     /* Principals authenticated by above *ai2 */
#ifdef P_ACCOUNT
    char        *acc_server;            /* Accounting server */
    char        *acc_name;              /* Account name */
    char        *acc_verifier;          /* Verifying string for account */
    char        *int_bill_ref;          /* Internal billing reference */
    int         session_id;             /* Id of session */
    char        *session_verifier;      /* Verifier string for session */
    struct amt_info  *allocated_amts;   /* Allocated amounts */
#endif
    union {
        int flg;		/* Flag or number */
        void *ptr;              /* Generic Pointer */
    } app;
    struct pfs_auth_info  *next;/* Next element in linked list            */
};

#ifdef P_ACCOUNT
struct decimal {  /* Amount = sig * 10^exp */
    long  sig;    /* Significant digits */
    short exp;    /* Exponent */
};

struct amt_info {
    char             *currency;         /* Type of currency */
    struct decimal   limit;             /* Max amount in currency */
    struct decimal   amt_spent;         /* Amount spent in currency */
    struct amt_info  *next;             /* Next element in linked list */
};
#endif

typedef struct pfs_auth_info *PAUTH;
typedef struct pfs_auth_info PAUTH_ST;

/* ai1: This field is not necessarily used on the server side         */
/* ai2: At the moment only one principal is specified per authenticator */

/* Values for ainfo_type */
/* Authentication methods */
#define PFSA_UNAUTHENTICATED    0x00000001
#define PFSA_KERBEROS           0x00000002
#define PFSA_P_PASSWORD         0x00000003

/* Authorization methods */
/*
#define xxxxx                   0x1000xxxx
*/

/* Accounting methods */
#define PFSA_CREDIT_CARD        0x20000001

/* Audit methods */
/*
#define xxxxx                   0x3000xxxx
*/

#define AUTHENTICATOR_SZ     1000 /*  A useful size for holding a Kerberos or
                                      other authenticator */

/********************/

/* p_initialize must be called before any functions in the PFS library are
   called.  */
/* Currently no options to set in the p_initialize_st */
struct p_initialize_st {
};

extern void p_thread_initialize(void);
extern void p_initialize(char *software_id, int flags, 
                         struct p_initialize_st * arg_st);
#ifndef NDEBUG
extern void p_diagnose(void);
#endif
/* No flags currently defined. */

/* Internal global function called by p_initialize */
extern void p__set_sw_id(char *app_sw_id);

/*********************/

#ifndef _TYPES_
#include <sys/types.h>
#endif _TYPES_
#include <stdio.h>              /* Needed for def of FILE, for args to
				   qfprintf.  Also gives us NULL and size_t, 
				   both of which we need. */ 

#if 0
#include <stddef.h>		/* guarantees us NULL and size_t, which ANSI
				   already gives us in <stdio.h> */
#endif
#include <stdarg.h>		/* needed for variable arglist function
				   prototypes (provides va_list type) */


/* Unfortunately, some buggy compilers (in this case, gcc 2.1 under
   VAX BSD 4.3) get upset if <netinet/in.h> is included twice. */
#if !defined(IP_OPTIONS)
#include <netinet/in.h> 
#endif
/* Status for directory queries - the following or a Prospero error code */
#define      DQ_INACTIVE   0   /* Query not presently active             */
#define      DQ_COMPLETE  -1   /* Directory query completed              */
#define      DQ_ACTIVE    -2   /* Directory query in progress            */

/* State information for directory query.  Used only in p_get_dir(). */
struct dqstate {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    int                 consistency;
#endif
    VLINK	dl;       /* Link to queried directory                 */
    char	*comp; /* Name of components to return       */
    int         stfree_comp;    /* should comp be stfreed() when dqstate is? */
    TOKEN	*acomp;	  /* Additional components to resolve          */
    long	flags;	  /* Flags		                       */
    int		fwdcnt;	  /* Number of fwd pointers to chase           */
    int		newlinks; /* Number of new links added this query      */
    int		mcomp;	  /* Multiple components to be resolved (flag) */
    int		unresp;   /* We have unresolved components to process  */
    VLINK	clnk;     /* Current link being filled in              */
    VLINK 	cexp; 	  /* The current ulink being expanded          */
    VLINK	pulnk;    /* Prev union link (insert new one after it) */
    RREQ	preq;	  /* Pending request structure            */
};


#ifdef P_KERBEROS
/* The name of the Kerberos Service to request tickets for, if */
/* P_KERBEROS is defined in <psite.h> */ 
/* WARNING: It is recommended that you not change this name as it may */
/*          affect inter-realm authentication. */
#define KERBEROS_SERVICE "prospero" 
#endif

/* Definitions for rd_vlink and rd_vdir */
#define		SYMLINK_NESTING 10       /* Max nesting depth for sym links */

/* Definition fo check_acl */
#define		ACL_NESTING     10       /* Max depth for ACL group nesting */

/* Flags for mk_vdir */
#define	     MKVD_LPRIV     1   /* Minimize privs for creator in new ACL    */

/* Flags for p_get_dir */
#define	     GVD_UNION       0  /* Do not expand union links 		     */
#define      GVD_EXPAND    0x1  /* Expand union links locally		     */
#define	     GVD_LREMEXP   0x3  /* Request remote expansion of local links   */
#define	     GVD_REMEXP    0x7  /* Request remote expansion of all links     */
#define	     GVD_VERIFY    0x8  /* Only verify args are for a directory      */
#define      GVD_FIND	  0x10  /* Stop expanding when match is found        */
#define	     GVD_ATTRIB   0x20  /* Request attributes from remote server     */
#define	     GVD_NOSORT	  0x40  /* Do not sort links when adding to dir      */
#define      GVD_MOTD     0x80  /* Request motd string from server           */
#define      GVD_MOTD_O  0x100  /* Request motd string from server           */
#define	     GVD_ASYNC   0x200  /* Fill in the directory asynchronously      */

#define	     GVD_EXPMASK  0x7   /* Mask to check expansion flags             */

/* Flags for rd_vdir */
#define	     RVD_UNION      GVD_UNION
#define	     RVD_EXPAND     GVD_EXPAND 
#define	     RVD_LREMEXP    GVD_LREMEXP
#define	     RVD_REMEXP     GVD_REMEXP
#define	     RVD_DFILE_ONLY GVD_VERIFY /* Only return ptr to dir file        */
#define      RVD_FIND       GVD_FIND   
#define      RVD_ATTRIB     GVD_ATTRIB
#define	     RVD_NOSORT	    GVD_NOSORT
#define	     RVD_NOCACHE    128

/* Flags for add_vlink */
#define	     AVL_UNION      1   /* Link is a union link                      */
#define	     AVL_INVISIBLE  2   /* Link is an invisible link.  (Not 
                                   compatible with AVL_UNION)                */

/* Flags for vl_insert */
#define	     VLI_NONAMECONFLICT 0   /* Do not insert links w/ conflicting link names  */
#define	     VLI_NOCONFLICT 0   /* Do not insert links w/ conflicting names,
                                   hostnames, and hsonames, unless they're
                                   replicas. */
#define      VLI_ALLOW_CONF 1   /* Allow links with conflicting names        */
#define	     VLI_NOSORT     2   /* Allow conflicts and don't sort            */

/* Flags for mapname */
#define      MAP_READWRITE  0   /* Named file to be read and written         */
#define	     MAP_READONLY   1   /* Named file to be read only                */
#define      MAP_PROMPT_OK  2   /* Ok to prompt user for a password if 
                                   necessary (e.g., FTP access method).      */

/* Flags for edit_object_info/pset_at()/pset_linkat() */
#define EOI_ADD 0x01
#define EOI_DELETE 0x02
#define EOI_DELETE_ALL 0x03
#define EOI_REPLACE 0x04

/* Flags for edit_acl */
/* Bits 0x3 of the flags; orthogonal flags */
#define	     EACL_NOSYSTEM   0x01
#define      EACL_NOSELF     0x02

/* Stuff under EACL_OP (bits 0x1C of the flags) */
#define      EACL_CREATE     0x00 /* works with EACL_NAMED *only* */
#define      EACL_DESTROY    0x04 /* works with EACL_NAMED *only* */
#define      EACL_DEFAULT    0x08
#define      EACL_SET        0x0C
#define      EACL_DELETE     0x10
#define      EACL_INSERT     0x14
#define      EACL_SUBTRACT   0x18
#define      EACL_ADD        0x1C
#define      EACL_OP    (EACL_DEFAULT|EACL_SET|EACL_INSERT|\
			 EACL_DELETE|EACL_ADD|EACL_SUBTRACT)

/* Stuff under EACL_OTYPE (bits 0xE0 of the flags) */
#define      EACL_LINK       0x00
#define      EACL_DIRECTORY  0x20 /* will merge with EACL_OBJECT Real Soon */
#define      EACL_OBJECT     0x60
#define      EACL_INCLUDE    0x40
#define      EACL_NAMED      0x80
#define      EACL_OTYPE (EACL_LINK|EACL_DIRECTORY|EACL_OBJECT|EACL_INCLUDE|EACL_NAMED)

#ifdef SERVER_SUPPORT_V1
/* Flags for modify_acl */
#define	     MACL_NOSYSTEM   0x01
#define      MACL_NOSELF     0x02
#define      MACL_DEFAULT    0x08
#define      MACL_SET        0x0C
#define      MACL_INSERT     0x14
#define      MACL_DELETE     0x10
#define      MACL_ADD        0x1C
#define      MACL_SUBTRACT   0x18
#define      MACL_LINK       0x00
#define      MACL_DIRECTORY  0x20
#define      MACL_OBJECT     0x60
#define      MACL_INCLUDE    0x40

#define      MACL_OP    (MACL_DEFAULT|MACL_SET|MACL_INSERT|\
			 MACL_DELETE|MACL_ADD|MACL_SUBTRACT)

#define      MACL_OTYPE (MACL_LINK|MACL_DIRECTORY|MACL_OBJECT|MACL_INCLUDE)

#endif

/* Access methods returned by pget_am() and specified as arguments to it. */
#define P_AM_ERROR			0
#define P_AM_FTP			1
#define P_AM_AFTP			2  /* Anonymous FTP  */
#define P_AM_NFS			4
#define P_AM_KNFS			8  /* Kerberized NFS */
#define P_AM_AFS		       16
#define P_AM_GOPHER                    32 /* Gopher TEXT or BINARY formats */
#define P_AM_LOCAL                     64 /* files that can be copied in local
                                             filesystem  */
#define P_AM_RCP                      128 /* Berkeley RCP protocol.  Not yet
                                             implemented. */
#define P_AM_TELNET                   256 /* This is not an access method for
                                             files, but for PORTALs. */
#define P_AM_PROSPERO_CONTENTS        512 /* Read the CONTENTS attribute. */
#define P_AM_WAIS		     1024 /* Read the WAIS document. */

/* Return codes */

#define		PSUCCESS	0
#define		PFAILURE	255

/*
 *     The string format we use has the size of the data are encoded as an
 *     integer starting sizeof (int) bytes before the start of the string.
 *     When safety checking is in place, it also has a consistency check
 *     encoded as an integer 3* sizeof (int) bytes before the start of the
 *     string.
 *
 *     The stsize() macro in <pfs.h> gives the size field of the string.
 */

/*     The bst_consistency_fld() macro is a reference to the consistency check
 *     field.   bst_size_fld() is a reference to the size of the buffer.
 *     bst_length_fld() is a reference to the length of the data.  A
 *     distinguished value of the maximum possible unsigned integer (UINT_MAX
 *     in <limits.h>) 
 *     indicates that the null termination convention should be used to 
 *     check for string length. 
 */     
#include <limits.h>

#define p__bst_size_fld(s) (((unsigned int *)(s))[-1])
#define p__bst_length_fld(s) (((unsigned int *)(s))[-2])
#define P__BST_LENGTH_NULLTERMINATED UINT_MAX /* distinguished value */
#ifndef ALLOCATOR_CONSISTENCY_CHECK
#define p__bst_header_sz (2 * sizeof (unsigned int))
#define p__bst_consistent(s) 1
#else
#define p__bst_consistency_fld(s) (((unsigned int *)(s))[-3])
#define p__bst_header_sz (3 * (sizeof (unsigned int)))
#define p__bst_consistent(s) \
    ((s == NULL) || p__bst_consistency_fld(s) == P__INUSE_PATTERN) 
#endif


void p_bst_set_buffer_length_explicit(char *buf, int buflen);
void p_bst_set_buffer_length_nullterm(char *buf, int buflen);

/* How many bytes allocated for a string?   Intended for internal use. */
#if 0
#define stsize(s) p__bstsize(s) /* deprecated interface; internal in any case */
#endif
/* for internals use */
#define p__bstsize(s)   ((s) == NULL ? 0 : (((unsigned int *)(s))[-1])) 
/* String length  --- published interface. */
extern int p_bstlen(const char *s);

/* FUNCTION PROTOTYPES */
#ifdef __cplusplus
extern "C" {
#endif

/* Procedures in libpfs.a */
/* These are procedures that call p__start_req(), p__add_req(),
   and ardp_send() to send Prospero Protocol requests to the server */ 
/* They are (or should be) documented in the library reference manual. */
extern int pget_am(VLINK,TOKEN *,int);
int add_vlink(const char direct[], const char lname[], VLINK, int);
extern VDIR apply_filters(VDIR, FILTER, VLINK, int);
int binencode(char*,size_t,char*,size_t);
int bindecode(char*,char*,size_t);
int del_vlink(const char vpath[], int);
ACL get_acl(VLINK,const char lname[],int);
int mk_vdir(const char vpath[],int);
int modify_acl(VLINK,const char lname[],ACL,int);
/* get_vdir() interface to p_get_dir() for backwards compatability. */
int get_vdir(VLINK, const char components[], VDIR, long flags, TOKEN *acomp); 
/* p_get_vdir() interface to p_get_dir() for backwards compatability. */
int p_get_vdir(VLINK, const char components[], VDIR, long flags, TOKEN *acomp); 
int p_get_dir(VLINK, const char components[], VDIR, long flags, TOKEN *acomp); 
PATTRIB pget_at(VLINK, const char atname[]);
int pset_at(VLINK,int,PATTRIB);
int pset_linkat(VLINK,char lname[],int, PATTRIB);
int rd_vdir(const char dirarg[], const char comparg[] ,VDIR,long);
VLINK rd_slink(const char path[]);
VLINK rd_vlink(const char path[]);
int update_link(const char dirname[], const char components[]);

/* Functions used to open real files from Prospero applications. */
extern int mapname(VLINK vl, char npath[], int npathlen, int readonly);
extern int pmap_nfs(char *host, char *rpath, char *npath, int npathlen,
                    TOKEN am_args);
extern int p__map_cache(VLINK vl,char *npath, int npathlen, TOKEN am_args);
extern int file_incache(char *local);
extern FILE *pfs_fopen(VLINK vl, const char * /*type */);
extern int pfs_open(VLINK vl, int flags);

/* Internal Prospero utility functions, not intended for use by applications
   programmers but still with external scope (thanks to the accursed C linkage
   model) */
extern int wcmatch(char*, char *);
void            initialize_defaults(void);
extern char *pget_wdhost(void), *pget_wdfile(void), *pget_wd(void);
extern char *pget_hdhost(void), *pget_hdfile(void), *pget_hd(void);
char    *pget_rdhost(void),    *pget_rdfile(void); 
char *pget_dhost(void), *pget_dfile(void), *pget_vsname(void);
extern TOKEN p__slashpath2tkl(char *nextcomp);
extern void p__tkl_back_2slashpath(TOKEN nextcomp_tkl, char *nextcomp);
extern char *p__tkl_2slashpath(TOKEN nextcomp_tkl);
void p__print_shellstring(int which);
int p__get_shellflag(const char *shellname);


/* UNIX utilities. */
long myaddress(void);
char *myhostname(void);
char *month_sname(int month_number);
int p__mkdirs(const char path[], int wantdir); 
int mkdirs(const char path[]); 
int stequal(const char *s1, const char *s2); /* test for string equality,
                                                accepting either as NULL  */
extern const char *sindex(const char *s1, const char *s2);
int stcaseequal(const char *s1, const char *s2);
char *readheader();
char            *p_timetoasn(time_t t1, char *target);
time_t		asntotime(const char timestring[]);

/* Utility functions that handle Prospero quoting. */
/* Return PSUCCESS or PFAILURE. */
int qfprintf(FILE *outf, const char fmt[], ...);
int vqfprintf(FILE *outf, const char fmt[], va_list ap);

/* Returns a count of the # of matches. */
int qsscanf(const char *input, const char *format, ...);
extern int p__qbstscanf(const char *sthead, const char *stpos, 
                        const char *format, ...);

/* Returns the # of bytes it wanted to write. */
size_t qsprintf(char *buf, size_t buflen, const char fmt[], ...);
size_t vqsprintf(char *buf, size_t buflen, const char fmt[], va_list);
/* Returns a pointer to a new string, in the style of stcopyr().
   Passing buf as NULL will cause the right thing to happen -- allocation but
   no freeing.  buf may be freed and new memory returned if buf is not big
   enough. */
extern char *qsprintf_stcopyr(char *buf, const char *fmt, ...);
extern char *vqsprintf_stcopyr(char *buf, const char *fmt, va_list ap);
/* These two are not yet a published interface. */
extern char *p__qbstprintf_stcopyr(char *buf, const char *fmt, ...);
extern char *p__vqbstprintf_stcopyr(char *buf, const char *fmt, va_list ap);
extern int p__fputbst(const char *bst, FILE *out);


/* Returns a pointer to the position of c within s, respecting quoting. */
char *qindex(const char *s, char c);
char *qrindex(const char *s, char c); /* like rindex(), but quoting. */

/* Handle User Level Name quoting */
char *p_uln_index(const char *s, char c);
char *p_uln_rindex(const char *s, char c);
extern char *p_uln_lastcomp_to_linkname(const char *s);

/* Networking utility functions. */
#ifdef ARDP_H_INCLUDED
/* Used by clients to construct request packets.. */
extern RREQ p__start_req(const char server_hostname[]);
extern int p__add_req(RREQ request, const char format[], ...);
extern int pv__add_req(RREQ request, const char fmt[], va_list ap);


#endif /* ARDP_H_INCLUDED */

/* Utility functions handling Prospero memory allocation and data structure 
   manipulation. */
ACL             acalloc(void);
void            acfree(ACL), aclfree(ACL);
void            acappfree(void (*)(ACL));
ACL 		  aclcopy(ACL acl);
extern int      acl_count, acl_max;

extern PATTRIB atcopy(PATTRIB at), atlcopy(PATTRIB at);
extern PATTRIB  atalloc(void);
extern void atfree(PATTRIB);
extern void atlfree(PATTRIB);
extern void            atappfree(void (*)(PATTRIB));
extern PATTRIB vatbuild(char *name, va_list ap);
int             equal_attributes(PATTRIB, PATTRIB);
extern int      pattrib_count, pattrib_max;

extern FILTER flalloc(void), flcopy(FILTER source, int r);
extern void flfree(FILTER);
extern void fllfree(FILTER);
int             equal_filters(FILTER f1, FILTER f2);
void            flappfree(void (*)(FILTER));
extern int      filter_count, filter_max;

PAUTH           paalloc(void);
void            pafree(PAUTH), palfree(PAUTH);
void            paappfree(void (*)(PAUTH));
extern int      pauth_count, pauth_max;

PFILE		pfalloc(void);
void            pffree(PFILE pf);
extern int      pfile_count, pfile_max;

P_OBJECT		oballoc(void);
extern void		ob_atput(P_OBJECT po, char *name, ...);
void             obfree(P_OBJECT), oblfree(P_OBJECT);
void            obappfree(void (*)(P_OBJECT));
extern int      p_object_count, p_object_max;

extern char *stcopyr(const char *, char *); 
extern void stfree(void *s);
extern char *stalloc(int size);
extern char *stcopy(const char *s);
extern int string_count, string_max;

TOKEN           qtokenize(const char *);
TOKEN           p__qbstokenize(const char * head, char *loc);
TOKEN           tkalloc(const char *), tkcopy(TOKEN);
TOKEN           tkappend(const char newstr[], TOKEN toklist);
int             equal_sequences(TOKEN,TOKEN);
void             tkfree(TOKEN), tklfree(TOKEN);
int  member(const char *, TOKEN); /* really a boolean; on or off */
int             length(TOKEN s);
char            *elt(TOKEN s, int nth);
extern int      token_count, token_max;


extern VLINK	        vl_delete(VLINK vll,char *lname,int number);
extern VLINK            vlalloc(void);
extern void             vlfree(VLINK), vllfree(VLINK);
void                    vlappfree(void (*)(VLINK));
extern VLINK            vlcopy(VLINK v, int r);
extern int              vlink_count, vlink_max;

/* End of function prototypes. */
#ifdef __cplusplus
};
#endif  

/* Miscellaneous useful definitions */
#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

#define AUTHORIZED      1
#define NOT_AUTHORIZED  0
#define NEG_AUTHORIZED  -1

#define FAILED		-1
/* This change made for SOLARIS:  memset is MT safe, bzero isnt */
#define ZERO(p)		memset((char *)(p), 0, sizeof(*(p)))

/* A minor C extension that handles a thorn in my side.  --swa */
#define strequal(s1, s2) (strcmp(s1, s2) == 0)
#define strnequal(s1, s2, n) (strncmp(s1, s2, n) == 0)

/* function form of internal_error; might be removable soon.. */
/* LINE commented out to shut up GCC spurious shadow warnings. */
void p__finternal_error(const char file[], int /* line */, const char mesg[]);
#define interr_buffer_full() \
    p__finternal_error(__FILE__, __LINE__, "A buffer filled up"); 

/* Is it a directory? 1 = yes, 0 = no, -1 = failure? */
extern int is_dir(const char native_filename[]);
extern int is_file(const char native_filename[]);
extern int stat_age(const char *path);
extern int p__is_out_of_memory;    /* used internally by p__fout_of_memory */
/* Stat and get a modification time.  Returns 0 upon failure. */
extern time_t mtime(const char native_dirname[]);

extern void p__fout_of_memory(const char file[], int lineno);
#define out_of_memory() \
    p__fout_of_memory(__FILE__, __LINE__); 

/* From copyfile.c */

extern int copyFile(char *source,char *destn); 
extern int renameOrCopy(char *source, char *destn);

/* From copyfile.c */

extern int copyFile(char *source,char *destn); 
extern int renameOrCopy(char *source, char *destn);

/* Threads stuff. */
extern int p_open_tcp_stream(const char host[], int port);

/* For debugging; declared in lib/pfs/pfs_debug.c.  Also see def. in <ardp.h>. */
extern int      pfs_debug;


/* Mutexes. */

/* Mutex stuff for pfs_threads on server side only still.. */
/* Note we still declerations of these if !PFS_THREADS*/
extern void p__init_mutexes(void); /* called by p_initialize() */
#ifndef NDEBUG
extern void p__diagnose_mutexes(void);
#endif
#ifdef PFS_THREADS
extern p_th_mutex        p_th_mutexATALLOC;
extern p_th_mutex        p_th_mutexACALLOC;
extern p_th_mutex        p_th_mutexFLALLOC;
extern p_th_mutex        p_th_mutexOBALLOC;
extern p_th_mutex        p_th_mutexPAALLOC;
extern p_th_mutex        p_th_mutexPFALLOC;
extern p_th_mutex        p_th_mutexTOKEN;
extern p_th_mutex        p_th_mutexVLINK;
extern p_th_mutex        p_th_mutexPFS_VQSCANF_NW_CS;
extern p_th_mutex        p_th_mutexPFS_VQSPRINTF_NQ_CS;
extern p_th_mutex        p_th_mutexPFS_TIMETOASN;
#endif                          /* PFS_THREADS */

extern int vl_insert(VLINK vl, VDIR vd, int allow_conflict);
extern int ul_insert(VLINK ul, VDIR vd, VLINK p);
extern int vl_equal(VLINK vl1, VLINK vl2);
extern void pflfree(PFILE pf);
extern int ucase(char *s);
extern int scan_error(char *erst, RREQ req);
extern int vp__add_req(RREQ req, const char fmt[], va_list ap);
extern int strcncmp(char *s1, char *s2, int n);
/* HOSTNAME commented out to shut up spurious GCC warnings. */
extern int vfsetenv(char * /* hostname */, char *filename, char *path);
extern void perrmesg(char *prefix, int no, char *text);
extern void pwarnmesg(char *prefix, int no, char *text);
extern int vl_comp(VLINK vl1, VLINK vl2);
extern int lookup_precedence_by_precedencename(const char t_precedence[]);
extern char *lookup_precedencename_by_precedence(int precedence);
extern void p__set_username(char *un);
#include <sys/socket.h>
extern int quick_connect(int s, struct sockaddr *name, int namelen, int timeout);
extern int quick_fgetc(FILE *stream, int timeout);
extern int quick_read(int fd, char *nptr, int nbytes, int timeout);
extern char *p_timetoasn_stcopyr(time_t ourtime, char *target);
extern int vcache2a(char *host, char *remote, char *local, char *method,
		   char *argv[], int manage_cache);
#include <implicit_fixes.h>

extern void pset_rd(char *rdhost, char *rdfile);
extern void pset_wd(char *wdhost, char *wdfile, char *wdname);
extern void pset_hd(char *hdhost, char *hdfile, char *hdname);
extern void pset_desc(char *dhost, char *dfile, char *vsnm);

/*****************************************************/
/* These changes depend on what old Prospero clients and servers still exist.
 * If old servers exist, then we might talk to them. 
 * this, in turn, requires the clients and servers to be aware of certain
 * bugs that they have to be backwards-compatible with and certain defines
 * that should be present.  */
/******************************************************/

/* These options used in server/list.c should be added to the following list: */
/* SERVER_SEND_NEW_MCOMP */
/* DONT_ACCEPT_OLD_MCOMP */
/* #define REALLY_NEW_FIELD */

/* These options used in lib/pfs/p_get_dir.c should be added to the following
   list: */ 
/* CLIENT_SEND_NEW_MCOMP */
/* DONT_ACCEPT_OLD_MCOMP */

#define V5_0_SERVERS_EXIST
#define V5_1_SERVERS_EXIST
#define V5_2_SERVERS_EXIST

#ifdef V5_0_SERVERS_EXIST
#define CLIENTS_REQUEST_VERSION_FROM_SERVER
#define SERVER_MIGHT_APPEND_NULL_TO_PACKET /* Present in 5.0, 5.1, and 5.2 */
#endif

#ifdef V5_1_SERVERS_EXIST
#define CLIENTS_REQUEST_VERSION_FROM_SERVER
#define SERVER_MIGHT_APPEND_NULL_TO_PACKET /* Present in 5.0, 5.1, and 5.2 */
#endif

#ifdef V5_2_SERVERS_EXIST
#define CLIENTS_REQUEST_VERSION_FROM_SERVER
#define SERVER_MIGHT_APPEND_NULL_TO_PACKET /* Present in 5.0, 5.1, and 5.2 */
#endif

#endif /* PFS_H_INCLUDED */
