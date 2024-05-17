/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include "typedef.h"
#include "header.h"
#ifdef OLD_FILES
#  include "old-host_db.h"
#else
#  include "host_db.h"
#endif
#include "error.h"
#include "debug.h"
#include "lang_hostdb.h"
#include "archie_dbm.h"
#include "archie_mail.h"
#include "archie_strings.h"
#include "start_db.h"
#include "lang_startdb.h"
#include "protos.h"


extern status_t host_table_add();
extern status_t get_port();



/*
 * make_header_hostdb_entry: create a header record from the given primary
 * host record
 */



status_t make_header_hostdb_entry(header_rec, hostdb_entry, flags)
   header_t *header_rec;      /* header record */
   hostdb_t *hostdb_entry;    /* primary host database record */
   int      flags;	      /* miscellanous flags */
{

   ptr_check(header_rec, header_t, "make_header_hostdb_entry", ERROR);
   ptr_check(hostdb_entry, hostdb_t, "make_header_hostdb_entry", ERROR);

   strcpy(header_rec -> primary_hostname, hostdb_entry -> primary_hostname);

   header_rec -> primary_ipaddr = hostdb_entry -> primary_ipaddr;

   hostdb_entry -> flags = flags;


   if(HDR_GET_OS_TYPE(header_rec -> header_flags))
      if(hostdb_entry -> os_type == 0)
         HDR_UNSET_OS_TYPE(header_rec -> header_flags);
      else
         header_rec -> os_type = hostdb_entry -> os_type;

   if(HDR_GET_TIMEZONE(header_rec -> header_flags))
      if(hostdb_entry -> timezone == 0)
         HDR_UNSET_TIMEZONE(header_rec -> header_flags);
      else
         header_rec -> timezone = hostdb_entry -> timezone;

   if(HDR_GET_ACCESS_METHODS(header_rec -> header_flags))
      if(strcmp(hostdb_entry -> access_methods, "") == 0)
         HDR_UNSET_ACCESS_METHODS(header_rec -> header_flags);
      else
         strcpy(header_rec -> access_methods, hostdb_entry -> access_methods);

   if(HDR_GET_PROSPERO_HOST(header_rec -> header_flags))
      if(HDB_IS_PROSPERO_SITE(hostdb_entry -> flags))
         header_rec -> prospero_host = YES;
      else
         header_rec -> prospero_host = NO;

    return(A_OK);
}

/*
 * make_header_hostaux_entry: create a header record from the give
 * auxiliary host database record
 */



void make_header_hostaux_entry(header_rec, hostaux_entry, flags)
   header_t *header_rec;	 /* header record */
   hostdb_aux_t *hostaux_entry;	 /* auxiliary host database */
   int      flags;		 /* misc flags */
{
   if(HDR_GET_GENERATED_BY(header_rec -> header_flags))
      if(hostaux_entry -> generated_by == 0)
         HDR_UNSET_GENERATED_BY(header_rec -> header_flags);
      else
         header_rec -> generated_by = hostaux_entry -> generated_by;

   if(HDR_GET_PREFERRED_HOSTNAME(header_rec -> header_flags))
      if(strcmp(hostaux_entry -> preferred_hostname, "") == 0)
         HDR_UNSET_PREFERRED_HOSTNAME(header_rec -> header_flags);
      else
        strcpy(header_rec -> preferred_hostname, hostaux_entry -> preferred_hostname);

   if(HDR_GET_SOURCE_ARCHIE_HOSTNAME(header_rec -> header_flags))
      if(hostaux_entry -> source_archie_hostname[0] == '\0')
         HDR_UNSET_SOURCE_ARCHIE_HOSTNAME(header_rec -> header_flags);
      else
         strcpy(header_rec -> source_archie_hostname, hostaux_entry -> source_archie_hostname);

   if(HDR_GET_ACCESS_COMMAND(header_rec -> header_flags))
      if(hostaux_entry -> access_command[0] == '\0')
         HDR_UNSET_ACCESS_COMMAND(header_rec -> header_flags);
      else
         strcpy(header_rec -> access_command, hostaux_entry -> access_command);

   if(HDR_GET_RETRIEVE_TIME(header_rec -> header_flags))
      if(hostaux_entry -> retrieve_time == 0)
         HDR_UNSET_RETRIEVE_TIME(header_rec -> header_flags);
      else
         header_rec -> retrieve_time = hostaux_entry -> retrieve_time;

   if(HDR_GET_PARSE_TIME(header_rec -> header_flags))
      if(hostaux_entry -> parse_time == 0)
         HDR_UNSET_PARSE_TIME(header_rec -> header_flags);
      else
         header_rec -> parse_time = hostaux_entry -> parse_time;

   if(HDR_GET_NO_RECS(header_rec -> header_flags))
      header_rec -> no_recs = hostaux_entry -> no_recs;

   if(HDR_GET_SITE_NO_RECS(header_rec -> header_flags))
      header_rec -> site_no_recs = hostaux_entry -> site_no_recs;

   if(HDR_GET_CURRENT_STATUS(header_rec -> header_flags))
      header_rec -> current_status = hostaux_entry -> current_status;

   if(HDR_GET_HCOMMENT(header_rec -> header_flags))
      if(hostaux_entry -> comment[0] == '\0')
	 HDR_UNSET_HCOMMENT(header_rec -> header_flags);
      else
	 strcpy(header_rec -> comment, hostaux_entry -> comment);
}


