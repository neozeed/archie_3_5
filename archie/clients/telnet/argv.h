#ifndef ARG_H
#define ARG_H

#include "ansi_compat.h"


extern char *argvFlattenSep PROTO((int ac, char **av, int start, int end, const char *sep));
extern char *argvflatten PROTO((int ac, char **av, int start));
extern int argvFindStrCase(int ac, char **av, int start, int end, const char *str);
extern int argvReplaceStr(int ac, char **av, int idx, const char *newstr);
extern int argvify PROTO((const char *str, int *ac, char ***av));
extern void argvfree PROTO((char **av));

#endif
