/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */



#include <malloc.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include "protos.h"
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
static short *priority_table = NULL;

extern int errno;

status_t read_host_table()
{
  struct stat st;
  int ret;
  file_info_t *config_file = create_finfo();

/*  error(A_INFO,"read_host_table","Entering read_host_table()"); */
  strcpy(config_file -> filename, start_db_filename( DEFAULT_START_TABLE ));


/*  
  if ( access(config_file->filename, R_OK) == -1 ) {
    error(A_INFO,"read_host_table","Cannot acees %s",config_file->filename);
    if ( open_file(config_file,O_WRONLY) == ERROR ) {
      error(A_ERR,"read_host_table","Error opening file %s",config_file->filename);      
      return ERROR;
    }    
    close_file(config_file);
  }
*/

  if ( stat(config_file->filename,&st) == -1 ) {
    if ( errno == ENOENT ) {
      error(A_ERR,"read_host_table","start_table %s not present assuming no entries",config_file->filename);
      
      if ( host_table != NULL )
        free(host_table);      
      host_table_num = 0;
      host_table_max = 0;
      host_table = NULL;
      return A_OK;
    }
    error(A_ERR,"read_host_table","Unable to stat file %s, errno = %d",config_file->filename,errno);

    return ERROR;
  }
  
  if ( open_file(config_file,O_RDONLY) == ERROR ) {
    error(A_ERR,"read_host_table","Error opening file %s",config_file->filename);
    return ERROR;
  }

  if ( host_table != NULL )
    free(host_table);

  host_table_num = st.st_size/sizeof(host_table_t);
  host_table_max =  host_table_num + HOST_TABLE_EXTRA;
  host_table = (host_table_t*)malloc(sizeof(host_table_t)*host_table_max);

/*  error(A_INFO,"read_host_table","Host table contains %d elements",host_table_num); */

  if ( host_table == NULL ) {
    error(A_ERR,"read_host_table","Unable allocate memory for host_table");
    return ERROR;
  }

  if ( (ret = fread(host_table,sizeof(host_table_t),host_table_num,config_file->fp_or_dbm.fp)) != host_table_num) {
    close_file(config_file);
    destroy_finfo(config_file);
    error(A_ERR,"read_host_table","fread() failed, returned value %d",ret);
    return ERROR;
  } 

  close_file(config_file);
  destroy_finfo(config_file);
/*  error(A_INFO,"read_host_table","Exiting read_host_table() successfully"); */
  return A_OK;
  
}



status_t write_host_table()
{
  int ret;
  file_info_t *config_file = create_finfo();

/*  error(A_INFO,"read_host_table","Entering read_host_table()"); */
  strcpy(config_file -> filename, start_db_filename( DEFAULT_START_TABLE ));

  
  if ( open_file(config_file,O_WRONLY) == ERROR ) {
      error(A_ERR,"write_host_table","Cannot open file %s",config_file->filename);
    return ERROR;
  }
  
  if ( (ret = fwrite(host_table,sizeof(host_table_t),host_table_num,config_file->fp_or_dbm.fp)) != host_table_num ) {
    close_file(config_file);
    destroy_finfo(config_file);
    error(A_ERR,"write_host_table","fwrite() failed, returned value %d",ret);
    return ERROR;
  }

  close_file(config_file);
  destroy_finfo(config_file);
/*  error(A_INFO,"write_host_table","Exiting write_host_table() successfully"); */
  return A_OK;
  
}


status_t host_table_find(ipaddr,hostname,port, index )
ip_addr_t *ipaddr;
hostname_t hostname;
int *port;
host_table_index_t *index;
{

  int i;

  if (( *ipaddr != 0 )  && ( hostname[0] != '\0'  ) && (*port>0) ) {
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
/*    error(A_ERR,"host_table_find","Cannot find host (1). Parameters passed: (ip %s) (hostname %s) (port %d) (index %d)", inet_ntoa(ipaddr_to_inet(*ipaddr)),hostname,*port, *index); */
    return ERROR;
  } 


  if ( (*ipaddr != 0)&&(*port>0) ) {
    for ( i = 0; i < host_table_num; i++ ) {
      if ((host_table[i].ipaddr == *ipaddr )&&(host_table[i].port == *port)) {
        *index = i;
        return A_OK;
      }
    }
/*    error(A_ERR,"host_table_find","Cannot find host (2). Parameters passed: (ip %s) (hostname %s) (port %d) (index %d)", inet_ntoa(ipaddr_to_inet(*ipaddr)),hostname,*port, *index);     */
    return ERROR;
  }


  if ( (hostname[0] != '\0')&&(*port>0)  ) {
    for ( i = 0; i < host_table_num; i++ ) {
      if ( !strcasecmp(host_table[i].hostname,hostname)&&(host_table[i].port == *port)) {
        *index = i;
        return A_OK;
      }
    }
/*    error(A_ERR,"host_table_find","Cannot find host (3). Parameters passed: (ip %s) (hostname %s) (port %d) (index %d)", inet_ntoa(ipaddr_to_inet(*ipaddr)),hostname,*port, *index);    */
    return ERROR;
  }

  if ( *index >= 0 && *index < host_table_num) {
    *ipaddr = host_table[*index].ipaddr;
    *port = host_table[*index].port;
    strcpy(hostname,host_table[*index].hostname);    
    return A_OK;
  }
  
/*  error(A_ERR,"host_table_find","Cannot find host (4). Parameters passed: (ip %s) (hostname %s) (port %d) (index %d)", inet_ntoa(ipaddr_to_inet(*ipaddr)),hostname,*port, *index); */
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
  memset(host_table[i].hostname,0,sizeof(hostname_t));
  strcpy(host_table[i].hostname,hostname);
  host_table[i].port = port;
  *index = i;
  host_table_num++;

  host_table_priority();
  return A_OK;
}

status_t host_table_replace(old_ipaddr, new_ipaddr, hostname, port )
ip_addr_t old_ipaddr;
ip_addr_t new_ipaddr;
hostname_t hostname;
int port;
{

  host_table_index_t ind;

  if ( host_table_find(&old_ipaddr, hostname, &port, &ind) != A_OK ){
    error(A_ERR,"host_table_replace","Trying to replace an ipaddr that originally did not exist in startdb");
    return ERROR;
  }

  if ( host_table[ind].ipaddr != old_ipaddr || strcmp(host_table[ind].hostname,hostname)) {
    error(A_ERR,"host_table_replace","Conflicting info for %s(%s). In host table %s(%s)",
          hostname,   inet_ntoa(ipaddr_to_inet(old_ipaddr)),
          host_table[ind].hostname,   inet_ntoa(ipaddr_to_inet(host_table[ind].ipaddr)));
    return ERROR;
  }

  host_table[ind].ipaddr = new_ipaddr;

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


int host_table_priority()
{
  int i;

  if ( priority_table == NULL ) {
    free(priority_table);
  }
  
  priority_table = (int*)(malloc(sizeof(int)*host_table_max));
  if ( priority_table == NULL ) {
    return 0;
  }

  for ( i = 0; i < host_table_num; i++ ) {

    priority_table[i] = domain_priority(host_table[i].hostname);
    
  }

  return 1;
}

int host_table_cmp(ind1,ind2)
host_table_index_t ind1,ind2;
{
  int p1,p2;
  
  p1 = priority_table[ind1];
  
  p2 = priority_table[ind2];

  /* Note that if p? == 0 it means not listed .. so goes last */


  return p1-p2;

}
