/*
 * Copyright (c) 1992, 1993, 1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 *
 * Written  by bcn 1989     modified 1989-1992
 * Modified by bcn 1/19/93  to eliminate server specific definitions
 * Modified by swa 5/2/94   to add section 3 and reorder things to be less
 *                          intimidating to neophytes.
 */

#include <usc-license.h>

/*
 * psite.h - site selected parameters for Prospero
 *
 * This file contains local definitions for the Prospero utilities.
 * It is expected that it will be different on different systems.
 */

/*
 * Section 1: Installation specifc definitions for the Prospero Applications
 *
 * The definitions in the following section provide information about
 * the proper configuration of Prospero clients on your system. 
 * They may change from system to system.
 */

/*
 * If P_BINARIES is defined and not empty, it defines the directory
 * in which the Prospero clients will look for executables.  If set in 
 * the users environment, P_BINARIES will override the definition
 * here.  If neither are defined, or if the definition is the empty
 * string, then the clients will use the user's search path to find
 * the binaries.
 *
 * The only executable program that the prospero clients and compatability 
 * library call currently is VCACHE.  
 */
#define P_BINARIES            "/pfs/bin"

/*
 * If P_PATH, if defined, is a directory that will be added to the
 * user's search path by the Prospero initialization script
 * (vfsetup.source for CSH users, vfsetup.profil for SH users).  
 * It only needs to be defined if the Prospero binaries will not already be 
 * in the user's search path.  Multiple directories separated by spaces may 
 * be specified.
 */
#define P_PATH                P_BINARIES


/*
 * P_AFS is the name of the directory under which AFS files are found.
 * If should be set only if your system supports the Andrew File System.  
 *
 * #define P_AFS "/afs"  
 */
#define P_AFS "/nfs/afs"

/*
 * P_KERBEROS, if defined, means that the clients will transmit Kerberos
 * (version 5) authentication information and that the server will understand
 * such information.  This should be #defined if your site uses Kerberos
 * authentication; otherwise, you won't have a use for it.  If you #define
 * this, then you also need to configure the top-level Makefile with the
 * location of the Kerberos libraries and include directory.
 * 
 * Note that most Kerberos sites out there are still using the older version 4,
 * which is not compatible with the new version 5.
 *
 * #define P_KERBEROS
 */


/*
 * P_P_PASSWORD, if defined, means that the clients will transmit password
 * authentication information.
 *
 * #define P_P_PASSWORD
 */
#define P_P_PASSWORD

/*
 * P_ACCOUNT, if defined, means that the clients will transmit
 * accounting information.
 *
 * #define P_ACCOUNT
 */




/*
 * Section 2: Site Definitions for the Prospero Applications
 *
 * The definitions in the following section define the Prospero site.
 * They will not necessarily change from system to system.  These definitions
 * should only be changed if you are setting up your own prospero site.   
 * Setting up a site is different than installing the software on a new
 * system.  Unless your site supports storage of new virtual systems, 
 * do not change these definitions. 
 */

/* Section 2a: Site definitions we expect you might need to change
 * for your site.
 */

/*
 * P_SITE_HOST and DIRECTORY define the system level name of the
 * virtual directory under which site data and meta-data are stored. 
 * It will vary from site to site.
 */
#define P_SITE_HOST           "PROSPERO.ISI.EDU"
#define P_SITE_DIRECTORY      "/pfs/pfsdat"

/*
 * P_SITE_MASTER_VSLIST is the name of the virtual directory (relative
 * to P_SITE) under which reference to local virtual systems can be
 * found.  When setting up a new site, choose a one component name
 * that seems appropriate.
 */
#define P_SITE_MASTER_VSLIST  "vs_root_isi"

/*
 * Section 2b:  Site Definitions we expect you to not change except in unusual
 *              circumstances.
 */

/*
 * P_GLOBAL_VS_ROOT* is a reference to the global root for the
 * underlying ugly-names of virtual systems.  These definitions should not
 * be changed unless your site is not connected to the Internet.
 */
