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

extern int errno;

#define DOMAIN_TABLE_EXTRA 10
#define DEFAULT_DOMAIN_ORDER "domain.order"

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
      return A_OK;
    }
    else {
      return ERROR;
    }
  }

  if ( open_file(config_file,O_RDONLY) == ERROR ) {
    return ERROR;
  }

  
  while (  fgets(buff,1024,config_file->fp_or_dbm.fp) != NULL ) {
    if ( domain_table_num == domain_table_max ) {
      domain_table_t *tmp;
      domain_table_max += DOMAIN_TABLE_EXTRA;
      tmp = (domain_table_t*) malloc(sizeof(domain_table_t)*domain_table_max);
      if ( tmp == NULL ) {
        close_file(config_file);
        return ERROR;
      }

      if ( domain_table ) {
        memcpy(tmp,domain_table,sizeof(domain_table_t)*domain_table_num);  
        free(domain_table);
      }
      domain_table = tmp;
    }
    buff[strlen(buff)-1] ='\0';
    if ( compile_domains(buff,domain_table[domain_table_num].domain_list,
                    domaindb, &(domain_table[domain_table_num].count)) == ERROR) {
      close_file(config_file);
      return ERROR;
    }
    domain_table_num++;
  }

  close_file(config_file);
  return A_OK;
  
}
                  



int domain_priority(hostname)
hostname_t hostname;
{
  int i,j;
  char *domain;

  domain = strrchr(hostname,'.');
  if ( domain == NULL ) {
    domain = hostname;
  }
  else domain++;
  
  
  
  for ( i = 0; i < domain_table_num; i++ ) {
    for ( j = 0; j < domain_table[i].count; j++ ) {
      if ( strcasecmp(domain_table[i].domain_list[j],domain) == 0 )
        return i;
    }
  }

  return -1;

}
