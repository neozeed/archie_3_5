/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(AIX) || defined (SOLARIS)
#include <unistd.h>
#endif
#include "protos.h"
#include "typedef.h"
#include "db_files.h"
#include "host_db.h"
#include "listd.h"
#include "error.h"
#include "files.h"
#include "times.h"
#include "master.h"
#include "protos.h"


char*	 READ_ARUPDATE_CONFIG_001= "Can't open configuration file %s";
char*	 READ_ARUPDATE_CONFIG_002= "\nLine %u: Duplicate entry from archie host '%s'. Ignoring.";
char*	 READ_ARUPDATE_CONFIG_003= "Line %u: Can't read database domains";
char*	 READ_ARUPDATE_CONFIG_004= "Line %u: Can't read maximum number of retrieval entries";
char*	 READ_ARUPDATE_CONFIG_005= "Line %u: Can't read database permissions";
char*	 READ_ARUPDATE_CONFIG_006= "Line %u: Can't read database frequency";
char*	 READ_ARUPDATE_CONFIG_007= "Line %u: Can't read database update time";
char*	 READ_ARUPDATE_CONFIG_008= "Line %u: Can't read database failure count";
char*	 READ_ARUPDATE_CONFIG_009= "Duplicate entry for database '%s' in archie host '%s'";
char*	 READ_ARUPDATE_CONFIG_010= "Empty configuration file %s";



/*
 * compare_attrib: comparison function for the udb_attrib_t type based on
 * the database name component
 */


int compare_attrib(a, b)
   udb_attrib_t *a;
   udb_attrib_t *b;

{
#if 0
#ifdef __STDC__

   extern int strcasecmp(char *, char *);

#else

   extern int strcasecmp();

#endif
#endif


   return(strcasecmp( a -> db_name, b -> db_name));
}


/*
 * read_arupdate_config: read the <config_file> and place the processed
 * information into the udb_config_t array
 */


status_t read_arupdate_config(config_file, config_info)
   file_info_t	*config_file;	 /* handle of the configuration file */
   udb_config_t *config_info;	 /* processed information from file */

