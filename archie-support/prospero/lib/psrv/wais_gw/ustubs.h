/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.
*/

/*----------------------------------------------------------------------*/
/* definitions that non-ansi (aka sun) C doesn't provide */

#ifndef USTUBS_H
#define USTUBS_H

#include "cdialect.h"

#ifndef  ANSI_LIKE

#include <sys/types.h>

#ifdef M_XENIX
#include <string.h>
#endif /* ndef M_XENIX */

#ifndef size_t
#ifndef M_XENIX
#define	size_t	unsigned long
#endif /* ndf M_XENIX */
#endif /* ndef size_t */

#ifndef ANSI_LIKE
#ifndef M_XENIX
#define time_t long
#endif /* ndef M_XENIX */
#endif /* ndef ANSI_LIKE */

#ifdef K_AND_R  /* this might be too general, but it is needed on vaxen */
#define void char
#endif /* ndef K_AND_R */

#ifdef __cplusplus
/* declare these as C style functions */
extern "C"
	{
#endif /* def __cplusplus */

#ifndef SCOUNIX
char *strstr(char *src, char *sub);
#endif

#ifdef SYSV
char *getwd (char *pathname);
long random(void);
long srandom(unsigned long seed);
#ifndef SCOUNIX
#define rename(f1,f2) {link((f1),(f2)); unlink((f1)); } 
#endif /*SCOUNIX*/
#endif /* defu SYSV */

#if !(defined(NeXT) || defined(Mach))
#ifndef M_XENIX
#ifndef cstar
char* malloc(size_t size);
char* calloc(size_t nelem,size_t elsize);
#ifndef SCOUNIX
void free(char* ptr);
#endif
char* realloc(char* ptr,size_t size);
#ifndef mips
#ifndef hpux
#ifndef vax
#ifndef SCOUNIX
char* memcpy(char* s1,char* s2,size_t c);
#endif
void* memmove(void* s1,void* s2,size_t n);
#endif /* ndef vax */
#endif /* ndef hpux */
#endif /* ndef mips */
char *strcat(char *s1, char *s2);
#endif /* ndef cstar */
#endif /* ndef M_XENIX */
#endif /* not NeXT or Mach */

long atol(char *s);

#ifdef __cplusplus
	}
#endif /* def __cplusplus */

#else /* def ANSI_LIKE */

#endif /* else ndef ANSI_LIKE */

/*----------------------------------------------------------------------*/

#endif /* ndef USTUBS_H */

