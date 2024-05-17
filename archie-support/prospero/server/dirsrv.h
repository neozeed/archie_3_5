/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1993             by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

/*
 * CONFLICT_BETWEEN_DEFINITION_OF_NULL_IN_SYS_PARAM_H_AND_STDDEF_H:
 * 			include/pprot.h, server/dirsrv.h
 * This appears to be an unpleasant interaction between GCC
 * version 1.41 and the Ultrix version 4.2A system include files.  The
 * compiler complained about conflicting definitions of NULL in <sys/param.h>
 * and <stddef.h>.  The definitions are in fact the same, but with different
 * spacing, which the ANSI standard says should be irrelevant.
 * (Section 3.8.3, 'Macro Replacement' clearly states that:
 * "An object currently defined as a macro without use of lparen (an 
 *  object-like macro) may be reedefined by another #define preprocessing
 *  directive provided that the second definition is an object-like macro
 *  definition and the two replacement lists are identical."
 * It also clearly states that, in considering whether the replacment lists
 * are identical, "all white-space separations are considered identical"
 * You can #define this if you encounter this bug.  It will slightly slow
 * down the compilation if this is left #defined, but not very much.
 * I suggest you define it by modifying the  definition of MACHDEF in the 
 * top-level MAKEFILE, since this gross kludge is necessary both in 
 * pprot.h and in pmachine.h.
 */
#if defined (NOTDEFINED)
#define CONFLICT_BETWEEN_DEFINITION_OF_NULL_IN_SYS_PARAM_H_AND_STDDEF_H
#endif

#define MAX_VERSION	5	/* Highest version number supported	 */
#define MIN_VERSION	1	/* Lowest version number supported	 */

/* Commands */

#define UNKNOWN          (-1)     /* Unknown command. */
#define UNIMPLEMENTED	 0	/* Unimplemented   			  */
#define VERSION		 1	/* Set or request protocol version number */
/* AUTHENTICATOR is the V1 form. */
#define AUTHENTICATOR	 2	/* Provides authentication information	  */
#define AUTHENTICATE	 2	/* Provides authentication information	  */
#define DIRECTORY	 3	/* Set current working directory	  */
#define LIST		 4	/* List directory links matching args	  */
#define GET_OBJECT_INFO	 5	/* Requests information about a file	  */
#define EDIT_OBJECT_INFO 6	/* used to change file attributes 	  */
#define CREATE_LINK	 7	/* Adds a new link to a directory	  */
#define DELETE_LINK	 8	/* Removes existing link from directory	  */
/* MODIFY_LINK is the V1 command. */
#define MODIFY_LINK      9	/* Changes information about link	  */
#define EDIT_LINK_INFO   9
#define CREATE_OBJECT    10	/* Creates new file and returns pointer	  */
#define TERMINATE        11	/* Requests that server terminate -V1only */
/* CREATE_DIRECTORY was a V1 command; obsolete in V5. */
#define CREATE_DIRECTORY 12     /* Create new directory and returns ptr   */
#define UPDATE		 13     /* Update link by chasing forwarding ptr  */
#define STATUS           14     /* Return server status                   */
#define LIST_ACL         15     /* Return access control list             */
/* MODIFY_ACL is the V1 command. */
#define MODIFY_ACL       16     /* Modify access control list             */
#define EDIT_ACL         16     /* Modify access control list             */
/* This is obsolete. */
#define PACKET           17     /* Multi packet request (not implemented) */
#define RESTART		 18	/* Requests that server restart itself-V1only*/
#define ATOMIC           19     /* Not yet implemented. */
#define PARAMETER	 20     /* Server specific parameter operations   */
#define OBSOLETE         21     /* This command is obsolete.  Give up.  */

/* These are global variables which are used by the dirsrv code.  They are
   defined in "dirsrv.c."  They are mostly used by the stats generator. */
extern char    		prog[];
extern int 		fault_count;
extern char		last_request[ARDP_PTXT_LEN_R];
extern char		*last_error;
extern char		st_time_str[40];

extern int		in_port;
extern char		*portname;

extern int		req_count; /* How many requests total? */
#ifdef SERVER_SUPPORT_V1
extern int              v1_oldform_req_count;  /* How many Old format V1
                                                  requests? */
