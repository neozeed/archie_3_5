#ifndef PROSP_H
#define PROSP_H

#include <sys/types.h>
#include <sys/stat.h> /* so pmachine.h won't redefine stuff */

#include "pfs.h"
#include "perrno.h"
#include "pmachine.h"
#include "pprot.h"    /* For VFPROT_VNO, the Prospero version number */
#include "parchie.h"

extern int pfs_debug;

#endif
