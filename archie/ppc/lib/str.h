#ifndef STR_H
#define STR_H

#include "ansi_compat.h"


extern char *strend proto_((char *s));
extern char *strncopy proto_((char *s1, const char *s2, int n));
extern char *strndup proto_((const char *s, int n));
#ifdef VARARGS
 extern char *strsdup(/*varargs*/);
#else
 extern char *strsdup(const char *s, ...);
#endif
extern char *strskip proto_((char *s, const char *skip));
extern char *strterm proto_((char *s, int c));
extern char *tr proto_((char *s, int src, int dst));
extern char *trimright proto_((char *s, const char *tchars));
extern char *worddup proto_((const char *s, int n));
extern int memsprintf proto_((char **buf, const char *fmt, ...));
extern int qwstrnsplit proto_((const char *src, char *dst[], int n, const char **remain));
extern int splitwhite proto_((const char *src, int *ac, char ***av));
#if 0
extern int strnsplit proto_((const char *splitstr, const char *src, char *dst[], int n, const char **remain));
#endif
extern int strrspn proto_((char *s, const char *cset));
#ifdef VARARGS
 extern int vslen(/*varargs*/);
#else
# if 1
  extern int vslen(const char *fmt, va_list ap);
#else
  extern int vslen(const char *fmt, ...);
# endif
#endif
#endif
