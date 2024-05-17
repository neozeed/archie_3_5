/*
 * pserver.h - server specific parameters for Prospero
 *
 * This file contains local definitions for the Prospero server.
 * It is expected that it will be different on different systems.
 */
/*
 * Written  by bcn 1989     as part of psite.h
 * Modified by bcn 1/19/93  to leave only server specific definitions
 */

/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

/*
 * This option is temporary, until we make the forwarding mechanism more
 * efficient. 
 * It hikes the efficiency considerably.  Developers having long turnaround
 * times on rd_vdir() need it.  Talk to swa@ISI.EDU.
 */

#define SERVER_DO_NOT_SUPPORT_FORWARDING

/* 
 * SERVER_SUPPORT_V1 indicates that the server is to reply to version 1
 * Prospero Protocol requests.  It is recommended that you define
 * this for the time being since there are many Version 1 clients still
 * in use.  Note that version 1 clients will *not* be able to speak
 * to certain new services so it's a good idea to upgrade your clients.
 */
#define     SERVER_SUPPORT_V1
#define     SERVER_DONT_FLAG_V1

/*
 * P_DIRSRV_BINARY defines the full path by which pstart will
 * find the binary for the directory server.
 */
#if 0
#define P_DIRSRV_BINARY       "/usr/pfs/bin/dirsrv"
#else
#define P_DIRSRV_BINARY       (char *) get_pfs_file("bin/dirsrv")
#endif

/*
 * PROSPERO_CONTACT specifies the e-mail address of the individual
 * or individuals responsible for running the Prospero server on the 
 * local system.  If specified, this string will appear in responses to 
 * status requests.  This definition is optional.
 *
 * #define PROSPERO_CONTACT "yourname@host.org.domain"
 */
#define PROSPERO_CONTACT "info-prospero@ISI.EDU"

/* 
 * PSRV_HOSTNAME specifies the name of the host on which the server 
 * will run.  It only needs to be defined if gethostbyname does not 
 * return the name of the host as a fully qualified domain name.
 *
 * #define PSRV_HOSTNAME	      "HOST.ORG.DOMAIN"
 *
 */

/* 
 * PSRV_USER_ID specifies the user name under which the Prospero
 * server will run.  The server will run under the home group of
 * PSRV_USER_ID, but (in systems which support membership in multiple
 * groups) not under any supplementary groups to which that user may
 * belong.
 */
#if 0
#define PSRV_USER_ID             "pfs"
#else
#define PSRV_USER_ID             "archie"
#endif


/* 
 * P_RUNDIR is the name of the directory under which the Prospero
 * server will run.  If left out, it defaults to /tmp.
 *
 * #define PSRV_RUNDIR	      "/tmp"
 */

/*
 * PSRV_LOGFILE is the name of the file in which events are logged.  
 * By default Prospero logs all requests if PSRV_LOGFILE is defined.  If you 
 * would like to change the list of events that are logged, or if you would
 * like to use syslog for logging, edit the file plog.h.
 */
#define PSRV_LOGFILE             "/pfs/pfs.log"

/*
 * PSRV_ROOT defines that part of the system's directory tree for which
 * directory information will be readable over the network.  Directory
 * information will also be available for files under AFTPDIRECTORY and
 * AFSDIRECTORY if these access methods are supported.
 *
 * If PSRV_ROOT is not defined, then directory information will only be
 * available for those files under AFTPDIRECTORY or AFSDIRECTORY.
 *
 * #define PSRV_ROOT "/" 
 */
#define PSRV_ROOT "/pfs/info-tree" 

/*
 * PSRV_READ_ONLY specifies that the server is only to accept
 * operations that read the directory structure. No changes to virtual
 * directories stored on this system will be allowed.  NODOTDOT is used to
 * disallow the use of path names that include "..".  If you are
 * restricting the part of the directory tree which will be accessible
 * (i.e. if PSRV_ROOT is other than "/"), NODOTDOT should be set.
 *
 * #define PSRV_READ_ONLY
 * #define NODOTDOT
 */
#if 0
#define PSRV_READ_ONLY
#define NODOTDOT
#endif




