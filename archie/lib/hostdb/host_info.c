/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "typedef.h"
#include "header.h"
#include "host_db.h"
#include "error.h"
#include "debug.h"
#include "db_ops.h"
#include "archie_dbm.h"
#include "lang_hostdb.h"

#include "protos.h"
#include "archie_strings.h"




void set_aux_origin( hostaux_entry, dn, index )
  hostdb_aux_t *hostaux_entry;
  char *dn;
  index_t index;
{

  if ( hostaux_entry -> origin == (hostaux_index_t*) NULL ) {
    hostaux_entry -> origin = (hostaux_index_t*) malloc(sizeof(hostaux_index_t));
  }

  if ( hostaux_entry -> origin != NULL ) {
    strcpy(hostaux_entry -> origin -> access_methods, dn );
    hostaux_entry -> origin -> hostaux_index = index;
  }

}


/*
 * get_preferred_name: obtain the preferred name of the given primary IP
 * address. preferred and primary hostnames will be returned. Host name
 * stored in a static area
 */

#ifdef OLD


char *get_preferred_name(address, primary_hostname, hostdb, hostbyaddr)
   ip_addr_t address;		 /* the IP address of the host to be gotten */
   hostname_t  primary_hostname; /* the primary hostname of the host */
   file_info_t *hostdb;		 /* The primary host database */
   file_info_t *hostbyaddr;	 /* The reverse host database */

{
   static hostname_t  hostname;
   hostbyaddr_t	  hostbyaddr_rec;
   hostdb_t	  hostdb_rec;

   if(get_dbm_entry(&address, sizeof(ip_addr_t), &hostbyaddr_rec, hostbyaddr) == ERROR){


      /* "Can't find address in host address cache" */

      error(A_ERR, "get_preferred_name", GET_PREFERRED_NAME_001);
      return((char *) NULL);
   }

   strcpy(primary_hostname, hostbyaddr_rec.primary_hostname);

   if(get_dbm_entry(hostbyaddr_rec.primary_hostname, strlen(hostbyaddr_rec.primary_hostname) + 1, &hostdb_rec, hostdb) == ERROR){

      /* "Can't find name from host address cache in primary host database" */

      error(A_ERR, "get_preferred_name", GET_PREFERRED_NAME_002);
      return((char *) NULL);
   }

   if(hostdb_rec.preferred_hostname[0] == '\0')
      strcpy(hostname, hostdb_rec.primary_hostname);
   else
      strcpy(hostname, hostdb_rec.preferred_hostname);

   return(hostname);
}

#endif

/*
 * get_hostnames: get a sorted list of all the hostnames in the primary
 * host database
 */


status_t get_hostnames(hostdb, hostlist, hostcount, domain_list, domain_count)
  file_info_t *hostdb;   /* primary host database */
  hostname_t **hostlist; /* vector of hostnames returned */
  int *hostcount;        /* count of number of hostnames returned */
  domain_t *domain_list; /* list of domains that have to match */
  int domain_count;
{
  extern int strcasecmp();

  datum host_search;
  int count;
  int max_hosts = DEFAULT_NO_HOSTS;

  ptr_check(hostdb, file_info_t, "get_hostnames", ERROR);
  ptr_check(hostlist, hostname_t *, "get_hostnames", ERROR);
  ptr_check(hostcount, int, "get_hostnames", ERROR);

  if ((*hostlist = (hostname_t *)malloc(DEFAULT_NO_HOSTS * sizeof(hostname_t)))
      == (hostname_t *) NULL)
  {
    /* "Can't malloc space for hostlist" */
    error(A_SYSERR, "get_hostnames", GET_HOSTNAMES_001);
    return ERROR;
  }
 
  for (host_search = dbm_firstkey(hostdb->fp_or_dbm.dbm), count = 0;
      host_search.dptr != (char *) NULL;)
  {
    if (domain_list != (domain_t *)NULL)
    {
      if (find_in_domains(host_search.dptr, domain_list, domain_count))
      {
        strcpy((*hostlist)[count], host_search.dptr);
        count++;
      }
    }
    else
    {
      strcpy((*hostlist)[count], host_search.dptr);
      count++;
    }

    host_search = dbm_nextkey(hostdb->fp_or_dbm.dbm);
    if (count >= max_hosts)
    {
      max_hosts += DEFAULT_ADD_HOSTS;
      if ((*hostlist = (hostname_t *)realloc(*hostlist, max_hosts * sizeof(hostname_t)))
          == (hostname_t *)NULL)
      {
        /* "Can't realloc space for hostlist" */
        error(A_SYSERR, "get_hostnames", GET_HOSTNAMES_002);
        return ERROR;
      }
    }
  }

  (*hostlist)[count][0] = '\0';

  /* sort resulting list */

  qsort((char *)*hostlist, count, sizeof(hostname_t), strcasecmp);
  *hostcount = count;
   
  return A_OK;
}



