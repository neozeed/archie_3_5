/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include "pserver.h"
#include "typedef.h"
#include "master.h"
#include "db_files.h"

char *get_pfs_home()

{
#ifdef __STDC__

   extern int strcasecmp(char *, char *);

#else
   
   extern int strcasecmp();
#endif

   struct passwd *passwd_ptr;
   static pathname_t home_path;

   if(home_path[0] == '\0')
      if((passwd_ptr = getpwnam(PSRV_USER_ID)) != (struct passwd *) NULL){
	 if(strcasecmp(PSRV_USER_ID, ARCHIE_USER) == 0)
	    sprintf(home_path, "%s/%s", get_archie_home(), PROSPERO_SUBDIR);
	 else
	    strcpy(home_path, passwd_ptr -> pw_dir);
      }
      else
	 sprintf(home_path, "%s/%s", get_archie_home(), PROSPERO_SUBDIR);

   return(home_path);
}
	

char *get_pfs_file(name)
   char *name;

{
   static pathname_t pfs_path;

   sprintf(pfs_path, "%s/%s", get_pfs_home(), name);

   return(pfs_path);
}


