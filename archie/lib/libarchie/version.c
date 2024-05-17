/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#include <stdio.h>
#include "typedef.h"

/* This number has to be changed for upgrades */


static char *version_number = "3.2beta";

char *ar_version()
{
   static pathname_t version_str;

   if(version_str[0] == '\0')
      sprintf(version_str, "archie %s;  %s", version_number, COMPILED_DATE);

   return(version_str);
}
