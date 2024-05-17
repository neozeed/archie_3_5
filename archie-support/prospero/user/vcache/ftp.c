/*
 * Derived from Berkeley source code.  Those parts are
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
static char sccsid[] = "@(#)ftp.c	5.19 (Berkeley) 6/29/88";
#endif /* not lint */

#include "ftp_var.h"		/* Dangerous ambiguous includes !!*/

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/param.h>

#include <netinet/in.h>
#include <arpa/ftp.h>
#include <arpa/telnet.h>

#include <stdio.h>
#include <posix_signal.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <pwd.h>
#include <string.h>

#include <pcompat.h>
#include <pmachine.h>
#include <perrno.h>
#include "vcache_macros.h"
#include <vcache.h>
#include <pfs.h>		/* For quick_connect */
#include <implicit_fixes.h>
#include <sockettime.h>

#ifdef SOLARIS
/* Should be defined in stdio.h */
extern FILE *fdopen(const int fd, const char *opt);
#endif
#ifdef AIX			/* lucb */
#include <sys/select.h>
#endif

#ifndef F_OPEN_TIMEOUT
#define F_OPEN_TIMEOUT 5
#endif

#ifndef F_READ_TIMEOUT
#define F_READ_TIMEOUT 60
#endif

struct	sockaddr_in hisctladdr;
struct	sockaddr_in data_addr;
int	data = -1;
int	abrtflag = 0;
int	ptflag = 0;
int	connected;
struct	sockaddr_in myctladdr;
uid_t	getuid();

FILE	*cin, *cout;
FILE	*dataconn();

char	localerrst[MAXPATHLEN+10];

char	*myhostname();
#include <pfs.h>                /* for quick_connect(). */
/* Re-stated in the following declaration.  Here to explain the problem: this
   is difficult to declar, since SOCKADDR is different from call to call.  So
   we use a cast. */
extern int 
quick_connect(int s, struct sockaddr *name,int namelen, int timeout);

/* Forward declerations */
static void ptransfer(char *direction, long bytes, struct timeval *t0,
		      struct timeval *t1, char *local,char *remote);
static void tvsub(struct timeval *tdiff, struct timeval *t1, 
		  struct timeval *t0);
static void proxtrans(	char *cmd, char *local, char  *remote);
static int getreply(int expecteof);
static int initconn();

/* Returns hostname, or if fails then returns 0 and closes socket */
char *
hookup(char *host, int port)
{
	int s,len;
#ifndef SELECT_APPROACH
	register struct hostent *hp = 0;
	static char hostnamebuf[80];

	bzero((char *)&hisctladdr, sizeof (hisctladdr));
	hisctladdr.sin_addr.s_addr = inet_addr(host);
	if (hisctladdr.sin_addr.s_addr != -1) {
		hisctladdr.sin_family = AF_INET;
		(void) strncpy(hostnamebuf, host, sizeof(hostnamebuf)-1);
	}
	else {
	        assert(P_IS_THIS_THREAD_MASTER());
		hp = gethostbyname(host);
		if (hp == NULL) {
			ERRSYS("Unknown host %s:%s %s", host);
			code = -1;
			return((char *) 0);
		}
		hisctladdr.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr_list[0],
		    (caddr_t)&hisctladdr.sin_addr, hp->h_length);
		(void) strncpy(hostnamebuf, hp->h_name, sizeof(hostnamebuf)-1);
	}
	hostname = hostnamebuf;
	s = socket(hisctladdr.sin_family, SOCK_STREAM, 0);
	if (s < 0) {
		ERRSYS("ftp: socket:%s %s");
		code = -1;
		return (0);
	}
	/* Note gopherget does ... = htons(port) */
	hisctladdr.sin_port = port;
	while (quick_connect(s, (struct sockaddr *) &hisctladdr, 
                             sizeof (hisctladdr), F_OPEN_TIMEOUT) < 0) {
		if (hp && hp->h_addr_list[1]) {
			int oerrno = errno;

			TRACE(5,"ftp: connect to address %s: ",
				inet_ntoa(hisctladdr.sin_addr));
			errno = oerrno;
			hp->h_addr_list++;
			bcopy(hp->h_addr_list[0],
			     (caddr_t)&hisctladdr.sin_addr, hp->h_length);
			TRACE(5, "Trying %s...\n",
				inet_ntoa(hisctladdr.sin_addr));
			(void) close(s);
			s = socket(hisctladdr.sin_family, SOCK_STREAM, 0);
			if (s < 0) {
				ERRSYS("ftp: socket:%s %s");
				code = -1;
				return (0);
			}
			continue;
		}
		ERRSYS("ftp: connect:%s %s");
		code = -1;
		goto bad;
	}
#else
	if ((s = quick_open_tcp_stream(host,port,F_OPEN_TIMEOUT)) < 0) {
	  ERRSYS("ftp: socket:%s %s");
	  code = -1;
	  return 0;
	}
