/*
/*
 * Copyright (c) 1993,1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
/*
 * pfs_enable - Disable Virtual File System interpetation of filenames to
 *          syscalls 
 *
 *          Included in the library in case users forget to
 *          include it themselves and initialize.
 *
 *          This is initialized in p_initialize() (in libpfs) to PMAP_DISABLE.
 *          By default, 
 *          which means that file names will be 
 */

#include <pcompat.h>

int pfs_enable;


