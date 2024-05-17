/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _CORE_ENTRY_H
#define _CORE_ENTRY_H

#include <values.h>

#include "typedef.h"
#include "ansi_compat.h"

#define CSE_SET_DIR(cse) (cse.flags |= 0x01)
#define CSE_SET_NON_DIR(cse) (cse.flags &= ~0xfffe)
#define CSE_IS_DIR(cse) (cse.flags & 0x01)

#define CSE_SET_LINK(cse) (cse.flags |= 0x02)
#define CSE_SET_NON_LINK(cse) (cse.flags &= ~0x02)
#define CSE_IS_LINK(cse) (cse.flags & 0x02)

#define CSE_SET_SUBPATH(cse) (cse.flags |= 0x04)
#define CSE_SET_NON_SUBPATH(cse) (cse.flags &= ~0x04)
#define CSE_IS_SUBPATH(cse) (cse.flags & 0x04)

#define CSE_SET_KEY(cse) (cse.flags |= 0x08)
#define CSE_SET_NON_KEY(cse) (cse.flags &= ~0x08)
#define CSE_IS_KEY(cse) (cse.flags & 0x08)

#define CSE_SET_PORT(cse) (cse.flags |= 0x10)
#define CSE_SET_NON_PORT(cse) (cse.flags &= ~0x10)
#define CSE_IS_PORT(cse) (cse.flags & 0x10)

#define CSE_SET_NAME(cse) (cse.flags |= 0x20)
#define CSE_SET_NON_NAME(cse) (cse.flags &= ~0x20)
#define CSE_IS_NAME(cse) (cse.flags & 0x20)

#define CSE_SET_DOC(cse) (cse.flags |= 0x40)
#define CSE_SET_NON_DOC(cse) (cse.flags &= ~0x40)
#define CSE_IS_DOC(cse) (cse.flags & 0x40)

#define CSE_IS_FILE(cse) (!cse.flags)

#define CSE_STORE_WEIGHT(cse,key) ((cse.size = (int)(key*MAXINT)))
#define CSE_GET_WEIGHT(cse) (((double)cse.size/(double)(MAXINT)))

typedef struct
{
    file_size_t size ;
    date_time_t date ;
    index_t parent_idx ;
    index_t child_idx ;
    perms_t perms ;
    flags_t flags;
    date_time_t rdate ;    
} core_site_entry_t ;

#endif
