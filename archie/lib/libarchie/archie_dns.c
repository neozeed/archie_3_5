/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>	     
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include "typedef.h"
#include "archie_dns.h"
#include "archie_strings.h"
#include "archie_inet.h"
#include "archie_dbm.h"
#ifdef OLD_FILES
#  include "old-host_db.h"
#else
#  include "host_db.h"
#endif
#include "error.h"
#include "lang_libarchie.h"


/*
 * Lookup the given site name. If DNS_LOCAL_ONLY is the dns_type then only
 * look in the local hostdb, DNS_LOCAL_FIRST means to look in the local
 * hostdb first and then if that fails, look to full DNS. DNS_EXTERN_ONLY
 * means ignore the local hostdb completely
 */


AR_DNS *ar_gethostbyname(site_name, lookup_type, hostdb)
   char *site_name;		/* Site to be looked up */
   dns_t lookup_type;		/* Type of lookup to be performed */
   file_info_t *hostdb;		/* hostdb access info */
{
  extern struct hostent *ar_ghbn();
  /*   extern struct hostent *gethostbyname();*/

  hostname_t hostname;
  struct hostent *host_entry;

  strcpy(hostname, site_name);

  /* Make it lowercase for comparison */

  make_lcase(hostname);

  switch(lookup_type){
   
  case DNS_LOCAL_ONLY:
    if(hostdb == (file_info_t *) NULL){

	    /* "hostdb pointer is NULL" */

	    error(A_INTERR,"ar_gethostbyname", AR_GETHOSTBYNAME_001);
	    return((AR_DNS *) NULL);
    }
   
    host_entry = ar_ghbn( hostname, hostdb);
    break;
	 
  case DNS_LOCAL_FIRST:
    if(hostdb == (file_info_t *) NULL){

	    /* "hostdb pointer is NULL" */

	    error(A_INTERR,"ar_gethostbyname", AR_GETHOSTBYNAME_001);
	    return((AR_DNS *) NULL);
    }

    if((host_entry = ar_ghbn( hostname, hostdb )) == (struct hostent *) NULL )
    host_entry =  gethostbyname( hostname );

    break;
	   
  case DNS_EXTERN_ONLY:

    host_entry =  gethostbyname( hostname);
    break;

  default:

    return((AR_DNS *) NULL);
    break;
  }
   
  return(host_entry);
}

/*
 * ar_gethostbyaddr: analogous to ar_gethostbyname, but lookup is based on
 * the IP address.
 */


AR_DNS *ar_gethostbyaddr(ipaddr, lookup_type, hostbyaddr)
   ip_addr_t ipaddr;		/* Site to be looked up */
   dns_t lookup_type;		/* Type of lookup to be performed */
   file_info_t *hostbyaddr;	/* hostdb access info */
{
  struct hostent *host_entry;
  struct in_addr ineta;

  switch(lookup_type){
   
  case DNS_LOCAL_ONLY:
    if(hostbyaddr == (file_info_t *) NULL){

	    /* "hostbyaddr pointer is NULL" */

	    error(A_INTERR,"ar_gethostbyaddr", AR_GETHOSTBYADDR_001);
	    return((AR_DNS *) NULL);
    }
   
    host_entry = ar_ghba( ipaddr, hostbyaddr);
    break;
	 
  case DNS_LOCAL_FIRST:
    if(hostbyaddr == (file_info_t *) NULL){

	    /* "hostbyaddr pointer is NULL" */

	    error(A_INTERR,"ar_gethostbyaddr", AR_GETHOSTBYADDR_001);	 
	    return((AR_DNS *) NULL);
    }

    if((host_entry = ar_ghba( ipaddr, hostbyaddr )) == (struct hostent *) NULL ){

	    ineta = ipaddr_to_inet(ipaddr);

	    host_entry =  gethostbyaddr( (char *) &ineta, sizeof(struct in_addr), AF_INET );
      if ( ! host_entry)        /*wheelan*/
      {
        extern int h_errno;
        char *h;

        if (h_errno != 0)
        {
          switch (h_errno){
          case 1: h = "Authoritive Answer Host not found"; break;
          case 2: h = "Non-Authoritive Host not found, or SERVERFAIL"; break;
          case 3: h = "Non recoverable errors, FORMERR, REFUSED, NOTIMP"; break;
          case 4: h = "Valid name, no data record of requested type"; break;
          default: h = "<unlisted error>"; break;
          }
          error(A_ERR, "ar_gethostbyaddr", "gethostbyaddr(): h_errno = %d, %s",
                h_errno, h);
        }
      }
      
    }

    break;
	   
  case DNS_EXTERN_ONLY:

    ineta = ipaddr_to_inet(ipaddr);

    host_entry =  gethostbyaddr( (char *) &ineta, sizeof(struct in_addr), AF_INET );
    if ( ! host_entry)          /*wheelan*/
    {
      extern int h_errno;
      char *h;

      if (h_errno != 0)
      {
        switch (h_errno)
        {
        case 1: h = "Authoritive Answer Host not found"; break;
        case 2: h = "Non-Authoritive Host not found, or SERVERFAIL"; break;
        case 3: h = "Non recoverable errors, FORMERR, REFUSED, NOTIMP"; break;
        case 4: h = "Valid name, no data record of requested type"; break;
        default: h = "<unlisted error>"; break;
        }
        error(A_ERR, "ar_gethostbyaddr", "gethostbyaddr(): h_errno = %d, %s",
              h_errno, h);
      }
    }

    break;

  default:

    return((AR_DNS *) NULL);
    error(A_ERR,"ar_gethostbyaddr","ar_gethostbyaddr: Did not provide type of lookup.");
    break;
  }
   
  return(host_entry);
}



