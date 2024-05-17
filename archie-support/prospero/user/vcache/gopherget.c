/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>.
 */

/* gopherget.c
   Part of the program "vcache" in the Prospero distribution.
   This allows Prospero clients to use the GOPHER access method (i.e., to
   retrieve files sitting on Gopher servers.)
   Written: swa@isi.edu, 7/13/92 -- 7/15/92
   Comments improved 8/6/92
   Gopher TEXT access method made considerably looser 6/30/93
   Got rid of necessity for writev.  9/15/93
*/

#include <usc-copyr.h>

#include <stdio.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <perrno.h>

#include <pmachine.h>
#ifdef HAVE_WRITEV
#include <sys/uio.h>            /* for writev */
#endif

#include <pfs.h>                /* for assert() and internal_error() */
#include "vcache_macros.h"
/* If it doesnt respond in 5 secs, its probably gone away */
#define G_READ_TIMEOUT 5
#define G_OPEN_TIMEOUT 5


extern int pfs_debug, cache_verbose;

/* Use the Gopher protocol.  Get the file denoted by the SELECTOR_STRING
   from the Gopher server running at port PORT of HOST.  Stores that file in
   the local file named LOCAL.  Return non-zero on error; 0 if OK.  If it
   returns non-zero, the existence and contents of the file LOCAL are
   indeterminate.  (If this presents a problem, we'll have to rewrite the code
   to do the unlinking). */

int 
gopherget(char *host, char *local, char *selector_string, int port,
                     int gophertype /* actually char, but make sure default
                                       promotions work. */ )
{
    FILE *local_file;               /* file pointer for local destination */
    int remote_sock;            /* File Descriptor for remote socket. */
    int retval;                 /* error status return. */
    static int vcache_open_tcp_stream(), send_line();

    int (*retrieval_method)();  /* What method will we use to retrieve this
                                   file?  Null means none. */
    /* function to call to find out Gopher retrieval method.   */
    /* precede 'gophertype' with an _ to get rid of bogus whines from GCC 2.5.8
       with -Wshadow. */
    static int (*get_retrieval_method(int _gophertype))(); 


    TRACE(5, "vcache: Attempting to retrieve remote document \"%s\" \n\
via Gopher from port %d of host %s\n", selector_string, port, host);

    retrieval_method = get_retrieval_method(gophertype);
    if (!retrieval_method)
        return 1;               /* Failure. */

    /* Open Local Destination file.  Note that PFS has already been disabled. 
     */
    local_file = fopen(local, "w");
    if (local_file == NULL) {
        ERRSYS("vcache: Couldn't create the local file %s:%s %s ", local);
        return 1;
    }
    
    /* Open TCP stream to remote gopher server. */
        TRACE(5,"Calling vcache_open_tcp_stream(host = %s, port = %d)\n", 
	host, port);
    if ((remote_sock = vcache_open_tcp_stream(host, port)) < 0) {
            /* No need to set p_err_string, vcache_open_tcp_stream will */
        fclose(local_file);
        return 1;
    }

        TRACE(5,"Sending selector string to remote GOPHER: %s\n", 
               selector_string);
    /* At this point, if any errors occur, we still try to clean up as neatly
       as possible. */
    retval = send_line(remote_sock, selector_string);
    if (!retval)
        retval = (*retrieval_method)(remote_sock, local_file); 
    if (!retval && cache_verbose)
        puts("vcache: Retrieval completed.");
    
    if (fclose(local_file)) {
            TRACE(5, "vcache: Error attempting to close local file \
%s\n", local);
        retval = 1;
    }

    /* Close down both ends of the connection. shutdown() allegedly performs
       actions that close() defers. */ 
    if (shutdown(remote_sock, 2)) {
            ERRSYS("vcache: shutdown(remote_sock, 2) failed:%s %s");
        retval = 1;
    }
   if (close(remote_sock)) {
           ERRSYS("vcache: close(remote_sock) failed:%s %s");
       retval = 1;
   }

   if (retval)
       return 1;               /* failure. */
   else
       return 0;               /* Done.  Success! */
}


/* Open a TCP stream from here to the HOST at the PORT. */
/* Return socket descriptor on success, or -1 on failure. */
/* On failure set p_err_string to "user" friendly error message */
static int
vcache_open_tcp_stream(char host[],int port)
{
    TRACE(2,"vcache: quick_open_tcp_stream(host=%s;port=%d;time=%d)\n",
               host, port, G_OPEN_TIMEOUT);
/* Dont need to set error message - quick_open_tcp_stream sets p_err_string
   to user-friendly error message */
    return(quick_open_tcp_stream(host,port, G_OPEN_TIMEOUT));
}


/* Send a line to the remote host.  The line we're given is NOT crlf
   terminated. */ 
/* We use writev because it's more efficient than two writes (and saves copying
   operations).  The Berkeley TCP implementation lacks a "push" command, so
   there's an implicit push after each "write" or "writev".  */
