/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 *
 */


#include <stdio.h>
#include "typedef.h"
#include "header.h"
#ifdef OLD_FILES
#  include "old-host_db.h"
#else
#  include "host_db.h"
#endif
#include "error.h"
#include "debug.h"
#include "archie_dbm.h"
#include "lang_hostdb.h"


/*
 * update_host_db: update the primary host database with the give record.
 * "replace" is non-zero if an existing record is to be over-written
 */



host_status_t update_hostdb(hostbyaddr, hostdb, hostdb_rec, replace)
   file_info_t *hostbyaddr;   /* host address cache */
   file_info_t *hostdb;	      /* primary host database */
   hostdb_t    *hostdb_rec;   /* record to be inserted */
   int	       replace;	      /* non zero on overwrite */

{
   hostbyaddr_t hostaddr_rec;

   ptr_check(hostbyaddr, file_info_t, "update_hostdb", ERROR);
   ptr_check(hostdb, file_info_t, "update_hostdb", ERROR);
   ptr_check(hostdb_rec, hostdb_t, "update_hostdb", ERROR);      

   if(put_dbm_entry(hostdb_rec -> primary_hostname, strlen(hostdb_rec -> primary_hostname) + 1, hostdb_rec, sizeof(hostdb_t), hostdb, replace) == ERROR){

      /*  "Can't modify primary host database" */
      
      error(A_ERR, "update_hostdb", UPDATE_HOSTDB_001);
      return(HOST_UPDATE_FAIL);
   }

   hostaddr_rec.primary_ipaddr = hostdb_rec -> primary_ipaddr;
   strcpy(hostaddr_rec.primary_hostname, hostdb_rec -> primary_hostname);

   if(put_dbm_entry(&(hostdb_rec -> primary_ipaddr), sizeof(ip_addr_t), &hostaddr_rec, sizeof(hostbyaddr_t), hostbyaddr, replace)){

      /* "Can't modify host address cache" */

      error(A_ERR, "update_hostdb", UPDATE_HOSTDB_002);
      return(HOST_PADDR_FAIL);
   }

   return(HOST_OK);
}


/*
 * update_hostaux: update the auxiliary host database. Similar to
 * "update_hostdb"
 */



host_status_t update_hostaux(hostaux, hostname, hostaux_rec, replace)
   file_info_t *hostaux;      /* auxiliary host database */
   hostname_t  hostname;      /* Hostname of host to be updated */
   hostdb_aux_t *hostaux_rec; /* record to be updated */
   int	       replace;	      /* non zero on overwrite */
{
   pathname_t aux_name;

   ptr_check(hostaux, file_info_t, "update_hostdb", HOST_ERROR );
   ptr_check(hostaux_rec, hostdb_aux_t, "update_hostdb", HOST_ERROR);

   if((hostname == (char *) NULL) || (hostname[0] == '\0')){

      /* "Invalid hostname parameter" */

      error(A_ERR, "update_hostaux", UPDATE_HOSTAUX_001);
      return(HOST_ERROR);
   }

   if ( hostaux_rec -> origin == NULL ) {
     /* "Cannot get access method " */
      error(A_ERR, "update_hostaux", UPDATE_HOSTAUX_003);
      return(HOST_ACCESS_UPDATE_FAIL);
   }

   if ( hostaux_rec->origin->hostaux_index == -1 ) { /* Not in yet .. */
     index_t index;

     find_hostaux_last( hostname, hostaux_rec -> origin -> access_methods, &index, hostaux);

     index++;
     hostaux_rec->origin->hostaux_index = index;

   }
   sprintf(aux_name,"%s.%s.%d", hostname, hostaux_rec -> origin -> access_methods, (int)hostaux_rec -> origin -> hostaux_index);


   
   if(put_dbm_entry(aux_name, strlen(aux_name) + 1, hostaux_rec, sizeof(hostdb_aux_t), hostaux, replace) == ERROR){

      /* "Can't update auxiliary host database" */

      error(A_ERR, "update_hostaux", UPDATE_HOSTAUX_002);
      return(HOST_ACCESS_UPDATE_FAIL);
   }

   return(HOST_OK);

}



/*
 * check_new_hentry: check an entry to be made into the primary host
 * databases. Make sure it doesn't already exist and that the information
 * is consistent
 */



