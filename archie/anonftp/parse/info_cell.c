#include <string.h>
#include <stdio.h>
#include <stdlib.h> /* malloc? */
#include "parse.h"
#include "info_cell.h"
#include "error.h"
#include "lang_parsers.h"

/*
  The 'name' field is assumed to point to malloc'ed storage.
*/

int
dispose_Info_cell(ic)
  Info_cell *ic ;
{
  ptr_check(ic, Info_cell, "init_Info_cell", 0) ;

  if(ic->name != (char *)0)
  {
    free(ic->name) ;
  }
  free(ic) ;
  return 1;

#ifdef 0
#ifndef __STDC__
#ifndef AIX
  return free(ic) ;
#else
  free(ic) ;
  return 1;
#endif  
#else
  free(ic) ;
  return 1;
#endif
#endif
}


Info_cell *
new_Info_cell(str)
  char *str ;
{
  Info_cell *ic ;

  ptr_check(str, char, "init_Info_cell", 0) ;

  if((ic = (Info_cell *)malloc(sizeof(Info_cell))) == (Info_cell *)0)
  {
    /* "Error malloc'ing %ld bytes*/

    error(A_SYSERR, "new_Info_cell", NEW_INFO_CELL_001, (long)sizeof(Info_cell)) ;
    return ic ;
  }
  ic->name = str ;
  ic->idx = 0 ;
  ic->addr = (Pars_ent_holder *)0 ;
  return ic ;
}
