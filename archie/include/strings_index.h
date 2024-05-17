/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _STRINGS_INDEX_H_
#define _STRINGS_INDEX_H_

#ifdef OLD_FILES
#  include "old-site_file.h"
#else
#  include "site_file.h"
#endif


typedef struct{
    site_entry_ptr_t index;
    index_t	strings_offset;
} strings_idx_t;

#endif
