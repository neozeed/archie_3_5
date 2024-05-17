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

#include "archie_dbm.h"
#include "typedef.h"
#include "db_files.h"
#include "db_ops.h"
#include "webindexdb_ops.h"
#include "domain.h"
#include "header.h"
#include "start_db.h"
#include "error.h"
#include "lang_startdb.h"
#include "master.h"
#include "archie_strings.h"

#include "protos.h"


static int cmp_list( a, b )
  const void *a, *b;
{
  host_table_index_t c,d;
  c = *(host_table_index_t*)a;
  d = *(host_table_index_t*)b;
  return host_table_cmp((host_table_index_t) c,(host_table_index_t)d);
                        
}


static void reorder_list(index_list, num)
  host_table_index_t *index_list;
  int *num;
{
  int i;
  
  for ( i = 0; i < MAX_LIST_SIZE && index_list[i] != END_LIST;  )
    i++;

  if ( i > 1 ) {
    qsort(index_list,i, sizeof(host_table_index_t),cmp_list);
  }

  *num = i;

}

status_t reorder_start_dbs( old_start_db, new_start_db )
  file_info_t *old_start_db,*new_start_db;
{


  datum key;
  int i;
  host_table_index_t index_list[MAX_LIST_SIZE];

  for (key = dbm_firstkey(old_start_db->fp_or_dbm.dbm); key.dptr !=  NULL;
       key = dbm_nextkey(old_start_db->fp_or_dbm.dbm)) {

    if ( get_dbm_entry(key.dptr,key.dsize,index_list,old_start_db) == ERROR ) {
      error(A_INTERR,"reorder_start_dbs", "Unable to find value for known key");
      continue;
    }

    reorder_list(index_list, &i); 
    

    if ( put_dbm_entry(key.dptr,key.dsize,index_list,(i+1)*sizeof(host_table_index_t),new_start_db,0) == ERROR ) {
      return ERROR;
    }
  }
  return A_OK;
  
}
