/*
 * Copyright (c) 1991, 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

/* This code is a merger of the PSRVCHAT and PKL clients that appeared in
   Prospero versions through 5.1.  It has additional functionality to set and
   retrieve the MOTD.
   */

#include <usc-license.h>

#include <stdio.h>
#include <sys/param.h>
#include <string.h>             /* for strrchr() */
#include <pmachine.h>		/*SCOUNIX for sys/types.h for socket.h>
#include <sys/socket.h>		/* SCOUNIX for MAXHOSTNAMELEN */

#include <ardp.h>
#include <pfs.h>
#include <perrno.h>
#include <pprot.h>

char *prog;
int		pfs_debug = 0;

static char *username(void);
static void usage(char *progname), incompat(char *progname), 
    confirm(char *, char *);
static int add_stdin_as_token(RREQ req), add_stdin(RREQ req);


void
main(argc,argv)
    int		argc;
    char	*argv[];
{
    RREQ 	req;            /* Request we send. */
    char	dirhst[MAXHOSTNAMELEN]; /* host to send message to. */
    char *progname;   /* name of this program.  Used for error
                                   reporting.  */
    char            *parameter = NULL; /* used in parameter option. */

    /* What command shall we execute? */
    enum {UNDEF = 0, KILL, RESTART, SET, GET, COMMAND} function = UNDEF;
    /* If -command function specified, 0 = no headers; 
       VFPROT_VNO = current version headers; 1 = version 1 header. */
    int             headers = 0; 
    /* -force flag; Only compatible with -kill and -restart options. */
    int		forceflag = 0;

    int		tmp;
    int         pkl = 0;        /* set if this program was invoked as 'pkl'*/

    prog = argv[0];

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    if (progname = strrchr(argv[0], '/')) ++progname;
    else progname = argv[0];
    if (strequal(progname, "pkl")) pkl++;
    gethostname(dirhst,sizeof dirhst);

    while (*++argv) {
        if (strnequal(*argv, "-D", 2)) {
            pfs_debug = 1; /* Default debug level */
            if(argv[0][2]) {
                if (qsscanf(argv[0] + 2,"%d",&pfs_debug) != 1) {
                    fprintf(stderr, "Bad argument to -D flag\n");
                    usage(progname);
                }
            } else if(*argv) {
                tmp = qsscanf(*argv,"%d",&pfs_debug);
                if (tmp == 1) ++argv;
            }
        } else if (strnequal(*argv, "-N", 2)) {
            ardp_priority = ARDP_MAX_SPRI; /* Wait till Q empty */
            if((*argv)[2]) {
                if (qsscanf(argv[0] + 2,"%d",&ardp_priority) != 1) {
                    fprintf(stderr, "Bad argument to -N flag\n");
                    usage(progname);
                }
            } else if(*argv) {
                tmp = qsscanf(*argv,"%d",&ardp_priority);
                if (tmp == 1) ++argv;
            }
            if(ardp_priority > ARDP_MAX_SPRI) 
                ardp_priority = ARDP_MAX_PRI;
            if(ardp_priority < ARDP_MIN_PRI) 
                ardp_priority = ARDP_MIN_PRI;
            break;
        } else if (strequal(*argv, "-k") || strequal(*argv, "-kill")) {
            if (function) incompat(progname);
            function = KILL;
        } else if (strequal(*argv, "-f") || strequal(*argv, "-force")) {
            /* Test below whether this is appropriate. */
            forceflag++;
        } else if (strequal(*argv, "-r") || strequal(*argv, "-restart")) {
            if (function) incompat(progname);
            function = RESTART;
        } else if (strequal(*argv, "-m") || strequal(*argv, "-motd")) {
            if (function) incompat(progname);
            function = SET;
            parameter = "MOTD";
        } else if (strequal(*argv, "-s") || strequal(*argv, "-set")) {
            if (function) incompat(progname);
            function = SET;
            /* get parameter from next argument. */
            if (!*++argv) usage(progname); 
            parameter = *argv;
        } else if (strequal(*argv, "-g") || strequal(*argv, "-get")) {
            if (function) incompat(progname);
            function = GET;
            /* get parameter from next argument. */
            if (!*++argv) usage(progname);
            parameter = *argv;
        } else if (strequal(*argv, "-c") || strequal(*argv, "-command")) {
            if (function) incompat(progname);
            function = COMMAND;
        } else if (strequal(*argv, "-1")) {
            headers = 1;
        } else if (strequal(*argv, "-headers") || strequal(*argv, "-h")) {
            headers = VFPROT_VNO;
        } else {
            break;
        }
    }     
    if(argv[0] && argv[1]) {
        usage(progname);
    } else if (argv[0]) {
        qsprintf(dirhst, sizeof dirhst, "%s", argv[0]);
    }
    /* Done scanning command options. */
    /* Check whether they're compatible & confirm */
    if (!function) {
        if (pkl) function = KILL;
        else incompat(progname);
    }
    if (headers && function != COMMAND) {
        fprintf(stderr, "%s: The -headers and -1 options may only be \
specified with the -command option.", progname);
        usage(progname);
    }
    if (!forceflag && function == RESTART) confirm("restart", dirhst);
    else if (!forceflag && function == KILL) confirm("kill", dirhst);

    if (function == COMMAND && headers == 0) 
	req = ardp_rqalloc();
    else if (function == COMMAND && headers == 1) {
	req = ardp_rqalloc();
        p__add_req(req, "VERSION 1\nAUTHENTICATOR UNAUTHENTICATED %s\n", 
                   username());
    } else {
        req = p__start_req(dirhst);
    }
    tmp = 0;
    switch(function) {
    case KILL:
        p__add_req(req, "PARAMETER SET TERMINATE NOW\n");
        break;
    case RESTART:
        p__add_req(req, "PARAMETER SET RESTART NOW\n");
        break;
    case SET:
        p__add_req(req, "PARAMETER SET %'s", parameter);
        tmp = add_stdin_as_token(req);
        p__add_req(req, "\n");
        break;
    case GET:
        p__add_req(req, "PARAMETER GET %'s\n", parameter);
        break;
    case COMMAND:
        tmp = add_stdin(req);
        break;
    default:
        internal_error("unexpected value for variable function");
    }
    if (tmp) {
        fprintf(stderr, "%s: Error reading from standard input.",progname);
        exit(1);
    }
    if(!forceflag) printf("Sending message to %s.\n", dirhst);
    tmp = ardp_send(req,dirhst,0,ARDP_WAIT_TILL_TO);
    if(tmp && !forceflag) {
        fprintf(stderr,"%s",progname);
        perrmesg(" failed: ", 0, NULL);
        exit(1);
    }
    if (!forceflag && pfs_debug < 9) {
	fputs("Response:\n", stdout);
	for(; req->inpkt; req->inpkt = req->inpkt->next) {
	    fputs(req->inpkt->text, stdout);
	}
    }
    ardp_rqfree(req);
    exit(0);
}


