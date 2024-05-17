#undef __STDC__
#if (__STDC__ - 0 == 0) || defined(_POSIX_C_SOURCE)
#if ((__STDC__ - 0 == 0) && !defined(_POSIX_C_SOURCE)) || (_POSIX_C_SOURCE > 2)
/*
 * We need <sys/siginfo.h> for the declaration of siginfo_t.
 */
#include <sys/siginfo.h>

#endif

typedef struct {		/* signal set type */
	unsigned long	__sigbits[4];
} sigset_t;

typedef	struct {
	unsigned long	__sigbits[2];
} k_sigset_t;

/* #warning sigaction is now included */
struct sigaction {
	int sa_flags;
	union {
		void (*_handler)();
#if ((__STDC__ - 0 == 0) && !defined(_POSIX_C_SOURCE)) || (_POSIX_C_SOURCE > 2)
		void (*_sigaction)(int, siginfo_t *, void *);
#endif
	}	_funcptr;
	sigset_t sa_mask;
	int sa_resv[2];
};
#define	sa_handler	_funcptr._handler
#define	sa_sigaction	_funcptr._sigaction

/* this is only valid for SIGCLD */
#define	SA_NOCLDSTOP	0x00020000	/* don't send job control SIGCLD's */
#endif

#if (__STDC__ - 0 == 0) && !defined(_POSIX_C_SOURCE)
			/* non-comformant ANSI compilation	*/

/* definitions for the sa_flags field */
#define	SA_ONSTACK	0x00000001
#define	SA_RESETHAND	0x00000002
#define	SA_RESTART	0x00000004
#endif
#if ((__STDC__ - 0 == 0) && !defined(_POSIX_C_SOURCE)) || (_POSIX_C_SOURCE > 2)
#define	SA_SIGINFO	0x00000008
#endif
#if (__STDC__ - 0 == 0) && !defined(_POSIX_C_SOURCE)
#define	SA_NODEFER	0x00000010

/* this is only valid for SIGCLD */
#define	SA_NOCLDWAIT	0x00010000	/* don't save zombie children	 */

/* this is only valid for SIGWAITING */
#define	SA_WAITSIG	0x00010000	/* send SIGWAITING if all lwps block */

/*
 * use of these symbols by applications is injurious
 *	to binary compatibility, use _sys_nsig instead
 */
#define	NSIG	44	/* valid signals range from 1 to NSIG-1 */
#define	MAXSIG	43	/* size of u_signal[], NSIG-1 <= MAXSIG */
			/* Note: when changing MAXSIG, be sure to update the */
			/* sizes of u_sigmask and u_signal in uts/adb/u.adb. */

#define	S_SIGNAL	1
#define	S_SIGSET	2
#define	S_SIGACTION	3
#define	S_NONE		4

#define	MINSIGSTKSZ	2048
#define	SIGSTKSZ	8192

#define	SS_ONSTACK	0x00000001
#define	SS_DISABLE	0x00000002

struct sigaltstack {
	char	*ss_sp;
	int	ss_size;
	int	ss_flags;
};

typedef struct sigaltstack stack_t;

#endif

#define __STDC__ 1
