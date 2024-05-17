/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef SOLARIS22
#include <sys/utsname.h>
#endif
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "typedef.h"
#include "db_files.h"
#include "error.h"
#include "lang_libarchie.h"
#include "files.h"

#include "protos.h"

/*
 * Routines for determining where the archie system and its databases
 * reside
 */


/*
 * get_archie_home: look up the password entry for the ARCHIE_USER
 */

/* BUG: This should be changed to also take a parameter returning the
   same value*/

char *get_archie_home()
{
   struct passwd *passwd_ptr;
   static pathname_t home_path;

   if(home_path[0] == '\0')  {
     char *env_var;
     
     if ( (env_var =getenv("ARCH_USER")) == NULL ) {
       env_var = ARCHIE_USER;
     }
     if((passwd_ptr = getpwnam(env_var)) != (struct passwd *) NULL){
       strcpy(home_path, passwd_ptr -> pw_dir);
     }
     else
     strcpy(home_path,".");
   }
   return(home_path);
}
	

/*
 * set_master_db_dir: Set the master database directory. If the argument is
 * a NULL pointer then copy name and return it. If pointer points to NULL
 * string then get archie home directory and construct default master
 * database name, otherwise set master database name to given string.
 */



char *set_master_db_dir(master_db_dir)
  char *master_db_dir; /* Full pathname of master database directory */
{
   static pathname_t master_database_dir;
   static pathname_t master_dir_copy;

   struct stat statbuf;	   /* checking directory status */


   /* Return pointer to copy */

   if(master_db_dir == (char *) NULL){

      strcpy(master_dir_copy, master_database_dir);
      return(master_dir_copy);
   }

  if(master_db_dir[0] != '\0')
     strcpy(master_database_dir, master_db_dir);
  else{

     if(get_archie_home() != (char *) NULL)
	sprintf(master_database_dir,"%s/%s", get_archie_home(), DEFAULT_MASTER_DB_DIR);
     else
        strcpy(master_database_dir, DEFAULT_MASTER_DB_DIR);
  }

  if(stat(master_database_dir, &statbuf) == -1){

     /* "Can't access dirctory %s" */

     error(A_SYSERR,"set_master_db_dir",SET_MASTER_DB_DIR_001, master_database_dir);
     return((char *) NULL);
  }

  if(!S_ISDIR(statbuf.st_mode)){

     /* "%s is not a directory" */

     error(A_ERR,"set_master_db_dir", SET_MASTER_DB_DIR_002, master_database_dir);
     return((char *) NULL);
  }

  return(master_database_dir);
}


/*
 * get_master_db_dir: Get the name of the master database directory
 */


char *get_master_db_dir()
{
   return(set_master_db_dir((char *) NULL));
}


/*
 * master_db_filename: construct the full pathname of "filename" as a master
 * database file
 */


char *master_db_filename(filename)
   char *filename;
{
   static pathname_t tmp_db_filename;

   sprintf(tmp_db_filename,"%s/%s",get_master_db_dir(),filename);

   return(tmp_db_filename);
}


char *get_archie_hostname(host_string, stringlen)
   char *host_string;
   int stringlen;
{
   file_info_t *host_file = create_finfo();
   pathname_t inbuf;
   char *comment;
   pathname_t work_host;
   static hostname_t inthost;

#ifdef SOLARIS22
   struct utsname un;
#endif   

   sprintf(host_file -> filename, "%s/%s/%s", get_archie_home(), DEFAULT_ETC_DIR, ARCHIE_HOSTNAME_FILE);

   if(access(host_file -> filename, R_OK | F_OK) == -1){

#ifndef SOLARIS22
      if(gethostname(inthost, sizeof(inthost)) == -1){

	 if(host_string != (char *) NULL)
	    host_string[0] = '\0';

	 return((char *) NULL);
      }
#else
      if(uname(&un) == -1){

	 if(host_string != (char *) NULL)
	    host_string[0] = '\0';

	 return((char *) NULL);
      }

      strcpy(inthost, un.nodename);
#endif      

      if(host_string != (char *) NULL){
         strcpy(host_string, inthost);
         return(host_string);
      }
      else
	 return(inthost);
   }

   if(open_file(host_file, O_RDONLY) == ERROR){

      host_string[0] = '\0';
      return((char *) NULL);
   }

   while(1){

      if(fgets(inbuf, sizeof(inbuf), host_file -> fp_or_dbm.fp) != inbuf){

	 /* EOF */
	 close_file(host_file);
	 if(host_string != (char *) NULL)
	    host_string[0] = '\0';
	 return((char *) NULL);
      }

      if((comment = strchr(inbuf, COMMENT_CHAR)) != (char *) NULL)
	 *comment = '\0';

      if(sscanf(inbuf, "%s", work_host ) == 1){

	 /* found a string */

	 if(strlen(work_host) >= stringlen){

	    /* if the hostname given is longer than the buffer to be
	       returned, fail */

	    close_file(host_file);
	    if(host_string != (char *) NULL)
	       host_string[0] = '\0';

	    return((char *) NULL);
	 }

	 close_file(host_file);
	 if(host_string != (char *) NULL){
	    strcpy(host_string, work_host);
	    return(host_string);
	 }
	 else{
	    inthost[0] = '\0';
	    return(inthost);
	 }
      }
   }

}
