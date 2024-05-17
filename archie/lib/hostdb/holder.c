#if 0
#if 0
host_status_t update_hostdb(hostbyaddr, hostdb, hostaux_db, header_rec, flags, control_flags)
   file_info_t *hostbyaddr;
   file_info_t *hostdb;
   file_info_t *hostaux_db;   
   header_t    *header_rec;
   int	       flags;
   int	       control_flags;

{
  host_status_t host_status;

#if 0
  if(header_rec -> action_status == NEW)
     host_status = create_hostdb_ent(hostbyaddr, hostdb, hostaux_db, header_rec, flags, control_flags);
  else if(header_rec -> action_status == UPDATE)
     host_status = update_hostdb_ent(hostbyaddr, hostdb, hostaux_db, header_rec, flags, control_flags);
  else if(header_rec -> action_status == DELETE)
     host_status = delete_hostdb_ent(hostbyaddr, hostdb, hostaux_db, header_rec, flags, control_flags);
#endif
  
  return(host_status);
}



host_status_t create_hostdb_ent(hostbyaddr, hostdb, hostaux_db, header_rec, flags, control_flags)
   file_info_t *hostbyaddr;
   file_info_t *hostdb;
   file_info_t *hostaux_db;   
   header_t    *header_rec;
   int         flags;
   int         control_flags;
{
   AR_DNS *dns_ent;
   host_status_t host_status = 0;
   char aux_name[MAX_PATH_LEN];
   hostdb_aux_t hostaux_entry;
   hostdb_t hostdb_entry;
   char *tmp_ptr;
   hostbyaddr_t hostaddr_entry;


   if(H_IS_HOSTNAME_MODIFY(control_flags)){

      if(put_dbm_entry(hostdb_entry.primary_hostname,
		       strlen(hostdb_entry.primary_hostname) +1,
		       &hostdb_entry,
		       sizeof(hostdb_t),
		       hostdb,
		       0) == ERROR){

         return(host_status | HOST_DB_ERROR);
      }

      if(put_dbm_entry(&hostaddr_entry.primary_ipaddr,
		       sizeof(ip_addr_t),
		       &hostaddr_entry,
		       sizeof(hostbyaddr_t),
		       hostbyaddr,
		       0) == ERROR){

         return(host_status | HOST_DB_ERROR);
      }

   }



   /* Now check the auxiliary db for this access method */

   tmp_ptr = get_dns_primary_name(dns_ent);

   if(get_dbm_entry(tmp_ptr, strlen(tmp_ptr) + 1, &hostdb_entry, hostdb) == ERROR){
      return(host_status |= HOST_ACCESS_EXISTS);
   }

   if(!HDR_GET_ACCESS_METHODS(header_rec -> header_flags))
      return(host_status | HOST_ACCESS_NOT_GIVEN);

   sprintf(aux_name,"%s.%s",hostdb_entry.primary_hostname, make_lcase(header_rec -> access_methods));

   /* Check to see if this hostname.access method already exists. If not
      we can add it. */

   if(get_dbm_entry(aux_name, strlen(aux_name) + 1, &hostaux_entry, hostaux_db) != ERROR){

      make_hostaux_entry( &hostaux_entry, header_rec, flags);

      return(host_status | HOST_ACCESS_EXISTS);
   }

   memset(&hostaux_entry, '\0', sizeof(hostdb_aux_t));      
   make_hostaux_entry( &hostaux_entry, header_rec, flags, header_rec -> access_methods);

   if(H_IS_HOSTNAME_MODIFY(control_flags)){

      if(put_dbm_entry(aux_name, strlen(aux_name) + 1, &hostaux_entry, sizeof(hostdb_aux_t), hostaux_db, 0) == ERROR){

         return(host_status | HOST_ACCESS_INS_FAIL);
      }
   }

 return(host_status);
}




host_status_t update_hostdb_ent(hostbyaddr, hostdb, hostaux_db, header_rec, flags, control_flags)
   file_info_t *hostbyaddr;
   file_info_t *hostdb;
   file_info_t *hostaux_db;   
   header_t    *header_rec;
   int	       flags;
   int         control_flags;

