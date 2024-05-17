/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>

#include <pcompat.h>

int		pfs_default = -1;

char		*getenv();

extern int      pfs_debug;

void
p__get_pfs_default(void)
{
    char	*pfs_default_string;
    char        *pfs_debug_string;

    /* if pfs_disable_flag is set to disable, skip as there  */
    /* is probably a reason for it                           */
    if(pfs_enable == PMAP_DISABLE) return;

    /* Check PFS_DEFAULT and if set, set pfs_disable_flag */
    if(pfs_default == -1) {
        pfs_default_string = getenv("PFS_DEFAULT");
        if(pfs_default_string == 0) pfs_default = -2;
        else if(sscanf(pfs_default_string,"%d",&pfs_default) != 1)
            pfs_default = -2;
        else pfs_enable = pfs_default;
    }
}



