#ifndef DEFS_H
#define DEFS_H

#include <sys/param.h>
#include <errno.h>
#ifdef SOLARIS
# include <netdb.h>
#endif
#include "ansi_compat.h"


#define CR '\015'
#define CRLF "\015\012"
#define ERRSTR (errno < sys_nerr ? sys_errlist[errno] : "<unknown error>")
#define LF '\012'
#define STR(s) ((s) ? (s) : "<null>")
#define STRNCMP(a,b) strncmp(a, b, strlen(b))
#define dfprintf if (debug) efprintf
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))


typedef struct
{
  char host[MAXHOSTNAMELEN+1];
  char port[16];
} Where;


extern char *sys_errlist[];
extern int errno;
extern int sys_nerr;

extern Where us;
extern Where them;
extern const char *prog;
extern int debug;
extern int reuseaddr;

#endif