void make_hostaux_from_header(header_rec, hostaux_entry, flags)
   header_t *header_rec;
   hostdb_aux_t *hostaux_entry;
   flags_t  flags;
{

   
   if(HDR_GET_SOURCE_ARCHIE_HOSTNAME(header_rec -> header_flags))
      strcpy(hostaux_entry -> source_archie_hostname, header_rec -> source_archie_hostname);

   if(HDR_GET_PREFERRED_HOSTNAME(header_rec -> header_flags))
      strcpy(hostaux_entry -> preferred_hostname, header_rec -> preferred_hostname);
   
   if(HDR_GET_ACCESS_COMMAND(header_rec -> header_flags))
      strcpy(hostaux_entry -> access_command, header_rec -> access_command);

   if(HDR_GET_RETRIEVE_TIME(header_rec -> header_flags))
      hostaux_entry -> retrieve_time = header_rec -> retrieve_time;

   if(HDR_GET_PARSE_TIME(header_rec -> header_flags))
      hostaux_entry -> parse_time = header_rec -> parse_time;

   if(HDR_GET_UPDATE_TIME(header_rec -> header_flags))
      hostaux_entry -> update_time = header_rec -> update_time;

   if(HDR_GET_NO_RECS(header_rec -> header_flags))
      hostaux_entry -> no_recs = header_rec -> no_recs;

   if(HDR_GET_CURRENT_STATUS(header_rec -> header_flags))
      hostaux_entry -> current_status = header_rec -> current_status;

   if(HDR_GET_HCOMMENT(header_rec -> header_flags))
      strcpy(hostaux_entry -> comment, header_rec -> comment);
   else
      memset(hostaux_entry -> comment, '\0', sizeof(hostaux_entry -> comment));

   if(HDR_GET_SITE_NO_RECS(header_rec -> header_flags))
      hostaux_entry -> site_no_recs = header_rec -> site_no_recs;

   if(HDR_GET_GENERATED_BY(header_rec -> header_flags))
      hostaux_entry -> generated_by =  header_rec -> generated_by;
}



void make_hostdb_from_header(header_rec, hostdb_entry, flags)
   header_t *header_rec;
   hostdb_t *hostdb_entry;
   flags_t  flags;

{

   memset((char *) hostdb_entry, '\0', sizeof(hostdb_t));

   strcpy(hostdb_entry -> primary_hostname, header_rec -> primary_hostname);

   hostdb_entry -> primary_ipaddr = header_rec -> primary_ipaddr;

   hostdb_entry -> flags = flags;

   if(HDR_GET_ACCESS_METHODS(header_rec -> header_flags))
      strcpy(hostdb_entry -> access_methods, header_rec -> access_methods);


   if(HDR_GET_OS_TYPE(header_rec -> header_flags))
      hostdb_entry -> os_type = header_rec -> os_type;

   if(HDR_GET_TIMEZONE(header_rec -> header_flags))
      hostdb_entry -> timezone = header_rec -> timezone;

   

   if(HDR_GET_PROSPERO_HOST(header_rec -> header_flags)){
      if(header_rec -> prospero_host == YES)
         HDB_SET_PROSPERO_SITE(hostdb_entry -> flags);
   }

}