/* Return nonzero if failure; 0 if success */
static int
send_line(remote_sock, str)
    int remote_sock;            /* A connected TCP socket */
    char str[];
{
    int expected_count;         /*  # of bytes we expect to send. */
    int bytes_sent;             /* # of bytes we actually sent. */
#ifdef HAVE_WRITEV
    struct iovec iov[2];

    iov[0].iov_base = str;
    iov[0].iov_len = strlen(str);
    iov[1].iov_base = "\r\n";
    iov[1].iov_len = 2;
    expected_count = iov[0].iov_len + iov[1].iov_len;
    
    /* Selector strings are short enough that we should never send a partial
       packet. */
    bytes_sent = writev(remote_sock, iov, 2);
#else
    static char *buf = NULL;

    buf = qsprintf_stcopyr(buf, "%s\r\n", str);
    expected_count = strlen(buf);
    
    /* Gopher selector strings are short enough that we should never send a
       partial packet. */
    bytes_sent = write(remote_sock, buf, expected_count);
#endif                          /* HAVE_WRITEV */
    if (bytes_sent != expected_count) { 
            ERRSYS("vcache: Error sending selector string %s to gopher server. \
\tExpected to send %d bytes; write() or writev() reported sending only %d. %s %s",
                    str, expected_count, bytes_sent);
        return 1;
    }
    return 0;
}


/* Read a gopher text stream from in_sock and save it in out_file.
   Gopher text lines are terminated with crlf.
   Their last line consists of  ".\r\n". 
   This is implemented as a simple DFA.
  */
/* 6/30/93:
   We do full error recovery  & don't complain by default, since I've now
   learned that the format is followed rather loosely.   Thus the LOOSE
   definition. */
#define LOOSE
static
int
receive_gopher_text(in_sock, out)
    int in_sock;                /* Input socket for Gopher text */
    FILE *out;                  /* Where to save the received text. */
{
    char buf[BUFSIZ];           /* buffer for reading. */
    register int numread;                /* Number of bytes actually read.  */
    char *complaint = NULL;     /* Might be set to a string that contains a
                                   complaint about the remote format.  This is
                                   usually used for recoverable errors. */
    enum states { BOL,          /* Beginning of line */
                  GOT_BEG_DOT,  /* Got '.' following BOL */
                  MID_LINE,     /* middle of line */
                  GOT_DOT_CR,   /* Got ".\r",Expect LF */
                  GOT_CR,       /* Got CR; expect LF  */
                  EOT,           /* got ".\r\n" by itself */
#ifndef LOOSE
                  BAD           /* something gross happened.  We have
                                   complained about it and are now just marking
                                   time.  */
#endif
                  } state;      

    state = BOL;
    while((numread = quick_read(in_sock, buf, sizeof buf, G_READ_TIMEOUT)) > 0) {
        register int i;
        /* Process the characters we read. */
        for (i = 0; i < numread; ++i) {
            register char c = buf[i];    /* current character */
            
            switch(state) {
            case BOL:
#ifdef LOOSE
            case_bol:           /* a label to go to from EOT. */
#endif
                switch(c) {
                case '.':
                    state = GOT_BEG_DOT;
                    break;
                case '\r':
                    state = GOT_CR;
                    break;
                case '\n':
                    complaint = "Got LF without CR";
                    putc(c, out);
                    state = BOL;
                    break;
                default:
                    putc(c, out);
                    state = MID_LINE;
                    break;
                }
                break;
            case GOT_BEG_DOT:
                if (c == '.') {
                    putc('.', out);
                    state = MID_LINE;
                } else if (c == '\r') {
                    state = GOT_DOT_CR;
                } else if (c == '\n') {
                    complaint = "Got LF without CR";
                    state = EOT;
                } else {
                    complaint = "Got initial dot without following dot or EOL";
                    putc('.', out);
                    putc(c, out);
                    state = MID_LINE;
                }
                break;
            case MID_LINE:
                if (c == '\r') {
                    state = GOT_CR;
                } else if (c == '\n') {
                    complaint = "Got LF without CR";
                    putc('\n', out);
                    state = BOL;
                } else {
                    putc(c, out);
                    state = MID_LINE;
                }
                break;
            case GOT_DOT_CR:
                if (c != '\n') {
                    complaint = "Got . CR without LF";
#ifdef LOOSE
                    putc('.', out);
                    putc('\n', out);
                    goto case_bol;
#else
                    state = BAD;
#endif
                } else {
                    state = EOT;
                }
                break;
            case GOT_CR:
                if (c != '\n') {
                    complaint = "Got CR without LF";
#ifdef LOOSE
                    putc('\n', out);
                    putc(c, out);
                    goto case_bol;
#else
                    state = BAD;
#endif
                } else {
                    putc('\n', out);
                    state = BOL;
                }
                break;
            case EOT:
                /* If we get text after EOT, just keep on gobbling it up.  This
                   is not a fatal error, merely a warning.  */
                complaint = "Got text after EOT";
#ifdef LOOSE
                putc('.', out);
                putc('\n', out);
                goto case_bol;
#endif
                break;
#ifndef LOOSE
            case BAD:           /* nothing more to do */
                goto abort;
                break;
#endif
            default:
                ERR("vcache: Internal error in line %d of file %s:\
 unhandled case %s", __LINE__, __FILE__);
                return(-1);
                break;
            }
        }
    }       
 abort:
    if (complaint && cache_verbose)
        ERR("vcache: we encountered incorrect formatting when\
 reading from remote server: %s.  \
 This is not a serious problem by itself, and the retrieval did not\
 necessarily fail, but you should notify your system maintainer %s", complaint);

    if (numread < 0) {          /* If read() returned an error */
            ERRSYS("vcache: read from remote gopher server failed:%s %s");
        return -1;
    }

#ifndef LOOSE
    if (state == EOT)           /* normal completion (possibly with warnings,
                                   but all errors seem recoverable.) */
#endif
        return 0;               /* success */

#ifndef LOOSE
        ERR(stderr, "vcache: Premature end of transmission while \
reading text from remote gopher server. %s");
    return -1;
#endif
}



