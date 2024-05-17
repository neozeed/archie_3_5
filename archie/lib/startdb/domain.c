/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */



#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include "archie_dbm.h"
#include "typedef.h"
#include "db_files.h"
#include "domain.h"
#include "header.h"
#include "start_db.h"
#include "error.h"
#include "lang_startdb.h"
#include "master.h"
#include "archie_strings.h"
#include "files.h"

#include "protos.h"



typedef struct {
  domain_t domain_list[MAX_NO_DOMAINS];
  int count;
} domain_table_t;

/* Table containing host ip addresses */
static domain_table_t *domain_table = NULL;
static int domain_table_num = 0;
static int domain_table_max = 0;
static int domain_separator = -1;

extern int errno;

#define DOMAIN_TABLE_EXTRA 10
#define DEFAULT_DOMAIN_ORDER "domain.order"



static int empty_line( buff )
  char *buff;
{

  while( *buff != '\0' ) {
    if ( *buff  == '#' )
       return 1;
    if ( !isspace(*buff) )
      return 0;

    buff++;
  }

  return 1;
}



static int separator_line( buff )
  char *buff;
{

  while( *buff != '\0' ) {
    if ( !isspace(*buff) )
      if ( *buff == '*' )
        return 1;
      else
        return 0;

    buff++;
  }

  return 0;
}


status_t read_domain_table(domaindb)
file_info_t *domaindb;
{
  struct stat st;
  char buff[1024];
  file_info_t *config_file = create_finfo();

  if(config_file -> filename[0] == '\0'){
    sprintf(config_file -> filename, "%s/%s/%s", get_archie_home(), DEFAULT_ETC_DIR, DEFAULT_DOMAIN_ORDER);
  }

  if ( stat(config_file->filename,&st) == -1 ) {
    if ( errno == ENOENT ) { /* No such file */
      domain_table_num = 0;
      destroy_finfo(config_file);
      return A_OK;
    }
    else {
      destroy_finfo(config_file);
      return ERROR;
    }
  }

  if ( open_file(config_file,O_RDONLY) == ERROR ) {
    destroy_finfo(config_file);
    return ERROR;
  }

  
  while (  fgets(buff,1024,config_file->fp_or_dbm.fp) != NULL ) {

    if ( empty_line(buff) ) { /* Check if it is an empty line */
      continue;
    }
    
    
    if ( separator_line(buff) ) {
      if ( domain_separator != -1 ) {
        error(A_WARN,"read_domain_table","Multiple separator lines present in the domain order file");
      }
      else
        domain_separator = domain_table_num;
    }
    
    if ( domain_table_num == domain_table_max ) {
      domain_table_t *tmp;
      domain_table_max += DOMAIN_TABLE_EXTRA;
      tmp = (domain_table_t*) malloc(sizeof(domain_table_t)*domain_table_max);
      if ( tmp == NULL ) {
        close_file(config_file);
        destroy_finfo(config_file);
        return ERROR;
      }

      if ( domain_table ) {
        memcpy(tmp,domain_table,sizeof(domain_table_t)*domain_table_num);  
        free(domain_table);
      }
      domain_table = tmp;
    }

    if ( domain_separator == domain_table_num ) { /* This is a separator line*/
      domain_table[domain_table_num].count = 0;
      domain_table_num++;
      continue;
    }
    
    buff[strlen(buff)-1] ='\0';
    if ( compile_domains(buff,domain_table[domain_table_num].domain_list,
                    domaindb, &(domain_table[domain_table_num].count)) == ERROR) {
      close_file(config_file);
      destroy_finfo(config_file);
      return ERROR;
    }
    domain_table_num++;
  }

  if ( domain_separator == -1 )
    domain_separator = domain_table_num;
  
  close_file(config_file);
  destroy_finfo(config_file);
  
  
  return A_OK;
  
}
                  



int domain_priority(hostname)
hostname_t hostname;
{
  int i,j;
  int lh, ld;

/*  domain = strrchr(hostname,'.');
  if ( domain == NULL ) {
    domain = hostname;
  }
  else domain++;
  */
  
  lh = strlen(hostname);
  for ( i = 0; i < domain_table_num; i++ ) {
    for ( j = 0; j < domain_table[i].count; j++ ) {

      ld = strlen(domain_table[i].domain_list[j]);
      if ( ld <= lh ) {
        if ( strcasecmp(domain_table[i].domain_list[j],hostname+(lh-ld)) == 0 )
        return i-domain_separator;
      }
    }
  }

  return 0;

}

#if 0
void clear_domain_table()
{

  if ( domain_table_num ) {
    memset(domain_table,0,sizeof(domain_table_t)*domain_table_max);
    domain_table_num = 0;
  }
  
}


#endif
