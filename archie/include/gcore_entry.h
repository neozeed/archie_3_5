/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _GCORE_ENTRY_H_
#define _GCORE_ENTRY_H_

#include "typedef.h"
#include "ansi_compat.h"

#define CSE_SET_DIR(cse) (cse.ftype = '1')
#define CSE_SET_NON_DIR(cse) (cse.ftype = '0')
#define CSE_IS_DIR(cse) (cse.ftype == '1')

typedef struct
{
    int     ftype;
    int	    port;
    index_t parent_idx ;
    index_t child_idx ;
} gcore_site_entry_t ;

#endif
