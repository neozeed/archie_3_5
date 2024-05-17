#ifdef SUNOS

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

extern int accept proto_((int s, struct sockaddr *addr, int *addrlen));
extern void bcopy proto_((char *b1, char *b2, int lngth));
extern int bind proto_((int s, struct sockaddr *name, int namelen));
extern void bzero proto_((char *b, int lngth));
extern int chroot proto_((const char *dirname));
extern int connect proto_((int s, struct sockaddr *name, int namelen));
extern int fclose proto_((FILE *stream));
extern int fflush proto_((FILE *stream));
extern int fgetc proto_((FILE *stream));
extern int fprintf proto_((FILE *stream, const char *format, ...));
extern int fputc proto_((int c, FILE *stream));
/* extern int fputs proto_((char *s, FILE *stream)); */
/* extern int fread proto_((const char *ptr, int size, int nitems, FILE *stream)); */
extern int ftruncate proto_((int fd, off_t lngth));
/* extern int fwrite proto_((char *ptr, int size, int nitems, FILE *stream)); */
extern int getdtablesize proto_((void));
extern int gethostname proto_((char *name, int namelen));
extern int getpagesize proto_((void));
extern int getpeername proto_((int s, struct sockaddr *name, int *namelen));
/* extern int gettimeofday proto_((struct timeval *tp, struct timezone *tzp)); */
extern int hcreate proto_((unsigned nel));
extern int ioctl proto_((int fd, int request, caddr_t arg));
extern int listen proto_((int s, int backlog));
extern int munmap proto_((caddr_t addr, size_t len));
extern int nice proto_((int incr));
/* extern void perror proto_((char *s)); */
extern int printf proto_((const char *format, ...));
/* extern int putenv proto_((const char *string)); */
extern int puts proto_((const char *s));
extern int rand proto_((void));
extern int rename proto_((const char *path1, const char *path2));
/* extern int rewind proto_((FILE *stream)); */
/* extern int select proto_((int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)); */
extern void setbuf proto_((FILE *stream, char *buf));
extern int setpgrp proto_((int pid, int pgrp));
extern int setreuid proto_((int ruid, int euid));
extern int setsockopt proto_((int s, int level, int optname, char *optval, int optlen));
/* extern int setvbuf proto_((FILE *stream, char *buf, int type, int size)); */
extern int socket proto_((int domain, int type, int protocol));
extern int sscanf proto_((const char *s, const char *format, ...));
extern int strcasecmp proto_((const char *s1, const char *s2));
/* extern int strftime proto_((char *buf, int bufsize, const char *fmt, struct tm *tm)); */
extern int strncasecmp proto_((const char *s1, const char *s2, int len));
extern char *tail proto_((char *path));
extern int tgetent proto_((char *bp, const char *name));
extern time_t time proto_((time_t *tloc));
extern int tolower proto_((int c));
extern int toupper proto_((int c));
extern int ungetc proto_((int c, FILE *stream));
extern int usleep proto_((unsigned int useconds)); /* man page says int return value */
/* extern int vfprintf proto_((FILE *stream, const char *format, ...)); */
extern int vfsetenv proto_((char *hostname, char *filename, char *path));
/* extern int vsprintf proto_((char *s, const char *format, ...)); */

/* extern int perrmesg proto_((char *prefix, int no, char *text)); */
/* extern int write proto_((int fd, char *buf, int nbyte)); */
#endif