/*
 * ar_ghbn: The gethostbyname cache. Given a FQDN, lookup the address that
 * archie thinks it has. Return NULL if you can't find it.
 */


struct hostent *ar_ghbn(hostname, hostdb)
   hostname_t hostname;	/* Hostname to search for */
   file_info_t *hostdb; /* hostdb pointer */
{
   datum host_key;
   static datum db_result;
   static struct hostent hoststr;
   static hostname_t host_name;
   static hostname_t alias_name;
   static char *aliaslist[2];
   static ip_addr_t *netlist[2];
   hostdb_t hostdb_ent;

   if(hostname == (char *) NULL){

      /* "hostname is NULL" */

      error(A_INTERR,"ar_ghbn", AR_GHBN_001);
      return((struct hostent *) NULL);
   }

   if((hostdb == (file_info_t *) NULL) || (hostdb -> fp_or_dbm.dbm == (DBM *) NULL)){

   /* "hostdb pointer or contents is NULL" */

      error(A_INTERR, "ar_ghbn", AR_GHBN_002);
      return((struct hostent *) NULL);
   }

   /* First, see if the name as given is stored */

   host_key.dptr = hostname;
   host_key.dsize = strlen(hostname) + 1;

   db_result = dbm_fetch(hostdb -> fp_or_dbm.dbm,host_key);

   if(dbm_error(hostdb -> fp_or_dbm.dbm) != 0){
     dbm_clearerr(hostdb -> fp_or_dbm.dbm);
     return((struct hostent *) NULL);
   }

   if(db_result.dptr == (char *) NULL)
      return((struct hostent *) NULL);

   memcpy(&hostdb_ent, db_result.dptr, sizeof(hostdb_t));


   /* Otherwise, set up struct hostent for return */

   strcpy(host_name, hostdb_ent.primary_hostname);
   hoststr.h_name = host_name;

/*   strcpy(alias_name, hostdb_ent.preferred_hostname); */
   strcpy(alias_name, "");
   hoststr.h_aliases = aliaslist;

   aliaslist[0] = alias_name;
   aliaslist[1] = (char *) NULL;
   
   hoststr.h_addrtype = AF_INET;
   hoststr.h_length = sizeof(struct in_addr);

   hoststr.h_addr_list = (char **) netlist;
   netlist[0] = (ip_addr_t *) malloc(hoststr.h_length);
   netlist[1] = (ip_addr_t *) NULL;

   memcpy(netlist[0],&hostdb_ent.primary_ipaddr,hoststr.h_length);

   return(&hoststr);
}

/*
 * ar_ghba: analogous to ar_ghbn, but using the address as opposed to the
 * name
 */


