/*
 * Copyright (c) 1991, 1992, 1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <pfs_threads.h>

/* 
 * pcompat.h - Definitions for compatability library
 *
 * This file contains the defintions used by the compatability
 * library.  Among the definitions are the possible values for
 * the pfs_enable variable.  This file also contains the external 
 * declaration of that variable.  The module pfs_enable.o is included in 
 * libpfs.a because some of the routines in that library set it.
 * It can also be explicitly declared in a program that uses the compatability
 * library, if you wish.
 *
 * The only place it is checked, however, is in pfs_access, 
 * found in libpcompat.a
 */

#if 0                           /* this code is commented out because
                                   pfs_default is no longer an external
                                   interface.  --swa, April 13, 1994*/
extern	int pfs_default;        /* initialized to PMAP_UNINITIALIZED in
                                   libpcompat. Set to some other value in
                                   p__get_pfs_default(), which might include
                                   PMAP_UNSET.   Keeping this as a global
                                   variable isn't really necessary.  It does
                                   let the users know what the starting value
                                   of PFS_DEFAULT was, but I'm not sure that's
                                   strictly necessary. 
                                   This variable is currently used entirely
                                   internally to the PCOMPAT library, and could
                                   be renamed easily, since no users are using
                                   it as an interface. */
#endif

extern  int pfs_enable;         /* pfs_enable is declared in the PFS library,
                                   since routines in that library modify it.
                                   In that library, it defaults to PMAP_UNSET.
                                   This can be declared explicitly in a file
                                   linked with the  program using this library
                                   if the programmer wants to override the
                                   value. */ 

extern  int pfs_quiet;           /* see lib/pcompat/pfs_quiet.c.  This can be
                                    declared explicitly in a file linked with
                                    the  program using this library if the
                                    programmer wants to override the 
                                    value.  Non-zero value means system calls
                                    don't print any information. */

/* Definitions for values of pfs_enable */
#if 0                           /* this will probably never be needed, and can
                                   be deleted after Beta.5.3 goes out. 

                                   --swa, April 13, 1994*/
#define PMAP_UNINITIALIZED (-1) /* Used only for pfs_default.  Indicates that
                                   its starting value hasn't been read from the
                                   environment yet.  If the environment has
                                   already been searched for PFS_DEFAULT, then
                                   this will be reset to PMAP_UNSET. */
#endif
#define PMAP_DISABLE      0     /* always resolve names in UNIX */
#define PMAP_ENABLE       1     /* always reslve names in VS */
#define PMAP_COLON	  2     /* Resolve names in UNIX by default.  Names
                                   preceded with a colon are resolved in the
                                   VS.  */
#define PMAP_ATSIGN_NF	  3     /* Resolve names in VS by default.  Names
                                   preceded with an @ sign and names not found
                                   in the VS are resolved in the UNIX
                                   filesystem. */

#define PMAP_ATSIGN	  4     /* Resolve names in the VS by default. Names
                                   preceded with an @ sign are resolved in the
                                   UNIX filesystem. */
#define PMAP_UNSET        5     /* pfs_enable is initialized to this value by
                                   the pcompat library, unless the user program
                                   declares its own version of pfs_enable and
                                   initializes it to something else.  If
                                   pfs_enable is set to PMAP_UNSET, then
                                   p__initialize_pfs_enable() will be called by
                                   the PCOMPAT library routines in order to
                                   initialize pfs_enable to the current pfs
                                   default. 

                                   This allows a user program to initialize
                                   pfs_enable and not have it overridden by
                                   pfs_default.   */
                                   
/* Definitions for PFS_ACCESS */
#define PFA_MAP           0  /* Map the file name only                       */
#define PFA_CREATE        1  /* Create file if not found                     */
#define PFA_CRMAP         2  /* Map file name.  Map to new name if not found */
#define PFA_RO            4  /* Access to file is read only                  */


#define DISABLE_PFS(stmt) do {int DpfStmp; DpfStmp = pfs_enable;\
			      pfs_enable = PMAP_DISABLE; \
			      stmt; \
			      pfs_enable = DpfStmp;} while (0)

#define DISABLE_PFS_START() DpfStmp = pfs_enable; pfs_enable = PMAP_DISABLE;
#define DISABLE_PFS_END()   pfs_enable = DpfStmp;

#if 0                           /* This interface is dead as of 4/13/94 --swa
                                   */ 
void p__get_pfs_default();      /* only used here :) */
#define check_pfs_default() \
	do { if(pfs_default == -1) p__get_pfs_default(); } while (0)
#endif
/* New interface as of 4/13/94 --swa */
/* This is called by the p__compat_initialize() routine, iff pfs_enable is set
   to PMAP_UNSET.  It looks for the PFS_DEFAULT environment variable and
   attempts to set pfs_enable accordingly. 
*/
void p__set_pfs_enable_from_default(void);

extern int pfs_access(const char path[], char npath[], int npathlen, int
                      flags);

/* Subfunctions used internally by the PCOMPAT library. */

extern int p__getvdirentries(int fd,  char *buf,  int nbytes,  int *basep);

extern int p__readvdirentries(char *dirname);

extern int p__delvdirentries(int desc);

extern int p__seekvdir(int desc, int pos);

extern int p__getvdbsize(int desc, int pos);

#ifndef DIR
#include <pmachine.h>           /* for USE_SYS_DIR_H test. */

/* Needed if not already included. */
#ifdef USE_SYS_DIR_H
#include <sys/dir.h>
#else                           /* USE_SYS_DIR_H */
#include <dirent.h>
#endif                          /* USE_SYS_DIR_H */

#endif                          /* DIR */

/* Called by seekdir(). */
extern void p__seekdir(register DIR *dirp, long loc);

/* This is called internally by all pcompat library functions that call
   PFS library functions.  It makes sure that the PFS library and any libraries
   that PFS uses are properly initialized before usage. */
extern void p__compat_initialize(void);