/*
 * do_hostdb_update: modify the host databases. 
 *
 */


host_status_t do_hostdb_update(hostbyaddr, hostdb, hostaux_db, header_rec, dn, flags, control_flags)
   file_info_t *hostbyaddr;
   file_info_t *hostdb;
   file_info_t *hostaux_db;   
   header_t    *header_rec;
   char	       *dn;
   flags_t     flags;
   flags_t     control_flags;
{

  hostdb_t hostdb_rec;
  hostdb_t hostdb_rec1;


  host_status_t host_status = 0;

  pathname_t aux_name;
  hostdb_aux_t hostaux_entry;
  hostdb_aux_t hostaux_entry1;
  hostbyaddr_t hostbyaddr_rec;

  index_t index;

  /* Perform the pointer checks */
   
  ptr_check(hostbyaddr,file_info_t, "do_hostdb_update", ERROR);
  ptr_check(hostdb, file_info_t, "do_hostdb_update", ERROR);
  ptr_check(hostaux_db,file_info_t, "do_hostdb_update", ERROR);
  ptr_check(header_rec, header_t, "do_hostdb_update", ERROR);

  header_rec -> generated_by = INSERT;
  HDR_SET_GENERATED_BY(header_rec -> header_flags);

  /* create a hostdb record from the header */

  memset((char *) &hostdb_rec, '\0', sizeof(hostdb_t));
  make_hostdb_from_header(header_rec, &hostdb_rec, flags);
  
  memset((char *) &hostaux_entry, '\0', sizeof(hostdb_aux_t));  
  make_hostaux_from_header(header_rec, &hostaux_entry, flags);

  set_aux_origin(&hostaux_entry, header_rec->access_methods, -1);
  
  /*
   * Running from the command line. Have to look at the primary host
   * database ourselves. Not used if running from controlling process.
   */

  if((host_status = check_new_hentry(hostbyaddr, hostdb, &hostdb_rec, hostaux_db, &hostaux_entry, flags, control_flags)) != HOST_OK){
    switch(host_status = check_old_hentry(hostbyaddr, hostdb, &hostdb_rec, hostaux_db, &hostaux_entry, flags, control_flags)){

    case HOST_OK:
	    break;

    case HOST_UNKNOWN:

	    if((host_status = handle_unknown_host(header_rec -> primary_hostname, dn, &hostdb_rec, &hostaux_entry, hostdb, hostaux_db, hostbyaddr)) != HOST_OK)
      return(host_status);

	    /* "Site %s unknown to DNS. Site has been marked 'disabled'" */

	    write_mail(MAIL_HOST_FAIL, HANDLE_UNKNOWN_HOST_001, header_rec -> primary_hostname);
	    return(HOST_IGNORE);
	    break;

    case HOST_CNAME_MISMATCH:
    case HOST_CNAME_UNKNOWN:
      /*  call  dns_ent = ar_open_dns_name( header_rec->primary_hostname,
                          DNS_EXTERN_ONLY, hostdb )
          strcpy( header_rec->primary_hostname ,dns_ent );
       */

      /*	    hostdb_rec.preferred_hostname[0] = '\0'; */

	    if((host_status = update_hostdb(hostbyaddr, hostdb, &hostdb_rec, 1)) != HOST_OK)
      return(host_status);

	    break;
	       
    case HOST_PADDR_MISMATCH:

	    if((host_status = handle_paddr_mismatch(header_rec -> primary_hostname, header_rec -> access_command, dn, &hostdb_rec, hostdb, hostbyaddr)) != HOST_OK)
      return(host_status);

	    if((host_status = update_hostdb(hostbyaddr, hostdb, &hostdb_rec, 1)) != HOST_OK)
      return(host_status);

	    break;

    default:

	    /* "Host %s: %s" */

	    error(A_ERR, "do_hostdb_update", DO_HOSTDB_UPDATE_001, hostdb_rec.primary_hostname, get_host_error(host_status));
	    return(host_status);
	    break;
    }

  }
  else{

    if(put_dbm_entry(hostdb_rec.primary_hostname, strlen(hostdb_rec.primary_hostname) + 1, &hostdb_rec, sizeof(hostdb_t), hostdb, FALSE) != A_OK)
    return(HOST_INS_FAIL);

    hostbyaddr_rec.primary_ipaddr = hostdb_rec.primary_ipaddr;
    strcpy(hostbyaddr_rec.primary_hostname, hostdb_rec.primary_hostname);

    if(put_dbm_entry(&hostbyaddr_rec.primary_ipaddr, sizeof(ip_addr_t), &hostbyaddr_rec, sizeof(hostbyaddr_t), hostbyaddr, FALSE) == ERROR)
	  return(HOST_PADDR_FAIL);
  }

  if(get_dbm_entry(hostdb_rec.primary_hostname, strlen(hostdb_rec.primary_hostname) + 1, &hostdb_rec1, hostdb) == ERROR){

    /* "Entry for %s not in host database" */

    error(A_ERR,"do_hostdb_update", DO_HOSTDB_UPDATE_002, hostdb_rec.primary_hostname);
    return(ERROR);
  }

  hostdb_rec.primary_ipaddr = hostdb_rec1.primary_ipaddr;

  /* Host already in database */

  if ( get_hostaux_ent(hostdb_rec.primary_hostname, dn, &index,
                       hostaux_entry.preferred_hostname,
                       hostaux_entry.access_command,
                       &hostaux_entry1, hostaux_db) == ERROR ) {
                       
    pathname_t tmp_string;
    char **aux_dbs;
    char **aux_list;
    int unfound;
    char **newlist;
    int displacement;
      

    /* Add the anonftp access method to the current list */

    /* First break up the list */

    aux_dbs = aux_list = str_sep(hostdb_rec.access_methods, NET_DELIM_CHAR);

    unfound = 0;


    /* Try to find the database database one the list */

    while((*aux_list != (char *) NULL) && (unfound = strcasecmp(*aux_list, dn)))
    aux_list++;

    if(unfound){


      /* Can't find it so add it */

      if((newlist = (char **) realloc(aux_dbs, sizeof(char **) * (displacement = (aux_list - aux_dbs)) + 2)) == (char **) NULL){

        /* "Can't malloc space for new auxiliary list" */

        error(A_ERR, "do_hostdb_update", DO_HOSTDB_UPDATE_003);
        return(HOST_MALLOC_ERROR);
      }

      aux_dbs = newlist;
      aux_list = aux_dbs + displacement;
	 

      *aux_list = (char *) strdup(dn);
      *(aux_list +1) = (char *) NULL;
    }

    sprintf(hostdb_rec.access_methods, "%s", *aux_dbs);

    aux_list = aux_dbs + 1;

    while(*aux_list != (char *) NULL){

      sprintf(tmp_string, ":%s",*aux_list);
      strcat(hostdb_rec.access_methods, tmp_string);
      aux_list++;

    }

    index = 0;

    /* Insert it into the datababase */

    if(put_dbm_entry(hostdb_rec.primary_hostname, strlen(hostdb_rec.primary_hostname) + 1, &hostdb_rec, sizeof(hostdb_t), hostdb, TRUE) != A_OK){

      /* "Can't insert %s into primary host database" */

      error(A_ERR,"do_hostdb_update", DO_HOSTDB_UPDATE_004, hostdb_rec.primary_hostname);
      return(HOST_INS_FAIL);
    }

  }
  else{

    /* Check to see if it active */

    if(hostaux_entry1.current_status == ACTIVE)
    return(HOST_ACTIVE);

  }

  if(header_rec -> update_status == FAIL)
  hostaux_entry.fail_count++;

  hostaux_entry.generated_by = INSERT;
  hostaux_entry.current_status = hostaux_entry1.current_status;
  hostaux_entry.update_time = time((time_t *) NULL);

  sprintf(aux_name,"%s.%s.%d", hostdb_rec.primary_hostname,dn,(int)index);
          
  if(put_dbm_entry(aux_name, strlen(aux_name) + 1, &hostaux_entry, sizeof(hostdb_aux_t), hostaux_db, TRUE) != A_OK){

    /* "Can't insert database %s for %s in aux host database" */

    error(A_ERR, "do_hostdb_update", DO_HOSTDB_UPDATE_005, hostdb_rec.primary_hostname, dn);
    return(HOST_ACCESS_INS_FAIL);
  }

  header_rec -> primary_ipaddr = hostdb_rec.primary_ipaddr;
  HDR_SET_PRIMARY_IPADDR(header_rec -> header_flags);

  if(header_rec -> update_status == FAIL)
  return(HOST_IGNORE);

  free(hostaux_entry.origin);
  return(HOST_OK);
}


