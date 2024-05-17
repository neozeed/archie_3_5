/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _GSTRINGS_INDEX_H_
#define _GSTRINGS_INDEX_H_

#include "gsite_file.h"


typedef struct{
    gsite_entry_ptr_t index;
    index_t	strings_offset;
} strings_idx_t;

#endif