#define P_GLOBAL_VS_ROOT_HOST "PROSPERO.ISI.EDU"
#define P_GLOBAL_VS_ROOT_FILE "/pfs/pfsdat/vs_root_g"

/*
 * Section 2c:   Site definitions you should never have a need to change
 *               (they can be reconfigured because we did not want
 *               Prospero to have any particular directory names hardcoded 
 *               into it, and it will not hurt Prospero in any way if you
 *               change them, but we think it is unlikely you would want to..)
 */

/*
 * P_MASTER_VS is the name (relative to MASTER_VSLIST) of the virtual
 * system to be used when creating new virtual systems with newvs.
 * There should be no need to change these definitions.
 */
#define P_MASTER_VS           "master"
#define P_PROTOTYPE_VS        "prototype"

/* 
 * P_SITE_STORAGE is the name (relative to master) of a directory
 * with references to the storage areas of the individual systems that 
 * compose a site. There should be no need to change this
 * definitions.
 */
#define P_SITE_STORAGE        "pfs_storage"

/*
 * P_VS_STORAGE is the name (relative to the directories named in
 * P_SITE_STORAGE) of the directory under which new virtual systems
 * may be stored.  There should be no need to change this definition.
 */
#define P_VS_STORAGE          "local_vsystems"

/*
 * Section 3: Special-purpose configuration options for the Prospero
 *            Applications
 *
 * 
 * The configuration options in this section are used only to meet special
 * needs.  They have been added to meet the special needs of particular
 * Prospero commercial and research users.  They have general utility, but they
 * are more in the nature of optimizations and tuning.  Prospero will function
 * just fine if you leave them alone, although not as efficiently in some
 * applications.  
 *
 * Unless you are an experienced Prospero user, you do not want to
 * alter any of these definitions. 
 */

/*
 * If NFS is supported P_NFS can defined.  If set, the clients will attempt 
 * to use NFS to retrieve files from remote systems.  
 *
 * *WARNING* *WARNING* *WARNING* *WARNING* *WARNING* *WARNING* *WARNING*
 *  
 * If you will be using this option, you need to modify the 
 * function pmap_nfs() in the file lib/psrv/pmap_nfs.c to do proper NFS 
 * retrievals from the server.  You also will need to modify the function
 * check_nfs() in the file lib/psrv/check_nfs.c to make sure that your 
 * server publishes NFS data only for exportable filesystems, and that it 
 * publishes the data with the correct NFS filesystem names.  The version of
 * lib/psrv/check_nfs.c we provide is only a stub, and does not perform 
 * this function appropriately for most environments.
 *
 * This option is growing obsolescent with the existence of automounters.
 * It's still useful for using sites such as WU-ARCHIVE, which allow one to 
 * anonymously mount NFS partitions, and you are welcome to experiment with it
 * for those purposes.
 *
 * *WARNING*  *WARNING*  *WARNING* *WARNING* *WARNING* *WARNING* *WARNING*
 * 
 * Until you customize lib/psrv/check_nfs.c and lib/psrv/pmap_nfs.c, do NOT
 * #define P_NFS.   If your clients run an NFS automounter, 
 * you can accomplish the same result by defining SHARED_PREFIXES in the
 * pserver.h file.
 * 
 *
 * #define P_NFS
 */

/*
 * P_CACHE_* define locations for files associated with the caching on
 * the client.
 * 
 * P_CACHE_TMP is for temporary files during transfer, 
 * P_CACHE_VC  is for the cache itself
 * P_CACHE_P   is for personal copies of files to ensure they don't change
 *
 * Allthough these can be symbolic links, they MUST all be in the same
 * filesystem since hard-links are used to move them around
 * 
 * They must also match the definitions at the top of Makefile
 *
 * Note that each of these directories needs to be writable by whatever ids 
 * are running Prospero clients. Directories under P_CACHE_P can be restricted
 * to specific users, and if "vcache" was to run setuid "pfs" then 
 * P_CACHE_VC could be restricted to that user, although code would
 * then be needed in vcache to chown files to the particular user -- and 
 * that won't be possible on some (e.g. Sun) unixes.
 * (swa: I don't understand why mitra made the comment above.  This can be
 * accomplished by setting the sticky bit on directories.)
 *
 * This caching code was provided by mitra@path.net and is still experimental.
 * It is not a supported part of this release.
 *
 * It lacks a number of facilities that willl have to be present before this
 * becomes a supported part of the release.  Send email to
 * info-prospero@ISI.EDU if you want to know what changes need to be made.
 *
 * The strategy for expiring items from the cache is in lib/pfs/pmap_cache.c.
 * 
 * #define P_CACHE_ENABLED
 */