/*
 * activate_site: given the header record activate the auxiliary host
 * database for that record
 */



status_t activate_site(header_rec, dn, hostaux_db)
   header_t *header_rec;	 /* header record */
   char	    *dn;
   file_info_t *hostaux_db;	 /* auxiliary host database */

{
   hostdb_aux_t hostaux_rec;
   pathname_t aux_name;
   index_t index;

   ptr_check(header_rec, header_t, "activate_site", ERROR);
   ptr_check(hostaux_db, file_info_t, "activate_site", ERROR);

  if ( get_hostaux_ent(header_rec->primary_hostname,dn,&index,
                       header_rec->preferred_hostname,
                       header_rec->access_command,
                       &hostaux_rec, hostaux_db) == ERROR ) {


      /* "Can't find database %s for %s in auxiliary database" */

      error(A_ERR, "activate_site", ACTIVATE_SITE_001, header_rec -> primary_hostname, dn);
      return(ERROR);
   }

   hostaux_rec.current_status = ACTIVE;
   HADB_UNSET_FORCE_UPDATE(hostaux_rec.flags);
                        
   sprintf(aux_name,"%s.%s.%d", header_rec->primary_hostname,dn,(int)index);
   
   if(put_dbm_entry(aux_name, strlen(aux_name) + 1, &hostaux_rec, sizeof(hostdb_aux_t), hostaux_db, TRUE) == ERROR){

      /*  "Can't update database %s record for %s" */

      error(A_ERR, "activate_site", ACTIVATE_SITE_002, dn, header_rec -> primary_hostname);
      return(ERROR);
   }

   return(A_OK);
}


