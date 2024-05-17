#ifndef TERMINAL_H
#define TERMINAL_H

#include "ansi_compat.h"


extern int get_cols PROTO((void));
extern int get_rows PROTO((void));
extern int init_term PROTO((void));
extern int set_term PROTO((const char *term));
extern int set_tty PROTO((int ac, char **av));

#endif
