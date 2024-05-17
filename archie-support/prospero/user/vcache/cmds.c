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
static char sccsid[] = "@(#)cmds.c	5.8 (Berkeley) 6/29/88";
#endif /* not lint */

/*
 * FTP User Program -- Command Routines.
 */
#include "ftp_var.h"		/* Dangerous ambiguous includes !!*/
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/ftp.h>

#include <posix_signal.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>
#include <netdb.h>
#include <pmachine.h>		/* after netdb.h */
#include <perrno.h>
#include "vcache_macros.h"
#include <vcache.h>
extern	char *home;
extern	short gflag;
extern	char *getenv();
extern	char *index();
extern	char *rindex();
char *mname;
jmp_buf jabort;
/* Forward declerations */
static void disconnect(void);

/*
 * Connect to peer server and
 * auto-login, if possible.
 */
void
setpeer(char *hostn)
{
	char *host, *hookup();

 	host = hookup(hostn, ftpport);

	if (host) {
		connected = 1;
		if (autologin)
			(void) login(hostn);
	}
}

struct	types {
	char	*t_name;
	char	*t_mode;
	int	t_type;
	char	*t_arg;
} types[] = {
	{ "ascii",	"A",	TYPE_A,	0 },
	{ "binary",	"I",	TYPE_I,	0 },
	{ "image",	"I",	TYPE_I,	0 },
	{ "ebcdic",	"E",	TYPE_E,	0 },
	{ "tenex",	"L",	TYPE_L,	bytename },
	{ "ASCII",	"A",	TYPE_A,	0 },
	{ "BINARY",	"I",	TYPE_I,	0 },
	{ "IMAGE",	"I",	TYPE_I,	0 },
	{ "EBCDIC",	"E",	TYPE_E,	0 },
	{ "TENEX",	"L",	TYPE_L,	bytename },
	0
};

/*
 * Set transfer type.
 */
void
set_type(char *t)
{
	register struct types *p;
	int comret;

	for (p = types; p->t_name; p++)
		if (strcmp(t, p->t_name) == 0)
			break;
	if (p->t_name == 0) {
		ERR("%s: unknown mode %s", t);
		code = -1;
		return;
	}
	if ((p->t_arg != NULL) && (*(p->t_arg) != '\0'))
		comret = command ("TYPE %s %s", p->t_mode, p->t_arg, 0);
	else
		comret = command("TYPE %s", p->t_mode, 0);
	if (comret == COMPLETE) {
		(void) strcpy(typename, p->t_name);
		type = p->t_type;
	}
}
char *
onoff(bool)
	int bool;
{

	return (bool ? "on" : "off");
}

void disconnect();

/* Quiting at this level is nasty, it prevents the caching from working */
void
quitnoexit()
{

	if (connected)
		disconnect();
	pswitch(1);
	if (connected) {
		disconnect();
	}
}
#ifdef NEVERDEFINED
/*
 * Terminate session and exit.
 */
quit()
{
	quitnoexit();
	exit(0);
}
#endif
/*
 * Terminate session, but don't exit.
 */
static void
disconnect()
{
	extern FILE *cout;
	extern int data;

	if (!connected)
		return;
	(void) command("QUIT",0);
	if (cout) {
		(void) fclose(cout);
	}
	cout = NULL;
	connected = 0;
	data = -1;
	if (!proxy) {
		macnum = 0;
	}
}

#ifdef NEVERDEFINED
/* Luckily this isnt ever used, it does an exit which defeats caching */
fatal(msg)
	char *msg;
{

	ERR("ftp: %s\n", msg);
	exit(1);
}
#endif /*NEVERDEFINED*/

