#ifndef MODE_H
#define MODE_H

#include "ansi_compat.h"


#define M_EMAIL        0x01
#define M_HELP         0x02
#define M_INTERACTIVE  0x04
#define M_SYS_RC       0x08
#define M_USER_RC      0x10
#define M_NONE         0x20

/*bug: should change name with addition of HELP*/
#define M_ALL          (M_SYS_RC | M_USER_RC | M_EMAIL | M_INTERACTIVE)
#define M_EIU          (M_EMAIL | M_INTERACTIVE | M_USER_RC)

extern const char *mode_str PROTO((int m));
extern int current_mode PROTO((void));
extern int set_mode PROTO((int m));

#endif