extern int              v1_req_count; /* How many V1 format requests */
extern int		crdir_count;
#ifdef __STDC__
int convert_v1_ltype(char l_type[], VLINK cur_link);
#endif
#endif
extern int		crlnk_count;
extern int		crobj_count;
extern int		dellnk_count;
extern int		eoi_count;
extern int		goi_count;
extern int		list_count;
extern int		lacl_count;
extern int		eli_count;
extern int		eacl_count;
extern int		status_count;
extern int		upddir_count;
extern int		parameter_count;


#ifndef MAXPATHLEN
#ifdef CONFLICT_BETWEEN_DEFINITION_OF_NULL_IN_SYS_PARAM_H_AND_STDDEF_H
#undef NULL
#endif				/* #ifdef CONFLICT...  */
#include <sys/param.h>
/* Needed for SCOUNIX which doesnt define this in MAXPATHLEN */
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifdef CONFLICT_BETWEEN_DEFINITION_OF_NULL_IN_SYS_PARAM_H_AND_STDDEF_H
#ifndef NULL
#include <stddef.h>
#endif				/* #ifndef NULL */
#endif				/* #ifdef CONFLICT... */
#endif				/* #ifndef MAXPATHLEN */

/* These are shared data storage areas. */
extern char     shadow[MAXPATHLEN];
extern char	pfsdat[MAXPATHLEN];
extern char	dirshadow[MAXPATHLEN];
extern char	dircont[MAXPATHLEN];
extern char     *object_pool;
#define OBJECT_POOL "objectpool" /* This will be a subdirectory of the pfsdat
                                    area.  Should this go into an include file?
                                    Eventually, when we redesign the database
                                    format. */ 

extern char     root[MAXPATHLEN];

extern char	aftpdir[MAXPATHLEN];

extern char	afsdir[MAXPATHLEN];
extern char	*db_prefix;
extern char	hostname[MAXPATHLEN];    /* Server's host name */
extern char	hostwport[MAXPATHLEN+30]; /* Host name w/ port if non-standard
                                             */ 
extern char	*logfile_arg;


extern struct db_entry 	db_prefixes[];
extern int		db_num_ents;

/* This function is used by dirsrv_v1() and dirsrv(). */
extern int check_handle(char *handle);
extern void log_server_stats(void);

/* These functions are used by the new modular dirsrv() */

extern int create_link(RREQ, char **, char **, INPUT in, char [], int);
extern int create_object(RREQ, char **, char **, INPUT in, char [], int);
extern int delete_link(RREQ, char *, char *, INPUT in, char [], int);
extern int dforwarded(RREQ, char [], int, VDIR);
extern int obforwarded(RREQ, char [], long, P_OBJECT);
extern int dirsrv_v1(RREQ req,char *command_next);
extern int dlinkforwarded(RREQ req, OUTPUT out, char client_dir[], 
                          int dir_magic_no, VDIR dir, char *components);
extern int oblinkforwarded(RREQ req, OUTPUT out, char client_dir[], 
                          int dir_magic_no, P_OBJECT dirob, char *components);
extern int edit_acl(RREQ, char **, char **, INPUT in, char *, int);
extern int edit_link_info(RREQ, char **, char **, INPUT in, char [], int);
/* renamed to avoid conflict with edit_object_info in the PFS library. */
extern int srv_edit_object_info(RREQ, char *command, char *next_word, 
                                INPUT in);
extern int forwarded(RREQ, VLINK, VLINK, char objectname[]);
extern int get_object_info(RREQ, char *, char *, INPUT in);
extern int list(RREQ, char **, char **, INPUT in, char [], int);
extern int list_acl(RREQ req, char * command, char *next_word, 
                    INPUT in, char client_dir[], int dir_magic_no); 
extern int parameter(RREQ req, char *command, char *next_word);
extern void restart_server(int, char *);
extern int update(RREQ req, char *command, char *next_word, INPUT in,
                  char client_dir[], int dir_magic_no);
extern int version(RREQ, char *command, char *next_word);

/* dirsrv utilities. */
VLINK	check_fwd();


#ifdef PFS_THREADS
extern p_th_mutex p_th_mutexP_PARAMETER_MOTD;
#endif