{

#if 0
#ifdef __STDC__

   extern int strcasecmp(char *, char *);

#else

   extern int strcasecmp();

#endif
#endif
   char input_buf[1024];
   char tmp_buf[1024];
   char tmp_buf2[32];
   char tmp_buf3[32];         
   char *iptr;
   char *tmp_ptr;
   char *hold_ptr;
   char *end_ptr;      
   udb_attrib_t *attrib_ptr;

   int	count, lineno;
   int dbcount, i;
   int duplicate = 0;


   ptr_check(config_file, file_info_t, "read_arupdate_config", ERROR);
   ptr_check(config_info, udb_config_t, "read_arupdate_config", ERROR);
   

   if(open_file(config_file, O_RDWR) == ERROR){

      /* "Can't open configuration file %s" */
      error(A_ERR, "read_arupdate_config", READ_ARUPDATE_CONFIG_001, config_file -> filename);
      return(ERROR);
   }
   
   for(iptr = input_buf, lineno = 1, count = 0;
       fgets(iptr, BUFSIZ, config_file -> fp_or_dbm.fp) != (char *) NULL;
       lineno++){

      /* Handle continuation lines */

      if((tmp_ptr = strstr(iptr, CONTINUATION_LINE)) != (char *) NULL){
         *tmp_ptr = '\0';
	 iptr = tmp_ptr;
	 continue;
      }
         
      /* Terminate string at comment character */

      if((tmp_ptr = strchr(input_buf, COMMENT_CHAR)) != (char *) NULL)
         *tmp_ptr = '\0';

      /* Get rid of the newline */

      if((tmp_ptr = strrchr(input_buf, '\n')) != (char *) NULL)
         *tmp_ptr = '\0';

      /* Parse the input line */

      /* Check for a blank line */

      if(sscanf(input_buf,"%s",config_info[count].source_archie_hostname) != 1)
         continue;

#if 0
      for(i=0; i < count - 1; i++){

	 if(strcasecmp(config_info[count].source_archie_hostname, config_info[i].source_archie_hostname) == 0){

	    /* "\nLine %u: Duplicate entry from archie host '%s'. Ignoring." */

	    error(A_ERR,"read_arupdate_config", READ_ARUPDATE_CONFIG_002, lineno, config_info[i].source_archie_hostname);
	    config_info[count].source_archie_hostname[0] = '\0';
	    duplicate = 1;
	    break;
	 }
      }

      if(duplicate == 1){
         duplicate = 0;
         continue;
      }
#endif

      tmp_ptr = input_buf + strlen(config_info[count].source_archie_hostname);

      attrib_ptr = &config_info[count].db_attrib[0];

      for(dbcount = 0;
          sscanf(tmp_ptr,"%[ \t]%s",tmp_buf2,attrib_ptr[dbcount].db_name) == 2;
	  dbcount++){


	 tmp_ptr += strlen(attrib_ptr[dbcount].db_name) + strlen(tmp_buf2) + 1;

	 attrib_ptr[dbcount+1].db_name[0] = '\0';

         tmp_buf[0] = '\0';
 	 if((sscanf(tmp_ptr,"%[ \t]%s", tmp_buf, attrib_ptr[dbcount].domains) != 2) &&
	    (sscanf(tmp_ptr,"%s", attrib_ptr[dbcount].domains) != 1)){

	    /* "Line %u: Can't read database domains" */

	    error(A_ERR,"read_arupdate_config", READ_ARUPDATE_CONFIG_003, lineno);
	    return(ERROR);
	 }

	 tmp_ptr += strlen(tmp_buf) + strlen(attrib_ptr[dbcount].domains) + 1;


         tmp_buf[0] = '\0';
 	 if((sscanf(tmp_ptr,"%[ \t]%s", tmp_buf, tmp_buf2) != 2) &&
	    (sscanf(tmp_ptr,"%s", tmp_buf2) != 1)){

	    /* "Line %u: Can't read maximum number of retrieval entries" */

	    error(A_ERR,"read_arupdate_config", READ_ARUPDATE_CONFIG_004, lineno);
	    return(ERROR);
	 }

	 attrib_ptr[dbcount].maxno = atoi(tmp_buf2);
         tmp_ptr += strlen(tmp_buf) + strlen(tmp_buf2) + 1;

	 tmp_buf[0] = '\0';
 	 if((sscanf(tmp_ptr,"%[ \t]%[-rw]", tmp_buf, attrib_ptr[dbcount].perms) != 2) &&
	    (sscanf(tmp_ptr,"%[-rw]", attrib_ptr[dbcount].perms) != 1)){

	    /* "Line %u: Can't read database permissions" */

	    error(A_ERR,"read_arupdate_config", READ_ARUPDATE_CONFIG_005, lineno);
	    return(ERROR);
	 }

	 tmp_ptr += strlen(tmp_buf) + strlen(attrib_ptr[dbcount].perms) + 1;

	 tmp_buf[0] = '\0';
	 tmp_buf3[0] = '\0';
         if((sscanf(tmp_ptr,"%[0-9]%[dhm]", tmp_buf2, tmp_buf3) != 2) &&
            (sscanf(tmp_ptr,"%[ \t]%[0-9]%[dhm]", tmp_buf, tmp_buf2, tmp_buf3) != 3) &&
	    (sscanf(tmp_ptr,"%[0-9]", tmp_buf2) != 1) &&
            (sscanf(tmp_ptr,"%[ \t]%[0-9]", tmp_buf, tmp_buf2) != 2)){	

	    /* "Line %u: Can't read database frequency" */

	    error(A_ERR,"read_arupdate_config", READ_ARUPDATE_CONFIG_006, lineno);
	    return(ERROR);
	 }

	 attrib_ptr[dbcount].update_freq = atol(tmp_buf2);
	  
	 if(tmp_buf3[0] == 'h')
	   attrib_ptr[dbcount].update_freq *= 60;
	 else if(tmp_buf3[0] == 'd')
	   attrib_ptr[dbcount].update_freq *= 60 * 24;
       
	 tmp_ptr += strlen(tmp_buf2) + strlen(tmp_buf3) + strlen(tmp_buf) + 1;

	 tmp_buf[0] = '\0';
 	 if((sscanf(tmp_ptr,"%[ \t]%14[0-9]", tmp_buf, tmp_buf3) != 2) &&
	    (sscanf(tmp_ptr,"%14[0-9]", tmp_buf3) != 1)){

	    /* "Line %u: Can't read database update time" */

	    error(A_ERR,"read_arupdate_config",READ_ARUPDATE_CONFIG_007, lineno);
	    return(ERROR);
	 }
	  
	 attrib_ptr[dbcount].update_time = (date_time_t) cvt_to_inttime(tmp_buf3, 0);

	 tmp_ptr += strlen(tmp_buf) +strlen(tmp_buf3) + 1;

	 tmp_buf[0] = '\0';
 	 if((sscanf(tmp_ptr,"%[ \t]%1[0-9]", tmp_buf, tmp_buf3) != 2) &&
	    (sscanf(tmp_ptr,"%1[0-9]", tmp_buf3) != 1)){

	    /* "Line %u: Can't read database failure count" */

	    error(A_ERR,"read_arupdate_config", READ_ARUPDATE_CONFIG_008, lineno);
	    return(ERROR);
	 }
	  
	 attrib_ptr[dbcount].fails = atoi(tmp_buf3);

	 tmp_ptr += strlen(tmp_buf) +strlen(tmp_buf3) + 1;

	 /* unbundle multiple database names */

	 strcpy(tmp_buf, attrib_ptr[dbcount].db_name);

	 if((hold_ptr = strchr(tmp_buf, NET_DELIM_CHAR)) != (char *) NULL){

  	    end_ptr = tmp_buf + strlen(tmp_buf) + 1;

	    *hold_ptr = '\0';
	    strcpy(attrib_ptr[dbcount].db_name, tmp_buf);
	    i = dbcount;

	    for(++hold_ptr, dbcount;
	        (sscanf(hold_ptr,"%[^:]:",tmp_buf2) != EOF) &&
		(hold_ptr < end_ptr);){

	       dbcount++;
	       strcpy(attrib_ptr[dbcount].db_name, tmp_buf2);
	       strcpy(attrib_ptr[dbcount].domains, attrib_ptr[i].domains);
	       strcpy(attrib_ptr[dbcount].perms, attrib_ptr[i].perms);
	       attrib_ptr[dbcount].update_freq = attrib_ptr[i].update_freq;
	       attrib_ptr[dbcount].update_time = attrib_ptr[i].update_time;
	
	       hold_ptr += strlen(tmp_buf2) + 1;
	    }
	 }

#if 0
	 for(j = 0; j < dbcount -1; j++){
             if(strcasecmp(attrib_ptr[dbcount].db_name, attrib_ptr[j].db_name) == 0){

	       /* "Duplicate entry for database '%s' in archie host '%s'" */

	       error(A_ERR, "read_arupdate_config",READ_ARUPDATE_CONFIG_009,  attrib_ptr[j].db_name, config_info[count].source_archie_hostname);
	       duplicate = 1;
	     }
	 }
#endif
      }

      if(duplicate)
         return(ERROR);

      qsort(attrib_ptr, dbcount, sizeof(udb_attrib_t), compare_attrib);

      if((strcmp(attrib_ptr[0].db_name, DATABASES_ALL) == 0)
         && dbcount > 0){

	 attrib_ptr[1].db_name[0] = '\0';
         }

      iptr = input_buf;
      count++;

   }

   if(count == 0){

       error(A_ERR, "read_arupdate_config",READ_ARUPDATE_CONFIG_010, config_file -> filename);
       return(ERROR);
   }

   return(A_OK);

}
