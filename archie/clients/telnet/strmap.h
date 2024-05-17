#ifndef STRMAP_h
#define STRMAP_h

#include "ansi_compat.h"


typedef struct
{
  const char *from;
  const char *to;
} StrMap;


extern StrMap *newStrMap PROTO((const char *s, StrMap *m)); /* bug: should be more general; varargs? */
extern const char *mapFirstStr PROTO((const StrMap *m));
extern const char *mapStr PROTO((const char *s, const StrMap *map));
extern const char *mapNCaseStr PROTO((const char *s, const StrMap *map));
extern void freeStrMap PROTO((StrMap *m));
extern int mapEmpty PROTO((const StrMap *m));

extern const char *mapLangStr PROTO((const StrMap *map));
/* #ifdef NEW -- commented out Thu Oct 14 18:09:03 EDT 1993 */
extern const char *mapNCaseStrTo PROTO((const char *s, const StrMap *map));
extern const char *mapNCaseStrFrom PROTO((const char *s, const StrMap *map));
extern const char *mapStrTo PROTO((const char *s, const StrMap *map));
extern const char *mapStrFrom PROTO((const char *s, const StrMap *map));
/* #endif */

#endif