{
   host_status_t host_status = 0;
   hostname_t tmp_name;
   AR_DNS *dns_ent;
   hostdb_t host_entry;

#if 0
   /* Lookup site in database */

   if(get_dbm_entry(header_rec -> primary_hostname,
		    strlen(header_rec -> primary_hostname) + 1,
		    &host_entry, hostdb) == ERROR){

      return(HOST_UNFOUND);
   }

   /* If we've been given a preferred name */

   if(HDR_GET_PREFERRED_HOSTNAME(header_rec -> header_flags)){

      if(get_dbm_entry(header_rec -> preferred_hostname,
	               strlen(header_rec-> preferred_hostname) + 1,
		       &hostdb_entry, hostdb) == ERROR){

	 make_header_hostdb_entry(header_rec,&hostdb_entry,flags);
	   
         host_status |= HOST_ALREADY_EXISTS;
         return(host_status | HOST_STORED_OTHERWISE);
      }


   }

   if((hentry = search_in_preferred(header_rec -> primary_hostname, hostdb)) != (hostdb_t *) NULL){
 
      make_header_hostdb_entry(header_rec,hentry,flags);

      host_status |= HOST_ALREADY_EXISTS;
      return(host_status | HOST_STORED_OTHERWISE);
   }

   if((dns_ent = ar_open_dns_name( header_rec -> primary_hostname, DNS_EXTERN_ONLY, hostdb )) == (AR_DNS *) NULL){

      host_status |= HOST_ALREADY_EXISTS;
      return(host_status |= HOST_UNKOWN);
   }

   if(cmp_dns_addr( host_entry.primary_ipaddr, dns_ent) == ERROR){

      return(host_status |= HOST_PADDR_MISMATCH)

   }

   ar_dns_close(dns_ent);

   return(host_status);

#endif
}



void modify_hostaddr(dns_ent, hostbyaddr, hostdb, host_status, host_entry)
   AR_DNS *dns_ent;
   file_info_t *hostbyaddr;
   file_info_t *hostdb;
   host_status_t *host_status;
   hostdb_t *host_entry;

{
   hostbyaddr_t hbyaddr;

   if(delete_hostdb_entry(&(host_entry -> primary_ipaddr), sizeof(ip_addr_t), hostbyaddr) == ERROR){
      *host_status |= HOST_RENAME_DEL_FAIL;
      return;
   }

   strcpy(hbyaddr.primary_hostname, host_entry -> primary_hostname);

   hbyaddr.primary_ipaddr = *(get_dns_addr(dns_ent));

   if(put_dbm_entry(&hbyaddr.primary_ipaddr, sizeof(ip_addr_t), 
		    &hbyaddr, sizeof(hostbyaddr_t), hostbyaddr,
		    1) == ERROR){

      *host_status |= HOST_RENAME_INS_FAIL;
      return;
   }

   host_entry -> primary_ipaddr = hbyaddr.primary_ipaddr;

   if(put_dbm_entry(host_entry -> primary_hostname, strlen(host_entry -> primary_hostname) + 1,
		    &host_entry, sizeof(hostdb_t), hostdb,
		    TRUE) == ERROR){

      *host_status |= HOST_RENAME_INS_FAIL;
      return;
   }

}


/*
 * Modify the hostname in the database
 */


void modify_hostname(dns_ent, hostbyaddr, hostdb, header_rec, host_status, host_entry)
   AR_DNS *dns_ent;
   file_info_t *hostbyaddr;
   file_info_t *hostdb;
   header_t    *header_rec;
   host_status_t *host_status;
   hostdb_t *host_entry;

{
   hostname_t hold_hostname;
   hostbyaddr_t hbyaddr;
   hostdb_t host_tmp;

   hbyaddr.primary_ipaddr = header_rec -> primary_ipaddr;
   strcpy(hbyaddr.primary_hostname, get_dns_primary_name(dns_ent));

   if(put_dbm_entry(&hbyaddr,sizeof(hostbyaddr_t),
		    &(header_rec -> primary_ipaddr), sizeof(header_rec -> primary_ipaddr),
		    hostbyaddr, TRUE) == ERROR){

      *host_status |= HOST_RENAME_INS_FAIL;
      return;
   }

   strcpy(hold_hostname, host_entry -> primary_hostname);
		      
   strcpy(host_entry -> primary_hostname, get_dns_primary_name(dns_ent));

   /* Make sure replacement isn't already there */

   if(get_dbm_entry(host_entry -> primary_hostname,strlen(host_entry -> primary_hostname) + 1,
		    &host_tmp, hostdb) != ERROR){
		      
      *host_status |= HOST_RENAME_FAIL;
      return;
   }
		      
   if(put_dbm_entry(host_entry -> primary_hostname,strlen(host_entry -> primary_hostname) + 1,
		    &host_entry, sizeof(host_entry),
		    hostdb, FALSE) == ERROR){
		    
     *host_status |= HOST_RENAME_INS_FAIL;
     return;
   }

  if(delete_hostdb_entry(hold_hostname, strlen(hold_hostname) + 1, hostdb) == ERROR){

     *host_status |= HOST_RENAME_DEL_FAIL;
     return;
  }
}


void make_hostdb_entry( hostdb_entry, header_rec, dns_ent, flags)
   hostdb_t *hostdb_entry;
   header_t *header_rec;
   AR_DNS   *dns_ent;
   int      flags;

