/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
#include <stdio.h>
/*
 * pmachine.h - Processor/OS specific definitions
 */

/*
 * Machine types - Supported values
 *
 *   VAX, SUN, HP9000_S300, HP9000_S700, IBM_RTPC, ENCORE_NS32K,
 *   ENCORE_S93, ENCORE_S91, APOLLO, IBM_RS6000, IBM_PC
 * 
 *   MIPS_BE - MIPS Chip (Big Endian Byte Order)
 *   MIPS_LE - MIPS Chip (Little Endian Byte Order)
 *
 * Add others as needed.  
 *
 * Files that check these defintions:
 *   include/pmachine.h
 */

#include "pmachine-conf.h" /* lucb */

#ifdef 0 /* lucb */
#define P_MACHINE_TYPE 		"SUN"
#define SUN
#endif
/*
 * Operating system - Supported values
 * 
 * ULTRIX, BSD43, SUNOS (version 4), SUNOS_V3 (SunOS version 3), HPUX, SYSV,
 * MACH, DOMAINOS, AIX, SOLARIS (a.k.a. SunOS version 5), SCOUNIX
 *
 * Add others as needed.  
 *
 * Files that check these defintions:
 *   include/pmachine.h, lib/pcompat/opendir.c, lib/pcompat/readdir.c
 */

#ifdef 0 /* lucb */
#define P_OS_TYPE		"SUNOS"
#define SUNOS
#endif

/*
 * Miscellaneous definitions
 *
 * Even within a particular hardware and OS type, not all systems
 * are configured identically.  Some systems may need additinal
 * definitions, some of which are included below.   Note that for
 * some system types, these are automatically defined.
 *
 * define ARDP_MY_WINDOW_SZ if your system has some sort of internal limit
 *   on the number of UDP packets that can be queued.  SOLARIS has a limit of
 *   9 or less (6 seems to work for Pandora Systems), which inspired 
 *   this change.  If defined, the client will explicity request that 
 *   the server honor this window size.  This reduces retries and wasted
 *   messages. 
 * define NEED_MODE_T if mode_t is not typedefed on your system
 * define DD_SEEKLEN if your system doesn't support dd_bbase and dd_bsize
 * define DIRECT if direct is the name of your dirent structure
 *      This generally goes along with using sys/dir.h.
 * define USE_SYS_DIR_H if your system doesn't include sys/dirent.h, and
 *      sys/dir.h should be used instead.
 * define CLOSEDIR_RET_TYPE_VOID if your closedir returns void
 * Define GETDENTS if your system supports getdents instead of getdirentries
 * Define OPEN_MODE_ARG_IS_INT if your system has the optional third argument
 *   to open() as an int.  Don't #define it if the optional third argument to
 *   open() is a mode_t.  You will need to #undef this if you're using the
 *   sysV interface to SunOS.
 * Define SIGCONTEXT_LACKS_SC_PC if your system's sigcontext structure lacks
 *   an sc_pc member.  This appears to be the case on HPUX version 8.07 on the
 *   HP 9000 series 700 workstations.  I don't know if it's the case anywhere
 *   else, and I suspect it's a bug in the release that will be fixed later.
 * Define PROTOTYPE_FOR_OPEN_HAS_EMPTY_ARGLIST if your system prototypes the
 *   open() function with an empty argument list.  This is necessary because
 *   gcc requires the prototype to match the function invocation.
 * Define PROTOTYPE_FOR_SELECT_USES_INT_POINTER if your system prototypes the
 *   select() function with 'int *' for the second, third, and fourth arguments
 *   instead of 'fd_set *'.  
 * Define BSD_UNION_WAIT if your system doesn't support the new POSIX wait()
 *   interface and does support the old BSD wait() interface and 'union wait'
 *   member. 
 * Define INCLUDE_FILES_LACK_FDOPEN_PROTOTYPE if your system's <stdio.h> lacks
 *   an fdopen() prototype.
 * Define INCLUDE_FILES_LACK_POPEN_PROTOTYPE if your system's <stdio.h> lacks
 *   an popen() prototype.
 * Define INCLUDE_FILES_LACK_TEMPNAM_PROTOTYPE if your system's <stdio.h> lacks
 *   a prototype for tempnam().
 */

#ifdef SOLARIS
#define ARDP_MY_WINDOW_SZ 6
#endif

/* Not sure if we need this as well, we certainly need the definition
   of direct as dirent below */
#ifdef SCOUNIX
#define DIRECT
#endif

/*****************************************************************/
/* If your machine and OS type are listed above, and if your     */
/* configuration is relatively standard for your machine and     */
/* OS, there should be no need for any changes below this point. */
/*****************************************************************/

/*
 * Machine or OS dependent parameters
 *
 *  The comment at the head of each section names the paramter
 *  and the files that use the definition
 */

