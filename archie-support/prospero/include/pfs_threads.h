/*
 * Copyright (c) 1993-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

/* This is the Prospero interface to threads.  This threads package
   is currently used exclusively by the Prospero server when it is operating in
   multi-threaded mode.  

   At the moment, this stuff is experimental.  It will only be tested on the
   server side, not on the client.
*/

/*
 * The interface to the Prospero server's use of threads is fully defined in
 * this file.  That means that as long as you provide the external interface
 * herein defined, you're OK. */

/* This interface does not currently attempt to handle issues of cancelling
 * threads, although that will be a later extension. 
 */
/* Every thread (on the server) has an active RREQ associated with it.  Every
 * active RREQ has a live thread associated with it.  RREQs get moved off of
 * the queue of items to be processed to the list of items in progress when an
 * available thread is there for them.   When a thread is available,
 * ardp_accept() is called.  
 */

#ifndef PFS_THREADS_H

#define PFS_THREADS_H
#include <stdio.h>              /* needed for FILE */


/* #define P_MAX_NUM_THREADS here if you want any threads, and PFS_THREADS. */
/*#define PFS_THREADS */               /* Part of the external interface.  */

#ifdef PFS_THREADS
#define P_MAX_NUM_THREADS 61    /* This will define threads 0 through 60,
                                   quite a lot. */
/* You can easily set a lower value for the number of threads the server
 * will actually use, by modifying DIRSRV_SUB_THREAD_COUNT in dirsrv.c.
 */


#define PFS_THREADS_HAVE_SAFE_MALLOC /* Version  */
/* Define which PFS_THREADS package you have, options supported currently 
   include PFS_THREADS_SOLARIS or PFS_THREADS_FLORIDA - note NONE 
   of these must be defined if PFS_THREADS is undefined */
/* #define PFS_THREADS_SOLARIS */

#endif /* PFS_THREADS */

#include <pfs_utils.h>		/* assert */

/* To port to a new threads package
   - pick an identifer (e.g. PFS_THREADS_DCE).  
   --  Note PFS_THREADS_POSIX is reservered for post-balloting implementation
   --  Tell prospero-developers@isi.edu which you are using. 
   - Grep through all of the source for instances of PFS_THREADS_
   --  Editing these is documented where they apply
   --  This will include lib/ardp/p_th_self_num server/dirsrv
*/


#ifdef PFS_THREADS_FLORIDA
/* pthread.h is the include file from the Florida State University 
   PART project PTHREADS distribution.

   It prototypes all of the functions and interfaces whose names start with
   pthread_. 
*/
#include <pthread.h>
#endif /* PFS_THREADS_FLORIDA */
#ifdef PFS_THREADS_SOLARIS
/* Make sure we get our signal.h with _POSIX_C_SOURCE before thread.h
   includes sys/signal.h */
#include <posix_signal.h>
/* Thread.h prototypes most functions starting thr_ */
#include <thread.h>
#endif /* PFS_THREADS_SOLARIS */

#ifdef PFS_THREADS
/* Yield a unique integer between 0 and P_MAX_NUM_THREADS - 1 for the current
   thread.  Hopefully this function will go away eventually as the server's
   internal structure changes in a way that a reference to the current thread
   is passed around to all subfunctions.  But maybe not, too. 
   This is implemented using the per-thread key mechanisms. */
extern void p_th_allocate_self_num(void); /* external; must be called by thread
                                        after startup. */
extern void p_th_deallocate_self_num(void); /* external; must be called by
                                               thread before exit. */
#endif /*PFS_THREADS*/

#ifdef PFS_THREADS
extern int p__th_self_num(void); /* not external interface. */
#else
#define p__th_self_num() 0
#endif /* PFS_THREADS */

/* We don't include a trailing ; at the end of our declarations and definitions
   because a ; by itself in a declarations list is treated by C as a statement,
   not as an empty declaration. This means that the AUTOSTAT declaration has to
   be the last one. */ 


/* This declares an automatic variable name VARNAME of type char **.  *VARNAME
   will be a reference to a persistent per-thread static variable of type char
   *, with function scope.  Note that *varname will be initially NULL, but may
   expand with time.  Part of the external interface.*/
