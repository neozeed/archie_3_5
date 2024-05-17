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
#include "ansi_compat.h"
#include "protos.h"
#include "archie_dbm.h"
#include "typedef.h"
#include "db_files.h"
#include "domain.h"
#include "header.h"
#include "start_db.h"
#include "error.h"
#include "master.h"
#include "archie_strings.h"
#include "files.h"

extern char *start_db_filename();

#define HOST_TABLE_EXTRA 5

typedef struct {
  ip_addr_t ipaddr;
  hostname_t hostname;
  int port;
} host_table_t;


/* Table containing host ip addresses */
static host_table_t *host_table = NULL;
static int host_table_num = 0;
static int host_table_max = 0;


status_t read_host_table()
{
  struct stat st;
  file_info_t *config_file = create_finfo();


  if(config_file -> filename[0] == '\0'){
      if(config_file -> filename[0] == '\0' )
         strcpy(config_file -> filename, start_db_filename( DEFAULT_START_TABLE ));
  }

  if ( open_file(config_file,O_RDONLY) == ERROR ) {
    return ERROR;
  }

  if ( stat(config_file->filename,&st) == -1 ) {
    return ERROR;
  }

  if ( host_table != NULL )
    free(host_table);

  host_table_num = st.st_size/sizeof(host_table_t);
  host_table_max =  host_table_num + HOST_TABLE_EXTRA;
  host_table = (host_table_t*)malloc(sizeof(host_table_t)*host_table_num);

  if ( host_table == NULL ) {
    return ERROR;
  }

  if ( fread(host_table,sizeof(host_table_t),host_table_num,config_file->fp_or_dbm.fp) != host_table_num) {
    close_file(config_file);
    destroy_finfo(config_file);
    return ERROR;
  } 

  close_file(config_file);
  destroy_finfo(config_file);
  return A_OK;
  
}



status_t write_host_table()
{
  file_info_t *config_file = create_finfo();


  if(config_file -> filename[0] == '\0'){
      if(config_file -> filename[0] == '\0' )
         strcpy(config_file -> filename, start_db_filename( DEFAULT_START_TABLE ));
  }

  if ( open_file(config_file,O_RDONLY) == ERROR ) {
    return ERROR;
  }

  open_file(config_file,O_WRONLY);
  
  if ( fwrite(host_table,sizeof(host_table_t),host_table_num,config_file->fp_or_dbm.fp) != host_table_num ) {
    return ERROR;
  }

  close_file(config_file);
  destroy_finfo(config_file);
  return A_OK;
  
}


status_t host_table_find(ipaddr,hostname,port, index )
ip_addr_t *ipaddr;
hostname_t hostname;
int *port;
host_table_index_t *index;
{

  int i;

  if (!(*port)){
    error(A_ERR,"host_table_find","Did not supply the port number.");
    return ERROR;
  }

  if (( *ipaddr != 0 )  && ( hostname[0] != '\0'  )) {
    for ( i = 0; i < host_table_num; i++ ) {
      if ( (host_table[i].ipaddr == *ipaddr)&&(host_table[i].port == *port) ) {
        *index = i;
        return A_OK;
      }
 if ( !strcasecmp(host_table[i].hostname,hostname)&&(host_table[i].port == *port) ) {
        *index = i;
        return A_OK;
      }
    }
    return ERROR;
  } 

  if ( *ipaddr != 0 ) {
    for ( i = 0; i < host_table_num; i++ ) {
      if ((host_table[i].ipaddr == *ipaddr )&&(host_table[i].port == *port)) {
        *index = i;
        return A_OK;
      }
    }
    return ERROR;
  }


  if ( hostname[0] != '\0'  ) {
    for ( i = 0; i < host_table_num; i++ ) {
      if ( !strcasecmp(host_table[i].hostname,hostname)&&(host_table[i].port == *port)) {
        *index = i;
        return A_OK;
      }
    }
    return ERROR;
  }

  if ( *index != -1 ) {
    *ipaddr = host_table[*index].ipaddr;
    *port = host_table[*index].port;
    return A_OK;
  }

  return ERROR;
}


status_t host_table_add(ipaddr,hostname,port, index )
ip_addr_t ipaddr;
hostname_t hostname;
int port;
host_table_index_t *index;
{

  int i;

  if ( host_table_find(&ipaddr, hostname, &port, index) == A_OK )
    return A_OK;
  
  for ( i = 0; i < host_table_num; i++ ) {
    if ( host_table[i].ipaddr == 0 ) {
      *index = i;
      host_table[i].ipaddr = ipaddr;
      strcpy(host_table[i].hostname,hostname);
      host_table[i].port = port;
      return A_OK;
    }
  }
  
  if ( host_table_num == host_table_max ) {
    host_table_t *tmp;
    
    host_table_max += HOST_TABLE_EXTRA;
    tmp = (host_table_t*) malloc(host_table_max*sizeof(host_table_t));
    if ( tmp == NULL ) {
      return ERROR;
    }

    if ( host_table != NULL ) {
      memcpy(tmp,host_table,host_table_num*sizeof(host_table_t));      
      free(host_table);
    }
    
    host_table = tmp;
  }
  
  host_table[i].ipaddr = ipaddr;
  strcpy(host_table[i].hostname,hostname);
  host_table[i].port = port;
  *index = i;
  host_table_num++;

  return A_OK;
}


status_t host_table_del(index)
host_table_index_t index;
{
  
  host_table[index].ipaddr = 0;
  host_table[index].hostname[0] = '\0';
  host_table[index].port = 0;
  return A_OK;

}


int host_table_cmp(ind1,ind2)
host_table_index_t ind1,ind2;
{
  int p1,p2;
  
  p1 = domain_priority(host_table[ind1].hostname);
  
  p2 = domain_priority(host_table[ind2].hostname);

  /* Note that if p? == -1 it means not listed .. so goes last */

  if ( p1 == -1 ) {
    if ( p2 == -1 )
    return -1;
    else
    return 1;
  }

  if ( p2 == -1 ) {
    return -1;
  }

  return p1-p2;
  
}







