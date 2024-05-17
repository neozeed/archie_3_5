/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1991, 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

/*
 * CONFLICT_BETWEEN_DEFINITION_OF_NULL_IN_SYS_PARAM_H_AND_STDDEF_H:
 * 			include/pprot.h, server/dirsrv.h, server/shadowcvt.c
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
 * top-level MAKEFILE, since this gross kludge is necessary in 
 * include/pprot.h, server/dirsrv.h, and server/shadowcvt.c.
 */
#if defined (NOTDEFINED)
#define CONFLICT_BETWEEN_DEFINITION_OF_NULL_IN_SYS_PARAM_H_AND_STDDEF_H
#endif

#ifndef MAXPATHLEN
#ifdef CONFLICT_BETWEEN_DEFINITION_OF_NULL_IN_SYS_PARAM_H_AND_STDDEF_H
#undef NULL
#endif				/* #ifdef CONFLICT...  */
#include <sys/param.h>          /* XXX This should Change.  MAXPATHLEN is too
                                   UNIX-specific.   It is almost always 1024,
                                   but we should use MAX_VPATH instead. */
/* Not defined on SCO Unix */
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifdef CONFLICT_BETWEEN_DEFINITION_OF_NULL_IN_SYS_PARAM_H_AND_STDDEF_H
#ifndef NULL
#include <stddef.h>
#endif				/* #ifndef NULL */
#endif				/* #ifdef CONFLICT... */
#endif				/* #ifndef MAXPATHLEN */

/* Protocol Definitions */

#define	       VFPROT_VNO	5      /* Protocol Version Number           */

#define	       DIRSRV_PORT      1525   /* Server port used if not in srvtab */
#define        PROSPERO_PORT	191    /* Officially assigned prived port   */

#define	       SEQ_SIZE		32     /* Max size of sequence text in resp */ 
/* This definition is now obsolete.  It only exists to support Version 1
   code; version 5 code dynamically allocates buffers. */
#define	       MAX_DIR_LINESIZE 160+MAXPATHLEN /* Max linesize in directory */

#define	       MAX_FWD_DEPTH    20     /* Max fwd pointers to follow        */

#define S_AD_SZ		sizeof(struct sockaddr_in)