#ifdef PFS_THREADS
#define AUTOSTAT_CHARPP(VARNAME) \
    static char *(VARNAME##_target[P_MAX_NUM_THREADS]); /* NULL by default. */\
    char **VARNAME = (VARNAME##_target + p__th_self_num())
#else
#define AUTOSTAT_CHARPP(VARNAME) \
    static char *VARNAME##_target = NULL; \
    char **VARNAME = &VARNAME##_target
#endif /* PFS_THREADS */
    

/* ditto, for int* .  Part of the external interface. */
#ifdef PFS_THREADS
#define AUTOSTAT_INTP(VARNAME) \
    static int (VARNAME##_target[P_MAX_NUM_THREADS]); /* automatically 0 */\
    int *VARNAME = (VARNAME##_target + p__th_self_num())
#else
#define AUTOSTAT_INTP(VARNAME) \
    static int VARNAME##_target = 0; \
    int *VARNAME = &VARNAME##_target
#endif /* PFS_THREADS */

/* ditto, for an arbitrary type* .  Part of the external interface. */
#ifdef PFS_THREADS
#define AUTOSTAT_TYPEP(TYPE,VARNAME) \
    static (TYPE) (VARNAME##_target[P_MAX_NUM_THREADS]); /* automatically 0 */\
    (TYPE) *VARNAME = (VARNAME##_target + p__th_self_num())
#else
#define AUTOSTAT_TYPEP(TYPE,VARNAME) \
    static (TYPE) VARNAME##_target = 0; \
    (TYPE) *VARNAME = &VARNAME##_target
#endif /* PFS_THREADS */

#ifdef PFS_THREADS
#define AUTOSTATIC_VAR_INITIALIZED(TYPE,VARNAME,INITIALIZER) \
    static TYPE (VARNAME##_target[P_MAX_NUM_THREADS]); /* automatically 0 */\
    TYPE *VARNAME = (VARNAME##_target + p__th_self_num())
#else
#define AUTOSTATIC_VAR_INITIALIZED(VARNAME) \
    static TYPE VARNAME##_target = 0; \
    TYPE *VARNAME = &VARNAME##_target
#endif /* PFS_THREADS */


/* Possible alternative interface for a common case.  I'm experimenting with
   it.  Experimental part of external interface. */
#if 1
#define SCRATCHBUF_CHARPP_DEF(VARNAME) AUTOSTAT_CHARPP(VARNAME)
/* Set using *VARNAME = or passing VARNAME as a pointer to the target. */
#define SCRATCHBUF_CHARPP_CLEANUP(VARNAME)

#else
/* This version of the interface will always allocate a new buffer upon
   entry.  It has the sole advantage that it does not leave static buffers
   around.  It has disadvantages too: presumably the many calls to malloc()
   will not be an efficient way of using the processor. */
/* A good optimizing compiler should clean up the following (I hope). */
#define SCRATCHBUF_CHARPP_DEF(VARNAME) \
    char * VARNAME##_target = NULL; \
    char **VARNAME = &VARNAME##_target

/* Set using *VARNAME = or passing VARNAME as a pointer to the target. */
#define SCRATCHBUF_CHARPP_CLEANUP(VARNAME) do { \
    stfree(*VARNAME);  \
    *VARNAME = NULL; \
} while(0)

#endif


/* The following abstraction is used for statics (per-file externs) and 
   real externs. */

/* Externs need separate declarations and data definitions. */
/* Each extern will need one of these after the declaration, to keep the old
   stuff working. */
#ifdef PFS_THREADS
#define EXTERN_LONG_DECL(VARNAME) \
    extern long p_th_ar##VARNAME[P_MAX_NUM_THREADS]
#define EXTERN_LONG_DEF(VARNAME) \
    long p_th_ar##VARNAME[P_MAX_NUM_THREADS]
#define PERFILE_STATIC_LONG_DEF(VARNAME) \
    static long p_th_ar##VARNAME[P_MAX_NUM_THREADS]
#define EXTERN_INT_DECL(VARNAME) \
    extern int p_th_ar##VARNAME[P_MAX_NUM_THREADS]
#define EXTERN_INT_DEF(VARNAME) \
    int p_th_ar##VARNAME[P_MAX_NUM_THREADS]
#define PERFILE_STATIC_INT_DEF(VARNAME) \
    static int p_th_ar##VARNAME[P_MAX_NUM_THREADS]
#define EXTERN_CHARP_DECL(VARNAME) \
    extern char * p_th_ar##VARNAME[P_MAX_NUM_THREADS]
#define EXTERN_CHARP_DEF(VARNAME) \
    char * p_th_ar##VARNAME[P_MAX_NUM_THREADS]
#define PERFILE_STATIC_CHARP_DEF(VARNAME) \
    static char * p_th_ar##VARNAME[P_MAX_NUM_THREADS]
#define EXTERN_TYPEP_DECL(TYPE,VARNAME) \
    extern TYPE p_th_ar##VARNAME[P_MAX_NUM_THREADS]
#define EXTERN_TYPEP_DEF(TYPE,VARNAME) \
    TYPE p_th_ar##VARNAME[P_MAX_NUM_THREADS]
#else
#define EXTERN_LONG_DECL(VARNAME) \
    extern long p_th_ar##VARNAME[1]
#define EXTERN_LONG_DEF(VARNAME) \
    long p_th_ar##VARNAME[1]
#define PERFILE_STATIC_LONG_DEF(VARNAME) \
    static long p_th_ar##VARNAME[1]
#define EXTERN_INT_DECL(VARNAME) \
    extern int p_th_ar##VARNAME[1]
#define EXTERN_INT_DEF(VARNAME) \
    int p_th_ar##VARNAME[1]
#define PERFILE_STATIC_INT_DEF(VARNAME) \
    static int p_th_ar##VARNAME[1]
#define EXTERN_CHARP_DECL(VARNAME) \
    extern char * p_th_ar##VARNAME[1]
#define EXTERN_CHARP_DEF(VARNAME) \
    char * p_th_ar##VARNAME[1]
#define PERFILE_STATIC_CHARP_DEF(VARNAME) \
    static char * p_th_ar##VARNAME[1]
#define EXTERN_TYPEP_DECL(VARNAME) \
    extern TYPE p_th_ar##VARNAME[1]
#define EXTERN_TYPEP_DEF(VARNAME) \
    TYPE p_th_ar##VARNAME[1]
#endif /* PFS_THREADS */
/* In any case, follow definitions with a:
   #define VARNAME p_th_arVARNAME[p__th_self_num()] */


#if 0                           /* first attempt  */
/* Externs need separate declarations and data definitions. */
#define EXTERN_INTP_DECL(VARNAMEP) extern int *VARNAMEP
/* Each extern will need one of these after the declaration, to keep the old
   stuff working. */
/* #define VARNAME (*VARNAMEP) */

/* Definition. */
#define EXTERN_INTP_DEF(VARNAMEP) int *VARNAMEP
#endif /* #if 0 */

/* Mutexes,implemented on top of the pthreads package.  */
/* p_th_mutex is an exported data type, used when we mutex functions or
   collections of functions.  See lib/ardp/ptalloc.c for an example of this.
*/
/*
   Do so in init code, to keep
   from depending on ONCE implementation.   Again, this interface is only
   called directly by Prospero and ARDP code when a set of functions need to be
   mutexed; otherwise, the *_MUTEXED_* variable declaration and definition
   interfaces below are used. */

#ifndef NDEBUG
#define zz(a)	assert((a) == 0)
#else
#define zz(a)	a
#endif

#if defined(PFS_THREADS) && !defined(NDEBUG)
#define DIAGMUTEX(MX1,MX2)  if (p_th_mutex_islocked(&p_th_mutex##MX1 )) { printf(mutex_locked_msg,MX2); }
extern char mutex_locked_msg[];
#else
#define DIAGMUTEX(MX1,MX2) do { } while(0)
#endif

#ifdef PFS_THREADS_FLORIDA
typedef pthread_mutex_t p_th_mutex;
#define p_th_mutex_init(MUTEX) \
	pthread_mutex_init(&(MUTEX), (pthread_mutexattr_t *) NULL)
#define p_th_mutex_lock(MUTEX) { pthread_mutex_lock(&(MUTEX)) }
#define p_th_mutex_unlock(MUTEX) pthread_mutex_unlock(&(MUTEX))
#define p_th_mutex_islocked(MUTEX) (pthread_mutex_trylock(&(MUTEX)) == -1)
#define p_th_mutex_trylock(MUTEX) 		\
    ( (p_th_mutex_trylock(&(MUTEX)) != -1)  	\
    ? p_th_mutex_lock(MUTEX)			\
    : TRUE )
#endif /*PFS_THREADS_FLORIDA*/

#ifdef PFS_THREADS_SOLARIS
typedef mutex_t p_th_mutex;
#define p_th_mutex_init(MUTEX) zz(mutex_init(&(MUTEX), USYNC_THREAD, NULL))
#define p_th_mutex_lock(MUTEX) zz(mutex_lock(&(MUTEX)))
#define p_th_mutex_unlock(MUTEX) zz(mutex_unlock(&(MUTEX)))
extern int p_th_mutex_islocked(mutex_t *mp);
#define p_th_mutex_trylock(MUTEX) (mutex_trylock(&(MUTEX)))
#endif /*PFS_THREADS_SOLARIS*/

#ifndef PFS_THREADS
#define p_th_mutex_init(MUTEX) do { } while(0)
#define p_th_mutex_lock(MUTEX) do { } while(0)
#define p_th_mutex_unlock(MUTEX) do { } while(0)
/* Used only in ardp_accept() */
#define p_th_mutex_islocked(MUTEX) 0
#define p_th_mutex_trylock(MUTEX) 0
#endif /*!PFS_THREADS*/


/* Handling of global shared variables (e.g., ardp_runQ) */
/* Mutex initialized in ardp_mutexes.c  (ardp_init_mutexes()). */
/* Variable defined and declared as mutexed. */
#ifdef PFS_THREADS
#define EXTERN_MUTEX_DECL(VARNAME) \
    extern p_th_mutex p_th_mutex##VARNAME
#define EXTERN_MUTEX_DEF(VARNAME) \
    p_th_mutex p_th_mutex##VARNAME

/* Initialize the mutex (in ardp_init_mutexes(), pfs_init_mutexes(),
   dirsrv_init_mutexes()) */
#define EXTERN_MUTEXED_INIT_MUTEX(VARNAME) \
        p_th_mutex_init(p_th_mutex##VARNAME)
#else
/* No-ops if not threaded. */
#define EXTERN_MUTEX_DECL(VARNAME)
#define EXTERN_MUTEX_DEF(VARNAME)
#define EXTERN_MUTEXED_INIT_MUTEX(VARNAME) do {} while(0) /* no-op */
#endif /* PFS_THREADS */

#define EXTERN_ALLOC_DECL(VARNAME) \
    EXTERN_MUTEX_DECL(VARNAME)
#define EXTERN_MUTEXED_DECL(TYPE,VARNAME) \
    EXTERN_MUTEX_DECL(VARNAME); \
    extern TYPE VARNAME
#define EXTERN_MUTEXED_DEF(TYPE,VARNAME) \
    EXTERN_MUTEX_DEF(VARNAME); \
    TYPE VARNAME
#define EXTERN_MUTEXED_DEF_INITIALIZER(TYPE,VARNAME,INITIALIZER) \
    EXTERN_MUTEX_DEF(VARNAME); \
    TYPE VARNAME = (INITIALIZER)

/* Lock and unlock a mutexed external variable.  */
/* In order to avoid potential deadlock situations, all variables must be
   locked in alphabetical order. */
   
#ifdef PFS_THREADS
#define EXTERN_MUTEXED_LOCK(VARNAME)          p_th_mutex_lock(p_th_mutex##VARNAME)
#define EXTERN_MUTEXED_UNLOCK(VARNAME)        p_th_mutex_unlock(p_th_mutex##VARNAME)
#else
#define EXTERN_MUTEXED_LOCK(VARNAME) do {} while (0)
#define EXTERN_MUTEXED_UNLOCK(VARNAME) do {} while (0)
#endif /* PFS_THREADS */

/* This is found in assertions inside functions that expect to be called only
   when they are running as the 'master' thread in Prospero. */
/* The master thread is the one that is started off.  When we're
   single-threaded, that's the master. */
/* This is used entirely in assertions. */
#define P_IS_THIS_THREAD_MASTER() (p__th_self_num() == 0)


#ifdef PFS_THREADS_FLORIDA
/* Note that under the Draft 7 standard, pthread_detach
   takes a pthread_t, whereas under Draft 6, it takes a pointer to
   a pthread_t.  This implementation uses Draft 6. */
#define p_th_t pthread_t
#define p_th_create_detached(NEWTHREAD,FUNC,ARG) ( 		\
	pthread_create(&NEWTHREAD, (pthread_attr_t *) NULL, 	\
		(pthread_funct_t) FUNC, (any_t) ARG)		\
	: pthread_detach(&NEWTHREAD)				\
	) 
#define p_th_self pthread_self
#define p_th_equal(a,b) pthread_equal(a,b)
#endif

#ifdef PFS_THREADS_SOLARIS
#define p_th_t thread_t
/* caller needs to check this for errors */
#define p_th_create_detached(NEWTHREAD,FUNC,ARG) \
	thr_create(NULL,0,(void *)FUNC,ARG,THR_DETACHED, &NEWTHREAD)
#define p_th_self thr_self
/* thread_t is an int so ...*/
#define p_th_equal(a,b)  (a == b)
#endif


/* Mutexed versions of everything that might possibly call malloc() and free().
   We can yank this out once we have thread-safe libraries, but that might be a
   few years off. */

/* Note that we have converted over all of the server code that might be
   called when multi-threaded to not call any of the printf() family of
   functions.  With a thread_safe malloc() now available from FSU, that
   means we need none of these special definitions.  Hooray! */

#if defined(PFS_THREADS) && !defined(PFS_THREADS_SOLARIS) && !defined(PFS_THREADS_FLORIDA)

/* commented out for the implemented thread implementations.*/

#define free(P) p_th_free((P))
extern void p_th_free(void *P);
#define malloc(P) p_th_malloc((P))
extern void *p_th_malloc(unsigned size);
#define calloc(A,B) p_th_calloc((A),(B))
extern void *p_th_calloc(unsigned nelem, unsigned size);
#define _filbuf(p) p_th__filbuf((p))
#define _flsbuf(p,q) p_th__flsbuf((p),(q))
#define fgets(a,b,c) p_th_fgets((a),(b), (c))
extern char *p_th_fgets(char *s, int n, FILE *stream);
#define gets(a) p_th_gets((a))
extern char *p_th_gets(char *s);
#define fputs(a,b) p_th_fputs((a),(b))
extern int p_th_fputs(const char *s, FILE *stream);
#define puts(a) p_th_puts((a))
extern int p_th_puts(const char *s);
#define sprintf p_th_sprintf
extern char *p_th_sprintf();
#define fprintf  p_th_fprintf
#define printf p_th_printf
#define fflush(a) p_th_fflush((a))
extern int p_th_fflush(FILE *);
#define fgetc(a) p_th_fgetc((a))
extern int p_th_fgetc(FILE *);
#define fputc(a,b) p_th_fputc((a),(b))
extern int p_th_fputc();
#define fread(a,b,c,d) p_th_fread((a),(b),(c), (d))
extern int p_th_fread(char *ptr, int size, int nitems, FILE*stream);
#define fwrite(a,b,c,d) p_th_fwrite((a),(b),(c), (d))
extern int p_th_fwrite(const char *ptr, int size, int nitems, FILE *stream);
#define fopen(a,b) p_th_fopen((a),(b))
extern FILE *p_th_fopen(const char *a, const char *b);
#define fclose(a) p_th_fclose((a))
extern int p_th_fclose(FILE *a);

#endif /* !defined(PFS_THREADS_FLORIDA) && !defined(PFS_THREADS_SOLARIS */

/* also in ardp.h */
extern FILE *locked_fopen(const char *a, const char *b);
extern int locked_fclose_A(FILE *a, const char*filename, int readonly);
extern void locked_clear(FILE *a);
int locked_fclose_and_rename(FILE *afile, const char *tmpfilename, const char *filename, int retval);

#if 0
#define locked_fopen(a,b) fopen((a),(b))
#define locked_fclose_A(a,b,c) fclose((a))
#define locked_clear(a) 
#endif /* 0 */

extern void p__th_set_self_master();




#endif /* PFS_THREADS_H */