host_status_t handle_unknown_host(hostname, dbname, hostdb_rec, hostaux_entry, hostdb, hostaux_db, hostbyaddr)
   char *hostname;
   char *dbname;
   hostdb_t *hostdb_rec;
   hostdb_aux_t *hostaux_entry;
   file_info_t *hostdb;
   file_info_t *hostaux_db;
   file_info_t *hostbyaddr;
{

   host_status_t host_status;
   index_t index;

   ptr_check(hostname, char, "handle_unknown_host", HOST_ERROR);
   ptr_check(dbname, char, "handle_unknown_host", HOST_ERROR);
   ptr_check(hostdb_rec, hostdb_t, "handle_unknown_host", HOST_ERROR);
   ptr_check(hostaux_entry, hostdb_aux_t, "handle_unknown_host", HOST_ERROR);
   ptr_check(hostdb, file_info_t, "handle_unknown_host", HOST_ERROR);
   ptr_check(hostaux_db, file_info_t, "handle_unknown_host", HOST_ERROR);
   ptr_check(hostbyaddr, file_info_t, "handle_unknown_host", HOST_ERROR);

   /* The host has disappeared since the last time it was
      updated */

   memset(hostaux_entry, '\0', sizeof(hostdb_aux_t));

   if(get_dbm_entry(hostname, strlen(hostname) + 1, hostdb_rec, hostdb) == ERROR){

      /* we don't currently have the host stored */

      hostdb_rec -> primary_ipaddr = 0;
/*      hostdb_rec -> preferred_hostname[0] = '\0'; */

      strcpy(hostdb_rec -> access_methods, dbname);
      index = 0;

   }
   else{

      /* we do have the host */

      get_hostaux_ent(hostname, dbname, &index, NULL, NULL, hostaux_entry, hostaux_db);

   }

   set_aux_origin(hostaux_entry,dbname,index);

   hostaux_entry -> update_time = (date_time_t) time((time_t *) NULL);
   hostaux_entry -> retrieve_time = (date_time_t) time((time_t *) NULL);

   hostaux_entry -> current_status = DISABLED;
   

   if(((host_status = update_hostdb(hostbyaddr, hostdb, hostdb_rec, 1)) == ERROR)
      || ((host_status = update_hostaux(hostaux_db, hostname, hostaux_entry, 1)) == ERROR)) 
      return(host_status);

   return(HOST_OK);
}




