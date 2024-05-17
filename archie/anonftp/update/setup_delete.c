/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <memory.h>
#include <malloc.h>
#include "typedef.h"
#include "strings_index.h"
#include "site_file.h"
#include "db_ops.h"
#include "lang_anonftp.h"
#include "files.h"
#include "error.h"
#include "archie_inet.h"
#include "delete_anonftp.h"
#include "protos.h"
#include "../startdb/start_db.h"
#include "../startdb/lang_startdb.h"

#ifndef __STDC__
   extern status_t host_table_find();
   extern status_t update_start_dbs();
#endif

/*
 * setup_delete: performs the deletion of the site from the anonftp
 * database
 */



status_t setup_delete(delete_file, start_db, recno, ipaddr, port)
   file_info_t *delete_file;  /* The file containing the data in the anonftp database */
   file_info_t *start_db;
   int recno;		      /* Number of records in the data */
   ip_addr_t ipaddr;	      /* The IP address of the site being deleted */
  int port;
{
  int count;
  host_table_index_t hin;

  full_site_entry_t *site_ent_ptr;

  hostname_t hname;

  struct stat statbuf;


  hname[0] = '\0';

  ptr_check(delete_file, file_info_t, "setup_delete", ERROR);
   
  if(delete_file -> fp_or_dbm.fp == (FILE *) NULL){

    /* "No input file %s" */

    error(A_ERR,"setup_delete", SETUP_DELETE_002, delete_file -> filename);
    return(A_OK);
  }


  if(fstat(fileno(delete_file -> fp_or_dbm.fp), &statbuf) == -1){

    /* "Can't fstat() input site file %s" */

    error(A_SYSERR,"setup_delete", SETUP_DELETE_011, delete_file -> filename);
    return(A_OK);
  }
  else{

    if(statbuf.st_size != recno * sizeof(full_site_entry_t)){

      /* "Number of records in host auxiliary database (%d) different from implied by file size (%d)" */
	 
      error(A_WARN, "setup_delete", SETUP_DELETE_012, recno, statbuf.st_size / sizeof(full_site_entry_t));

      recno = statbuf.st_size / sizeof(full_site_entry_t);
    }
  }
   

  /* Allocate space for internal list */

  /*
     if((checklist = malloc(sizeof(char) * recno)) == (char *) NULL){
     
     / "Can't malloc space for internal list" /
     
     error(A_SYSERR, "setup_delete", SETUP_DELETE_001);
     return(A_OK);
     }
     
     memset(checklist, '\0', recno * sizeof(char));
     */


  if(mmap_file(delete_file, O_RDWR) == ERROR){

    /* "Can't mmap site file to be deleted %s" */

    error(A_ERR, "setup_delete", SETUP_DELETE_003, delete_file -> filename);
    return(A_OK);
  }

  
  if ( host_table_find( &ipaddr, hname, &port , &hin) == ERROR )
  {
    /* "Site %s should be in host/start table but is not. Corruption\n" */
    error(A_ERR,"setup_delete","Site %s should be in host/start table but is not. Corruption\n",
          (char *)(inet_ntoa( ipaddr_to_inet(ipaddr))));
    return ERROR;
  }


  for(site_ent_ptr = (full_site_entry_t *) delete_file -> ptr, count =0;
      count < recno; count++, site_ent_ptr++){

    /* if the pointer to next record on chain is in the same file 
       (internal link) follow chain until it points out of the site
       (external link) */


    if( !(CSE_IS_SUBPATH((*site_ent_ptr)) ||
          CSE_IS_PORT((*site_ent_ptr))  ||
          CSE_IS_NAME((*site_ent_ptr))) ){
      /* site_ent_ptr->flags != (flags_t)1 ){ */
      if ( update_start_dbs(start_db, site_ent_ptr->strt_1, hin, DELETE_SITE ) == ERROR )
      {
        /* "Could not update start/host table with  %s ." */
        error(A_ERR,"insert_compare_hstart_db","Could not update start/host table with  %s .\n",
              (char *)(inet_ntoa( ipaddr_to_inet(ipaddr))) );
        return ERROR;
      }
      
    }

  }
  
  if(munmap_file(delete_file) == ERROR){

    /* "Can't unmap site file %s" */
      
    error(A_ERR, "setup_delete", SETUP_DELETE_007, delete_file -> filename);
  }


#if !defined(__STDC__) && !defined(AIX)
  /*
     if(checklist)
     if(free(checklist) == -1){
     
     / "Can't free internal list" /
     
     error(A_SYSERR, "setup_delete", SETUP_DELETE_010);
     }
     #else
     free(checklist);
     */
#endif      

  return(A_OK);
}
