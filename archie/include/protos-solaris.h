#ifdef SOLARIS

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

extern void bcopy proto_((char *b1, char *b2, int mlength));
extern int bind proto_((int s, struct sockaddr *name, int namelen));
extern void bzero proto_((char *b, int slength));
extern int chroot proto_((const char *dirname));
extern int connect proto_((int s, struct sockaddr *name, int namelen));
extern int fgetc proto_((FILE *stream));
extern FILE *fdopen(int fildes, const char *type);
extern int fileno proto_((FILE *)); 
/*extern int fprintf proto_((FILE *stream, const char *format, ...)); */
extern int fputc proto_((int c, FILE *stream));
extern int ftruncate proto_((int fd, off_t slength));
extern int getdtablesize proto_((void));
extern int getopt(int, char *const *, const char *); 
extern int getpagesize proto_((void));
extern int getpeername proto_((int s, struct sockaddr *name, int *namelen));
/*extern int gettimeofday proto_((struct timeval *tp, struct
timezone *tzp)); */
extern int hcreate proto_((unsigned nel));
extern int isascii proto_((int));
extern int kill proto_((pid_t, int));
extern int listen proto_((int s, int backlog));
extern int munmap proto_((caddr_t addr, size_t len));
extern int nice proto_((int incr));
extern int printf proto_((const char *format, ...));
/* extern int pset_wd proto_((char *wdhost, char *wdfile, char *wdname));*/
extern int putenv proto_((const char *string));
extern int puts proto_((const char *s));
extern int rand proto_((void));
extern int rename proto_((const char *path1, const char *path2));
extern int select proto_((int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout));
extern void setbuf proto_((FILE *stream, char *buf));
extern int setreuid proto_((int ruid, int euid));
extern int socket proto_((int domain, int type, int protocol));
extern int strcasecmp proto_((const char *s1, const char *s2));
extern char *strdup proto_((const char *));
/* extern int strncasecmp proto_((const char *s1, const char *s2, int len)); */
extern char	*tempnam(const char *, const char *); 
extern int tgetent proto_((char *bp, const char *name));
extern char *tail proto_((char *path));
extern time_t time proto_((time_t *tloc));
extern int toascii proto_((int));
extern int tolower proto_((int c));
extern int toupper proto_((int c));
extern void tzset proto_((void));
extern char *tzname[2];
extern int ungetc proto_((int c, FILE *stream));
extern int usleep proto_((unsigned int useconds)); /* man page says int return value */


/* extern int perrmesg proto_((char *prefix, int no, char *text)); */
/* extern int write proto_((int fd, char *buf, int nbyte)); */
#endif 
