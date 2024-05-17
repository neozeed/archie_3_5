#ifndef UNIXCOMPAT_H
#define UNIXCOMPAT_H

#include "ansi_compat.h"


extern int regain_root PROTO((void));
extern int revoke_root PROTO((void));
extern void u_sleep PROTO((unsigned int usecs));

#endif