host_status_t check_new_hentry(hostbyaddr, hostdb, hostdb_rec, hostdb_aux, hostaux_rec, flags, control_flags)
   file_info_t  *hostbyaddr;	 /* host address cache */
   file_info_t  *hostdb;	 /* primary host database */
   hostdb_t     *hostdb_rec;	 /* host record to be checked */
   file_info_t  *hostdb_aux;	 /* primary host database */
   hostdb_aux_t *hostaux_rec;	 /* host record to be checked */
   int	        flags;		 /* Misc flags */
   int	        control_flags;	 /* Control flags */
{
   hostdb_t *hentry;
   hostdb_t hostdb_entry;
   AR_DNS *dns_ent;
   hostbyaddr_t hbyaddr;
   host_status_t host_status = 0;


   ptr_check(hostbyaddr, file_info_t, "check_new_hentry", HOST_ERROR);
   ptr_check(hostdb, file_info_t, "check_new_hentry", HOST_ERROR);
   ptr_check(hostdb_rec, hostdb_t, "check_new_hentry", HOST_ERROR);      


   /* Check primary hostname */

   if((dns_ent = ar_open_dns_name( hostdb_rec -> primary_hostname, DNS_EXTERN_ONLY, hostdb )) == (AR_DNS *) NULL){

      return(HOST_UNKNOWN);
   }

   hostdb_entry = *hostdb_rec;

   /* Get the primary hostname for the site */

   hostdb_entry.primary_ipaddr = *get_dns_addr(dns_ent);

   if(cmp_dns_name(hostdb_entry.primary_hostname, dns_ent) != A_OK){

      strcpy(hostdb_entry.primary_hostname,get_dns_primary_name(dns_ent));

      if(get_dbm_entry(hostdb_entry.primary_hostname,
                       strlen(hostdb_entry.primary_hostname) + 1,
		       &hostdb_entry, hostdb) != ERROR){

         *hostdb_rec = hostdb_entry;
         return(HOST_STORED_OTHERWISE);
      }

      return(HOST_NAME_NOT_PRIMARY);
   }

   ar_dns_close(dns_ent);

   /* This checks if the host already exists under the given primary name */

   if(get_dbm_entry(hostdb_rec -> primary_hostname,
                     strlen(hostdb_rec-> primary_hostname) + 1,
		     &hostdb_entry, hostdb) != ERROR){


      *hostdb_rec = hostdb_entry;
      return(HOST_ALREADY_EXISTS);
   }

   /* Check to see if given primary name is another record's preferred name */

   if((hentry = search_in_preferred(hostdb_rec -> primary_hostname, hostdb, hostdb_aux)) != (hostdb_t *) NULL){

      *hostdb_rec = *hentry;

      host_status =  HOST_STORED_OTHERWISE;
      return(host_status);

   }

   /* If the preferred name is being given */

   if( hostaux_rec != NULL && hostaux_rec -> preferred_hostname[0] != '\0'){

      /* Check to see if another record is stored under this name */

      if((get_dbm_entry(hostaux_rec -> preferred_hostname,
	                strlen(hostaux_rec-> preferred_hostname) + 1,
		        &hostdb_entry, hostdb) != ERROR)){

         *hostdb_rec = hostdb_entry;
         return(HOST_STORED_OTHERWISE);

      }

      /* Check to see if given preferred name is another records preferred name */

      if((hentry = search_in_preferred(hostaux_rec -> preferred_hostname, hostdb, hostdb_aux)) != (hostdb_t *) NULL){
 
         *hostdb_rec = *hentry;
         return(HOST_STORED_OTHERWISE);

      }

      /* make sure that preferred name maps back to primary name in DNS */

      if((dns_ent = ar_open_dns_name( hostaux_rec -> preferred_hostname, DNS_EXTERN_ONLY, hostdb )) == (AR_DNS *) NULL){

         return(HOST_CNAME_UNKNOWN);
      }

#ifdef SOLARIS
      if((cmp_dns_name(hostdb_rec -> primary_hostname, dns_ent) == ERROR) &&
         (cmp_dns_name(hostaux_rec -> preferred_hostname, dns_ent) == ERROR)) {
#else
      if(cmp_dns_name(hostdb_rec -> primary_hostname, dns_ent) == ERROR) {
#endif
	 /* preferred name %s doesn't map back to primary name %s but instead to %s */
	 error(A_INTERR, "check_new_hentry", CHECK_NHENTRY_001, 
	       hostaux_rec -> preferred_hostname, 
	       hostdb_rec -> primary_hostname,
	       get_dns_primary_name(dns_ent)
	       );
	 return(HOST_CNAME_MISMATCH);
      }

   } /* endif preferred given */


   /* Check the hostbyaddr cache */

   if(get_dbm_entry(&(hostdb_entry.primary_ipaddr), sizeof(ip_addr_t), &hbyaddr, hostbyaddr) != ERROR){
      get_dbm_entry(hbyaddr.primary_hostname,
	                strlen(hbyaddr.primary_hostname) + 1,
		        &hostdb_entry, hostdb);

      *hostdb_rec = hostdb_entry;
      
      return(HOST_PADDR_EXISTS);
   }


   hostdb_rec -> primary_ipaddr = hostdb_entry.primary_ipaddr;

   return(HOST_OK);

}

/*
 * check_old_hentry: check an "old" primary host database entry.
 */



host_status_t check_old_hentry(hostbyaddr, hostdb, hostdb_rec, hostaux_db, hostaux_rec, flags, control_flags)
   file_info_t  *hostbyaddr; /* host address cache */
   file_info_t  *hostdb;     /* primary host database */
   hostdb_t     *hostdb_rec; /* primary host database record */
   file_info_t  *hostaux_db;     /* aux host database */
   hostdb_aux_t *hostaux_rec; /* aux  host database record */
   flags_t       flags;
   flags_t       control_flags;
{
#ifdef __STDC__

   extern int strcasecmp(char *, char *);

#else

  extern int strcasecmp();

#endif  

   hostdb_t hostdb_entry;
   hostdb_aux_t hostaux_entry;
   AR_DNS *dns_ent;
   hostbyaddr_t hbyaddr;
   ip_addr_t ipaddress;
   index_t index;
   
   ptr_check(hostbyaddr, file_info_t, "check_old_hentry", HOST_ERROR);
   ptr_check(hostdb, file_info_t, "check_old_hentry", HOST_ERROR);
   ptr_check(hostdb_rec, hostdb_t, "check_old_hentry", HOST_ERROR);      


   /* Check primary hostname */

   if((dns_ent = ar_open_dns_name( hostdb_rec -> primary_hostname, DNS_EXTERN_ONLY, hostdb )) == (AR_DNS *) NULL){

      return(HOST_UNKNOWN);
   }

   /* Get the primary hostname for the site */


   if(cmp_dns_name(hostdb_rec -> primary_hostname, dns_ent) != A_OK){

      strcpy(hostdb_rec -> primary_hostname,get_dns_primary_name(dns_ent));

      if(get_dbm_entry(hostdb_rec -> primary_hostname,
                       strlen(hostdb_rec-> primary_hostname) + 1,
		       &hostdb_entry, hostdb) != ERROR){

         *hostdb_rec = hostdb_entry;
         return(HOST_ALREADY_EXISTS);
      }

      return(HOST_NAME_NOT_PRIMARY);
   }

   ipaddress = hostdb_rec -> primary_ipaddr;

   if(cmp_dns_addr(&ipaddress, dns_ent) != A_OK){

      hostdb_rec -> primary_ipaddr = *get_dns_addr(dns_ent);
      return(HOST_PADDR_MISMATCH);
   }
      
   ar_dns_close(dns_ent);

   /* This makes sure the host already exists under the given primary name */

   if(get_dbm_entry(hostdb_rec -> primary_hostname,
                     strlen(hostdb_rec-> primary_hostname) + 1,
		     &hostdb_entry, hostdb) == ERROR){

      return(HOST_DOESNT_EXIST);
   }
#if 0
   if ( get_hostaux_ent(hostdb_entry.primary_hostname,
                        hostaux_rec->origin->access_methods, &index,
                        hostaux_rec->preferred_hostname,
                        hostaux_rec->access_command, &hostaux_entry,
                        hostaux_db) == ERROR ) {

     return (HOST_CNAME_MISMATCH);
   }
   
#endif
   /* Check the hostbyaddr cache */

   if(get_dbm_entry(&(hostdb_rec -> primary_ipaddr), sizeof(ip_addr_t), &hbyaddr, hostbyaddr) == ERROR)
      return(HOST_PADDR_FAIL);


   return(HOST_OK);

}
