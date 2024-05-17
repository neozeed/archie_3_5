/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _PARSER_FILE_H
#define _PARSER_FILE_H
#include "defines.h"
#include "core_entry.h"



/*
    Used by anything producing input for 'insert'.  It would be followed by
    the string whose length is given by 'slen'.
*/


typedef struct
{
    core_site_entry_t core;
    strlen_t slen;

    /*
     * rest of record consists of 4-byte boundary aligned, padded, variable
     * length string
     */

} parser_entry_t;

#endif