{
   strcpy(hostdb_entry -> primary_hostname, make_lcase(get_dns_primary_name(dns_ent)));
   strcpy(header_rec -> primary_hostname, hostdb_entry -> primary_hostname);

   hostdb_entry -> primary_ipaddr = *get_dns_addr(dns_ent);
   header_rec -> primary_ipaddr = hostdb_entry -> primary_ipaddr;

   hostdb_entry -> flags = flags;

   if(HDR_GET_PREFERRED_HOSTNAME(header_rec -> header_flags))
      strcpy(hostdb_entry -> preferred_hostname, make_lcase(header_rec -> preferred_hostname));

   if(HDR_GET_OS_TYPE(header_rec -> header_flags))
      hostdb_entry -> os_type = header_rec -> os_type;

   if(HDR_GET_TIMEZONE(header_rec -> header_flags))
      hostdb_entry -> timezone = header_rec -> timezone;


   if(HDR_GET_ACCESS_METHODS(header_rec -> header_flags))
      strcpy(hostdb_entry -> access_methods, header_rec -> access_methods);


   if(HDR_GET_PROSPERO_HOST(header_rec -> header_flags)){
      if(header_rec -> prospero_host == YES)
         HDB_SET_PROSPERO_SITE(hostdb_entry -> flags);
      else
         HDB_UNSET_PROSPERO_SITE(hostdb_entry -> flags);
   }
   
}

void make_hostaux_entry( hostaux_entry, header_rec, flags, aux_name)
   hostdb_aux_t *hostaux_entry;
   header_t     *header_rec;
   int          flags;
   char	        *aux_name;

{

#ifdef __STDC__
extern	time_t	time(time_t *);
#else
extern	time_t	time();
#endif

   hostaux_entry -> flags = flags;

   strcpy(hostaux_entry -> access_methods, aux_name);

   if(HDR_GET_GENERATED_BY(header_rec -> header_flags))
      hostaux_entry -> generated_by = header_rec -> generated_by;


   if((HDR_GET_SOURCE_ARCHIE_HOSTNAME(header_rec -> header_flags)) && (header_rec -> source_archie_hostname[0] != '\0'))
      strcpy(hostaux_entry -> source_archie_hostname, header_rec -> source_archie_hostname);


   if((HDR_GET_ACCESS_COMMAND(header_rec -> header_flags)) && (header_rec -> access_command[0] != '\0'))
      strcpy(hostaux_entry -> access_command, header_rec -> access_command);


   if(HDR_GET_RETRIEVE_TIME(header_rec -> header_flags))
      hostaux_entry -> retrieve_time = header_rec -> retrieve_time;


   if(HDR_GET_PARSE_TIME(header_rec -> header_flags))
      hostaux_entry -> parse_time = header_rec -> parse_time;


   /* Give the time now as the update time */
     
   hostaux_entry -> update_time = time(NULL);
      
   if(HDR_GET_NO_RECS(header_rec -> header_flags))
      hostaux_entry -> no_recs = header_rec -> no_recs;


   if(HDR_GET_CURRENT_STATUS(header_rec -> header_flags))
      hostaux_entry -> current_status = header_rec -> current_status;
}

#endif


#if 0


   if((hentry = search_in_preferred(header_rec -> primary_hostname, hostdb)) != (hostdb_t *) NULL){

      make_header_hostdb_entry(header_rec,hentry,flags);

      host_status |= HOST_ALREADY_EXISTS;
      return(host_status | HOST_STORED_OTHERWISE);
   }


   /* Otherwise check for it externally in DNS*/

   if((header_rec -> preferred_hostname[0] != '\0') &&
      (dns_ent = ar_open_dns_name(header_rec -> preferred_hostname, DNS_EXTERN_ONLY, hostdb)) == (AR_DNS *) NULL){

      return(host_status | HOST_CNAME_UNKNOWN);
   }
   else{
      if((dns_ent = ar_open_dns_name(header_rec -> primary_hostname, DNS_EXTERN_ONLY, hostdb)) == (AR_DNS *) NULL){
         return(host_status | HOST_UNKNOWN);
      }
   }

#endif


status_t hostaux_db_activate(header_rec, hostaux_db)
   header_t *header_rec;
   file_info_t *hostaux_db;

{
   hostdb_aux_t hostaux_entry;
   char aux_name[MAX_PATH_LEN];

   
   sprintf(aux_name,"%s.%s",header_rec -> primary_hostname, header_rec -> access_methods);

   if(get_dbm_entry(aux_name, strlen(aux_name) + 1, &hostaux_entry, hostaux_db) == ERROR)
      return(ERROR);

   hostaux_entry.current_status = ACTIVE;

   if(put_dbm_entry(aux_name, strlen(aux_name) + 1, &hostaux_entry, sizeof(hostdb_aux_t), hostaux_db, DBM_INSERT) == ERROR)
      return(ERROR);

   return(A_OK);
}

#endif
