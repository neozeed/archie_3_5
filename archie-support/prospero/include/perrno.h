/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#ifndef PFS_PERRNO_H
#define PFS_PERRNO_H

#include <pfs_threads.h>

/* this file and lib/pfs/perrmesg.c should be updated simultaneously */

/*
 * perrno.h - definitions for perrno
 *
 * This file contains the declarations and defintions of of the external
 * error values in which errors are returned by the pfs and psrv
 * libraries.
 */

extern char	*p_err_text[];
extern char	*p_warn_text[];

EXTERN_INT_DECL(perrno);
#define perrno p_th_arperrno[p__th_self_num()]
EXTERN_INT_DECL(pwarn);
#define pwarn p_th_arpwarn[p__th_self_num()]
EXTERN_CHARP_DECL(p_warn_string);
#define p_warn_string p_th_arp_warn_string[p__th_self_num()]
EXTERN_CHARP_DECL(p_err_string);
#define p_err_string p_th_arp_err_string[p__th_self_num()]


/* Error codes returned or found in perrno */

#ifndef PSUCCESS
#define	PSUCCESS		0
#endif

/* Error codes 1 through 20 are reserved for the ardp library */
/* Defined in include/ardp.h */

/* vl_insert */
#define VL_INSERT_ALREADY_THERE	21	/* Link already exists	        */
#define VL_INSERT_CONFLICT	22	/* Link exists with same name   */

/* ul_insert */
#define UL_INSERT_ALREADY_THERE 25	/* Link already exists		*/
#define UL_INSERT_SUPERSEDING   26	/* Replacing existing link	*/
#define UL_INSERT_POS_NOTFOUND  27	/* Prv entry not in dir->ulinks */

/* rd_vdir */
#define RVD_DIR_NOT_THERE	41	/* Temporary NOT_FOUND		    */
#define RVD_NO_CLOSED_NS	42	/* Namespace not closed w/ object:: */
#define RVD_NO_NS_ALIAS		43	/* No alias for namespace NS#:      */
#define RVD_NS_NOT_FOUND	44	/* Specified namespace not found    */

/* pfs_access */
#define PFSA_AM_NOT_SUPPORTED   51      /* Access method not supported  */

/* p__map_cache */
#define PMC_DELETE_ON_CLOSE     55	/* Delete cached copy on close   */
#define PMC_RETRIEVE_FAILED     56      /* Unable to retrieve file       */

/* mk_vdir */
/* #define MKVD_ALREADY_EXISTS     61	* Directory already exists      */
/* #define MKVD_NAME_CONFLICT	62	* Link with name already exists */

/* vfsetenv */
#define VFSN_NOT_A_VS		65	/* Not a virtual system          */
#define VFSN_CANT_FIND_DIR	66	/* Not a virtual system          */

/* add_vlink */
/* #define ADDVL_ALREADY_EXISTS    71	* Directory already exists      */
/* #define ADDVL_NAME_CONFLICT	72	* Link with name already exists */

/* pset_at */
#define PSET_AT_TARGET_NOT_AN_OBJECT 81 /* The link passed to PSET_AT() has a
                                           TARGET such that it does not
                                           refer to an object, so we can't set
                                           any object attributes on it. */

/* Error codes for parsing problems. */
#define PARSE_ERROR           101     /* General parsing syntax error . */

/* Local error codes on server */

/* dsrdir */
#define DSRDIR_NOT_A_DIRECTORY 111	/* Not a directory name		*/
/* dsrfinfo */
#define DSRFINFO_NOT_A_FILE    121      /* Object not found             */
#define DSRFINFO_FORWARDED     122      /* Object has moved             */

/* Error codes that may be returned by various procedures               */
#define PFS_FILE_NOT_FOUND     230      /* File not found               */
#define PFS_DIR_NOT_FOUND      231      /* Directory in path not found  */
#define PFS_SYMLINK_DEPTH      232	/* Max sym-link depth exceeded  */
#define PFS_ENV_NOT_INITIALIZED	233	/* Can't read environment	*/
#define PFS_EXT_USED_AS_DIR    234	/* Can't use externals as dirs  */
#define PFS_MAX_FWD_DEPTH      235	/* Exceeded max forward depth   */

/* Error codes returned by directory server                    */
/* some of these duplicate errors from individual routines     */
/* some of those error codes should be eliminated              */
#define DIRSRV_WAIS	       240	/* WAIS gateway failure */
#define DIRSRV_GOPHER	       241	/* Gopher gateway failure */
#define DIRSRV_AUTHENT_REQ     242      /* Authentication required       */
#define DIRSRV_NOT_AUTHORIZED  243      /* Not authorized                */
#define DIRSRV_NOT_FOUND       244      /* Not found                     */
#define DIRSRV_BAD_VERS        245
#define DIRSRV_NOT_DIRECTORY   246
#define DIRSRV_ALREADY_EXISTS  247	/* Identical link already exists */
#define DIRSRV_NAME_CONFLICT   248	/* Link with name already exists */
#define DIRSRV_TOO_MANY        249	/* Too many matches to return    */

#define DIRSRV_UNIMPLEMENTED   251      /* Unimplemented command         */
#define DIRSRV_BAD_FORMAT      252
#define DIRSRV_ERROR           253
#define DIRSRV_SERVER_FAILED   254      /* Unspecified server failure    */

#ifndef PFAILURE
#define	PFAILURE 	       255      /*  Random other complaint. */
#endif

/* If DEBUG_PFAILURE is defined, then the no-op function it_failed() will be
   called right before any function originates a PFAILURE return.  This can be
   handy in tracking down an error in library usage when a library call is
   returning PFAILURE to you.  This is normally not enabled so that we can
   avoid the overhead of the unnecessary call to it_failed(). 
   it_failed() is in lib/pfs/perrmesg.c. */
#ifdef DEBUG_PFAILURE
#define RETURNPFAILURE do { it_failed();  return(PFAILURE); } while(0)
#else
#define RETURNPFAILURE return(PFAILURE)
#endif

/* Warning codes */

#define PNOWARN			 0	/* No warning indicated		 */
#define PWARN_OUT_OF_DATE	 1	/* Software is out of date       */
#define PWARN_MSG_FROM_SERVER	 2      /* Warning in p_warn_string      */
#define PWARN_UNRECOGNIZED_RESP  3	/* Unrecognized line in response */
#define PWARNING	       255	/* Warning in p_warn_string      */

/* Function to reset error and warning codes */
extern void p_clear_errors(void);
#if 0
/* This macro is deprecated and will go away soon.  Use p_clear_errors()
	instead. */
#define clear_prospero_errors() p_clear_errors()
#endif

#endif /*PFS_PERRNO_H*/
