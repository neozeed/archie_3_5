#ifndef ARCHIE_ANSI_COMPAT_H
#define ARCHIE_ANSI_COMPAT_H

#define proto_ PROTO	/* Bill's sensitivity */

#ifdef __STDC__
#  define PROTO(arglist) arglist
#else
#  define const
#  define volatile
#  define PROTO(arglist) ()
#endif

#endif
