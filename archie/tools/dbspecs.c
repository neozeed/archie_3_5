/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <curses.h>
#include <stdlib.h>
#include "protos.h"
#include "host_manage.h"
#include "error.h"
#include "files.h"
#include "archie_strings.h"
#include "lang_tools.h"

#ifdef __svr4__
#ifdef strrchr
#undef strrchr
#endif
#endif


/*
 * read_dbspec: read the configuration file for the access commands display
 * configuration and put them into dbspec_list
 */


status_t read_dbspecs(dbspec_config, dbspec_list)
   file_info_t *dbspec_config;	    /* configuration file */
   dbspec_t *dbspec_list;	    /* resulting list of config specs */

{

   char input_buf[BUFSIZ];
   char input_str[BUFSIZ];
   char dbstring[BUFSIZ];
   dbspec_t *dbspec;
   dbfield_t *dbfield;
   char **field_list, **field_ptr;
   char **subfield_list;
   int  count = 0;


   ptr_check(dbspec_config, file_info_t, "read_dbspecs", ERROR);
   ptr_check(dbspec_list, dbspec_t, "read_dbspecs", ERROR);   


   input_str[0] = input_buf[0] = '\0';

   dbspec = dbspec_list;

   dbfield = &(dbspec -> subfield[0]);
   dbfield -> colno = 0;
   dbfield -> lineno = LCUR_DBSPEC;

   strcpy(dbspec -> dbspec_name, DEF_TEMPLATE_NAME);

   strcpy(dbfield -> field_name, DEF_TEMPLATE_FNAME);
   dbfield -> max_field_width = COLS - strlen(DEF_TEMPLATE_FNAME);
   dbfield -> writable = 1;

   if(open_file(dbspec_config, O_RDONLY) == ERROR){

      /* "Can't open configuration file %s" */

      error(A_WARN, "read_dbspecs", READ_DBSPECS_001, dbspec_config -> filename);
      return(A_OK);
   }


   dbspec = &dbspec_list[1];

   while(fgets(input_buf, sizeof(input_buf), dbspec_config -> fp_or_dbm.fp) != (char *) NULL){

      strcat(input_str, input_buf);
      input_str[strlen(input_str) - 1] = '\0';

      if(strrchr(input_buf,DBSPECS_END) == (char *) NULL)
         continue;
      else{

	 sscanf(input_str,"%s {%[^}]}", dbspec -> dbspec_name, dbstring);

	 
	 field_list = str_sep(dbstring, NET_DELIM_CHAR);

	 for(count = 0, field_ptr = field_list, dbfield = &(dbspec -> subfield[0]); 
	    *field_ptr != (char *) NULL;
	    field_ptr++, dbfield++){


	    if(count >= MAX_NO_FIELDS){
	       memset(dbspec, '\0', sizeof(dbspec_t));
       	       strcpy(dbspec -> dbspec_name, DEF_TEMPLATE_NAME);
	       return(A_OK);
	    }

	    subfield_list = str_sep(*field_ptr, NET_SEPARATOR_CHAR);

	     /* scanf instead of strcpy gets rid of whitespace */

	    sscanf(subfield_list[0],"%s",dbfield -> field_name);

	    input_str[0] = input_buf[0] = '\0';

	    dbfield -> max_field_width = atoi(subfield_list[1]);

	    if(subfield_list[2] != (char *) NULL)
	       sscanf(subfield_list[2],"%s",dbfield -> def_str);
	    else
	       dbfield -> def_str[0] = '\0';

	    if(subfield_list[3] != (char *) NULL){

	       if(subfield_list[3][0] == 'W')
		  dbfield -> writable = 1;
		else
		  dbfield -> writable = 0;
	    }
	    else
	       dbfield -> writable = 0;

	 count++;

	 }

	 dbspec++;
	 dbspec -> dbspec_name[0] = '\0';

      }
   }


   return(A_OK);
}

/*
 * get_new_acommand: lookup the given database name "name" in the
 * dbspec_list and return the associated new access_command for a header
 */


status_t get_new_acommand(name, str, dbspec_list)
   char *name;
   char *str;
   dbspec_t *dbspec_list;
{
#if 0
#ifdef __STDC__

   extern int strcasecmp(char *, char *);

#else

   extern int strcasecmp();

#endif
#endif
   dbspec_t *dbspec;
   int finished;
   access_comm_t access_buf;
   access_comm_t tmp_buf;
   dbfield_t *dbfield;
   char **n_list = (char **) NULL;
   char **p_list = (char **) NULL;

   ptr_check(name, char, "get_new_acommand", ERROR);
   ptr_check(str, char, "get_new_acommand", ERROR);
   ptr_check(dbspec_list, dbspec_t, "get_new_acommand", ERROR);

   for(dbspec = dbspec_list, finished = FALSE;
       !finished && dbspec -> dbspec_name[0] != '\0';){

      if(strcasecmp(dbspec -> dbspec_name, name) == 0)
	 finished = TRUE;
      else
	 dbspec++;
   }

   if(!finished)
      dbspec = dbspec_list;

   dbfield = &(dbspec -> subfield[0]);

   tmp_buf[0] = access_buf[0] = '\0';

   n_list = str_sep(str, NET_DELIM_CHAR);
   p_list = n_list;

   for(finished = FALSE; !finished && (dbfield -> field_name[0] != '\0');dbfield++, p_list++){

      if(strcasecmp(dbfield -> curr_val, dbfield -> def_str) == 0)
	sprintf(access_buf,":");
     else
        if(dbfield -> curr_val[0] != '\0')
	   sprintf(access_buf,"%s:",dbfield -> curr_val);

      strcat(tmp_buf, access_buf);

   }

   tmp_buf[strlen(tmp_buf) - 1] = '\0';

   free_opts(n_list);

   strcpy(str, tmp_buf);

   return(A_OK);
}