#endif /*SELECT_APPROACH*/
	len = sizeof (myctladdr);
	if (getsockname(s, (char *)&myctladdr, &len) < 0) {
		ERRSYS("ftp: getsockname:%s %s");
		code = -1;
		goto bad;
	}
	cin = fdopen(s, "r");
	cout = fdopen(s, "w");
	if (cin == NULL || cout == NULL) {
		ERRSYS("ftp: fdopen failed.%s: %s");
		if (cin)
			(void) fclose(cin);
		if (cout)
			(void) fclose(cout);
		code = -1;
		goto bad;
	}
	TRACE(5,"Connected to %s.\n", hostname);
	if (getreply(0) > 2) { 	/* read startup message from server */
		if (cin)
			(void) fclose(cin);
		if (cout)
			(void) fclose(cout);
		code = -1;
		goto bad;
	}
#ifdef SO_OOBINLINE
	{
	int on = 1;

	if (setsockopt(s, SOL_SOCKET, SO_OOBINLINE, &on, sizeof(on)) < 0 )
		ERRSYS("ftp: setsockopt:%s %s");
		}
#endif SO_OOBINLINE

#ifdef SELECT_APPROACH
  return(host);
#else
	return (hostname);
#endif /*SELECT_APPROACH*/
bad:
	(void) close(s);
	return ((char *)0);
}

int
login(char *host)
{
	char tmp[80];
	char *user, *pass, *l_acct, *getlogin(), *getpass();
	int n, aflag = 0;

	char *myhstnm, username[120], password[120], account[120];
	struct passwd *whoiampw;

	assert(P_IS_THIS_THREAD_MASTER()); /* getpwuid MT-Unsafe */
	DISABLE_PFS(whoiampw = getpwuid(getuid()));

	user = pass = l_acct = 0;
	if (anonlogin) {
		user = "anonymous";

		if((myhstnm = myhostname()) == NULL)
			myhstnm = "";

		pass = password;
		if (whoiampw == NULL) pass = "PROSPERO";
		else sprintf(pass,"PROSPERO(%s)@%s",whoiampw->pw_name,myhstnm);
	}
	while (user == NULL) {
		char *myname;

		DISABLE_PFS(myname = getlogin());

		if (myname == NULL) {
			if (whoiampw != NULL)
				myname = whoiampw->pw_name;
		}
		DISABLE_PFS(code = ruserpass(host, &user, &pass, &l_acct));
		if (user) {
		    strncpy(username, user, sizeof(username)-1);
		    free(user);
		    user = username;
		}
		if (pass) {
		    strncpy(password, pass, sizeof(password)-1);
		    free(pass);
		}
		if (l_acct) {
		    strncpy(account, l_acct, sizeof(account)-1);
		    free(l_acct);
		    l_acct = account;
		}
		if (code < 0) {
		    user = pass = l_acct = NULL;
		}
		if (user) myname = user;	/* Use name found in .netrc */
		if (myname) {
			TRACE(1,"Name (%s:%s): ", host, myname);
		} else
			TRACE(1,"Name (%s): ", host);
		(void) fgets(tmp, sizeof(tmp) - 1, stdin);
		tmp[strlen(tmp) - 1] = '\0';
		if (*tmp == '\0')
			user = myname;
		else
			user = tmp;
	}
	/* Get around bizarre bug in SCO where need putc before first fprintf*/
	n = fputc('U',cout); 
	if (verbose) fputc('U',stderr);
	n = command("SER %s", user);
	if (n == CONTINUE) {
		if (!anonlogin)
		    pass = getpass("Password:");
		n = command("PASS %s", pass);
	}
	if (n == CONTINUE) {
		aflag++;
		l_acct = getpass("Account:");
		n = command("ACCT %s", l_acct);
	}
	if (n != COMPLETE) {
		ERR("Login failed.");
		return (0);
	}
	if (!aflag && l_acct != NULL)
		(void) command("ACCT %s", l_acct);
	if (proxy)
		return(1);
#ifdef UNDEFINED
	/* Do not execute FTP macros */
	for (n = 0; n < macnum; ++n) {
		if (!strcmp("init", macros[n].mac_name)) {
			(void) strcpy(line, "$init");
			makeargv();
			domacro(margc, margv);
			break;
		}
	}
#endif UNDEFINED
	return (1);
}

SIGNAL_RET_TYPE cmdabort()
{
	extern jmp_buf ptabort;

	TRACE(1,"\n");
	abrtflag++;
	if (ptflag)
		longjmp(ptabort,1);
}