struct hostent *ar_ghba(ipaddr, hostbyaddr)
   ip_addr_t ipaddr;	      /* addressed to be searched for */
   file_info_t *hostbyaddr;   /* hostbyaddr database pointer */
{
   static struct hostent hoststr;
   static hostname_t host_name;
   static char *aliaslist[2];
   static ip_addr_t *netlist[2];
   hostbyaddr_t hostbyaddr_rec;

   if((hostbyaddr == (file_info_t *) NULL) || (hostbyaddr -> fp_or_dbm.dbm == (DBM *) NULL)){

      /* "hostbyaddr pointer or contents is NULL" */

      error(A_INTERR, "ar_ghba", AR_GHBA_001);
      return((struct hostent *) NULL);
   }

   /* First, see if the name as given is stored */

   if(get_dbm_entry(&ipaddr, sizeof(ip_addr_t), &hostbyaddr_rec, hostbyaddr) == ERROR)
     return((struct hostent *) NULL);

   /* Otherwise, set up struct hostent for return */

   strcpy(host_name, hostbyaddr_rec.primary_hostname);
   hoststr.h_name = host_name;

   aliaslist[0] = (char *) NULL;
   
   hoststr.h_addrtype = AF_INET;
   hoststr.h_length = sizeof(struct in_addr);

   hoststr.h_addr_list = (char **) netlist;
   /*
    *  bug: h_addr_list[0] is a memory leak.  If you fix it here, remove the
    *       free from the telnet-client code.
    */
   netlist[0] = (ip_addr_t *) malloc(hoststr.h_length);
   netlist[1] = (ip_addr_t *) NULL;

   memcpy(netlist[0],&hostbyaddr_rec.primary_ipaddr,hoststr.h_length);

   return(&hoststr);
}



/*
 * ar_open_dns_name: The upper levels of the system see a DNS record as an
 * opaque data type. This "opens" that record
 */

AR_DNS *ar_open_dns_name( hostname, dns_type, hostdb )
   hostname_t  hostname;   /* hostname to be opened */
   dns_t dns_type;	   /* Look for it locally, externally or both */
   file_info_t *hostdb;	   /* hostdb database pointer */
{

   AR_DNS *tmp_hent = 0;
   AR_DNS *hostentry;

   if(hostname == (char *) NULL){

      /* "hostname is NULL" */

      error(A_INTERR, "ar_open_dns_name", AR_OPEN_DNS_NAME_001);
      return((AR_DNS *) NULL);
   }

   if((dns_type != DNS_EXTERN_ONLY) && ((hostdb == (file_info_t *) NULL) || (hostdb -> fp_or_dbm.dbm == (DBM *) NULL))){

      /* "hostdb pointer or contents is NULL" */

      error(A_INTERR, "ar_open_dns_name", AR_OPEN_DNS_NAME_002);
      return((AR_DNS *) NULL);
   }

   switch(dns_type){

      case DNS_EXTERN_ONLY:

         if((tmp_hent = ar_gethostbyname(hostname, dns_type, (file_info_t *) NULL)) == (AR_DNS *) NULL)
           return((AR_DNS *) NULL);
      break;

      case DNS_LOCAL_ONLY:
      case DNS_LOCAL_FIRST:

         if((tmp_hent = ar_gethostbyname(hostname, dns_type, hostdb)) == (AR_DNS *) NULL)
           return((AR_DNS *) NULL);
      break;
   }

   if((hostentry = (AR_DNS *)  malloc(sizeof(AR_DNS))) == (AR_DNS *) NULL){

      /* "Can't malloc() space for hostentry" */

      error(A_INTERR, "ar_open_dns_name", AR_OPEN_DNS_NAME_003);
      return((AR_DNS *) NULL);
   }
      
   *hostentry = *tmp_hent;

   return(hostentry);

}

/*
 * ar_open_dns_addr: analogous to ar_open_dns_name, using address instead
 */


AR_DNS *ar_open_dns_addr( ipaddr, dns_type, hostbyaddr )
   ip_addr_t ipaddr;	/* Address to be opened */
   dns_t dns_type;	/* open locally, externally or try both */
   file_info_t *hostbyaddr; /* hostbyaddr database pointer */
{

   AR_DNS *tmp_hent = 0;
   AR_DNS *hostentry;

   if((dns_type != DNS_EXTERN_ONLY) &&
      ((hostbyaddr == (file_info_t *) NULL)
        || (hostbyaddr -> fp_or_dbm.dbm == (DBM *) NULL))){

      /* "hostbyaddr pointer or contents is NULL" */

      error(A_INTERR, "ar_open_dns_addr", AR_OPEN_DNS_ADDR_001);
      return((AR_DNS *) NULL);
   }
      

   switch(dns_type){

      case DNS_EXTERN_ONLY:

         if((tmp_hent = ar_gethostbyaddr(ipaddr, dns_type, (file_info_t *) NULL)) == (AR_DNS *) NULL)
           return((AR_DNS *) NULL);
      break;

      case DNS_LOCAL_ONLY:
      case DNS_LOCAL_FIRST:

         if((tmp_hent = ar_gethostbyaddr(ipaddr, dns_type, hostbyaddr)) == (AR_DNS *) NULL)
           return((AR_DNS *) NULL);
      break;
   }

   if((hostentry = (AR_DNS *)  malloc(sizeof(AR_DNS))) == (AR_DNS *) NULL){

      /* "Can't malloc() space for hostentry" */

      error(A_INTERR, "ar_open_dns_addr", AR_OPEN_DNS_ADDR_002);
      return((AR_DNS *) NULL);
   }
      
   *hostentry = *tmp_hent;

   return(hostentry);

}