/* Read a gopher binary stream from in_sock and save it in out_file.
   Keep on consuming it until end of transmission. */
static
int
receive_gopher_binary(in_sock, out)
    int in_sock;                /* Input socket for Gopher text */
    FILE *out;                  /* Where to save the received text. */
{
    char buf[BUFSIZ];           /* buffer for reading & writing. */
    register int numread;                /* Number of bytes actually read.  */

    while((numread = quick_read(in_sock, buf, sizeof buf, G_READ_TIMEOUT)) > 0) {
        /* We might be writing to a pipe, so we might not be able to write an
           entire buffer at once.  
           Write out the buffer, possibly in multiple chunks. */
        int i;
        int numwritten;
        for (i = 0; i < numread; i += numwritten) {
            numwritten = fwrite(&buf[i], sizeof buf[0], numread - i, out);
            if (numwritten == 0) { /* write error! */
                    ERR("vcache: receive_gopher_binary(): write error %s");
                return -1;      /* error */
            }
        }
    }
    if (numread == 0) /* EOF! */
        return 0;               /* success */
    else if (numread < 0) {   /* If read() returned an error */
            ERRSYS("vcache: read from remote gopher server failed:%s %s");
        return -1;
    } else {
        /* should never get here! */
        ERR("vcache: Internal error at line %d of  file %s. %s",
                __LINE__, __FILE__);
        return(-1);
        /*NOTREACHED*/
    }
}



/* Check 1st character of gopher selector string to decide if retrieval
   method is supported.  If no retrieval method, punt.
   This table of retrieval methods comes from Edward Vielmetti's 13 July 1992
   article in alt.gopher.
 */


/* 
These use the Gopher TEXT retrieval method.
0       A_FILE          Text file.
4       A_MACHEX        Macintosh Binhex (.hqx) text file.
c       A_CALENDAR      Calendar text file.
e       A_EVENT         Event text file.
M       A_MIME          MIME (RFC 1341) text file.

These use the Gopher BINARY retrieval method.
9       A_UNIXBIN       Binary file.
g       A_GIF           GIF (Graphics Interchange Format) binary file.
s       A_SOUND         Sound (8 bit u-law, no headers ?) binary file.


things to do with data as it's flying back at you:
- treat as binary, read until connection drops,
        send to file    A_UNIXBIN
        send to process A_SOUND
        send to screen  A_GIF
- treat as text, read til "." alone on line,
        send to screen  A_FILE
        send to process A_MIME
                        A_CALENDAR
                        A_EVENT
        send to file    A_MACHEX
*/
/* In addition, the gopher protocol.txt document describes two obsolete types
   (use of these types is discouraged) which Ed has not mentioned in his
   letter.  I'll include them in the table
   below, because we want to support as many types as possible.   They are:
   Type 5: DOS binary archive of some kind (binary)
   Type 6: UUencoded UNIX file (text).
*/

struct {
    char type_char;
    int (*func)();
    char *name;
} retrieval_method_tab[] = {
    {'0', receive_gopher_text, "GOPHER_TEXT"},
    {'4', receive_gopher_text, "GOPHER_TEXT"},
    {'c', receive_gopher_text, "GOPHER_TEXT"},
    {'e', receive_gopher_text, "GOPHER_TEXT"},
    {'M', receive_gopher_text, "GOPHER_TEXT"},
    {'9', receive_gopher_binary, "GOPHER_BINARY"},
    {'g', receive_gopher_binary, "GOPHER_BINARY"},
    {'s', receive_gopher_binary, "GOPHER_BINARY"},
    {'5', receive_gopher_binary, "GOPHER_BINARY"},
    {'6', receive_gopher_text, "GOPHER_TEXT" },
    {'\0', NULL, NULL}
};


static
int
(*get_retrieval_method(int gophertype))() /* actually char. */
{
    int i;
    
    for (i = 0; retrieval_method_tab[i].type_char != '\0'; ++i) {
        if (gophertype == retrieval_method_tab[i].type_char) {
            TRACE(5,"Using retrieval method: %s\n", 
                       retrieval_method_tab[i].name);
            return retrieval_method_tab[i].func;
        }
    }
        ERR("vcache: Can't retrieve document -- item type %c \
unsupported. %s", gophertype);
    return NULL;
}
