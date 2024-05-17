#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>
#include <stdio.h>
#include "ansi_compat.h"


extern int copy_to_stderr proto_((FILE *fp));
extern int efprintf(FILE *fp, const char *fmt, ...);
extern int error(const char *file, const char *fmt, ...);

#endif