int
command(fmt,a1,a2,a3,a4,a5,a6,a7)
    char *fmt;
    int a1,a2,a3,a4,a5,a6,a7;
{
	int r;
	SIGNAL_RET_TYPE (*oldintr)();
	SIGNAL_RET_TYPE (*cmdabpt)() = cmdabort;

	abrtflag = 0;
	if (verbose) {
		fprintf(stderr,"---> ");
		fprintf(stderr,fmt,a1,a2,a3,a4,a5,a6,a7);
		fprintf(stderr,"\n");
	}
	if (cout == NULL) {
		ERRSYS("No control connection for command:%s %s");
		code = -1;
		return (0);
	}
	oldintr = signal(SIGINT,cmdabpt);
	fprintf(cout, fmt, a1,a2,a3,a4,a5,a6,a7);
	fprintf(cout, "\r\n");
	(void) fflush(cout);
	cpend = 1;
	r = getreply(!strcmp(fmt, "QUIT"));
	if (abrtflag && oldintr != SIG_IGN)
		(*oldintr)();
	(void) signal(SIGINT, oldintr);
	return(r);
}

char reply_string[BUFSIZ];		/* last line of previous reply */

#include <ctype.h>

static int
getreply(int expecteof)
{
	register int c, firstchar;
	register int dig;
	register char *cp;
	int originalcode = 0, continuation = 0;
	SIGNAL_RET_TYPE  (*oldintr)();
	SIGNAL_RET_TYPE  (*cmdabpt)() = cmdabort;

	int pflag = 0;
	char *pt = pasv;

	oldintr = signal(SIGINT,cmdabpt);
	for (;;) {
		dig = firstchar = code = 0;
		cp = reply_string;
		while ((c = quick_fgetc(cin,F_READ_TIMEOUT)) != '\n') {
			if (verbose) 
				fputc(c,stderr);
			if (c == IAC) {     /* handle telnet commands */
				switch (c = quick_fgetc(cin,F_READ_TIMEOUT)) {
				case WILL:
				case WONT:
					c = getc(cin);
					fprintf(cout, "%c%c%c",IAC,DONT,c);
					(void) fflush(cout);
					break;
				case DO:
				case DONT:
					c = getc(cin);
					fprintf(cout, "%c%c%c",IAC,WONT,c);
					(void) fflush(cout);
					break;
				default:   /* Including EOF */
					break;
				}
				continue;
			}
			dig++;
			if (c == EOF) {
				if (expecteof) {
					(void) signal(SIGINT,oldintr);
					code = 221;
					return (0);
				}
				lostpeer();
				if (errno == ETIMEDOUT) {
			  /* Unfortunately haveto return error this way*/
				  ERR("421 Service timed out");
				} else {
				ERR("421 Service not available, remote server has closed connection");
				}
				code = 421;
				return(4);
			}
			if (c != '\r' && proxflag) 
					TRACE(1,"%s:%c",hostname,c);
			if (dig < 4 && isdigit(c))
				code = code * 10 + (c - '0');
			if (!pflag && code == 227)
				pflag = 1;
			if (dig > 4 && pflag == 1 && isdigit(c))
				pflag = 2;
			if (pflag == 2) {
				if (c != '\r' && c != ')')
					*pt++ = c;
				else {
					*pt = '\0';
					pflag = 3;
				}
			}
			if (dig == 4 && c == '-') {
				if (continuation)
					code = 0;
				continuation++;
			}
			if (firstchar == 0)
				firstchar = c;
			if (cp < &reply_string[sizeof(reply_string) - 1])
				*cp++ = c;
		} /*while*/
		if (verbose > 0) {
		    if (verbose <= 0) fputs(" (remote)",stderr);
			(void) putc(c,stderr);
			(void) fflush (stderr);
		}
		if (continuation && code != originalcode) {
			if (originalcode == 0)
				originalcode = code;
			continue;
		}
		*cp = '\0';
		if (firstchar != '1')
			cpend = 0;
		(void) signal(SIGINT,oldintr);
		if (code == 421 || originalcode == 421)
			lostpeer();
		if (abrtflag && oldintr != cmdabpt && oldintr != SIG_IGN)
			(*oldintr)();
		return (firstchar - '0');
	} /*for*/
}

int
empty(mask, sec)
	struct fd_set *mask;
	int sec;
{
	struct timeval t;

	t.tv_sec = (long) sec;
	t.tv_usec = 0;
	return(select(32, mask, (struct fd_set *) 0, (struct fd_set *) 0, &t));
}

jmp_buf	recvabort;

void
abortrecv()
{

	mflag = 0;
	abrtflag = 0;
	if (verbose)  {
	TRACE(1,"\nreceive aborted\nwaiting for remote to finish abort\n");
	(void) fflush(stdout);
	}
	longjmp(recvabort, 1);
}

#undef type                     /* needed because the compatability with the
                                   old formula VLINKS #defines 'type'. */
