/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#define _BSD_SOURCE

#include <unistd.h>
#include <stdio.h>
#ifdef __STDC__
#include <stdlib.h>
#endif
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dirent.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <varargs.h>
#include "archie_dbm.h"
#include "master.h"
#include "files.h"
#include "error.h"
#include "db_ops.h"
#include "db_files.h"
#include "lang_libarchie.h"
#include "archie_strings.h"
#include "db_files.h"
#include "options.h"

#define OPTIONS_FILENAME "options"

static file_info_t *option = NULL;
static option_t *option_list = NULL;
static int option_num = 0;
static int option_max = 0;


status_t open_option_file(option,name)
  file_info_t *option;
  char *name;
{

  if ( option->filename[0] == '\0' ) {
    sprintf(option->filename, "%s/%s/%s%s", get_archie_home(), DEFAULT_ETC_DIR, name, SUFFIX_OPTIONS);
  }


  if ( open_file(option,O_RDONLY ) == ERROR )  {
    error(A_ERR,"open_option_file", "Unable to open option file: %s",option->filename);
    return ERROR;
  }

  return A_OK;
}



status_t read_options(option, option_list,num,max )
  file_info_t *option;
  option_t **option_list;
  int *num, *max;
{

  char buff[100];
  option_t *tmp;
  
  while ( fgets(buff,99,option->fp_or_dbm.fp) != NULL ) {

    char **av;
    int ac;
    
    buff[strlen(buff)-1] = '\0';
    if ( ! splitwhite(buff, &ac, &av )){
      error(A_ERR,"read_options", "Unable to process line: %s",buff);
      return ERROR;
    }
                                   
    if ( av[0][0] == '#' ) /* Comment */
    continue;

    if ( ac != 3 ) {
      error(A_ERR,"read_options", "Wrong number (%d) of items on line: %s", ac, buff);
      return ERROR;
    }

    /* At this point we have the right one. */
    if ( *num == *max ) {
      *max += 10;
      if ( *max != 10 ) {
        tmp = (option_t*)realloc(*option_list, *max*sizeof(option_t));
        if ( tmp == NULL ) {
          free(*option_list);
          error(A_ERR,"find_option", "Unable to allocate more memory");
          return ERROR;
        }
        *option_list = tmp;
      }
      else {
        *option_list = (option_t*)malloc(sizeof(option_t)**max);
        if ( *option_list == NULL ) {
          error(A_ERR,"find_option", "Unable to allocate memory");
          return ERROR;
        }
      }
    }
    strcpy((*option_list)[*num].name,av[0]);
    strcpy((*option_list)[*num].type,av[1]);
    strcpy((*option_list)[*num].path,av[2]);
    (*num)++;

  }
  return A_OK;
}



status_t close_option_file(option)
  file_info_t *option;
{
  return close_file(option);
}

  

status_t get_option_path(name,type,path)
  char *name;
  char *type;
  pathname_t path;
{
  int i;
  
  if ( option == NULL ) {
    option = create_finfo();
    if ( open_option_file(option, OPTIONS_FILENAME) == ERROR ) {
      return ERROR;
    }
    if ( read_options(option, &option_list, &option_num, &option_max) == ERROR ) {


    }
    close_option_file(option);
  }

  for ( i = 0; i < option_num; i++ ){
    if (strcasecmp(option_list[i].name, name) == 0 ) {
      if ( strcasecmp(option_list[i].type, type) == 0 ) {
        strcpy(path,option_list[i].path);
        return A_OK;
      }
    }
  }

  return ERROR;
}
  


status_t get_option_list(name,list)
  char *name;
  option_t **list;
{
  int counter;
  int i;
  
  if ( option_num <= 0 ) {

    return ERROR;
  }

  for( i = 0; i < option_num; i++) {
    if ( strcasecmp(name, option_list[i].name) == 0 )
      counter++;
  }
  
}
