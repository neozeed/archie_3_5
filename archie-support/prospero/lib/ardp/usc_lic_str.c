/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

/* This string is modified in ardp_xmit.c.  That forces the C compiler to
   include it in the Prospero binaries.  If you know a less kludgy way to
   force a fancy optimizing C compiler to include it in the binaries, please
   let us know.
*/
char *usc_license_string = "PROSPERO(TM) Copyright (c) 1991-1993 University of Southern California\nPROSPERO is a trademark of the University of Southern California\n";