/*
 * PSRV_SHADOW is the name of the shadow hierarchy that contains additional 
 * information about Prospero files.  PSRV_STORAGE is the name of the directory
 * under which new files can be created, if that option is supported by
 * your distribution.  PSRV_SECURITY is the name of the directory where 
 * security information is stored by the prospero server.  All three names are
 * relative to the home directory for the 
 * user ID specified by P_USER_ID.  
 *
 * You will normally have no desire to change this, but it IS posible and legal
 * to do so.  If you do change it, then please make a corresponding change in
 * section III of Makefile.config.
 */

#define PSRV_SHADOW              "shadow"
#define PSRV_STORAGE             "pfsdat"
#define PSRV_SECURITY            "security"

/*
 * If PSRV_UNDER_UDIR is defined, pstart calculates the full path names of
 * of the directories corresponding to PSRV_SHADOW and PSRV_STORAGE and
 * PSRV_SECURITY by
 * finding the home directory for the Prospero user ID, then appending
 * PSRV_SHADOW or PSRV_STORAGE or PSRV_SECURITY.
 *
 * If PSRV_UNDER_UDIR is not defined, or if dirsrv is called directly,
 * the full path names are taken from PSRV_FSHADOW and PSRV_FSTORAGE and
 * PSRV_FSECURITY.  These
 * should be stable names for those directories.  They may be aliases
 * (symbolic links) if the directories move around.  If they are symbolic
 * links, the links should be updated whenever the directories move.
 *
 * Note that if PSRV_UNDER_UDIR is set, the full path names are calculated 
 * only when the Prospero server is started by pstart.  If you expect
 * to start the server by hand (for example, when debugging), PSRV_FSHADOW 
 * and PSRV_FSTORAGE should also be defined.
 *
 * In any case these must match the definitions at the top of Makefile
 */

#define PSRV_UNDER_UDIR

#define PSRV_FSHADOW	      "/pfs/shadow"
#define PSRV_FSTORAGE	      "/pfs/pfsdat"
#define PSRV_FSECURITY	      "/pfs/security"

/*
 * AFTPDIRECTORY contains the name of the root directory as seen through
 * anonymous FTP.  It is used to determine which files may be accessed
 * using the anonymous FTP access mechanism, and to determine the file name
 * that must be used when doing so.  It also adds to the set of files about 
 * which directory information is available.
 *
 * Note: AFTPDIRECTORY should be a stable name for the FTP directory.
 *       It may be an alias (symbolic link) if the FTP directory
 *       moves around.  If it is a symbolic link, that symbolic link should
 *       be updated whenever the directory moves.
 */
/* #define AFTPDIRECTORY           "/usr/ftp" */

/*
 * AFTPUSER contains the name of the anonymous FTP user (usually ftp)
 * If defined, pstart will look up the home directory for this user
 * and use it instead of AFTPDIRECTORY.  If it fails, the value of
 * AFTPDIRECTORY (defined above) will be used.  
 *
 * Note that if AFTPUSER is set, the path name of the FTP directory is
 * calculated only when the Prospero server is started by pstart. 
 * If you expect to start the server by hand (for example, when
 * debugging), AFTPDIRECTORY should also be defined.
 *
 * Also note that when started by pstart, if AFTPUSER is defined, the
 * path taken from the password file will override that specified by
 * AFTPDIRECTORY.  If AFTPDIRECTORY is a stable path, do not define 
 * AFTPUSER.
 *
 * #define AFTPUSER 		"ftp"
 */

/*
 * NFS_EXPORT should be defined if your system allows others to 
 * access files using NFS.  If defined, you will have to modify
 * lib/psrv/check_nfs.c so that the directory server is aware of 
 * your system's policy for NFS mounts (this is necessary so that 
 * the directory server will not tell the client to use NFS when
 * such access is not supported).
 *
 * #define NFS_EXPORT
 */

/*
 * AFSDIRECTORY contains the name of the root of the AFS directory
 * hierarchy.  It should only be defined if your host supports AFS
 * and you are willing to make information about AFS files available
 * through your server.  AFSDIRECTORY is used to to determine which files
 * may be accessed using the AFS access mechanism.  It also adds to
 * the set of files about which directory information is available.
 *
 * Note: AFSDIRECTORY must be the true name for the directory
 *       after all symbolic links have been expanded.
 *
 * #define AFSDIRECTORY           "/afs"
 */