/*
 * cmp_dns_name: compare the primary name in a given DNS entry with the
 * given hostname. Case instensitive
 */


status_t cmp_dns_name( hostname, dns_ent )
   hostname_t hostname;	   /* Hostname to be compared */
   AR_DNS *dns_ent;	   /* DNS entry */

{
#if 0 
#ifdef __STDC__

   extern int strcasecmp(char *, char *);

#else
   
   extern int strcasecmp();
#endif
#endif

   if((hostname == (char *) NULL) || (dns_ent == (AR_DNS *) NULL)){

      /* "hostname or DNS entry is NULL" */

      error(A_INTERR, "cmp_dns_name", CMP_DNS_NAME_001);
      return(ERROR);
   }

   if(strcasecmp( hostname, dns_ent -> h_name) == 0)
      return(A_OK);
   else
      return(ERROR);
}

/*
 * cmp_dns_addr: analogous to cmp_dns_name, but comparing the primary IP
 * address
 */
  

status_t cmp_dns_addr( address, dns_ent )
   ip_addr_t *address;	   /* IP address to be compared */
   AR_DNS *dns_ent;	   /* DNS entry */

{
   int i;

   if(dns_ent == (AR_DNS *) NULL){

      /* "DNS entry is NULL" */

      error(A_INTERR, "cmp_dns_name", CMP_DNS_ADDR_001);
      return(ERROR);
   }

   for(i = 0; dns_ent -> h_addr_list[i] != (char *) NULL; i++){

      if(memcmp(address, dns_ent -> h_addr_list[i], sizeof(ip_addr_t)) == 0)
        return(A_OK);
   }

   return(ERROR);

}


/*
 * get_dns_primary_name: return the primary hostname from the given DNS
 * entry. The result is held in a static area and so should be copied on
 * return
 */


char *get_dns_primary_name(dns_ent)
   AR_DNS *dns_ent;

{
   static hostname_t tmp_hostname;

   if(dns_ent == (AR_DNS *) NULL){

      /* "DNS entry is NULL" */

      error(A_INTERR, "get_dns_primary_name", GET_DNS_PRIMARY_NAME_001);
      return((char *) NULL);
   }

   strcpy( tmp_hostname, dns_ent -> h_name);

   make_lcase(tmp_hostname);
 
   return(tmp_hostname);
}

/*
 * get_dns_addr: analogous to get_dns_primary_name. Return primary IP
 * address. Result is a pointer to static holding area
 */


ip_addr_t *get_dns_addr(dns_ent)
   AR_DNS *dns_ent;

{
   static ip_addr_t *hostaddr;

   if(dns_ent == (AR_DNS *) NULL){

      /* "DNS entry is NULL" */

      error(A_INTERR,"get_dns_addr", GET_DNS_ADDR_001);
      return((ip_addr_t *) NULL);
   }

   if(dns_ent -> h_addr_list == 0){

      error(A_INTERR,"get_dns_addr", "h_add_list of dns_ent is NULL");
      return((ip_addr_t *) NULL);
   }
      
   
   hostaddr = (ip_addr_t *) (dns_ent -> h_addr_list[0]);

   return(hostaddr);
}


/*
 * ar_dns_close: close the current DNS entry and free the associated space
 */


status_t ar_dns_close(dns_ent)
   AR_DNS *dns_ent;

{
   if(dns_ent == (AR_DNS *) NULL){

      /* "DNS entry is NULL" */

      error(A_INTERR,"get_dns_addr", AR_DNS_CLOSE_001);
      return(ERROR);
   }

#if !defined(__STDC__) && !defined(AIX)

   if(free(dns_ent) == 0){

      /* "Can't free dns entry" */

      error(A_SYSERR,"ar_dns_close", AR_DNS_CLOSE_002);
      return(ERROR);
   }

#else

   free(dns_ent);
#endif
   

   return(A_OK);
}