/*
 * search_in_preferred: check to see if the given hostname is the preferred
 * name of another record
 */



hostdb_t *search_in_preferred(hostname, hostdb, hostdb_aux)
  hostname_t hostname;	   /* given hostname */
  file_info_t *hostdb;	   /* primary host database */
  file_info_t *hostdb_aux;	/* auxilary host database */

{

   static hostdb_t hostdb_ent;
   static hostdb_aux_t hostaux_ent;
   hostname_t *hostlist;
   hostname_t *hostptr;
   int hostcount,flag;
   

   ptr_check(hostdb, file_info_t, "search_in_preferred", ((hostdb_t *) NULL));
   
   if((hostname == (char *) NULL) || (hostname[0] == '\0')){

      /* "Invalid hostname parameter" */

      error(A_ERR, "update_hostaux", SEARCH_IN_PREFERRED_001);
      return((hostdb_t *) NULL);
   }
      
   /*
    * Go through and find out if is stored as a preferred name
    */

   if(get_hostnames(hostdb, &hostlist, &hostcount, (domain_t *) NULL, 0) == ERROR)
      return((hostdb_t *) NULL);

   for(hostptr = hostlist; *hostptr[0] != '\0'; hostptr++){
     char **ac;
     int i;
     index_t j;
     
   retry_get:
     if(get_dbm_entry(*hostptr, strlen(*hostptr) + 1, &hostdb_ent, hostdb) == ERROR)
	 continue;
     
     if((ac = str_sep(hostdb_ent.access_methods, NET_DELIM_CHAR)) == (char **) NULL)
     continue;
     flag = 0;

     for ( i = 0; ac[i] != NULL; i++ ){
       if ( ac[i][0] == '\0' ) {
         int j;
         pathname_t  t1,t2;
         t1[0] = t2[0] = '\0';
         for (j=0; ac[j] != NULL; j++ ) {
           if ( ac[j][0] != '\0' ) {
             if (t1[0] == '\0' ) {
               strcpy(t1,ac[j]);
             }
             else {
               sprintf(t2,"%s:%s",t1,ac[j]);
               strcpy(t1,t2);
             }
           }
         }
         strcpy(hostdb_ent.access_methods, t1);
         put_dbm_entry(*hostptr, strlen(*hostptr) + 1, &hostdb_ent, sizeof(hostdb_ent), hostdb,TRUE);
         goto retry_get;
       }
     }

     
     for ( i = 0; ac[i] != NULL && flag == 0; i++ ) {

       if ( get_hostaux_ent(hostdb_ent.primary_hostname, ac[i], &j,
                            hostname, NULL,
                            &hostaux_ent, hostdb_aux ) == A_OK ) {

           flag = 1;
           break;
         }
     }
     free(ac);
     if ( flag  )
       break;
   }

   free(hostlist);

   if(*hostptr[0] == '\0')
      return((hostdb_t *) NULL);
   else
      return(&hostdb_ent);
}


status_t get_hostaux_entry(hostname, dbname, index, hostaux_ent, hostaux)
   char *hostname;
   char *dbname;
   index_t index;
   hostdb_aux_t *hostaux_ent;
   file_info_t *hostaux;

