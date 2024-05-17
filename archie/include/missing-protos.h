/* Missing prototype definitions */


#ifdef SOME_MISSING_PROTOS

#if !defined(SOLARIS) && !defined(AIX)
extern int sscanf PROTO((char *s, char *format, ...));
#endif

#endif
#ifdef SOME_MISSING_PROTOS
extern int tolower PROTO((int c));
extern int toupper PROTO((int c));
#endif