int
recvrequest(char *cmd, char *local, char *remote, char *recv_mode)
{
	FILE *fout, *din = 0, *mypopen();
	int (*closefunc)(), mypclose(), fclose();
	SIGNAL_RET_TYPE (*oldintr)(), (*oldintp)(); 
	void abortrecv();
	int oldverbose, oldtype = 0, tcrflag, nfnd;
	char buf[BUFSIZ], *gunique(), msg;
	long bytes = 0, hashbytes = sizeof (buf);
	struct fd_set mask;
	register int c, d;
	  int retval = 0;
	struct timeval start, stop;

	sprintf(localerrst,"%s (local)",local);

	if (proxy && strcmp(cmd,"RETR") == 0) {
		proxtrans(cmd, local, remote);
		return(0);
	}
	closefunc = NULL;
	oldintr = NULL;
	oldintp = NULL;
	tcrflag = !crflag && !strcmp(cmd, "RETR");
	if (setjmp(recvabort)) {
		while (cpend) {
			(void) getreply(0);
		}
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		if (oldintr)
			(void) signal(SIGINT, oldintr);
		code = -1;
		return(-1);
	}
	oldintr = signal(SIGINT, abortrecv);
	if (strcmp(local, "-") && *local != '|') {
		if (access(local, 2) < 0) {
			char *dir = strrchr(local, '/');

			if (errno != ENOENT && errno != EACCES) {
				ERRSYS("%s%s: %s",localerrst);
				(void) signal(SIGINT, oldintr);
				code = -1;
				return(-1);
			}
			if (dir != NULL)
				*dir = 0;
			d = access(dir ? local : ".", 2);
			if (dir != NULL)
				*dir = '/';
			if (d < 0) {
				ERRSYS("%s%s: %s", localerrst);
				(void) signal(SIGINT, oldintr);
				code = -1;
				return(-1);
			}
			if (!runique && errno == EACCES &&
			    chmod(local,0600) < 0) {
				ERRSYS("%s%s: %s",localerrst);
				(void) signal(SIGINT, oldintr);
				code = -1;
				return(-1);
			}
			if (runique && errno == EACCES &&
			   (local = gunique(local)) == NULL) {
				(void) signal(SIGINT, oldintr);
				code = -1;
				return(-1);
			}
		}
		else if (runique && (local = gunique(local)) == NULL) {
			(void) signal(SIGINT, oldintr);
			code = -1;
			return(-1);
		}
	}
	if (initconn()) {
		(void) signal(SIGINT, oldintr);
		code = -1;
		return(-1);
	}
	if (setjmp(recvabort))
		goto abort;
	if (strcmp(cmd, "RETR") && type != TYPE_A) {
		oldtype = type;
		oldverbose = verbose;
		if (!debug)
			verbose = 0;
		set_type("ascii");
		verbose = oldverbose;
	}
	if (remote) {
		if((retval =  command("%s %s", cmd, remote, 0)) != PRELIM) {
			(void) signal(SIGINT, oldintr);
		        p_err_string = qsprintf_stcopyr(p_err_string, 
						"ftp: %s", &reply_string);
			if (oldtype) {
				if (!debug)
					verbose = 0;
				switch (oldtype) {
					case TYPE_I:
						set_type("binary");
						break;
					case TYPE_E:
						set_type("ebcdic");
						break;
					case TYPE_L:
						set_type("tenex");
						break;
				}
				verbose = oldverbose;
			}
			return(-1);
		}
	} else {
		if (command("%s", cmd, 0) != PRELIM) {
			(void) signal(SIGINT, oldintr);
			if (oldtype) {
				if (!debug)
					verbose = 0;
				switch (oldtype) {
					case TYPE_I:
						set_type("binary");
						break;
					case TYPE_E:
						set_type("ebcdic");
						break;
					case TYPE_L:
						set_type("tenex");
						break;
				}
				verbose = oldverbose;
			}
			return(-1);
		}
	}
	din = dataconn("r");
	if (din == NULL)
		goto abort;
	if (strcmp(local, "-") == 0)
		fout = stdout;
	else if (*local == '|') {
		oldintp = signal(SIGPIPE, SIG_IGN);
		fout = mypopen(local + 1, "w");
		if (fout == NULL) {
			ERRSYS("%s%s: %s", localerrst+1);
			goto abort;
		}
		closefunc = mypclose;
	}
	else {
		fout = fopen(local, recv_mode);
		if (fout == NULL) {
			ERRSYS("%s%s: %s",localerrst);
			goto abort;
		}
		closefunc = fclose;
	}
	(void) gettimeofday(&start, (struct timezone *)0);
	switch (type) {

	case TYPE_I:
	case TYPE_L:
		errno = d = 0;
		while ((c = quick_read(fileno(din), buf, sizeof (buf),F_READ_TIMEOUT)) > 0) {
			if ((d = write(fileno(fout), buf, c)) < 0)
				break;
			bytes += c;
			if (hash) {
				(void) putchar('#');
				(void) fflush(stdout);
			}
		}
		if (hash && bytes > 0) {
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (c < 0)
			ERRSYS("netin:%s %s");
		if (d < 0)
			ERRSYS("%s%s: %s", localerrst);
		break;

	case TYPE_A:
		while ((c = quick_fgetc(din,F_READ_TIMEOUT)) != EOF) {
			while (c == '\r') {
				while (hash && (bytes >= hashbytes)) {
					(void) putchar('#');
					(void) fflush(stdout);
					hashbytes += sizeof (buf);
				}
				bytes++;
				if ((c = quick_fgetc(din, F_READ_TIMEOUT)) 
				    != '\n' || tcrflag) {
					if (ferror (fout))
						break;
					(void) putc ('\r', fout);
				}
				/*if (c == '\0') {
					bytes++;
					continue;
				}*/
			}
			(void) putc (c, fout);
			bytes++;
		}
		if (hash) {
			if (bytes < hashbytes)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (ferror (din))
			ERRSYS("netin:%s %s");
		if (ferror (fout))
			ERRSYS("%s:%s %s",local);
		break;
	}
	if (closefunc != NULL)
		(*closefunc)(fout);
	(void) signal(SIGINT, oldintr);
	if (oldintp)
		(void) signal(SIGPIPE, oldintp);
	(void) gettimeofday(&stop, (struct timezone *)0);
	(void) fclose(din);
	(void) getreply(0);
	if (bytes > 0 && verbose)
		ptransfer("received", bytes, &start, &stop, local, remote);
	if (oldtype) {
		if (!debug)
			verbose = 0;
		switch (oldtype) {
			case TYPE_I:
				set_type("binary");
				break;
			case TYPE_E:
				set_type("ebcdic");
				break;
			case TYPE_L:
				set_type("tenex");
				break;
		}
		verbose = oldverbose;
	}
	return(0);
abort:

/* abort using RFC959 recommended IP,SYNC sequence  */

	(void) gettimeofday(&stop, (struct timezone *)0);
	if (oldintp)
		(void) signal(SIGPIPE, oldintr);
	(void) signal(SIGINT,SIG_IGN);
	if (oldtype) {
		if (!debug)
			verbose = 0;
		switch (oldtype) {
			case TYPE_I:
				set_type("binary");
				break;
			case TYPE_E:
				set_type("ebcdic");
				break;
			case TYPE_L:
				set_type("tenex");
				break;
		}
		verbose = oldverbose;
	}
	if (!cpend) {
		code = -1;
		(void) signal(SIGINT,oldintr);
		return(-1);
	}

	fprintf(cout,"%c%c",IAC,IP);
	(void) fflush(cout); 
	msg = IAC;
/* send IAC in urgent mode instead of DM because UNIX places oob mark */
/* after urgent byte rather than before as now is protocol            */
	if (send(fileno(cout),&msg,1,MSG_OOB) != 1) {
		ERRSYS("abort:%s %s");
	}
	fprintf(cout,"%cABOR\r\n",DM);
	(void) fflush(cout);
	FD_ZERO(&mask);
	FD_SET(fileno(cin), &mask);
	if (din) { 
		FD_SET(fileno(din), &mask);
	}
	if ((nfnd = empty(&mask,10)) <= 0) {
		if (nfnd < 0) {
			ERRSYS("abort:%s %s");
		}
		code = -1;
		lostpeer();
	}
	if (din && FD_ISSET(fileno(din), &mask)) {
		while ((c = quick_read(fileno(din), buf, sizeof (buf),F_READ_TIMEOUT)) > 0)
			;
	}
	if ((c = getreply(0)) == ERROR && code == 552) { /* needed for nic style abort */
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		(void) getreply(0);
	}
	(void) getreply(0);
	code = -1;
	if (data >= 0) {
		(void) close(data);
		data = -1;
	}
	if (closefunc != NULL && fout != NULL)
		(*closefunc)(fout);
	if (din)
		(void) fclose(din);
	if (bytes > 0 && verbose)
		ptransfer("received", bytes, &start, &stop, local, remote);
	(void) signal(SIGINT,oldintr);
	return(0);
}

/*
 * Need to start a listen on the data channel
 * before we send the command, otherwise the
 * server's connect may fail.
 */
int sendport = -1;

/* Returns: 1 failure to socket  !! Can leave socket open on failure */
static int
initconn()
{
	register char *p, *a;
	int result, len, tmpno = 0;
	int on = 1;

noport:
	data_addr = myctladdr;
	if (sendport)
		data_addr.sin_port = 0;	/* let system pick one */ 
	if (data != -1)
		(void) close (data);
	data = socket(AF_INET, SOCK_STREAM, 0);
	if (data < 0) {
		ERRSYS("ftp socket:%s %s");
		if (tmpno)
			sendport = 1;
		return (1);
	}
	if (!sendport)
		if (setsockopt(data, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof (on)) < 0) {
			ERRSYS("ftp: setsockopt (reuse address):%s %s");
			goto bad;
		}
	if (bind(data, (struct sockaddr *)&data_addr, sizeof (data_addr)) < 0) {
		ERRSYS("ftp: bind:%s %s");
		goto bad;
	}
	if (options & SO_DEBUG &&
	    setsockopt(data, SOL_SOCKET, SO_DEBUG, (char *)&on, sizeof (on)) < 0)
		ERRSYS("ftp: setsockopt (ignored):%s %s");
	len = sizeof (data_addr);
	if (getsockname(data, (char *)&data_addr, &len) < 0) {
		ERRSYS("ftp: getsockname:%s %s");
		goto bad;
	}
	if (listen(data, 1) < 0)
		ERRSYS("ftp: listen:%s %s");
	if (sendport) {
		a = (char *)&data_addr.sin_addr;
		p = (char *)&data_addr.sin_port;
#define	UC(b)	(((int)b)&0xff)
		result =
		    command("PORT %d,%d,%d,%d,%d,%d",
		      UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
		      UC(p[0]), UC(p[1]), 0);
		if (result == ERROR && sendport == -1) {
			sendport = 0;
			tmpno = 1;
			goto noport;
		}
		return (result != COMPLETE);
	}
	if (tmpno)
		sendport = 1;
	return (0);
bad:
	(void) close(data), data = -1;
	if (tmpno)
		sendport = 1;
	return (1);
}

FILE *
dataconn(char *dataconn_mode)
{
	struct sockaddr_in from;
	int s, fromlen = sizeof (from);

	s = accept(data, (struct sockaddr *) &from, &fromlen);
	if (s < 0) {
		ERRSYS("ftp: accept:%s %s");
		(void) close(data), data = -1;
		return (NULL);
	}
	(void) close(data);
	data = s;
	return (fdopen(data, dataconn_mode));
}

void
ptransfer(direction, bytes, t0, t1, local, remote)
	char *direction, *local, *remote;
	long bytes;
	struct timeval *t0, *t1;
{
	struct timeval td;
	float s, bs;

	tvsub(&td, t1, t0);
	s = td.tv_sec + (td.tv_usec / 1000000.);
#define	nz(x)	((x) == 0 ? 1 : (x))
	bs = bytes / nz(s);
	if (verbose) {
	if (local && *local != '-')
		TRACE(5,"local: %s ", local);
	if (remote)
		TRACE(5,"remote: %s\n", remote);
	TRACE(5,"%ld bytes %s in %.2g seconds (%.2g Kbytes/s)\n",
		bytes, direction, s, bs / 1024.);
	}
}

void
tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

void
psabort()
{
	extern int abrtflag;

	abrtflag++;
}

void
pswitch(int flag)
{
	extern int proxy, abrtflag;
	SIGNAL_RET_TYPE (*oldintr)();
	static struct comvars {
		int connect;
		char name[MAXHOSTNAMELEN];
		struct sockaddr_in mctl;
		struct sockaddr_in hctl;
		FILE *in;
		FILE *out;
		int tpe;
		int cpnd;
		int sunqe;
		int runqe;
		int mcse;
		int ntflg;
		char nti[17];
		char nto[17];
		int mapflg;
		char mi[MAXPATHLEN];
		char mo[MAXPATHLEN];
		} proxstruct, tmpstruct;
	struct comvars *ip, *op;

	abrtflag = 0;
	oldintr = signal(SIGINT, psabort);
	if (flag) {
		if (proxy)
			return;
		ip = &tmpstruct;
		op = &proxstruct;
		proxy++;
	}
	else {
		if (!proxy)
			return;
		ip = &proxstruct;
		op = &tmpstruct;
		proxy = 0;
	}
	ip->connect = connected;
	connected = op->connect;
	if (hostname) {
		(void) strncpy(ip->name, hostname, sizeof(ip->name) - 1);
		ip->name[strlen(ip->name)] = '\0';
	} else
		ip->name[0] = 0;
	hostname = op->name;
	ip->hctl = hisctladdr;
	hisctladdr = op->hctl;
	ip->mctl = myctladdr;
	myctladdr = op->mctl;
	ip->in = cin;
	cin = op->in;
	ip->out = cout;
	cout = op->out;
	ip->tpe = type;
	type = op->tpe;
	if (!type)
		type = 1;
	ip->cpnd = cpend;
	cpend = op->cpnd;
	ip->sunqe = sunique;
	sunique = op->sunqe;
	ip->runqe = runique;
	runique = op->runqe;
	ip->mcse = mcase;
	mcase = op->mcse;
	ip->ntflg = ntflag;
	ntflag = op->ntflg;
	(void) strncpy(ip->nti, ntin, 16);
	(ip->nti)[strlen(ip->nti)] = '\0';
	(void) strcpy(ntin, op->nti);
	(void) strncpy(ip->nto, ntout, 16);
	(ip->nto)[strlen(ip->nto)] = '\0';
	(void) strcpy(ntout, op->nto);
	ip->mapflg = mapflag;
	mapflag = op->mapflg;
	(void) strncpy(ip->mi, mapin, MAXPATHLEN - 1);
	(ip->mi)[strlen(ip->mi)] = '\0';
	(void) strcpy(mapin, op->mi);
	(void) strncpy(ip->mo, mapout, MAXPATHLEN - 1);
	(ip->mo)[strlen(ip->mo)] = '\0';
	(void) strcpy(mapout, op->mo);
	(void) signal(SIGINT, oldintr);
	if (abrtflag) {
		abrtflag = 0;
		(*oldintr)();
	}
}

jmp_buf ptabort;
int ptabflg;

void
abortpt()
{
	TRACE(1,"\n");
	ptabflg++;
	mflag = 0;
	abrtflag = 0;
	longjmp(ptabort, 1);
}

void
proxtrans(cmd, local, remote)
	char *cmd, *local, *remote;
{
	void abortpt();
	int tmptype, oldtype = 0, secndflag = 0, nfnd;
	SIGNAL_RET_TYPE (*oldintr)();
	extern jmp_buf ptabort;
	char *cmd2;
	struct fd_set mask;

	if (strcmp(cmd, "RETR"))
		cmd2 = "RETR";
	else
		cmd2 = runique ? "STOU" : "STOR";
	if (command("PASV",0) != COMPLETE) {
		ERR("proxy server does not support third part transfers.");
		return;
	}
	tmptype = type;
	pswitch(0);
	if (!connected) {
		ERR("No primary connection");
		pswitch(1);
		code = -1;
		return;
	}
	if (type != tmptype) {
		oldtype = type;
		switch (tmptype) {
			case TYPE_A:
				set_type("ascii");
				break;
			case TYPE_I:
				set_type("binary");
				break;
			case TYPE_E:
				set_type("ebcdic");
				break;
			case TYPE_L:
				set_type("tenex");
				break;
		}
	}
	if (command("PORT %s", pasv, 0) != COMPLETE) {
		switch (oldtype) {
			case 0:
				break;
			case TYPE_A:
				set_type("ascii");
				break;
			case TYPE_I:
				set_type("binary");
				break;
			case TYPE_E:
				set_type("ebcdic");
				break;
			case TYPE_L:
				set_type("tenex");
				break;
		}
		pswitch(1);
		return;
	}
	if (setjmp(ptabort))
		goto abort;
	oldintr = signal(SIGINT, abortpt);
	if (command("%s %s", cmd, remote, 0) != PRELIM) {
		(void) signal(SIGINT, oldintr);
		switch (oldtype) {
			case 0:
				break;
			case TYPE_A:
				set_type("ascii");
				break;
			case TYPE_I:
				set_type("binary");
				break;
			case TYPE_E:
				set_type("ebcdic");
				break;
			case TYPE_L:
				set_type("tenex");
				break;
		}
		pswitch(1);
		return;
	}
	sleep(2);
	pswitch(1);
	secndflag++;
	if (command("%s %s", cmd2, local, 0) != PRELIM)
		goto abort;
	ptflag++;
	(void) getreply(0);
	pswitch(0);
	(void) getreply(0);
	(void) signal(SIGINT, oldintr);
	switch (oldtype) {
		case 0:
			break;
		case TYPE_A:
			set_type("ascii");
			break;
		case TYPE_I:
			set_type("binary");
			break;
		case TYPE_E:
			set_type("ebcdic");
			break;
		case TYPE_L:
			set_type("tenex");
			break;
	}
	pswitch(1);
	ptflag = 0;
	TRACE(5,"local: %s remote: %s\n", local, remote);
	return;
abort:
	(void) signal(SIGINT, SIG_IGN);
	ptflag = 0;
	if (strcmp(cmd, "RETR") && !proxy)
		pswitch(1);
	else if (!strcmp(cmd, "RETR") && proxy)
		pswitch(0);
	if (!cpend && !secndflag) {  /* only here if cmd = "STOR" (proxy=1) */
		if (command("%s %s", cmd2, local, 0) != PRELIM) {
			pswitch(0);
			switch (oldtype) {
				case 0:
					break;
				case TYPE_A:
					set_type("ascii");
					break;
				case TYPE_I:
					set_type("binary");
					break;
				case TYPE_E:
					set_type("ebcdic");
					break;
				case TYPE_L:
					set_type("tenex");
					break;
			}
			if (cpend) {
				char msg[2];

				fprintf(cout,"%c%c",IAC,IP);
				(void) fflush(cout); 
				*msg = IAC;
				*(msg+1) = DM;
				if (send(fileno(cout),msg,2,MSG_OOB) != 2)
					ERRSYS("abort:%s %s");
				fprintf(cout,"ABOR\r\n");
				(void) fflush(cout);
				FD_ZERO(&mask);
				FD_SET(fileno(cin), &mask);
				if ((nfnd = empty(&mask,10)) <= 0) {
					if (nfnd < 0) {
						ERRSYS("abort:%s %s");
					}
					if (ptabflg)
						code = -1;
					lostpeer();
				}
				(void) getreply(0);
				(void) getreply(0);
			}
		}
		pswitch(1);
		if (ptabflg)
			code = -1;
		(void) signal(SIGINT, oldintr);
		return;
	}
	if (cpend) {
		char msg[2];

		fprintf(cout,"%c%c",IAC,IP);
		(void) fflush(cout); 
		*msg = IAC;
		*(msg+1) = DM;
		if (send(fileno(cout),msg,2,MSG_OOB) != 2)
			ERRSYS("abort:%s %s");
		fprintf(cout,"ABOR\r\n");
		(void) fflush(cout);
		FD_ZERO(&mask);
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,10)) <= 0) {
			if (nfnd < 0) {
				ERRSYS("abort:%s %s");
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	pswitch(!proxy);
	if (!cpend && !secndflag) {  /* only if cmd = "RETR" (proxy=1) */
		if (command("%s %s", cmd2, local, 0) != PRELIM) {
			pswitch(0);
			switch (oldtype) {
				case 0:
					break;
				case TYPE_A:
					set_type("ascii");
					break;
				case TYPE_I:
					set_type("binary");
					break;
				case TYPE_E:
					set_type("ebcdic");
					break;
				case TYPE_L:
					set_type("tenex");
					break;
			}
			if (cpend) {
				char msg[2];

				fprintf(cout,"%c%c",IAC,IP);
				(void) fflush(cout); 
				*msg = IAC;
				*(msg+1) = DM;
				if (send(fileno(cout),msg,2,MSG_OOB) != 2)
					ERRSYS("abort:%s %s");
				fprintf(cout,"ABOR\r\n");
				(void) fflush(cout);
				FD_ZERO(&mask);
				FD_SET(fileno(cin), &mask);
				if ((nfnd = empty(&mask,10)) <= 0) {
					if (nfnd < 0) {
						ERRSYS("abort:%s %s");
					}
					if (ptabflg)
						code = -1;
					lostpeer();
				}
				(void) getreply(0);
				(void) getreply(0);
			}
			pswitch(1);
			if (ptabflg)
				code = -1;
			(void) signal(SIGINT, oldintr);
			return;
		}
	}
	if (cpend) {
		char msg[2];

		fprintf(cout,"%c%c",IAC,IP);
		(void) fflush(cout); 
		*msg = IAC;
		*(msg+1) = DM;
		if (send(fileno(cout),msg,2,MSG_OOB) != 2)
			ERRSYS("abort:%s %s");
		fprintf(cout,"ABOR\r\n");
		(void) fflush(cout);
		FD_ZERO(&mask);
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,10)) <= 0) {
			if (nfnd < 0) {
				ERRSYS("abort:%s %s");
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	pswitch(!proxy);
	if (cpend) {
		FD_ZERO(&mask);
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,10)) <= 0) {
			if (nfnd < 0) {
				ERRSYS("abort:%s %s");
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	if (proxy)
		pswitch(0);
	switch (oldtype) {
		case 0:
			break;
		case TYPE_A:
			set_type("ascii");
			break;
		case TYPE_I:
			set_type("binary");
			break;
		case TYPE_E:
			set_type("ebcdic");
			break;
		case TYPE_L:
			set_type("tenex");
			break;
	}
	pswitch(1);
	if (ptabflg)
		code = -1;
	(void) signal(SIGINT, oldintr);
}

void
reset()
{
	struct fd_set mask;
	int nfnd = 1;

	FD_ZERO(&mask);
	while (nfnd > 0) {
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,0)) < 0) {
			ERRSYS("reset:%s %s");
			code = -1;
			lostpeer();
		}
		else if (nfnd) {
			(void) getreply(0);
		}
	}
}

char *
gunique(local)
	char *local;
{
	static char new[MAXPATHLEN];
	char *cp = strrchr(local, '/');
	int d, count=0;
	char ext = '1';

	sprintf(localerrst,"%s (local)",local);

	if (cp)
		*cp = '\0';
	d = access(cp ? local : ".", 2);
	if (cp)
		*cp = '/';
	if (d < 0) {
		ERRSYS("%s%s: %s", localerrst);
		return((char *) 0);
	}
	(void) strcpy(new, local);
	cp = new + strlen(new);
	*cp++ = '.';
	while (!d) {
		if (++count == 100) {
			ERR("runique: can't find unique file name.");
			return((char *) 0);
		}
		*cp++ = ext;
		*cp = '\0';
		if (ext == '9')
			ext = '0';
		else
			ext++;
		if ((d = access(new, 0)) < 0)
			break;
		if (ext != '0')
			cp--;
		else if (*(cp - 2) == '.')
			*(cp - 1) = '1';
		else {
			*(cp - 2) = *(cp - 2) + 1;
			cp--;
		}
	}
	return(new);
}
