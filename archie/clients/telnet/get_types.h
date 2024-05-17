#ifndef GET_TYPES_H
#define GET_TYPES_H

#include "ansi_compat.h"


enum osort_type_t
{
  SORT_DBORDER,
  SORT_FILENAME,
  SORT_FILESIZE,
  SORT_HOSTNAME,
  SORT_MODTIME,
  SORT_R_FILENAME,
  SORT_R_FILESIZE,
  SORT_R_HOSTNAME,
  SORT_R_MODTIME
};

typedef enum osort_type_t osort_type;


extern char get_search_type PROTO((void));
extern int get_sort_type PROTO((void));

#endif
