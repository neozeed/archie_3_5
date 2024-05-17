#ifndef INPUT_H
#define INPUT_H

#include "ansi_compat.h"


extern char *readline PROTO((const char *prompt));
extern int new_hist_context PROTO((void));
extern int set_hist_context PROTO((int n));
extern void add_history PROTO((const char *line));

#endif