/*
 *  BYTE_ORDER: lib/psrv/plog.c, lib/psrv/check_acl.c
 *  #ifdefs  by lucb
 */

#ifdef BIG_ENDIAN
#  undef BIG_ENDIAN
#endif

#ifdef LITTLE_ENDIAN
#  undef LITTLE_ENDIAN
#endif

#ifdef BYTE_ORDER
#  undef BYTE_ORDER
#endif

#define BIG_ENDIAN		1
#define LITTLE_ENDIAN		2

#if defined(SUN)        || defined(HP9000_S300) || defined(HP9000_S700) || \
    defined(IBM_RTPC) || defined(IBM_RS6000) || \
    defined(ENCORE_S91) || defined(ENCORE_S93)  || defined(APOLLO)   || \
    defined(MIPS_BE)
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#endif

/*
 * PUTENV: lib/pfs/vfsetenv.c
 *
 * PUTENV must be defined if your C library supports the putenv
 * call instead of setenv (e.g. Ultrix and SunOS).
 */
#if defined(ULTRIX) || defined(SUNOS) || defined(SUNOS_V3) || defined(HPUX) \
    || defined(SOLARIS) || defined(SCOUNIX) || defined(AIX)
#define PUTENV
#endif

/*
 * BADSETENV: lib/pfs/penviron.c
 *
 * Older BSD 4.3 systems have a bug in the C library routine setenv.
 * Define BADSETENV if you wish to compile a working version of this
 * this routine.
 * 
 * #define BADSETENV
 */

/*
 * NOREGEX: lib/pfs/wcmatch.c
 *
 * NOREGEX must be defined if your C library does not support the
 * re_comp and re_exec regular expression routines.
 */
#if defined(HPUX) || defined(SOLARIS) || defined(SCOUNIX)
#define NOREGEX
#endif

/*
 * String and byte manipulating 
 * procedures: lib/pfs/sindex.c, lib/pcompat/pfs_access.c
 */
#if defined(HPUX) || defined(SYSV) || defined(SOLARIS)
#define index		strchr
#define rindex		strrchr
#define bcopy(a,b,n)	memmove(b,a,n)
#define bzero(a,n)	memset(a,0,n)
#define bcmp		memcmp
#endif

/*
 * getwd: server/pstart.c
 */
#if defined(HPUX) || defined(SYSV) || defined(SOLARIS)
#define getwd(d)	getcwd(d, MAXPATHLEN)
#endif

/*
 * SETSID:  server/dirsrv.c
 *
 * SETSID is to be defined if the system supports the POSIX
 * setsid() routine to create a new session and set the process 
 * group ID.
 */
#if defined(HPUX) || defined(SOLARIS) || defined(SCOUNIX)
#define SETSID
#endif

/*
 * NFILES: user/vget/pclose.c
 *
 * NFILES is the size of the descriptor table.
 */
#if defined(HPUX)
#define NFILES _NFILE
#elif defined (SOLARIS)
#define NFILES sysconf(_SC_OPEN_MAX)
#else
#define NFILES getdtablesize()
#endif

/*
 * SIGNAL_RET_TYPE: user/vget/ftp.c, user/vget/pclose.c
 *
 * This is the type returned by the procedure returned by
 * signal(2) (or signal(3C)).  In some systems it is void, in others int.
 *
 */
#if defined (BSD43) || defined(SUNOS_V3)
#define SIGNAL_RET_TYPE int
#else
#define SIGNAL_RET_TYPE void
#endif

/*
 * CLOSEDIR_RET_TYPE_VOID: lib/pcompat/closedir.c
 *
 * If set, closedir() returns void.
 */
#if defined (NOTDEFINED) 
#define CLOSEDIR_RET_TYPE_VOID
#endif

/*
 * DIRECT: lib/pcompat/ *dir.c app/ls.c
 *
 *  Use direct as the name of the dirent struct
 */
#if defined(DIRECT)
#define dirent direct
#endif

/*
 * USE_SYS_DIR_H: lib/pcompat/ *dir.c app/ls.c
 *
 *  Include the file <sys/dir.h> instead of <dirent.h>
 */
#if defined (NOTDEFINED)
#define USE_SYS_DIR_H           /* slowly fading out of necessity */
                                /* Hopefully it's finally dead. */
#endif

/*
 * DIR structure definitions: lib/pcompat/telldir.c,opendir.c
 */
#if defined (SUNOS) || defined(SUNOS_V3)
#define dd_bbase dd_off
#endif

#if defined (DD_SEEKLEN)
#define dd_bbase dd_seek
#define dd_bsize dd_len
#endif
 
/*
 * GETDENTS: lib/pcompar/readdir.c
 *
 * Define GETDENTS if your system supports getdents instead of
 * getdirentries.
 */