/*
 * PSRV_KERBEROS, if defined, means that the server will understand
 * Kerberos (version 5) authentication information. This should be
 * #defined if your site uses Kerberos authentication; otherwise, you
 * won't have a use for it.  If you #define this, then you also need
 * to make the appropriate definitions in the top-level makefile.
 *
 * Note that most Kerberos sites still use the older version 4,
 * which is not supported by Prospero.
 *
 * #define PSRV_KERBEROS
 */

/*
 * KERBEROS_SRVTAB, if defined, tells the server the location of the
 * server's keytab file that contains its Kerberos key. Define this
 * only if, firstly, you are using Kerberos (i.e. PSRV_KERBEROS is
 * defined), and, secondly, if you want to keep this key in a location
 * different from its default location (typically /etc/v5srvtab).
 *
 * #define KERBEROS_SRVTAB "/usr/pfs/v5srvtab"
 */

/*
 * PSRV_P_PASSWORD, if defined, means that server will understand
 * the Prospero password authentication information.  It will not do you
 * any harm to define this, and we have left it turned on by default.
 * The passwording mechanism is described more fully in the user's manual 
 * (or will be after it's updated).
 */
#define PSRV_P_PASSWORD

#ifdef PSRV_P_PASSWORD
/* PSRV_PW_FILE contains the name of the servers password file used
 * for P_PASSWORD authentication. Define this only if you are using
 * this type of authentication.
 *
 * #define PSRV_PW_FILE "/usr/pfs/ppasswd"
 */
#define PSRV_PW_FILE "/pfs/ppasswd"
#endif

/*
 * PSRV_ACCOUNT, if defined, the server will understand accounting
 * information and may charge clients for services.
 *
 * #define PSRV_ACCOUNT
 */

/* 
 * DEFAULT_ACL, SYSTEM_ACL, OVERRIDE_ACL and MAINT_ACL are access
 * control lists used to control access to Prospero directories.
 * DEFAULT_ACL applies to those directories for which no individual
 * access control list has been specified.  All access control lists
 * initially include SYSTEM_ACL, but the entry may be explicitly
 * removed by the administrator of a directory or link.  OVERRIDE_ACL
 * grants rights to a directory regardless of the contents of any
 * other access control list.  MAINT_ACL is an access control list
 * for performing remote maintenance on the server.
 */
#if 0
#define DEFAULT_ACL  {{ACL_ANY,      NULL, "YvlGgrwu"                     }}
#else
#define DEFAULT_ACL  {{ACL_ANY,      NULL, "ALRMDIWYvlGgr", "*@%.%.%.%"}}
#endif
#define SYSTEM_ACL   {{ACL_ASRTHOST, NULL, "ALRWD",   "pfs@128.9.*.*"}}

#define OVERRIDE_ACL {{ACL_TRSTHOST, NULL, "A",       "root@%.%.%.%"},\
                      {ACL_OWNER,    NULL, "A"}}

#ifdef 0
#define MAINT_ACL    {{ACL_ASRTHOST, NULL, "STUP", "pfs@128.9.*.*"},\
                      {ACL_ASRTHOST, NULL, "STU",  "*@128.9.*.*"},\
                      {ACL_ANY,      NULL, "Y"}}
#endif
#define MAINT_ACL    {{ACL_ASRTHOST, NULL, "STUP", "root@%.%.%.%"},\
                      {ACL_ASRTHOST, NULL, "STUP", "root@192.77.55.2"},\
                      {ACL_ANY,      NULL, "Y"}}

/*
 * PSRV_ARCHIE, if defined, indicates that your server is to 
 * make available information from the archie database. You
 * must already have the archie database installed on your
 * machine and you must follow the additional directions in
 * lib/psrv/archie2 or lib/psrv/archie3 to configure the server.
 *
 * #define PSRV_ARCHIE
 */
#define PSRV_ARCHIE

