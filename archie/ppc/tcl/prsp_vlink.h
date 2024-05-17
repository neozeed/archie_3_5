#ifndef PRSP_VLINK_H
#define PRSP_VLINK_H

#include "tcl.h"
#include "ppc.h"


extern int Prsp_VInit(Tcl_Interp *interp);
extern VLINK vlinkFromTclList(Tcl_Interp *interp, const char *vstr);
extern PATTRIB atListFromTclArray(Tcl_Interp *interp, int ac, char **av);
extern PATTRIB atFromTclList(Tcl_Interp *interp, const char *atstr);

#endif