#if defined (SOLARIS)
#define GETDENTS
#endif
#if defined (GETDENTS)
#define getdirentries(F,B,N,P) getdents(F,B,N)
#endif

/*
 * NEED MODE_T typedef: ls.c
 *
 * Define this if mode_t is not defined by your system's include
 * files (sys/types.h or sys/stdtypes.h or sys/stat.h).
 */

#if defined (NEED_MODE_T)
typedef unsigned short mode_t;
#endif

/*
 * OPEN_MODE_ARG_IS_INT: used: lib/pcompat/open.c
 * Define OPEN_MODE_ARG_IS_INT if your system has the optional third argument
 *   to open() as an int.  Don't #define it if the optional third argument to
 *   open() is a mode_t.  You will need to #undef this if you're using the
 *   sysV interface to SunOS.
 */
/* Not sure how MACH and DOMAINOS actually need it; this is a guess. */
#if defined(ULTRIX) || defined(BSD43) || defined(SUNOS) || defined(SUNOS_V3) \
	|| defined(MACH) || defined(DOMAINOS)
#define OPEN_MODE_ARG_IS_INT
#endif

/*
 * SIGCONTEXT_LACKS_SC_PC typedef: server/dirsrv.c
 * 
 * Define SIGCONTEXT_LACKS_SC_PC if your system's sigcontext structure lacks
 *   an sc_pc member.  This appears to be the case on HPUX version 8.07 on the
 *   HP 9000 series 700 workstations.  I don't know if it's the case anywhere
 *   else, and I suspect it's a bug in the release that will be fixed later.
 */

#if (defined(HPUX) && defined(HP9000_S700)) || defined(SOLARIS) || defined(SCOUNIX)
#define SIGCONTEXT_LACKS_SC_PC
#endif

/*
 * PROTOTYPE_FOR_OPEN_HAS_EMPTY_ARGLIST: lib/pcompat/open.c
 * See the file for how this is used.  In GCC, an old-style function prototype
 * is not compatible with a full ANSI function definition containing a ...
 * (variable argument list).  This definition makes sure that we use the
 * appropriate definition of open() to correspond with the system include
 * files.
 */
/* 
 * This is triggered if we're using the GCC fixed <sys/fcntlcom.h> under an
 * ANSI C compiler; means that open() will be fully prototyped.
 */

#if !defined(HPUX) && !defined(SOLARIS) && (!defined(_PARAMS) && !defined(__STDC__))
#define PROTOTYPE_FOR_OPEN_HAS_EMPTY_ARGLIST
#endif

/*
 * Define PROTOTYPE_FOR_SELECT_USES_INT_POINTER if your system prototypes the
 *   select() function with 'int *' for the second, third, and fourth arguments
 *   instead of 'fd_set *'.   This avoids compilation warnings in libardp.
 */
#if defined(HPUX)
#define PROTOTYPE_FOR_SELECT_USES_INT_POINTER
#endif


#ifdef PROTOTYPE_FOR_SELECT_USES_INT_POINTER
#define select(width, readfds, writefds, exceptfds, timeout) \
  select(width, (int *) readfds, (int *) writefds, (int *) exceptfds, timeout)
#endif

/*
 * Define BSD_UNION_WAIT if your system doesn't support the new POSIX wait()
 *   interface and does support the old BSD wait() interface and 'union wait'
 *   member. 
 * Used in: lib/pcompat/pmap_cache.c, lib/pcompat/pmap_nfs.c, user/vget.c
 */

#if defined(NOTDEFINED)
#define BSD_UNION_WAIT
#endif

#if defined(SOLARIS)
/* In theory these are defined in stdio.h, but they're not present (at least in
 * Solaris 2.3 they're not) and GCC doesn't fix this correctly right now. 
 */
#define INCLUDE_FILES_LACK_FDOPEN_PROTOTYPE 
#define INCLUDE_FILES_LACK_POPEN_PROTOTYPE 
#define INCLUDE_FILES_LACK_TEMPNAM_PROTOTYPE
#endif /* SOLARIS */

/*
 * Catch any definitions not in system include files
 *
 *  The comment at the head of each section names the paramter
 *  and the files that use the definition
 */

/*
 * OPEN_MAX: Maximum number of files a process can have open
 */
#ifndef OPEN_MAX
#define OPEN_MAX 64
#endif

/* MAXPATHLEN is still needed despite recommended change to pprot.h
   since some files include sys/param.h but not pprot.h and use MAXPATHLEN

   XXX MAXPATHLEN will eventually disappear from Prospero.   SCO doesn't
   provide that interface, and neither does Posix.  All the fixed-length
   code that uses MAXPATHLEN sized buffers will have to be rewritten to use
   flexible-length names.  -swa, Mar 10, 1994
*/

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifdef SCOUNIX
/* sys/types.h is needed to define u_long, it must be before the FD_SET stuff*/
/* I dont know why this is so, but remember it from another port */
#define vfork fork
#endif

