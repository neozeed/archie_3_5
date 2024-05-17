/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#include <errno.h>
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
#include "lang_exchange.h"
#include "files.h"
#include "times.h"
#include "master.h"
#include "protos.h"

/*
 * configfile.c: subroutines associated with reading and writing the
 * exchange subsystem
 */

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

/*
 * write_arupdate_config: write the configuration file from the internal
 * configuration information
 */

status_t write_arupdate_config(config_file, config_info, tmp_dir)
   file_info_t	*config_file;	 /* configuration file to write to */
   udb_config_t *config_info;	 /* internal configurtion information */
   char	        *tmp_dir;	 /* temporary directory name */
{
  int i;
  int hours, days, mins;
  udb_attrib_t *attrib_ptr;
  file_info_t *new_file = create_finfo();
  pathname_t tmp_string;
  pathname_t old_filename;
  char timestring[32];

  ptr_check(config_file, file_info_t, "write_arupdate_config", ERROR);
  ptr_check(config_info, udb_config_t, "write_arupdate_config", ERROR);
   
  sprintf(old_filename,"%s_old", config_file -> filename);

  sprintf(tmp_string,"%s/%s", get_archie_home(), DEFAULT_ETC_DIR);

  strcpy(new_file -> filename, get_tmp_filename(tmp_dir));

  if(open_file(new_file, O_WRONLY) == ERROR){

    /* "Can't open configuration file %s" */

    error(A_ERR, "write_arupdate_config", WRITE_ARUPDATE_CONFIG_001, new_file -> filename);
    return(ERROR);
  }

  for(i = 0; config_info[i].source_archie_hostname[0] != '\0'; i++){

    attrib_ptr = &config_info[i].db_attrib[0];       

    mins = attrib_ptr -> update_freq;

    if((days = mins / (60 * 24)) == 0){

      /* less that one day */

      if((hours = mins / 60) == 0){

        /* less than one hour */

        sprintf(timestring,"%d", mins);
      }
      else{

        if(mins % 60 == 0){

          /* integral number of hours */

          sprintf(timestring,"%dh", hours);
        }
        else{

          sprintf(timestring,"%d", mins);
        }
      }
    }
    else{

      /* more or equal to one day */

      if(mins % (60 * 24) == 0){

        /* integral number of days */

        sprintf(timestring,"%dd", days);
      }
      else{

        sprintf(timestring,"%d", mins);
      }
    }
   
      
    if(fprintf(new_file -> fp_or_dbm.fp, "%s %s %s %d %s %s %s %d",
               config_info[i].source_archie_hostname,
               attrib_ptr -> db_name,
               attrib_ptr -> domains,
               attrib_ptr -> maxno,
               attrib_ptr -> perms,
               timestring,
               cvt_from_inttime(attrib_ptr -> update_time),
               attrib_ptr -> fails) == 0){

      error(A_SYSERR, "write_arupdate_config", "Can't write updated output line for %s", 					config_info[i].source_archie_hostname);
      return(ERROR);
    }
					

    for(++attrib_ptr; attrib_ptr -> db_name[0] != '\0'; attrib_ptr++){

      if(fprintf(new_file -> fp_or_dbm.fp,"\\\n") == 0){

        error(A_SYSERR, "write_arupdate_config", "Can't write continuation line for %s", config_info[i].source_archie_hostname);
        return(ERROR);
      }
	    
	    
      mins = attrib_ptr -> update_freq;

      if(mins / (60 * 24) == 0){
        hours = mins / 60;
        days = 0;
      }
      else{
        if(mins % (60 * 24) == 0){
          days = mins / (60 * 24);
          hours = 0;
        }
        else{
          hours = mins / 60;
          days = 0;
        }
      }

      if(fprintf(new_file -> fp_or_dbm.fp, "\t\t%s %s %d %s %d%s %s %d",
                 attrib_ptr -> db_name,
                 attrib_ptr -> domains,
                 attrib_ptr -> maxno,
                 attrib_ptr -> perms,
                 hours == 0 ? days : hours,
                 hours == 0 ? "d" : "h",
                 cvt_from_inttime(attrib_ptr -> update_time),
                 attrib_ptr -> fails) == 0){
        error(A_SYSERR, "write_arupdate_config", "Can't write updated output line for %s", 					config_info[i].source_archie_hostname);
        return(ERROR);
      }

    }

    if(fprintf(new_file -> fp_or_dbm.fp, "\n") == 0){
      error(A_SYSERR, "write_arupdate_config", "Can't write newline for %s", config_info[i].source_archie_hostname);
      return(ERROR);
    }
  }

  fflush(new_file -> fp_or_dbm.fp);

   
  if(unlink(old_filename) == -1){
    if(errno != ENOENT){

      /* "Can't unlink old configuration file %s" */

      error(A_SYSERR,"write_arupdate_config", WRITE_ARUPDATE_CONFIG_002, old_filename);
      return(ERROR);
    }
  }

  if(link(config_file -> filename, old_filename) == -1){

    /* "Can't link in new configuration file %s" */

    error(A_SYSERR,"write_arupdate_config", WRITE_ARUPDATE_CONFIG_003, config_file -> filename);
    return(ERROR);
  }

  if(archie_rename(new_file -> filename, config_file -> filename) == -1){

    /* "Can't rename temporary file %s to new configuration file %s" */

    error(A_SYSERR,"write_arupdate_config", WRITE_ARUPDATE_CONFIG_004, new_file -> filename, config_file -> filename);
    return(ERROR);
  }

  close_file(new_file);

  destroy_finfo(new_file);

  close_file(config_file);

  return(A_OK);
	      
}
