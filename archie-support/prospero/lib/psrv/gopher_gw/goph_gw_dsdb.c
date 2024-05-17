/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

/* Definitions of constants */
#define MAX_NUM_GOPHER_LINKS       3000	/* maximum # of directory links to
                                           return before giving up. */

/* Archie server we redirect ftp:hostname@ queries to. */
/* Don't define this for now, since it can lead to odd results. */
/* #define ARCHIE_SERVER       "ARCHIE.SURA.NET" */

#include <sys/types.h>
#include <sys/socket.h>         /* includes SOL_SOCKET */
#include <netinet/in.h>
#include <sys/uio.h>		/* for writev(), struct iovec */
#include <netdb.h>
#include <errno.h>
#include <pmachine.h>

#include <pserver.h>
#include <ardp.h>               /* unixerrstr() prototype. */
#include <pfs.h>		/* assert, internal_error, prototypes */
#include <psrv.h>
#include <perrno.h>
#include <plog.h>
#include "gopher.h"
#include <string.h>
#ifdef PSRV_WAIS_GW
#include <wais-source.h>
#include <psite.h>		/* For WAIS_SOURCE_DIR */
#endif

/* 5 secs seems a bit short for this */
#ifndef G_OPEN_TIMEOUT
#define G_OPEN_TIMEOUT 10
#endif
#ifndef G_READ_TIMEOUT
#define G_READ_TIMEOUT 10
#endif

#ifndef GFTP_READ_TIMEOUT
#define GFTP_READ_TIMEOUT 20
#endif

#ifndef GSEARCH_READ_TIMEOUT
#define GSEARCH_READ_TIMEOUT   30
#endif

extern char hostwport[];
extern int quick_open_tcp_stream(char host[], int port, int timeout);
extern char *quick_fgets(char *s, int n, FILE *stream, int timeout);

static GLINK quick_get_menu(char *host, int port, char *selector, int tme);
static void glinks_to_dirob(GLINK glist, P_OBJECT ob);
static void atput(VLINK vl, char *name,...);
static void ftp_check_am(GLINK gl, VLINK r, char *textorbin);
static void ftp_check_dir(GLINK gl, VLINK r);
static int wais_check_dir(GLINK gl, VLINK r);
static void zap_trailing_spaces(char *host);


/* This function is passed an empty object which has been initialized with
   oballoc().  It returns a possibly populated directory and
   DIRSRV_NOT_FOUND or PSUCCESS.  Note that the directory will need
   to have links within it freed, even if an error code is returned.  */

/* This sets p_warn_string and pwarn if an unrecognized gopher line is
   received.  That is good. */

/* This code needs to return attributes in response to a GET-OBJECT-INFO query.
   */