/* PSRV_GOPHER_GW, if defined, indicates that this server will have
 * the capability to make Gopher protocol queries in response to
 *  Prospero protocol queries.   If you install this option, you must
 *  uncomment the line reading 
 *        #SUBDIR=gopher_gw.dir
 *  in lib/psrv/Makefile, and uncomment the line reading:
 *        #DB_LIBS=  ../lib/psrv/gopher_gw/libpgoph_gw.a
 *  in server/Makefile.  This option *is* presently supported.  
 *
 * #define PSRV_GOPHER_GW
 */


/* 
 * Gopher servers frequently gateway queries to anonymous FTP sites.  When
 * they return links in this form, all retrievals then proceed through
 * that server.  This is inefficient.
 *
 * You can set GOPHER_GW_NEARBY_GOPHER_SERVER to be a
 * Gopher server at your site or nearby that you want the gopher
 * gateway to to gateway Gopher AFTP queries through
 * instead of the GOPHER server that returned the AFTP links.  Thanks to 
 * mitra@path.net for this optimization. 
 *
 * If you don't define this, things will still work OK; the gopher server 
 * just won't rewrite such  links.  In other words, if you don't define this,
 * the gopher gateway will exhibit the default behavior for most Gopher
 * clients, as distributed.  
 *
 * #define GOPHER_GW_NEARBY_GOPHER_SERVER "gopher.your.net(70)"
 */ 

/* PSRV_WAIS_GW, if defined, indicates that this server will have
 * the capability to make Wais protocol queries in response to
 *  Prospero protocol queries.   If you install this option, you must
 *  uncomment the line reading 
 *        #SUBDIR=wais_gw.dir
 *  in lib/psrv/Makefile, and uncomment the line reading:
 *        #DB_LIBS=  ../lib/psrv/wais_gw/libwais_gw.a
 *  in server/Makefile.  This option *is* presently supported.  
 * #define PSRV_WAIS_GW
 */



/* 
 * PSRV_GOPHER, if defined, indicates that your server is to make
 * available information from gopher files locally stored on your
 * machine. You must already have gopher installed on your machine and
 * you must follow the additional directions in lib/psrv/gopher to
 * configure the server.  This option is not presently supported.
 *
 * XXX This might not ever be a supported part of the distribution, if it 
 *     turns out that the existing GOPHER_GW code meets the needs this 
 *     was intended to fulfill.  Please send feedback to 
 *     info-prospero@isi.edu.
 *
 * #define PSRV_GOPHER
 */

/*
 * SHARED_PREFIXES, if defined, indicates that the host on which your
 * server runs shares some part of its filesystem with other specified
 * hosts (perhaps via an NFS auto-mounter).  If you define
 * SHARED_PREFIXES appropriately, then Prospero clients on machines
 * with access to the shared part of the filesystem will be able to
 * retrieve files on the server that might be unreachable via another
 * access method.  
 * 
 * You should specify SHARED_PREFIXES only if you want the server to
 * return a LOCAL access-method for files in these areas on your
 * server.  If you don't understand this definition, just leave it
 * undefined.
 * 
 * The format of the SHARED_PREFIXES definition is a concatenated series of
 * entries, each one describing a particular subtree of the filesystem
 * that is exported.    Each entry is composed of 4 subitems.  The subitems
 * are:
 * 
 *  a) Name (or names, if alternate names for directories are present) of
 *     the subtree on the server host.  The last name is followed by a NULL
 *  b) Name of the subtree on the client host(s).  There must be no more
 *     than one of these entries.  This subitem is followed by a NULL.
 *  c) Internet addresses of the hosts which have this subtree as part
 *     of the local filesystem namespace.  These can be wildcarded;
 *      - any octet may be replaced with a '*' (indicating match any).
 *	- any octet may be replaced with a '%' (indicating to match
 *        with the value of this octet in the server host's address)
 *     This subitem is followed by a NULL.
 *  d) exception list: a list of hosts which do NOT have this subtree
 *     as part of the local filesystem namespace.  This lets one exclude
 *     special hosts or subnets that don't share in an organization's
 *     overall networked filesystem.  This subitem is followed by a NULL.
 * 
 * 
 * An example may make this clearer.  Here is the entry we use on the
 * server on PROSPERO.ISI.EDU:
 *
 * #define SHARED_PREFIXES { \
 *    "/nfs", NULL, "/nfs", NULL, "128.9.*.*", NULL, NULL, \
 *    "/ftp", NULL, "/auto/gum/gum/ftp", NULL, "128.9.*.*", NULL, NULL, \
 *    "/afs", NULL, "/nfs/afs", NULL, "128.9.*.*", NULL, NULL }
 */