host_status_t handle_cname_mismatch(hostname, access_command, dbname, hostdb_rec, hostdb, hostbyaddr)
   char *hostname;
   access_comm_t access_command;
   char *dbname;
   hostdb_t *hostdb_rec;
   file_info_t *hostdb;
   file_info_t *hostbyaddr;
{
   host_status_t host_status;
   hostdb_t local_hostdb_rec;
   hostbyaddr_t hbyaddr_rec1,hbyaddr_rec2;
   pathname_t new_addr;
   pathname_t old_addr;
   int port = 0;

   ptr_check(hostname, char, "handle_paddr_mismatch", HOST_PTR_NULL);
   ptr_check(dbname, char, "handle_paddr_mismatch", HOST_PTR_NULL);
   ptr_check(hostdb_rec, hostdb_t, "handle_paddr_mismatch", HOST_PTR_NULL);
   ptr_check(hostdb, file_info_t, "handle_paddr_mismatch", HOST_PTR_NULL);
   ptr_check(hostbyaddr, file_info_t, "handle_paddr_mismatch", HOST_PTR_NULL);

   if(get_dbm_entry(hostname, strlen(hostname) + 1, &local_hostdb_rec, hostdb) == ERROR){

      /* "Can't find hostname %s in primary host database" */

      error(A_INTERR, "handle_paddr_mismatch", HANDLE_PADDR_MISMATCH_001, hostname);
      return(HOST_DOESNT_EXIST);
   }

   if(get_dbm_entry(&(local_hostdb_rec.primary_ipaddr),
                    sizeof(ip_addr_t), &hbyaddr_rec1, hostbyaddr) == ERROR){

      /* "Can't find ipaddr record for hostname %s ipaddr %s" */

      error(A_INTERR, "handle_paddr_mismatch", HANDLE_PADDR_MISMATCH_002, hostname, inet_ntoa(ipaddr_to_inet(local_hostdb_rec.primary_ipaddr)));
      return(HOST_DB_ERROR);
   }
   /*  call  dns_ent = ar_open_dns_name( header_rec->preferred_hostname,
                         DNS_EXTERN_ONLY, hostdb )
       make a copy of the hostdb_rec to new_hostdb_rec
       make_hostdb_from_header(header_rec, &new_hostdb_rec, flags);
       strcpy( new_hostdb_rec->primary_hostname ,dns_ent );
       call update_hostdb(hostbyaddr, hostdb, &new_hostdb_rec, 1)
       
       the old record must go
       delete_from_hostdb(hostname, dbname, index, hostaux_db);
       */

   strcpy(hbyaddr_rec2.primary_hostname, hostname);
   hbyaddr_rec2.primary_ipaddr = hostdb_rec -> primary_ipaddr;

   if(put_dbm_entry(&(hostdb_rec -> primary_ipaddr), sizeof(ip_addr_t), &hbyaddr_rec2, sizeof(hostbyaddr_t), hostbyaddr, 0) == ERROR){

      /* "Can't insert new IP address %s into host address cache" */

      error(A_ERR, "handle_paddr_mismatch", HANDLE_PADDR_MISMATCH_003, inet_ntoa(ipaddr_to_inet(hostdb_rec -> primary_ipaddr)));
      return(HOST_PADDR_EXISTS);
   }

   if(delete_dbm_entry(&(hbyaddr_rec1.primary_ipaddr), sizeof(ip_addr_t), hostbyaddr) == ERROR){

      /* "Can't delete old IP address %s from host address cache" */

      error(A_ERR, "handle_paddr_mismatch", HANDLE_PADDR_MISMATCH_004, inet_ntoa(ipaddr_to_inet(hbyaddr_rec1.primary_ipaddr)));
      return(HOST_DB_ERROR);
   }

   /* Will change the start-table from old ipaddr to new ipaddr */

   if( get_port( access_command, dbname, &port ) == ERROR ){
     error(A_ERR,"handle_paddr_mismatch","Could not find the port for this site %s", hostdb_rec->primary_hostname);
     return(ERROR);
   }
   if( host_table_replace( local_hostdb_rec.primary_ipaddr, hostdb_rec -> primary_ipaddr, hostname, port ) == ERROR){
     error(A_ERR,"handle_paddr_mismatch","Could not replace ipaddr %s with %s for site %s", inet_ntoa(ipaddr_to_inet(local_hostdb_rec.primary_ipaddr)), inet_ntoa(ipaddr_to_inet(hostdb_rec->primary_ipaddr)), hostdb_rec->primary_hostname);
     return(ERROR);
   }
   /* "Primary IP address for %s has changed from %s to %s" */

   strcpy(old_addr, inet_ntoa(ipaddr_to_inet(local_hostdb_rec.primary_ipaddr)));
   strcpy(new_addr, inet_ntoa(ipaddr_to_inet(hostdb_rec -> primary_ipaddr)));

   write_mail(MAIL_HOST_SUCCESS, HANDLE_PADDR_MISMATCH_005, hostname, old_addr, new_addr);

   error(A_INFO, "handle_paddr_mismatch", HANDLE_PADDR_MISMATCH_005, hostname, old_addr, new_addr);

   local_hostdb_rec.primary_ipaddr = hostdb_rec -> primary_ipaddr;

   if((host_status = update_hostdb(hostbyaddr, hostdb, hostdb_rec, 1)) != HOST_OK)
     return(host_status);


   return(HOST_OK);
}



   if(put_dbm_entry(&(hostdb_rec -> primary_ipaddr), sizeof(ip_addr_t), &hbyaddr_rec2, sizeof(hostbyaddr_t), hostbyaddr, 0) == ERROR){

      /* "Can't insert new IP address %s into host address cache" */

      error(A_ERR, "handle_paddr_mismatch", HANDLE_PADDR_MISMATCH_003, inet_ntoa(ipaddr_to_inet(hostdb_rec -> primary_ipaddr)));
      return(HOST_PADDR_EXISTS);
   }

   if(delete_dbm_entry(&(hbyaddr_rec1.primary_ipaddr), sizeof(ip_addr_t), hostbyaddr) == ERROR){

      /* "Can't delete old IP address %s from host address cache" */

      error(A_ERR, "handle_paddr_mismatch", HANDLE_PADDR_MISMATCH_004, inet_ntoa(ipaddr_to_inet(hbyaddr_rec1.primary_ipaddr)));
      return(HOST_DB_ERROR);
   }

   /* Will change the start-table from old ipaddr to new ipaddr */

   if( get_port( access_command, dbname, &port ) == ERROR ){
     error(A_ERR,"handle_paddr_mismatch","Could not find the port for this site %s", hostdb_rec->primary_hostname);
     return(ERROR);
   }
   if( host_table_replace( local_hostdb_rec.primary_ipaddr, hostdb_rec -> primary_ipaddr, hostname, port ) == ERROR){
     error(A_ERR,"handle_paddr_mismatch","Could not replace ipaddr %s with %s for site %s", inet_ntoa(ipaddr_to_inet(local_hostdb_rec.primary_ipaddr)), inet_ntoa(ipaddr_to_inet(hostdb_rec->primary_ipaddr)), hostdb_rec->primary_hostname);
     return(ERROR);
   }
   /* "Primary IP address for %s has changed from %s to %s" */

   strcpy(old_addr, inet_ntoa(ipaddr_to_inet(local_hostdb_rec.primary_ipaddr)));
   strcpy(new_addr, inet_ntoa(ipaddr_to_inet(hostdb_rec -> primary_ipaddr)));

   write_mail(MAIL_HOST_SUCCESS, HANDLE_PADDR_MISMATCH_005, hostname, old_addr, new_addr);

   error(A_INFO, "handle_paddr_mismatch", HANDLE_PADDR_MISMATCH_005, hostname, old_addr, new_addr);

   local_hostdb_rec.primary_ipaddr = hostdb_rec -> primary_ipaddr;

   if((host_status = update_hostdb(hostbyaddr, hostdb, hostdb_rec, 1)) != HOST_OK)
     return(host_status);


   return(HOST_OK);
}


