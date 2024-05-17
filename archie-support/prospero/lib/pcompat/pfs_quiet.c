/*
 * Copyright (c) 1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <pcompat.h>


/* Written: Cliff Neuman, 1989, 1991. */
/* Updated documentation and declarations: swa@ISI.EDU, 1994. */

/*
 * pfs_quiet - Printing of error messages on prospero failure
 *
 *          This file initializes pfs_quiet to 0.  It is included
 *          the pcompat library library in case it is left out
 *          by an application.  A value of 0 means that open and
 *          other redefined system calls will print an error 
 *          message on the standard error stream before returning
 *          if an error was detected by Prospero.
 *
 */

int	pfs_quiet = 0;