/* These definitions will have no effect unless P_CACHE_ENABLED is defined. */
#define P_CACHE_P     "/nfs/u5/divirs/vcache/p"
#define P_CACHE_VC    "/nfs/u5/divirs/vcache/vc"
#define P_CACHE_TMP   "/nfs/u5/divirs/vcache/tmp"


/* 
 * This option is not currently supported in the release, because it needs
 * cleanup; notes available from SWA on what steps need to be taken to clean
 * it up to be of production quality.
 *
 * It lacks a number of facilities that will have to be present before this
 * becomes a supported part of the release.  Send email to
 * info-prospero@ISI.EDU if you want to know what changes need to be made.
 *
 * You can have the ACCESS-METHOD interpreting code that does retrievals
 * linked directly with your client executables.  This significantly bloats
 * executable size, but gains you more speed on the retrievals (as long as you
 * manage to avoid thrashing.)  This optimization is useful in circumstances
 * where the cost of slower response is greater than the cost of buying some
 * more RAM for your machine.  
 *
 * N.B.: if your machine is paging or swapping 
 * under these usage conditions, this optimization will cost
 * you time rather than saving it, unless you are able to configure
 * libpvcache on your system to be a shared library where only one copy of the
 * library exists in RAM for all of the clients.
 *
 * This is a traditional space/time tradeoff.
 * To set the tradeoff in favor of reducing time and increasing space,
 * define INCREASE_CLIENT_EXECUTABLE_SIZE_DO_NOT_EXEC_VCACHE.  By default,
 * a separate executable named 'vcache' is run to perform some types of 
 * retrievals.
 *
 * If you define this, then also edit VCACHE_LIBS in the top-level Prospero
 * Makefile.  You will need to link VCACHE_LIBS with all the client programs
 * that want to use the VCACHE library.
 *
 * #define INCREASE_CLIENT_EXECUTABLE_SIZE_DO_NOT_EXEC_VCACHE
 */



/* Location of wais sources, note trailing slash.  MITRAism.
 *
 * Note that it is not clear to me (swa) why this is useful to have for the
 * clients.  Isn't this a server-specific thing?  It's on my TODO list.
 */
#define WAIS_SOURCE_DIR "/usr/local/wais/wais-sources/"

/*
 * Filters are not supported in the default distribution because they 
 * currently only work on the VAX.  We are working on resolving these 
 * problems. 
 */ 
#define P_NO_FILTERS

/* If you want to use the UNIX compatability library (documented in the
 * Prospero Library Reference Manual), remove the definition of
 * P_NO_PCOMPAT and follow the additional directions in the INSTALLATION file.
 * This option is currently slightly broken, because Prospero has undergone
 * a number of changes.  it can easily be reported.
 *
 */
#ifdef AIX	/* lucb */
#define P_NO_PCOMPAT /**/
#else
/* #define P_NO_PCOMPAT */
#endif

/*
 * This option exists for sites that have special applications.  If this option
 * is defined, the pcompat library's open() routines will prompt for an FTP
 * username and password to retrieve files, if necessary.  This may increase
 * the amount of information available through applications linked with the
 * compatability library, but at the cost of changing the user interface
 * to applications such as 'cat' which never normally prompt for a password.
 * Only #define this if you have a special application.
 * 
 * #define PCOMPAT_SUPPORT_FTP
 */

