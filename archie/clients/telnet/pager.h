#ifndef PAGER_H
#define PAGER_H

#include <stdio.h>


extern int pager PROTO((const char *file_name));
extern int pager_fp PROTO((FILE *fp));

#endif