int
gopher_gw_dsdb(RREQ req,	/* Request pointer (unused)           */
               char *hsoname,	/* Name of the directory                 */
               long version,/* Version #; currently ignored */
               long magic_no,	/* Magic #; currently ignored */
               int flags,	/* Currently only recognize DRO_VERIFY */
               struct dsrobject_list_options *listopts,	/* options (use *remcompp
                                                           and *thiscompp) */
               P_OBJECT ob) 
{	/* Object to be filled in */
    int tmp;
    char *cp;
    AUTOSTAT_CHARPP(hostp);     /* For threads. */
    int port;
    char type;
    char *selectorcp;		/* a character pointer that might point to the
                                   first part of the selector or possibly the
                                   complete selector. */
    char *selector;		/* the final selector */
    GLINK glist;		/* list of gopher links */
    int timeout;		/* Timeout for lines (not connect) */

    ob->version = 0;	/* unversioned */
    ob->inc_native = VDIN_PSEUDO;
    ob->flags = P_OBJECT_DIRECTORY;
    ob->magic_no = 0;
    ob->acl = NULL;
    tmp = qsscanf(hsoname, "GOPHER-GW/%&[^(](%d)/%c%r/%r",
		  hostp, &port, &type, &cp, &selectorcp);
    if (tmp < 4)
	return DIRSRV_NOT_FOUND;

    if (type == '1') {
	if (tmp < 5)
	    return DIRSRV_NOT_FOUND;
	selector = selectorcp;
    } else if (type == '7') {
	AUTOSTAT_CHARPP(sp);	/*  Memory allocated for selector */
	if (tmp < 4)
	    return DIRSRV_NOT_FOUND;
	else if (tmp == 4) {
	    selectorcp = "";
	}	/* if tmp = 5, selectorcp already set. */
	/* Now set the selector. */
	if (!listopts || !listopts->thiscompp || !*listopts->thiscompp ||
		strequal(*listopts->thiscompp, "")
            || strequal(*listopts->thiscompp, "*"))
	    /* last test against * might be inappropriate  */
	    return PSUCCESS;	/* Display no contents for the directory */
	/* Set the selector. */
	selector = *sp = qsprintf_stcopyr(*sp, "%s%s%s", selectorcp,
                           (*selectorcp) ? "\t" : "", *listopts->thiscompp);
	/* We just used up thiscompp, so we'd better reset it. */
	/* Oops - && here was meaningless */
	/* I changed this so that the else is not done if listopts->remcompp 
	   is null, in that case (as in when called by get_object_info) 
	   listopts->thiscompp is also null so "*listopts->thiscompp  = NULL"
	   generates a segment violation, if this is the case then we didnt 
	   use a component anyway, so no need to advance to the next  */
	if (listopts->remcompp) {
            if (*listopts->remcompp) {
                *listopts->thiscompp = (*listopts->remcompp)->token;
                *listopts->remcompp = (*listopts->remcompp)->next;
            } else {
                *listopts->thiscompp = NULL;
            }
        }
	/* Now that the selector is set, fall through. */
    } else {	/* unknown type for gatewaying */
	return DIRSRV_NOT_FOUND;
    }
    /* don't bother retrieving the contents if just verifying.  */
    if (flags & DRO_VERIFY)
	return PSUCCESS;
    if (type == '7')  {
      timeout = GSEARCH_READ_TIMEOUT;
    } else {
      if (strncmp(selector,"ftp:",4)== 0 ) {
	timeout = GFTP_READ_TIMEOUT;
      } else {
	  timeout = G_READ_TIMEOUT;
      }
    }
    if ((glist = quick_get_menu(*hostp, port, selector, timeout))
	== NULL && perrno)
	return perrno;
    glinks_to_dirob(glist, ob);
    gllfree(glist);
    return PSUCCESS;
}


static GLINK quick_next_menu_item(FILE * fp, int timeout);
extern int p_open_tcp_stream(const char host[], int port);

