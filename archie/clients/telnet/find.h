#ifndef FIND_H
#define FIND_H

#include <stdio.h>
#include "ansi_compat.h"


extern int anonftp_find PROTO((int ac, char **av, FILE *ofp));
extern int find_it PROTO((int ac, char **av, FILE *ofp));

#endif