/* End of stuff required for SCO Unix */


/*
 * FD_SET: lib/pfs/dirsend.c, user/vget/ftp.c
 */
#include <sys/types.h>
/* FD_SET is provided in SOLARIS by <sys/select.h>.  This include file does not
   exist on SunOS 4.1.3; on SunOS it's provided in <sys/types.h>.  
   No <sys/select.h> on HP-UX.
   Don't know about other versions of UNIX. */
#ifdef SOLARIS
#include <sys/select.h>		/* Include it before override it */
#endif
#ifndef FD_SET
#define	NFDBITS		32
#define	FD_SETSIZE	32
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#endif
 
/*
 * howmany: app/ls.c
 */
#ifndef	howmany
#define	howmany(x, y)   ((((u_int)(x))+(((u_int)(y))-1))/((u_int)(y)))
#endif

/*
 * MAXHOSTNAMELEN: user/vget/ftp.c
 */
#ifdef SOLARIS
#include <netdb.h>
#endif

#ifdef SUNOS
#include <sys/param.h>
#endif

#ifdef AIX
#include <sys/param.h>
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

/*
 * O_ACCMODE: lib/pcompat/open.c
 */
#ifndef AIX             /* lucb */
#include <sys/fcntl.h>  /* Make sure include before overridden */
#else
#include <fcntl.h>      /* Make sure include before overridden */
#endif
#ifndef O_ACCMODE
#define O_ACCMODE         (O_RDONLY|O_WRONLY|O_RDWR)
#endif

/*
 * Definitions from stat.h: app/ls.c lib/pfs/mkdirs.c
 */
#include <sys/stat.h>	/* Make sure included before overridden */
#ifndef S_IFMT
#define S_IFMT	 070000
#endif
#ifndef S_IFDIR
#define S_IFDIR	 040000
#endif
#ifndef S_IFCHR
#define S_IFCHR 020000
#endif
#ifndef S_IFBLK
#define S_IFBLK 060000
#endif
#ifndef S_IXUSR
#define S_IXUSR 0100
#endif
#ifndef S_IXGRP
#define S_IXGRP 0010
#endif
#ifndef S_IXOTH
#define S_IXOTH 0001
#endif
#ifndef S_ISDIR
#define	S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISCHR
#define S_ISCHR(m)  ((S_IFLNK & m) == S_IFCHR)
#endif  
#ifndef S_ISBLK
#define S_ISBLK(m)  ((S_IFLNK & m) == S_IFBLK)
#endif

/* The ULTRIX include files don't bother to prototype the non-ansi string
   manipulation functions.
*/
#ifdef ULTRIX
char *index(), *rindex();
#endif

#ifndef SCOUNIX
#define HAVE_WRITEV
#endif

#if defined(SCOUNIX) || defined(SOLARIS)
#define POSIX_SIGNALS
#endif

/* Define this if your system doesn't support the readv() and writev()
   scatter/gather I/O primitives.  If they're present, a couple of minor
   optimizations happen.
*/
#ifdef SCOUNIX
#define NO_IOVEC
#endif

/* SOLARIS doesn't have sys_nerr or sys_errlist.  It provides the strerror()
   interface instead.  All references to strerror() or to sys_nerr and
   sys_errlist in Prospero now go through the unixerrstr() function in
   lib/ardp/unixerrstr.c --swa
*/
#if defined(SOLARIS)
#define HAVESTRERROR
#else
#undef HAVESTRERROR
#endif

#ifdef  INCLUDE_FILES_LACK_FDOPEN_PROTOTYPE 
/* This seems to be the case only on SOLARIS, at least through 2.3*/
/*Should be defined in stdio.h */
extern FILE *fdopen(const int fd, const char *opts);
#endif

#ifdef INCLUDE_FILES_LACK_TEMPNAM_PROTOTYPE
/* Supposed to be defined in stdio.h, not there in Solaris2.3 */
extern char	*tempnam(const char *, const char *);
#endif

#ifdef  INCLUDE_FILES_LACK_POPEN_PROTOTYPE 
/* This seems to be the case only on SOLARIS, at least through 2.3. */
/*Should be defined in stdio.h */
extern FILE *popen(const char *, const char *);
#endif

#if defined(SCOUNIX) 
#define TCPTIMEOUTS
#else
/* Definitely not working on SOLARIS yet */
#undef TCPTIMEOUTS
#endif

#if defined(SOLARIS)
/* Currently used only in lib/psrv/ppasswd.c.
   Solaris prototypes the library function crypt() in the crypt.h
   include file. */
#define CRYPT_FUNCTION_PROTOTYPE_IN_CRYPT_H
#endif
