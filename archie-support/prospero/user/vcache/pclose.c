/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)pclose.c	1.2 (Berkeley) 3/7/86";
#endif not lint

#include <stdio.h>
#include <posix_signal.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <stdlib.h>           /* For malloc and free */

#include <pmachine.h>
#include <implicit_fixes.h>
#include <pfs_threads.h>	/* For P_IS_THIS_THREAD_MASTER */

#define	tst(a,b)	(*mode == 'r'? (b) : (a))
#define	RDR	0
#define	WTR	1

static	int *popen_pid;
static	int nfiles;

#ifdef SOLARIS
/* Should be defined in stdio.h */
extern FILE *fdopen(const int fd, const char *opt);
#endif

FILE *
mypopen(cmd,mode)
	char *cmd;
	char *mode;
{
	int p[2];
	int myside, hisside, pid;

	if (nfiles <= 0)
 		nfiles = NFILES;

	if (popen_pid == NULL) {
		popen_pid = (int *)malloc((unsigned) nfiles * sizeof *popen_pid);
		if (popen_pid == NULL)
			return (NULL);
		for (pid = 0; pid < nfiles; pid++)
			popen_pid[pid] = -1;
	}
	if (pipe(p) < 0)
		return (NULL);
	myside = tst(p[WTR], p[RDR]);
	hisside = tst(p[RDR], p[WTR]);
#if defined(AIX)
	if ((pid = fork()) == 0) {
#else
	if ((pid = vfork()) == 0) {
#endif
		/* myside and hisside reverse roles in child */
		(void) close(myside);
		if (hisside != tst(0, 1)) {
		  assert(P_IS_THIS_THREAD_MASTER()); /* SOLARIS: dup2 MT-Unsafe */
			(void) dup2(hisside, tst(0, 1));
			(void) close(hisside);
		}
		execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
		_exit(127);
	}
	if (pid == -1) {
		(void) close(myside);
		(void) close(hisside);
		return (NULL);
	}
	popen_pid[myside] = pid;
	(void) close(hisside);
	return (fdopen(myside, mode));
}

void
pabort()
{
	extern int mflag;

	mflag = 0;
}

int
mypclose(ptr)
	FILE *ptr;
{
	int child, pid;
#ifdef POSIX_SIGNALS
	sigset_t	set,omask_set;
#else
	int	omask;
#endif /*POSIX_SIGNALS*/
	void pabort();
	SIGNAL_RET_TYPE (*istat)();
#ifdef BSD_UNION_WAIT
/* This is guesswork, SCO doesnt support "union wait", so I copied from vget.c*/
	union wait status;
#else
	int status;
#endif
	child = popen_pid[fileno(ptr)];
	popen_pid[fileno(ptr)] = -1;
	(void) fclose(ptr);
	if (child == -1)
		return (-1);
	istat = signal(SIGINT, pabort);

#ifdef POSIX_SIGNALS
	sigemptyset(&set);
	sigaddset(&set,SIGQUIT);
	sigaddset(&set,SIGHUP);
	sigprocmask(SIG_BLOCK, &set, &omask_set);
#else
	omask = sigblock(sigmask(SIGQUIT)|sigmask(SIGHUP));
#endif
	while ((pid = wait(&status)) != child && pid != -1)
		;
#ifdef POSIX_SIGNALS
	sigprocmask(SIG_SETMASK,&omask_set,(sigset_t *)NULL);
#else
	(void) sigsetmask(omask);
#endif /*POSIX_SIGNALS*/
	(void) signal(SIGINT, istat);
	return (pid == -1 ? -1 : 0);
}
