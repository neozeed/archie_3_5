/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <search.h>
#include <string.h>

#include "typedef.h"
#include "site_file.h"
#include "error.h"
#include "debug.h"

typedef struct {
  int recno;
  index_t start;
} site_index_t;


static int site_index_cmp( a, b )
  void *a,*b;
{
  return ((site_index_t*)a)->start - ((site_index_t*)b)->start;
}

status_t create_site_index( filename, out_array, no_recs )
  pathname_t filename;
  full_site_entry_t *out_array;    /* Output "array" (mmaped output file) */
  int no_recs;
{
  int i;
  pathname_t index_filename;
  int fd, ret;
  site_index_t *index_list = NULL;

  index_list = (site_index_t*) malloc(sizeof(site_index_t)*no_recs);
  if ( index_list == NULL ) {
    return ERROR;
  }

  for ( i = 0; i < no_recs; i++ ) {
    index_list[i].recno = i;
    index_list[i].start = out_array[i].strt_1;
  }

  qsort((char*)index_list, i, sizeof(site_index_t), site_index_cmp);

  /* At this point we can create the file */
  sprintf(index_filename,"%s%s", filename, SITE_INDEX_SUFFIX);

  fd = open(index_filename,O_WRONLY|O_CREAT,0644);
  if ( fd == -1 ) {
    error(A_ERR,"Couldn't open idx file %s for this site.",index_filename);
    return ERROR;
  }

  ret = write(fd, index_list, sizeof(site_index_t)*no_recs);
  if ( ret !=  sizeof(site_index_t)*no_recs ) {
    error(A_ERR,"Couldn't write idx file %s for this site.",index_filename);
    return ERROR;
  }
  
  close(fd);     
  return A_OK;
}



status_t search_site_index(index_file, start, list, num )
  file_info_t *index_file;
  index_t start;
  int **list;
  int *num;
{

  int *tmp;
  int index_num, index_start, index_end;
  site_index_t *res, *ptr;
  site_index_t key;
  int nel,i;

  key.start = start;
  key.recno = 0;

  nel = index_file->size / sizeof(site_index_t);
  res = (site_index_t*) bsearch (&key, index_file->ptr, nel, sizeof (site_index_t),
                 site_index_cmp);

  
  if ( res == NULL ) {
    return ERROR;
  }

  index_num = res -(site_index_t*)index_file->ptr;
  for ( i = index_num-1; i>=0 ; i--)
     if ( ((site_index_t*)index_file->ptr + i)->start != start )
       break;
  index_start = i+1;

  for ( i = index_num+1; i< nel ; i++)
     if ( ((site_index_t*)index_file->ptr + i)->start != start )
       break;
  index_end = i-1;

  *num = index_end-index_start+1;
  *list = (int*)malloc(sizeof(int)*(*num));
  
  if ( *list == NULL ) {
    return ERROR;
  }
  
  for( i = index_start; i <= index_end; i++ ){
    *((*list)+(i-index_start)) = ((site_index_t*)index_file->ptr + i)->recno ;
  }
    
  return A_OK;
}