{
   pathname_t tmp_str;
   hostaux_index_t *hit;

   ptr_check(hostname, char, "get_hostaux_entry", ERROR);
   ptr_check(dbname, char, "get_hostaux_entry", ERROR);
   ptr_check(hostaux_ent, hostdb_aux_t,"get_hostaux_entry", ERROR);
   ptr_check(hostaux, file_info_t, "get_hostaux_entry", ERROR);

   sprintf(tmp_str, "%s.%s.%d", hostname, dbname, (int)index);

   hit = hostaux_ent->origin;
   if(get_dbm_entry(tmp_str, strlen(tmp_str) + 1, hostaux_ent, hostaux) == ERROR){

#if 0
      /* "Can't find database %s for site %s" */

      error(A_ERR, "get_hostaux_ent", GET_HOSTAUX_ENT_001, dbname, hostname);
#endif
      hostaux_ent->origin = hit;
      return(ERROR);
   }

   hostaux_ent->origin = hit;
   return(A_OK);
}



status_t get_hostaux_ent(pri_name, dbname, index, pref_name, access_command, hostaux_entry,  hostaux_db)
  char *pri_name;
  char *dbname;
  index_t *index;
  char *pref_name;
  access_comm_t access_command;
  hostdb_aux_t *hostaux_entry;
  file_info_t *hostaux_db;
{
  pathname_t aux_name;
  hostdb_aux_t hostaux_rec;
  int p1, p2;


  if ( get_port(access_command, dbname, &p1) == ERROR ) {
    return ERROR;
  }
  
  *index = 0;
  while ( 1 ) {
    sprintf(aux_name,"%s.%s.%d",pri_name,dbname,(int)(*index));
    if(get_dbm_entry(aux_name, strlen(aux_name) + 1, &hostaux_rec, hostaux_db) == ERROR){

      return(ERROR);
    }

    if ( pref_name != NULL && strcasecmp(hostaux_rec.preferred_hostname, pref_name) ) {
      (*index)++;
      continue;
    }

    
    if ( get_port(hostaux_rec.access_command, dbname, &p2) == ERROR ) {
      (*index)++;
      continue;
    } 

    if ( p1 != p2 ) {
      (*index)++;
      continue;
    }
    
    hostaux_rec.origin = hostaux_entry->origin;
    *hostaux_entry = hostaux_rec;
    return(A_OK);
  }

  return(A_OK); /* Keep compiler happy */

}


status_t delete_hostaux_entry(hostname, hostaux_ent, hostaux)
   char *hostname;
   hostdb_aux_t *hostaux_ent;
   file_info_t *hostaux;

{
   pathname_t tmp_str;
   index_t index, i;


   ptr_check(hostname, char, "get_hostaux_ent", ERROR);
   ptr_check(hostaux_ent, hostdb_aux_t,"get_hostaux_ent", ERROR);
   ptr_check(hostaux, file_info_t, "get_hostaux_ent", ERROR);


   find_hostaux_last(hostname,
                     hostaux_ent->origin->access_methods,
                     &index, hostaux);
   
   sprintf(tmp_str, "%s.%s.%d", hostname,
           hostaux_ent->origin->access_methods,
           (int)hostaux_ent->origin->hostaux_index);

   
   if(delete_dbm_entry(tmp_str, strlen(tmp_str) + 1, hostaux) == ERROR){

#if 0
      /* "Can't find database %s for site %s" */

      error(A_ERR, "get_hostaux_entry", GET_HOSTAUX_ENT_001, dbname, hostname);
#endif
      return(ERROR);
   }

   for ( i = hostaux_ent->origin->hostaux_index; i < index; i++ ) {
     hostdb_aux_t hostaux_rec;
     
     sprintf(tmp_str, "%s.%s.%d", hostname,
             hostaux_ent->origin->access_methods, (int)i+1);
     
     if(get_dbm_entry(tmp_str, strlen(tmp_str) + 1, &hostaux_rec, hostaux) == ERROR){
       return ERROR;
     }

     sprintf(tmp_str, "%s.%s.%d", hostname,
             hostaux_ent->origin->access_methods, (int)i);
     
     if (put_dbm_entry(tmp_str, strlen(tmp_str) + 1, &hostaux_rec, sizeof(hostaux_rec), hostaux, TRUE ) == ERROR){
       return ERROR;
     }
   }

   if (hostaux_ent->origin->hostaux_index != index ) {
     sprintf(tmp_str, "%s.%s.%d", hostname,
             hostaux_ent->origin->access_methods,(int)index);


     if(delete_dbm_entry(tmp_str, strlen(tmp_str) + 1, hostaux) == ERROR){

       return(ERROR);
     }
   }
   
   return(A_OK);
}