/* Returns NULL & sets perrno on error. */
static GLINK
quick_get_menu(char *host, int port, char *selector, int timeout)
{
    GLINK retval = NULL;
    GLINK gl;
    FILE *fp;			/* used for reading only.  Tried to use it for
                                    reading & writing without success; see
                                    comment below. */
    int fd;
    int tmp;			/* return value from write() and read()*/
    int nlinks = 0;		/* # of links in directory */
#ifndef NO_IOVEC
    struct iovec iov[2];
#endif

    fd = quick_open_tcp_stream(host, port, G_OPEN_TIMEOUT);	
	/* Copied from vcache */
    if (fd < 0) {
	perrno = DIRSRV_GOPHER;	/* work on error reporting */
	return NULL;
    }

#if ! defined(NO_IOVEC) && ! defined(PFS_THREADS)
    /* Current implementation of threads does not include writev(). */
    iov[0].iov_base = selector;
    iov[0].iov_len = strlen(selector);
    iov[1].iov_base = "\r\n";
    iov[1].iov_len = 2;
    tmp = writev(fd, iov, 2);
    if (tmp != iov[0].iov_len + iov[1].iov_len) {
#else
    tmp = write(fd, selector, strlen(selector));
    if (tmp != strlen(selector)) {
	p_err_string = qsprintf_stcopyr(p_err_string,
					"Couldn't send selector to gopher \
connection to %s: Call to write(%d,%s,%d) failed: errno %d: \
%s", host, fd, selector, strlen(selector), errno, unixerrstr());
    redo_close:
	if(close(fd) == -1 && errno == EINTR) goto redo_close;
	perrno = DIRSRV_GOPHER;
	return NULL;
    }
    tmp = write(fd, "\r\n", 2);
    if (tmp != 2) {
	p_err_string = qsprintf_stcopyr(p_err_string,
					"Couldn't send selector to gopher \
connection to %s: Call to write(%d, \"\\r\\n\", 2) failed: errno %d: %s", 
                                        host, fd, errno, unixerrstr());
#endif
    redo_close2:
	if(close(fd) == -1 && errno == EINTR) goto redo_close2;
	perrno = DIRSRV_GOPHER;
	return NULL;
    }
    fp = fdopen(fd, "r");	/* open fp just for reading */
    /* Comment from Alan Emtage of Bunyip Information Systems which explains
       why I'm treating the writing end as a raw file descriptor and the
       reading end through the stdio library:

       From: bajan@bunyip.com (Alan Emtage)
       Date: Sat, 28 Aug 1993 10:01:32 -0400

       Hi Steve,
       You have a comment in the gopher GW code to the effect that "this
       experiment didn't work.... if you know why, tell me". Well I will :-)

       For some reason which I haven't figured out, you can't a _socket_ file
       descriptor with fdopen in any form of update mode (you were trying to do
       it with "r+"). However it can be done if you open the fd _twice_. One
       FILE * for reading one for writing. Then everything works fine. What is
       going on in the underlying layers I don't know, but I'd bet that the
       symptoms you got was that you were reading things that you had
       previously written to the FILE * (and thought had just gone down the
       pipe to the remote connection).

       Hope this helps.

       -Alan
   */
    perrno = PSUCCESS;
    while (gl = quick_next_menu_item(fp,timeout)) {
	if (++nlinks > MAX_NUM_GOPHER_LINKS) {
	    perrno = DIRSRV_TOO_MANY;
	    gllfree(retval);
	    fclose(fp);
	    return NULL;
	}
	APPEND_ITEM(gl, retval);
    }
    fclose(fp);
redo_close3:
    if(close(fd) == -1 && errno == EINTR) goto redo_close3;
    if (perrno != PSUCCESS) {
	    gllfree(retval);
	    return(NULL);	/* Return error code from next_menu_item*/
    }
    return retval;
}

#if 0
/* This client opens a TCP stream to the host/port & returns a FILE pointer. */
/* Returns NULL on failure; sets p_err_string */
FILE *
fopen_stream(char *host, int port)
{
    int fd = quick_open_tcp_stream(host, port, G_OPEN_TIMEOUT);
    if (fd < 0)
	return NULL;
    return fdopen(fd, "r+");
}
#endif


/* Reads the next valid menu item from the stream FP.  Returns NULL if
   no more, or if more than 3 erroneous lines go by. If a line is sent
   longer than 1024 characters, it is definitely illegal according to the
   Gopher spec, which requests no more than 255 chars for the selector and 80
   for the description. */
/* Need to work on the error reporting. */
static GLINK
quick_next_menu_item(FILE * fp, int timeout)
{
    GLINK r;
    char buf[1024];
    int buflen;			/* # of characters in this buffer. */
    int nbad = 0;


  again:
    if (nbad > 3)
	return NULL;

    if (!quick_fgets(buf, sizeof buf, fp, timeout)) {
	if (errno = ETIMEDOUT) {
		p_err_string = qsprintf_stcopyr(p_err_string,
		 "Timed out after %d secs waiting for response", timeout);
		perrno = DIRSRV_GOPHER;
	return NULL;
	}
	assert(FALSE);
    }
    buflen = strlen(buf);
    if (sizeof buf - 1 == buflen) {	/* line too long; missed CRLF */
	++nbad;
	goto again;
    }
    r = glalloc();
    /* The standard specifies a trailing CR, but apparantly 
       e.g. archive.egr.msu.edu(70) this is not guarranteed */
    assert(buf[buflen -1] == '\n');
    if (buf[buflen - 1] == '\n')
      buf[--buflen] = '\0';
    if (buf[buflen - 1] == '\r')
	buf[--buflen] = '\0';
    /* Check for end of menu */
    if (strequal(buf, ".")) {
	glfree(r);
	return NULL;	/* the end has been spotted. */
    }
#if 0
    if (qsscanf(buf, "%c%&[^\t]%*1[\t]%&(^\t)%*1[\t]%&[^\t]%*1[\t]%d",
		&r->type, &r->name, &r->selector, &r->host, &r->port) < 5) {
	/* Note that %&( is not yet implemented, so we're doing it this way
       instead. */
#else
    if ((qsscanf(buf, "%c%&[^\t]%*1[\t]%&[^\t]%*1[\t]%&[^\t]%*1[\t]%d",
		 &r->type, &r->name, &r->selector, &r->host, &r->port) < 5)
	    && ((r->selector = stcopyr("", r->selector)),
		(qsscanf(buf, "%c%&[^\t]%*1[\t]%*1[\t]%&[^\t]%*1[\t]%d",
			 &r->type, &r->name, &r->host, &r->port) < 4))) {
#endif
	++nbad;
	p_warn_string = qsprintf_stcopyr(p_warn_string,
			 "Encountered unrecognized Gopher message: %s", buf);
	pwarn = PWARNING;
	glfree(r);
	goto again;
    }
    nbad = 0;
    /* Some special cases, for error messages that appear to be menus */
    if(strncmp(r->name,"Server error",12) == 0) {
	p_err_string = qsprintf_stcopyr(p_err_string,r->name);
	perrno = DIRSRV_GOPHER;
	return NULL;
    }
    r->protocol_mesg = stcopy(buf);
    return r;
}

static VLINK gltovl(GLINK gl);
static void add_collation_order(int linknum, VLINK vl);

/* It is ok to modify the GLINKs, as long as glist is left at the head of a
   list of stuff to be freed with gllfree() by the caller of this function.

   A lot of the hair in this function is here because we must make sure that
   Gopher replicas are returned as Prospero replicas.  For now, the only way to
   make sure two objects are treated as replicas is for them to be links with
   the same positive ID REMOTE value. */
static void
glinks_to_dirob(GLINK glist, P_OBJECT ob)
{
    int menu_entry_number = 0;	/* # of menu entries read */
    int replica_list = 0;	/* nonzero if handling replica list */
    long current_magic_no = 0L;	/* set magic starting hash */
    VLINK vl = NULL;		/* last vlink processed */
    GLINK gl;			/* index */
    for (gl = glist; gl; gl = gl->next) {
	/* If we found this link is a replica, put a magic # on the head of the
           list. */
	if (!replica_list && gl->type == '+') {
	    if (!vl)
		continue;	/* no previous gopher link for this to be a
                                   replica of; perhaps we could not convert the
                                   gopher link to a virtual link. */
	    if (!current_magic_no)
		current_magic_no = generate_magic(vl);
	    while (magic_no_in_list(++current_magic_no, ob->links)) {
		/* Check for numeric overflow */
		if (current_magic_no <= 0)
		    current_magic_no = 1;
	    }
	    vl->f_magic_no = ++current_magic_no;	/* new magic no */
	}
	if (gl->type != '+')
	    replica_list = 0;
	else {
	    assert(gl->previous);
	    gl->type = gl->previous->type;	/* gets type from head of list. */
	}
	if ((vl = gltovl(gl)) == NULL) {
	    replica_list = 0;	/* break the chain of replicas */
	    continue;	/* can't convert it */
	}
	if (replica_list) {
	    vl->f_magic_no = current_magic_no;
	    add_collation_order(menu_entry_number, vl);
	} else {
	    add_collation_order(++menu_entry_number, vl);
	}
	APPEND_ITEM(vl, ob->links);
    }
}


#ifdef NEVERDEFINED
/* I intend using this to return attributes for GIET-OBJECT-INFO calsl
   but I need docs on what to return first */
/* Converts a hsoname back to a gl */
static GLINK
hsoname2gl(char *hsoname) 
{
	GLINK gl = glalloc();

    	tmp = qsscanf(hsoname, "GOPHER-GW/%&[^(](%d)/%c/%r",
		  &gl->host, &gl->port, &gl->type, &gl->selectorcp);
	switch (&gl->type) {
	'0':	if (tmp < 4) goto hsoname2gl_bad;
		break;	
	'1':	if (tmp < 4) goto hsoname2gl_bad;
		break;
	'7':	if (tmp < 3) goto hsoname2gl_bad;
		break;
	default:
		return NULL;	/* Unsupported type */
	}
	/* name,selector,protocol_mesg arent set by this */
	return gl;

hsoname2gl_bad:
	glfree(gl);
	return gl;
}
#endif

/* Returns NULL if couldn't convert the gopher link GL to a Prospero link VL.
   Otherwise returns a new VLINK. */
static VLINK
gltovl(GLINK gl)
{
    VLINK r = vlalloc();
    PATTRIB at;
    /* check if something is an AFTP link. If so, handle it appropriately.*/
    /* add AFTP Access method */

    r->name = gl->name;
    gl->name = NULL;
    atput(r, "GOPHER-MENU-ITEM", gl->protocol_mesg, (char *) 0);

    switch (gl->type) {
    case '0':	/* Text files */
    case 'c':	/* Calendar Text files (not in RFC1436) */
    case 'e':	/* Event Text files (not in RFC1436) */
	/* This comes first because, in the current client implementation,
           the server is expected to return access methods in what it believes
           the preferred order is.  We prefer direct AFTP connections to
           overloading the intermediary Gopher gateways. */
	r->target = stcopyr("EXTERNAL", r->target);
	ftp_check_am(gl, r, "TEXT");
	atput(r, "ACCESS-METHOD", "GOPHER", "", "", "", "", "TEXT",
	      (char *) 0);
	r->host = qsprintf_stcopyr(r->host, "%s(%d)", gl->host, gl->port);
	r->hsoname = gl->selector;
	gl->selector = NULL;
	atput(r, "OBJECT-INTERPRETATION", "DOCUMENT", "TEXT", "ASCII",
	      (char *) 0);
	break;
    case '1':	/* Directories */
	r->host = stcopy(hostwport);
	r->target = stcopyr("DIRECTORY", r->target);
	r->hsoname = qsprintf_stcopyr(r->hsoname,
				      "GOPHER-GW/%s(%d)/%c/%s", gl->host,
				      gl->port, gl->type, gl->selector);
	atput(r, "OBJECT-INTERPRETATION", "DIRECTORY", (char *) 0);
	ftp_check_dir(gl, r);
	break;
    case '4':	/* Macintosh BinHex (.hqx) text file */
	r->target = stcopyr("EXTERNAL", r->target);
	ftp_check_am(gl, r, "TEXT");
	atput(r, "ACCESS-METHOD", "GOPHER", "", "", "", "", "TEXT",
	      (char *) 0);
	r->host = qsprintf_stcopyr(r->host, "%s(%d)", gl->host, gl->port);
	r->hsoname = gl->selector;
	gl->selector = NULL;
	atput(r, "OBJECT-INTERPRETATION", "EMBEDDED", "BINHEX", "DATA",
	      (char *) 0);
	break;
    case '5':	/* PC-Dos binary files. */
    case '9':	/* Generic binary files */
	r->target = stcopyr("EXTERNAL", r->target);
	ftp_check_am(gl, r, "BINARY");
	atput(r, "ACCESS-METHOD", "GOPHER", "", "", "", "", "BINARY",
	      (char *) 0);
	r->host = qsprintf_stcopyr(r->host, "%s(%d)", gl->host, gl->port);
	r->hsoname = gl->selector;
	gl->selector = NULL;
	atput(r, "OBJECT-INTERPRETATION", "DATA", (char *) 0);
	break;
    case '6':	/* UNIX UUencoded file. */
	r->target = stcopyr("EXTERNAL", r->target);
	ftp_check_am(gl, r, "TEXT");
	atput(r, "ACCESS-METHOD", "GOPHER", "", "", "", "", "TEXT",
	      (char *) 0);
	r->host = qsprintf_stcopyr(r->host, "%s(%d)", gl->host, gl->port);
	r->hsoname = gl->selector;
	gl->selector = NULL;
	atput(r, "OBJECT-INTERPRETATION", "EMBEDDED", "UUENCODE", "DATA",
	      (char *) 0);
	break;
    case '7':	/* Gopher searches */
	r->target = stcopyr("DIRECTORY", r->target);
	r->host = stcopy(hostwport);
	atput(r, "OBJECT-INTERPRETATION", "SEARCH", (char *) 0);
#ifdef PSRV_WAIS_GW
        /* wais_check_dir() rewrites wais searches as appropriate, and 
           reformats the link to be a WAIS one. */
	if (wais_check_dir(gl, r))
            break;              /* done */
#endif
        /* Write the link as a gopher search link. */
	r->hsoname =
	    qsprintf_stcopyr(r->hsoname, "GOPHER-GW/%s(%d)/7/%s",
			     gl->host, gl->port, gl->selector);
	atput(r, "QUERY-METHOD", "gopher-query(search-words)",
	      "${search-words}", "", (char *) 0);
	atput(r, "QUERY-ARGUMENT", "search-words",
	      "Index word(s) to search for", "mandatory char*", "%s", "",
	      (char *) 0);
	atput(r, "QUERY-DOCUMENTATION", "gopher-query()", "This is a Gopher \
protocol query exported through Prospero.  Since it is not a native Prospero \
query, we have very little documentation available for you.", (char *) 0);
	atput(r, "QUERY-DOCUMENTATION", "search-words", "This string says \
what you're searching for or what information you're specifying.",
	      "Type any string.  Sometimes SPACEs are treated as implied \
'and' operators.  Sometimes the words 'and', 'or', and 'not' are recognized \
as boolean operators.", (char *) 0);
	break;
    case '8':{	/* TELNET session */
	    char *m = "";	/* message */
	    r->target = stcopyr("EXTERNAL", r->target);
	    if (*(gl->selector))
		m = qsprintf_stcopyr((char *) NULL,
				     "Use the account name \"%s\" to log in",
				     gl->selector);
	    atput(r, "ACCESS-METHOD", "TELNET", "", "", "", "", m, (char *) 0);
	    zap_trailing_spaces(gl->host);
	    /* Gopher uses a telnet port of 0 to mean the default telnet port. */
	    if (gl->port == 0) {
		r->host = gl->host;
		gl->host = NULL;
	    } else {
		r->host = qsprintf_stcopyr(r->host, "%s(%d)", gl->host, gl->port);
	    }
	    atput(r, "OBJECT-INTERPRETATION", "PORTAL", (char *) 0);
	    if (*m)
		stfree(m);
	    break;
	}
    case 'I':	/* Generic Image. */
    case ':':	/* Gopher+ bitmap image type; not in RFC1436 */
	r->target = stcopyr("EXTERNAL", r->target);
	ftp_check_am(gl, r, "BINARY");
	atput(r, "ACCESS-METHOD", "GOPHER", "", "", "", "", "BINARY",
	      (char *) 0);
	r->host = qsprintf_stcopyr(r->host, "%s(%d)", gl->host, gl->port);
	r->hsoname = gl->selector;
	gl->selector = NULL;
	atput(r, "OBJECT-INTERPRETATION", "IMAGE", (char *) 0);
	break;
    case 'M':	/* MIME.  Not in RFC1436. */
	r->target = stcopyr("EXTERNAL", r->target);
	ftp_check_am(gl, r, "TEXT");
	atput(r, "ACCESS-METHOD", "GOPHER", "", "", "", "", "TEXT", (char *) 0);
	r->host = qsprintf_stcopyr(r->host, "%s(%d)", gl->host, gl->port);
	r->hsoname = gl->selector;
	gl->selector = NULL;
	atput(r, "OBJECT-INTERPRETATION", "DOCUMENT", "MIME", (char *) 0);
	break;

    case 'T':{	/* TN3270 session */
	    char *m = "";	/* message */
	    r->target = stcopyr("EXTERNAL", r->target);
	    zap_trailing_spaces(gl->host);
	    /* Gopher uses a telnet port of 0 to mean the default telnet port. */
	    if (gl->port == 0) {
		r->host = gl->host;
		gl->host = NULL;
	    } else {
		r->host = qsprintf_stcopyr(r->host, "%s(%d)", gl->host, gl->port);
	    }
	    if (*(gl->selector))
		m = qsprintf_stcopyr((char *) NULL,
				     "Use the account name \"%s\" to log in",
				     gl->selector);
	    atput(r, "ACCESS-METHOD", "TN3270", "", "", "", "", m, (char *) 0);
	    atput(r, "OBJECT-INTERPRETATION", "PORTAL", (char *) 0);
	    if (*m)
		stfree(m);
	    break;
	}
    case 'g':	/* Gif */
	r->target = stcopyr("EXTERNAL", r->target);
	ftp_check_am(gl, r, "BINARY");
	atput(r, "ACCESS-METHOD", "GOPHER", "", "", "", "", "BINARY",
	      (char *) 0);
	r->host = qsprintf_stcopyr(r->host, "%s(%d)", gl->host, gl->port);
	r->hsoname = gl->selector;
	gl->selector = NULL;
	atput(r, "OBJECT-INTERPRETATION", "IMAGE", "GIF", (char *) 0);
	break;
    case 's':	/* SOUND.  Not in RFC1436. */
    case '<':	/* Gopher+ sound type; not in RFC1436 */
	r->target = stcopyr("EXTERNAL", r->target);
	ftp_check_am(gl, r, "BINARY");
	atput(r, "ACCESS-METHOD", "GOPHER", "", "", "", "", "BINARY",
	      (char *) 0);
	r->host = qsprintf_stcopyr(r->host, "%s(%d)", gl->host, gl->port);
	r->hsoname = gl->selector;
	gl->selector = NULL;
	atput(r, "OBJECT-INTERPRETATION", "SOUND", (char *) 0);
	break;
    case 'i':	/* the uncommon i type exists in the University
                                   of Iowa gopher variant, Panda.  */
	r->name = gl->name;
	gl->name = NULL;
	r->target = stcopyr("NULL", r->target);
	r->hosttype = stcopyr("NULL", r->hosttype);
	r->host = stcopyr("NULL", r->host);
	r->hsonametype = stcopyr("NULL", r->hsonametype);
	r->hsoname = stcopyr("NULL", r->hsoname);
	r->target = stcopyr("NULL", r->target);
	atput(r, "OBJECT-INTERPRETATION", "VOID", (char *) 0);
	break;
    case ';':	/* Gopher+ MOVIE type.   It is not at all clear
                                   whether this is being used yet.*/
	r->target = stcopyr("EXTERNAL", r->target);
	ftp_check_am(gl, r, "BINARY");
	atput(r, "ACCESS-METHOD", "GOPHER", "", "", "", "", "BINARY",
	      (char *) 0);
	r->host = qsprintf_stcopyr(r->host, "%s(%d)", gl->host, gl->port);
	r->hsoname = gl->selector;
	gl->selector = NULL;
	atput(r, "OBJECT-INTERPRETATION", "VIDEO", (char *) 0);
	break;
    case '.':	/* End of directory. */
	goto ignore;
	break;	/* go on */
    case '2':	/* CSO nameserver; No gateway (yet)  */
	p_warn_string = qsprintf_stcopyr(p_warn_string,
					 "Encountered gopher link to CSO nameserver; we don't support \
this type yet: %s", gl->protocol_mesg);
	pwarn = PWARNING;
	goto ignore;
	break;
    case '3':	/* Mitra thinks the error type should be
                                   ignored.  I am willing to go along with
                                   this, given that the standard UNIX Gopher
                                   client ignores it. */
	goto ignore;
	break;
    case 'w':	/* whois server */
	p_warn_string = qsprintf_stcopyr(p_warn_string,
					 "Encountered gopher link to WHOIS server; we don't support \
this type yet: %s", gl->protocol_mesg);
    case '-':	/* error message type? */
    default:	/* unknown type or type we can't process */
	p_warn_string = qsprintf_stcopyr(p_warn_string,
			       "Encountered unrecognized Gopher message: %s",
					 gl->protocol_mesg);
	pwarn = PWARNING;
      ignore:
	vlfree(r);
	r = NULL;
	break;
    }
    return r;
}


/* Check if we're an AFTP link. If so, add an AFTP access method. */
/* This check should be made before other access methods are added.
   The server is expected to return access methods in what it believes the
   preferred order is.  We prefer direct AFTP connections to overloading the
   intermediary Gopher gateways. */
/* Gopher AFTP links to files use the HSONAME format:
   ftp:<aftp-hostname>@/file-path (no trailing slash) */
static void
ftp_check_am(GLINK gl, VLINK r, char *textorbin)
{
    static char *ftp_hostname = NULL;
    char *ftp_rest;		/* rest of string */
    if (qsscanf(gl->selector, "ftp:%&[^@]@%r", &ftp_hostname, &ftp_rest) == 2) {
	int len = strlen(ftp_rest);
	if (ftp_rest[len - 1] != '/')
	    atput(r, "ACCESS-METHOD", "AFTP", "", ftp_hostname, "", ftp_rest,
		  textorbin, (char *) 0);
    }
}

/* Check if we're an AFTP link. If so, rewrite the HOST and HSONAME to go
   through the Prospero server at ARCHIE_SERVER (this can be changed). */
/* Gopher AFTP links to directories use the HSONAME format:
   ftp:<aftp-hostname>@/directory.../ */
static void
ftp_check_dir(GLINK gl, VLINK r)
{
    static char *ftp_hostname = NULL;
    int len;
    char *ftp_rest;		/* rest of string */
    if (qsscanf(gl->selector, "ftp:%&[^@]@%r", &ftp_hostname, &ftp_rest) == 2
#if 0
    /* Commented out, doesnt need to be / after @ if gatewaying thru gopher*/
	    && *ftp_rest == '/'
#endif
	) {
	len = strlen(ftp_rest);
	if (ftp_rest[len - 1] == '/') {
#if 0	/* commented out because of problem:not all
                                   archie servers index all AFTP sites. */
#ifdef ARCHIE_SERVER
	    ftp_rest[len - 1] = '\0';
	    r->host = stcopyr(ARCHIE_SERVER, r->host);
	    r->hsoname = qsprintf_stcopyr(r->hsoname, "ARCHIE/HOST/%s%s",
					  ftp_hostname, ftp_rest);
#endif
#endif
#ifdef GOPHER_GW_NEARBY_GOPHER_SERVER
	    r->hsoname = qsprintf_stcopyr(r->hsoname,
		  "GOPHER-GW/%s/1/ftp:%s@%s", GOPHER_GW_NEARBY_GOPHER_SERVER,
					  ftp_hostname, ftp_rest);

#endif
	}
    }
}

#ifdef PSRV_WAIS_GW
static int
wais_check_dir(GLINK gl, VLINK r)
{
    char *w_db = NULL;		/* Pointer into gl->selector e.g. zzz */
    char *w_path = NULL;	/* Pointer into gl->selector e.g. xxx/yyy/zzz */
    AUTOSTAT_CHARPP(w_db_srcp);	/* e.g. zzz.src */
    char *cp = NULL;		/* temporary pointer */
    WAISSOURCE thissource;		/* Points to source structure */
    if (qsscanf(gl->selector, "waissrc:%r", &w_path) != 1) {
	return FALSE;
    }
    if ((w_db = strrchr(w_path, '/')) == NULL) {
	w_db = w_path;
    } else {
	w_db++;
    }

    if ((cp = strstr(w_db, ".src")) == NULL) {
	*w_db_srcp = qsprintf_stcopyr(*w_db_srcp, "%s%s", w_db, ".src");
    } else {
	*w_db_srcp = stcopyr(w_db, *w_db_srcp);	/* *w_db_srcp = zzz.src */
    }
    /*!! Open and parse local src file */
    /* While this may s_alloc the source DONT free it, its on
	   a linked list of sources which persists while the server is up*/
    if ((thissource = findsource(*w_db_srcp, WAIS_SOURCE_DIR)) == NULL)
	return FALSE;

    if ((cp = strstr(w_db, ".src")) != NULL) {
      /* Need to do this AFTER findsource, otherwise can modify gl->selector
	 of non matching source */
      cp[0] = '\0';		 /* w_db = zzz */
    }
    r->hsoname = qsprintf_stcopyr(r->hsoname, "WAIS-GW/%s(%s)/QUERY/%s",
			      thissource->server, thissource->service, w_db);
    if (thissource->subjects)
	atput(r, "WAIS-SUBJECTS", thissource->subjects, (char *) 0);
    if (thissource->cost)
	atput(r, "WAIS-COST-NUM", thissource->cost, (char *) 0);
    if (thissource->units)
	atput(r, "WAIS-COST-UNIT", thissource->units, (char *) 0);
    if (thissource->maintainer)
	atput(r, "WAIS-COST-MAINTAINER", thissource->maintainer, (char *) 0);
    atput(r, "QUERY-METHOD", "wais-query(search-words)",
	  "${search-words}", "", (char *) 0);
    atput(r, "QUERY-ARGUMENT", "search-words",
	  "Index word(s) to search for", "mandatory char*", "%s", "",
	  (char *) 0);
    if (thissource->description)
	atput(r, "QUERY-DOCUMENTATION", "wais-query()",
	      thissource->description, (char *) 0);
    atput(r, "QUERY-DOCUMENTATION", "search-words", "This string says \
what you're searching for or what information you're specifying.",
	  "Type any string.  Sometimes SPACEs are treated as implied \
'and' operators.  Sometimes the words 'and', 'or', and 'not' are recognized \
as boolean operators.", (char *) 0);
    return (TRUE);
}
#endif

static void
zap_trailing_spaces(char *host)
{
    int len = strlen(host);
    while (--len >= 0) {
	if (host[len] == ' ')
	    host[len] = '\0';
	else
	    break;
    }
}

/* Add a collation-order attribute of the form NUMERIC <linknum> to the link
   VL */
static void
add_collation_order(int linknum, VLINK vl)
{
    PATTRIB ca = atalloc();
    char buf[40];		/* long enough to hold the ASCII
                                    representation of any int for the
                                    foreseeable future. */
    ca->precedence = ATR_PREC_LINK;
    ca->nature = ATR_NATURE_APPLICATION;
    ca->avtype = ATR_SEQUENCE;
    ca->aname = stcopy("COLLATION-ORDER");
    ca->value.sequence = tkappend("NUMERIC", ca->value.sequence);
    assert(qsprintf(buf, sizeof buf, "%d", linknum) <= sizeof buf);
    ca->value.sequence = tkappend(buf, ca->value.sequence);
    APPEND_ITEM(ca, vl->lattrib);
}


static void 
atput(VLINK vl, char *name,...)
{
    va_list ap;
    char *s;
    PATTRIB at = atalloc();

    va_start(ap, name);
    at->aname = stcopy(name);
    at->avtype = ATR_SEQUENCE;
    if (strequal(name, "ACCESS-METHOD"))
	at->nature = ATR_NATURE_FIELD;
    else
	at->nature = ATR_NATURE_APPLICATION;
    if (strequal(vl->target, "EXTERNAL"))
	at->precedence = ATR_PREC_REPLACE;	/* still not clear what to use */
    else
	at->precedence = ATR_PREC_OBJECT;
    while (s = va_arg(ap, char *)) {
	at->value.sequence =
	tkappend(s, at->value.sequence);
    }
    APPEND_ITEM(at, vl->lattrib);
    va_end(ap);
}