/******************************************************************
 * You should not need to change any definitions below this line. *
 ******************************************************************/

/*
 * DSHADOW and DCONTENTS are the names of the file in the shadow
 * directory used to store information about directories.  There
 * should be no need to change them.
 */
#define DSHADOW			".directory#shadow"
#define DCONTENTS		".directory#contents"

/* 
 * ARCHIE_PREFIX is an entry for archie that will appear
 * in the DATABASE_PREFIXES definition.  It should not be set
 * directly but is instead set automatically if PSRV_ARCHIE is
 * defined.  If not, it will be empty.  If set, it must be 
 * followed by a ,.
 */
#ifdef PSRV_ARCHIE
#define ARCHIE
#define ARCHIE_PREFIX {"ASCII", "ARCHIE", arch_dsdb, NULL, "ARCHIE"},
#ifdef SERVER_SUPPORT_V1
#define DATABASE_PREFIX "ARCHIE" /* Backwards compatability; v1 stuff */
#endif
#else 
#define ARCHIE_PREFIX
#endif


/* GOPHER_GW_PREFIX is an entry for GOPHER that will appear in the
 * DATABASE_PREFIXES definition.  It should not be set
 * directly but is instead set automatically if PSRV_GOPHER_GW is
 * defined.  If not, it will be empty.  If set, it must be 
 * followed by a ,.
 */
#ifdef PSRV_GOPHER_GW
#define GOPHER_GW_PREFIX {"ASCII", "GOPHER-GW", gopher_gw_dsdb, NULL, "GOPHER-GW"},
#else
#define GOPHER_GW_PREFIX
#endif


/* WAIS_GW_PREFIX is an entry for WAIS that will appear in the
 * DATABASE_PREFIXES definition.  It should not be set
 * directly but is instead set automatically if PSRV_WAIS_GW is
 * defined.  If not, it will be empty.  If set, it must be 
 * followed by a ,.
 */
#ifdef PSRV_WAIS_GW
#define WAIS_GW_PREFIX {"ASCII", "WAIS-GW", wais_gw_dsdb, NULL, "WAIS-GW"},
#else
#define WAIS_GW_PREFIX
#endif


/*
 * GOPHER_PREFIXES are the entries for gopher that will appear
 * in the DATABASE_PREFIXES definition.  It should not be set
 * directly but is instead set automatically if PSRV_GOPHER is
 * defined.  If not, it will be empty.  If set, it must be 
 * followed by a ,.
 *
 * XXX This is not yet a supported part of the distribution. 
 * XXX This might not ever be a supported part of the distribution, if it 
 *     turns out that the existing GOPHER_GW code meets the needs this 
 *     was intended to fulfill.
 *
 * If you run a Gopher server, define GOPHER to be a list of 
 * pairs of Gopher hierarchy root directories and TCP ports.  This list
 * must be terminated with the record {"", 0}.
 *
 * #define GOPHER {{"/nfs/pfs/gopherroot", 70}, {"", 0}}
 *
 */
#ifdef PSRV_GOPHER
#define GOPHER_PREFIX {"ASCII", "GOPHER", gopher_dsdb, NULL, "GOPHER"},
#else
#define GOPHER_PREFIX
#endif


/*
 * DATABASE_PREFIXES is an array of structures, each representing
 * a particular database prefix and its acompanying prospero query
 * function.  It is for use when the Prospero server provides information
 * from sources other than the file system.  It should be set
 * automatically if the appropriate database has been configured.  
 * you should not need to set it directly. The last element in the
 * array must be a NULL pointer.
 *
 * DATABASE_PREFIXES supersedes the old DATABASE_PREFIX option.
 *                              
 */

#define DATABASE_PREFIXES { ARCHIE_PREFIX GOPHER_GW_PREFIX GOPHER_PREFIX \
	WAIS_GW_PREFIX }

