#ifndef TELLWAIT_H
#define TELLWAIT_H

#include <sys/types.h>
#include "ansi_compat.h"


extern int no_tell_wait PROTO((void));
extern int tell_wait PROTO((void));
extern void tell_child PROTO((pid_t pid));
extern void tell_parent PROTO((pid_t pid));
extern void wait_child PROTO((void));
extern void wait_parent PROTO((void));

#endif
