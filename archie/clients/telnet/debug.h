#ifndef DEBUG_H
#define DEBUG_H

#include "ansi_compat.h"
#include "prosp.h"


#ifdef DEBUG
# define DBG(cmds) do { cmds } while (0)
#else
# define DBG(cmds)
#endif


#define d0fprintf if (dlev() >= 0) fprintf
#define d1fprintf if (dlev() >= 1) fprintf
#define d2fprintf if (dlev() >= 2) fprintf
#define d3fprintf if (dlev() >= 3) fprintf
#define d4fprintf if (dlev() >= 4) fprintf
#define d5fprintf if (dlev() >= 5) fprintf
#define d6fprintf if (dlev() >= 6) fprintf
#define d7fprintf if (dlev() >= 7) fprintf
#define d8fprintf if (dlev() >= 8) fprintf
#define d9fprintf if (dlev() >= 9) fprintf


extern int dlev PROTO((void));
extern int set_debug PROTO((const char *v));
extern int unset_debug PROTO((void));
extern void fprint_vlink PROTO((FILE *fp, VLINK v));
extern void uids PROTO((const char *s));

#endif
