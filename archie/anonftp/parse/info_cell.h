#ifndef	INFO_CELL_H
#define INFO_CELL_H

#include "typedef.h"
#include "pars_ent_holder.h"

typedef struct
{
  char *name ;
  index_t idx ;
  Pars_ent_holder *addr ;
} Info_cell ;

#ifdef __STDC__

extern int dispose_Info_cell(Info_cell *ic) ;
extern Info_cell *new_Info_cell(char *str) ;

#else

extern int dispose_Info_cell(/* Info_cell *ic */) ;
extern Info_cell *new_Info_cell(/* char *str */) ;

#endif
#endif
