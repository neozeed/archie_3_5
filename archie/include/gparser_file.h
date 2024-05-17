/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _GPARSER_FILE_H_
#define _GPARSER_FILE_H_
#include "defines.h"
#include "gcore_entry.h"



/*
    Used by anything producing input for 'insert'.  It would be followed by
    the string whose length is given by 'slen'.
*/


typedef struct
{
    gcore_site_entry_t core;
    strlen_t user_len;
    strlen_t sel_len;
    strlen_t host_len;

    /*
     * rest of record consists of 4-byte boundary aligned, padded, variable
     * length string
     */

} gparser_entry_t;

#endif
