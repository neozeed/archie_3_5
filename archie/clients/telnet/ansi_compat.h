#ifndef ANSI_COMPAT_H
#define ANSI_COMPAT_H

#ifdef __STDC__
#  define PROTO(arglist) arglist
#else
#  define const
#  define volatile
#  define PROTO(arglist) ()
#endif

#endif
