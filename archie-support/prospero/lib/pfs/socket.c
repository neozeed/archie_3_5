/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

/* These routines are pulled from lib/psrv/gopher_gw/goph_gw_dsdb.c
   and user/vcache/gopherget.c since they are common to both*/

#include <usc-license.h>

#include <netdb.h>		/* Must be before pmachine.h */
#include <pfs.h>
#include <pmachine.h>
#include <errno.h>
#include <perrno.h>
#include <posix_signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pfs_threads.h>
#include <sockettime.h>

#ifdef TIMEOUT_APPROACH
EXTERN_TYPEP_DEF(jmp_buf, Jmpenv);
#define Jmpenv p_th_arJmpenv[p__th_self_num()]

static void 
reprimeAlarm(int oldalarm, SIGNAL_RET_TYPE(*oldintr)())
{
	if (signal(SIGALRM, oldintr) == SIG_ERR)
		perror("signal died:"), exit(-1);
	alarm(oldalarm);
}
/* These are probably not used, can remove if usefull for threading */
EXTERN_INT_DEF(syscall_oldalarmtime);		/* Save old alarm time */
#define syscall_oldalarmtime p_th_arsyscall_oldalarmtime[p__th_self_num()]
/* SIGNAL_RET_TYPE (*syscall_oldintr)();*/ /* Old signal */
EXTERN_TYPEP_DEF(void *,syscall_oldintr); /* Old signal */
#define syscall_oldintr p_th_arsyscall_oldintr[p__th_self_num()]

static SIGNAL_RET_TYPE
alarmJmp(int sig)
{
	reprimeAlarm(syscall_oldalarmtime,syscall_oldintr);
	longjmp(Jmpenv,1);
}

#endif /*TIMEOUT_APPROACH*/

#ifdef SELECT_APPROACH
int
wait_till_stream_readable(FILE *str, int timeout)
{
  if (str->_cnt > 0) return 1;
  return (wait_till_readable(fileno(str),timeout));
}

#endif /*SELECT_APPROACH*/

/* This is adapted from interruptable_connect in gopher */
int
quick_connect(int s, struct sockaddr *name, int namelen, int timeout)
{
	int retval;

#ifdef TIMEOUT_APPROACH
	syscall_oldalarmtime = alarm(timeout);
        if ((syscall_oldintr = signal(SIGALRM, alarmJmp)) == SIG_ERR)
            perror("signal died:\n"), exit(-1);
     	if (setjmp(Jmpenv)) {
		/* Note alarmJmp will reprime alarm*/
		errno = ETIMEDOUT;
		return(-1);
     	}
#endif
	while ( (((retval = connect(s, name, namelen)) == -1)
		 && (errno == EINTR)));
#ifdef TIMEOUT_APPROACH
	reprimeAlarm(syscall_oldalarmtime,syscall_oldintr);
#endif
	return retval;
}

int
quick_read(int fd, char *nptr, int nbytes, int timeout)
{
	int retval;

#ifdef TIMEOUT_APPROACH
	syscall_oldalarmtime = alarm(timeout);
        if ((syscall_oldintr = signal(SIGALRM, alarmJmp)) == SIG_ERR)
            perror("signal died:\n"), exit(-1);
     	if (setjmp(Jmpenv)) {
		/* Note alarmJmp will reprime alarm*/
		errno = ETIMEDOUT;
		return(-1);
     	}
#endif /*TIMEOUT_APPROACH*/
#ifdef SELECT_APPROACH
	     switch (wait_till_readable(fd,timeout)) {
	     case -1:      p_err_string = qsprintf_stcopyr(p_err_string,
		      "INTERNAL: read select failed: %s", unixerrstr());
	              return NULL;
	     case 0:       p_err_string = qsprintf_stcopyr(p_err_string,
		      "Waited more than %d secs for response", timeout);
	              return NULL;
	     }
	     /* Default is going to be 1 - which is success */

#endif /*SELECT_APPROACH*/
	retval = read(fd, nptr, nbytes); /* socket or -1 */
#ifdef TIMEOUT_APPROACH
	reprimeAlarm(syscall_oldalarmtime,syscall_oldintr);
#endif
	return retval;
}

char *
quick_fgets(char *s, int n, FILE *stream, int timeout)
     /* Do a fgets, with opportunity for timeout handling, depending on system
	same return code as fgets, but can return NULL (and not set error)
	in some circumstances */
{
	char *retval;

#ifdef TIMEOUT_APPROACH
	syscall_oldalarmtime = alarm(timeout);
        if ((syscall_oldintr = signal(SIGALRM, alarmJmp)) == SIG_ERR)
            perror("signal died:\n"), exit(-1);
     	if (setjmp(Jmpenv)) {
		/* Note alarmJmp will reprime alarm*/
		errno = ETIMEDOUT;
		return(NULL);
     	}
#endif
#ifdef SELECT_APPROACH
	switch (wait_till_stream_readable(stream,timeout)) {
	     case -1:      p_err_string = qsprintf_stcopyr(p_err_string,
		      "INTERNAL: read select failed: %s", unixerrstr());
	              return NULL;
	     case 0:       p_err_string = qsprintf_stcopyr(p_err_string,
		      "waited more than %d secs for a response", timeout);
	       errno = ETIMEDOUT;
	              return NULL;
	     }
	     /* Default is going to be 1 - which is success */

#endif /*SELECT_APPROACH*/
	retval = fgets(s, n, stream);
#ifdef SELECT_APPROACH
	/* fgets can retunr an incomplete line, since it is non-blocking */
	if (retval) {
	  int buflen =  strlen(s);
	  if ((s[buflen -1 ] != '\n') && (n-1 > buflen)) {
	    retval = quick_fgets(s+buflen, n-buflen, stream, timeout);
	  }
	}
#endif /*SELECT_APPROACH*/
	    
#ifdef TIMEOUT_APPROACH
	reprimeAlarm(syscall_oldalarmtime,syscall_oldintr);
#endif
	return retval;
}

#if 0

int
quick_fgetc(FILE *stream, int timeout)
{
	int retval;

#ifdef TIMEOUT_APPROACH
	syscall_oldalarmtime = alarm(timeout);
        if ((syscall_oldintr = signal(SIGALRM, alarmJmp)) == SIG_ERR)
            perror("signal died:\n"), exit(-1);
     	if (setjmp(Jmpenv)) {
		/* Note alarmJmp will reprime alarm*/
		errno = ETIMEDOUT;
		return(EOF);
     	}
#endif
#ifdef SELECT_APPROACH
	switch (wait_till_stream_readable(stream,timeout)) {
	     case -1:      p_err_string = qsprintf_stcopyr(p_err_string,
	         "INTERNAL: quick_fgetc: select failed: %s", unixerrstr());
	       return EOF;
	     case 0:       p_err_string = qsprintf_stcopyr(p_err_string,
		      "waited more than %d secs for a response", timeout);
	       errno = ETIMEDOUT;
	              return EOF;
	     }
	     /* Default is going to be 1 - which is success */

#endif /*SELECT_APPROACH*/
	retval = getc(stream);
	    
#ifdef TIMEOUT_APPROACH
	reprimeAlarm(syscall_oldalarmtime,syscall_oldintr);
#endif
	return retval;


}
#endif

int quick_fgetc (FILE *stream, int timeout) 
{
  char     c[2];
  char    *retval;

  if (!(retval = quick_fgets(c, 2, stream, timeout))) {
    /* errno set in quick_fgets */
    return EOF;
  }
  return c[0];
}


    