/* 
 * If DIRSRV_EXPLAIN_LAST_RESTART is defined and the server detects an
 * internal error and restarts itself, the message that caused the
 * error will be explained in the logfile and in the message returned
 * by pstatus.  
 */

#define DIRSRV_EXPLAIN_LAST_RESTART

/*
 * If PSRV_CACHE_NATIVE is defined, the server will cache directories which
 * are entirely native (e.g., directories in the anonymous FTP area which
 * contain no links with special attributes defined on them.)
 * This option is unset by default because otherwise the prospero server
 * running on a big anonymous FTP site would consume too much disk space.
 *
 * #define PSRV_CACHE_NATIVE
 */


/*
 * If DNSCACHE_MAX is defined, then dirsrv will cache up to that many 
 * DNS addresses. "undef" this for no caching at all.
 *
 */

#define DNSCACHE_MAX 300

/*
 * If DIRECTORYCACHING is defined, then dirsrv will cache directories
 * on disk under /usr/pfs/shadow/{GOPHER-GW,WAIS-GW,<other-database-name>}
 * Note that this isn't a GREAT way of structuring the database, in case there
 * really is a directory named /GOPHER-GW on the machine (confusion).  This is
 * a risk we currently take.
 *
 * #define DIRECTORYCACHING
 */


/* These following definitions override those in plog.h 
 *
 * Place any definitions you like here that should override those in plog.h
 * By changing this file instead of <plog.h>, we can keep down the number of 
 * files that must be changed in order to configure Prospero.
 */

#define L_WTTIME_THRESHOLD   1
#define L_SVCTIME_THRESHOLD  1 
#define L_SYSTIME_THRESHOLD  9999
#define L_COMP_THRESHOLD     1  


#define INITIAL_LOG_VECTOR { \
    L_FIELDS_USER|L_FIELDS_HADDR|L_FIELDS_SW_ID|L_FIELDS_STIME,  /* L_FIELDS       */       \
    LOG_PROSPERO|LOG_NOTICE|PLOG_TOFILE,	  /* L_STATUS       */       \
    LOG_PROSPERO|LOG_CRIT|PLOG_TOFILE,	  	  /* L_FAILURE      */       \
    PLOG_TOFILE,			  	  /* L_STATS        */       \
    LOG_PROSPERO|LOG_ERR|PLOG_TOFILE,	  	  /* L_NET_ERR      */       \
    LOG_PROSPERO|LOG_ERR|PLOG_TOFILE,	  	  /* L_NET_RDPERR   */       \
    0,					  	  /* L_NET_INFO     */       \
    LOG_PROSPERO|LOG_INFO|PLOG_TOFILE,	  	  /* L_QUEUE_INFO   */       \
    LOG_PROSPERO|LOG_INFO|PLOG_TOFILE,	  	  /* L_QUEUE_COMP   */       \
    LOG_PROSPERO|LOG_ERR|PLOG_TOFILE,	  	  /* L_DIR_PERR     */       \
    LOG_PROSPERO|LOG_WARNING|PLOG_TOFILE, 	  /* L_DIR_PWARN    */       \
    LOG_PROSPERO|LOG_INFO|PLOG_TOFILE,	  	  /* L_DIR_PINFO    */       \
    LOG_PROSPERO|LOG_ERR|PLOG_TOFILE,	  	  /* L_DIR_ERR      */       \
    LOG_PROSPERO|LOG_WARNING|PLOG_TOFILE, 	  /* L_DIR_WARN     */       \
    LOG_PROSPERO|LOG_INFO|PLOG_TOFILE,	  	  /* L_DIR_REQUEST  */       \
    LOG_PROSPERO|LOG_INFO|PLOG_TOFILE,	  	  /* L_DIR_UPDATE   */       \
    LOG_PROSPERO|LOG_WARNING|PLOG_TOFILE, 	  /* L_AUTH_ERR     */       \
    LOG_PROSPERO|LOG_ERR|PLOG_TOFILE,	  	  /* L_DATA_FRM_ERR */       \
    LOG_PROSPERO|LOG_ERR|PLOG_TOFILE,		  /* L_DB_ERROR     */       \
    LOG_PROSPERO|LOG_INFO|PLOG_TOFILE	  	  /* L_DB_INFO      */       \
}