status_t delete_hostaux_ent(hostname, dbname,  hostaux)
  char *hostname;
  char *dbname;
  file_info_t *hostaux;
{
  pathname_t tmp_str;
  index_t index, i;


  ptr_check(hostname, char, "delete_hostaux_ent", ERROR);
  ptr_check(hostname, char, "delete_hostaux_ent", ERROR);
  ptr_check(hostaux, file_info_t, "delete_hostaux_ent", ERROR);


  find_hostaux_last(hostname, dbname, &index, hostaux);
   
  for ( i = 0 ; i < index; i++ ) {
     
    sprintf(tmp_str, "%s.%s.%d", hostname, dbname, (int)i);
    
     if(delete_dbm_entry(tmp_str, strlen(tmp_str) + 1, hostaux) == ERROR){
       return ERROR;
     }
  }

  return(A_OK);
}


void find_hostaux_last( hostname, dbname, index, hostaux)
  char *hostname;
  char *dbname;
  index_t *index;
  file_info_t *hostaux;
{
  index_t i;
  pathname_t aux_name;
  hostdb_aux_t hostaux_entry;
  
  i = 0;
  while ( 1 ) {
    sprintf(aux_name,"%s.%s.%d",hostname,dbname,(int)i);
    if(get_dbm_entry(aux_name, strlen(aux_name) + 1, &hostaux_entry, hostaux) == ERROR){
      *index = i-1;
      return ;
    }
    i++;
  }

}
  


void clean_hostaux(hostaux_db)
  hostdb_aux_t *hostaux_db;
{
/*  hostaux_index_t *hit;*/

/*  hit = hostaux_db->origin; */
  memset(hostaux_db, '\0', sizeof(hostdb_aux_t));
/*  hostaux_db->origin = hit; */
  set_aux_origin(hostaux_db, "", -1);
}


void copy_hostaux( h1, h2 )
  hostdb_aux_t *h1, *h2;
{
  
  hostaux_index_t *hit;
  *(h1) = *(h2);
  hit = h1->origin;
  *h1 = *h2;
  h1->origin = hit;
}

/*
 *  hit = (hostaux_index_t *)malloc(sizeof(hostaux_index_t)); *(h1) = *(h2);
 *  hit = h1->origin hit->hostaux_index = h1->origin->hostaux_index;
 *  strcpy(hit->access_methods,h1->origin->access_methods);
 *  
 *    *h1 = *h2; *
 *  memcpy(&(*h1),&(*h2), sizeof(hostdb_aux_t)); if( h1->origin )
 *  free(h1->origin); h1->origin = hit;*
 */

status_t get_index_from_port( hostname, dbname, port, index, hostaux)
  hostname_t hostname;
  char *dbname;
  int port;
  index_t *index;
  file_info_t *hostaux;
{

  index_t last;
  int i,p;
  hostdb_aux_t hostaux_rec;
  
  find_hostaux_last(hostname, dbname, &last, hostaux);

  for ( i = 0; i <= (int)last; i++ ) {

    if ( get_hostaux_entry( hostname, dbname, (index_t)i, &hostaux_rec, hostaux) == ERROR )
    continue;

    if ( get_port( hostaux_rec.access_command, dbname, &p ) == ERROR )
      continue;

    if ( p == port )
      return A_OK;
  }

  return (ERROR);
}
