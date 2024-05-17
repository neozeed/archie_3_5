/* The includes you need should really be specified in each file, but 
   there are so many places they are not included, that this is 
   the lazy way to do it */
/* This file works pretty well under Solaris, but these include files don't
   prototype all the stuff under SunOS 4.x that they do under Solaris.  So
   we can't run GCC with -Wimplicit under SunOS 4.x */
#ifndef PFS_IMPLICIT_FIXES_H
#define PFS_IMPLICIT_FIXES_H
#include <string.h>     /*strncmp etc */
#include <sys/types.h>  /*read etc */
#include <sys/uio.h>    /* read etc */
#include <unistd.h>     /* read etc */
#include <stdlib.h>     /* malloc etc */
#include <sys/stat.h>   /* For open */
#include <fcntl.h>      /* open */
#include <ctype.h>      /* isascii */
#include <sys/socket.h> /* for inet_ntoa*/
#include <netinet/in.h> /* for inet_ntoa */
#include <arpa/inet.h>  /* for inet_ntoa */

#ifdef SOLARIS 

/* might be needed by other SYSV derived systems? */
/* not used on HPUX or SCOUNIX.   */
/* Not a part of the Posix 1003.1 standard. */

#include <crypt.h>      /* for crypt */

#endif

#if defined(PFS_THREADS) && defined(PARANOID)
#define VLDEBUGBEGIN \
{             \
		extern int vlink_count, subthread_count; \
  int vlcount = ((subthread_count > 1) ? -1 : vlink_count);
#define VLDEBUGIN(vvvv) \
	{ if (vlcount != -1) vlcount += vl_nlinks(vvvv); }
#define VLDEBUGAT(at) \
		{ if (vlcount != -1) vlcount += at_nlinks(at); }
#define VLDEBUGEND \
  assert (subthread_count >1 || vlcount == -1 || vlcount == vlink_count); \
}

#else
#define VLDEBUGBEGIN
#define VLDEBUGIN(vl)
#define VLDEBUGAT(at)
#define VLDEBUGEND
#endif
#define VLDEBUGOB(ob) { VLDEBUGIN(ob->links); VLDEBUGIN(ob->ulinks); \
		      VLDEBUGAT(ob->attributes); }
#define VLDEBUGDIR(dir) { VLDEBUGIN(dir->links); VLDEBUGIN(dir->ulinks); \
			VLDEBUGAT(dir->attributes); }
#define VLDEBUGFI(fi) { \
			  VLDEBUGIN(fi->backlinks); \
			  VLDEBUGIN(fi->forward); VLDEBUGAT(fi->attributes); }

extern int subthread_count;
extern void it_failed(void);
#endif /*PFS_IMPLICIT_FIXES_H*/

