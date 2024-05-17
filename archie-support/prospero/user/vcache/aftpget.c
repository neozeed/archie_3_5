/*
 * Derived from Berkeley ftp code.  Those parts are
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
/* Made static to avoid conflicting with definition of 'copyright' in other
   programs. */
static char copyright[] =
"@(#) Copyright (c) 1985 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)main.c	5.11 (Berkeley) 6/29/88";
#endif /* not lint */

/*
 * FTP User Program -- Command Interface.
 */
#include "ftp_var.h"			/* Ambiguous includes !!*/
/* SCO needs sys/types.h before sys/socket.h */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <arpa/ftp.h>

#include <posix_signal.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <pwd.h>
#include <string.h>

#include <pfs.h>
#include <perrno.h>
#include <pcompat.h>
#include <pmachine.h>
#include "vcache_macros.h"

uid_t	getuid();
void	intr();
void	lostpeer();
extern	char *home;
char	*getlogin();
char	*readheader();

int
aftpget(host,local,ftsuffix,trans_mode)
    char	*host;
    char	*local;
    char	*ftsuffix;
    char	*trans_mode;
{
	int top;
	int retval;

        if (strequal(trans_mode, "TEXT")) trans_mode = "ASCII";
	/* Remove local if it already exists */
	unlink(local);

      assert(P_IS_THIS_THREAD_MASTER()); /*SOLARIS: getservbyname MT-Unsafe*/
	DISABLE_PFS(sp = getservbyname("ftp", "tcp"));

	if (sp == 0) {
	    	ERRSYS( "ftp: ftp/tcp: unknown service:%s %s");
		return(1);
	}
	ftpport = sp->s_port;

	doglob = 1;
	interactive = 1;
	autologin = 1;

	cpend = 0;           /* no pending replies */
	proxy = 0;	/* proxy not active */
	crflag = 1;    /* strip c.r. on ascii gets */
 
 	if (setjmp(toplevel))
 	    return(0);
 	(void) signal(SIGINT, intr);
 	(void) signal(SIGPIPE, lostpeer);
 
	DISABLE_PFS(setpeer(host)); 

	top = setjmp(toplevel) == 0;
	if (top) {
	    (void) signal(SIGINT, intr);
	    (void) signal(SIGPIPE, lostpeer);
	}
 	
	set_type(trans_mode);
	/* Do not disable here if output file is a Vname */
	DISABLE_PFS(retval = recvrequest("RETR", local, ftsuffix, "w"));
	quitnoexit(); /* This used to do an exit(0) in all cases */
	return(retval);
    } 

void
intr()
{

	longjmp(toplevel, 1);
}

void
lostpeer()
{
	extern FILE *cout;
	extern int data;

	if (connected) {
		if (cout != NULL) {
			(void) shutdown(fileno(cout), 1+1);
			(void) fclose(cout);
			cout = NULL;
		}
		if (data >= 0) {
			(void) shutdown(data, 1+1);
			(void) close(data);
			data = -1;
		}
		connected = 0;
	}
	pswitch(1);
	if (connected) {
		if (cout != NULL) {
			(void) shutdown(fileno(cout), 1+1);
			(void) fclose(cout);
			cout = NULL;
		}
		connected = 0;
	}
	proxflag = 0;
	pswitch(0);
}

/*char *
tail(filename)
	char *filename;
{
	register char *s;
	
	while (*filename) {
		s = rindex(filename, '/');
		if (s == NULL)
			break;
		if (s[1])
			return (s + 1);
		*s = '\0';
	}
	return (filename);
}
*/