#include <pwd.h>
#include <pcompat.h>            /* for DISABLE_PFS */

/* This routine gets the current user name. */
/* It is used for backwards-compatability to generate version 1 headers. */
static char *
username(void)
{
    struct passwd *whoiampw;
    int uid;
    /* find out who we are */
    assert(P_IS_THIS_THREAD_MASTER());
    DISABLE_PFS(whoiampw = getpwuid(uid = getuid()));
    if (whoiampw == 0) {
        static char tmp_uid_str[100];
        qsprintf(tmp_uid_str, sizeof tmp_uid_str, "uid#%d", uid);
        return tmp_uid_str;
    } else 
        return whoiampw->pw_name; /* static data; overwritten on next call to
                                      getpwuid(). */
}

static
void
incompat(char *progname)
{
    fprintf(stderr, "%s: Incompatible options or no useful options specified; \
must specify exactly one of -kill, -restart, -motd, -set, -get, or \
-command.\n", progname);
    usage(progname);
}

static
void
usage(char *progname)
{
    fprintf(stderr, 
            "Usage: %s [-D#] [-N<priority>] \
{ {-kill [-force]} | {-restart [-force]} \n\
\t| -motd | {-set <parameter>} | {-get <parameter>}\n\
\t| {-command [-headers | -1 ]} } [<server>]\n", progname);
   exit(1);
}


/* Abort if function not confirmed. */
static
void
confirm(char cmdname[], char dirhst[])
{
    int c;                      /* character */

    fprintf(stderr, "About to %s Prospero server on %s\n\
[Confirm with RETURN or NEWLINE; anything else to abort]", 
    cmdname, dirhst);
    c = getchar();
    if (c == '\n') return;
    for (;;) {
        if (c == EOF || c == '\n') break;
        c = getchar();
    }
    fprintf(stderr,"Aborted\n");
    exit(1);
}	



/* Adds the standard input to a Prospero request, as a single Prospero token.
    Return PSUCCESS normally; PFAILURE if it couldn't read from the standard
    input. */ 
static
int
add_stdin_as_token(RREQ req)
{
    int c;                      /* character */
    if (isatty(0))              /* warn the user. */
        puts("Reading from standard input:");
    p__add_req(req, " '");      
    while((c = getchar()) != EOF) {
        if (c == '\'') p__add_req(req, "''");
        else p__add_req(req, "%c", c);
    }
    p__add_req(req, "'");
    return ferror(stdin) ? PFAILURE : PSUCCESS;
}

/* Add the unedited contents of stdin to the current Prospero request
   packet(s).  Return PSUCCESS normally; PFAILURE if it couldn't read from 
   the standard input. */ 
static
int
add_stdin(RREQ req)
{
    char		line[MAX_DIR_LINESIZE];
    if (isatty(0))              /* warn the user. */
        puts("Reading from standard input:");
    while(gets(line))
        p__add_req(req, "%s\n", line);
    return ferror(stdin) ? PFAILURE : PSUCCESS;
}
