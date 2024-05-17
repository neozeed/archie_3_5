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
#include "error.h"
#include "archie_strings.h"
#include "archie_dbm.h"
#include "lang_hostdb.h"
#include "domain.h"
#include "missing-protos.h"

/*
 * domain.c: routines concerned with the handling of the archie
 * psuedo-domain system
 */


static int myabort = FALSE;
static int sanity_count = 0;

/*
 * compile_domains: This routine takes a set of domains and compares them
 * to what is in domaindb. The final result is a set of domain names in
 * domain_list which are atomic, ie are not the group name for any other
 * set of domains
 */


status_t compile_domains(domains, domain_list, domaindb, count)
  char *domains;         /* string of colon separated domain names */
  domain_t *domain_list; /* array of domain names */
  file_info_t *domaindb; /* domain database being referenced against */
  int *count;            /* number of entries in domain_list */
{
  char *tmp_ptr;
  pathname_t int_domain;

  myabort = FALSE;
  sanity_count = 0;

  ptr_check(domains, char, "compile_domains", ERROR);
  ptr_check(domain_list, domain_t, "compile_domains", ERROR);
  ptr_check(domaindb, file_info_t, "compile_domains", ERROR);
  ptr_check(count, int, "compile_domains", ERROR);

  *count = 0;

  /* If domains is DOMAIN_ALL character, then we need not go further */

  if (strcmp(domains, DOMAIN_ALL) == 0)
  {
    strcpy(domain_list[0], DOMAIN_ALL);
    *count = 1;
    return A_OK;
  }

  strcpy(int_domain, domains);
  strtok(int_domain, NET_DELIM_STR);
  tmp_ptr = int_domain;
  do
  {
    domain_construct(tmp_ptr, domain_list, domaindb, count);
  }
  while (((tmp_ptr = strtok((char *)NULL, NET_DELIM_STR)) != (char *)NULL) && ! myabort);

  if ( ! myabort)
  {
    qsort((char *)domain_list, *count, sizeof(domain_t), strrcmp);
  }
  else
  {
    return ERROR;
  }
      
  return A_OK;
}

   
/*
 * domain_construct: build the domain_list from the given domains with
 * respect to the domain_db
 */


status_t domain_construct(domain,domain_list,domaindb,count)
   char		*domain;      /* colon separated list of domains */
   domain_t	*domain_list; /* list of compiled domains returned */
   file_info_t	*domaindb;    /* domain database */
   int		*count;	      /* number of elements in domain_list */
{
   domain_struct domain_s;

   ptr_check(domain, char, "domain_construct", ERROR);
   ptr_check(domain_list, domain_t, "domain_construct", ERROR);
   ptr_check(domaindb, file_info_t, "domain_construct", ERROR);
   ptr_check(count, int, "domain_construct", ERROR);
   

   if( sanity_count > MAX_DOMAIN_DEPTH){
      myabort = TRUE;
   }
         

   if(get_dbm_entry(domain, strlen(domain) + 1, &domain_s, domaindb) == ERROR){

      sanity_count--;
      strcpy(domain_list[(*count)++],domain);
   }
   else{
      pathname_t tmp_domain;
      pathname_t domain_str;
      int index = 0;
      int len;

      domain_str[0] = '\0';
      strcpy(tmp_domain, domain_s.domain_def);

      len = strlen(tmp_domain);

      while((sscanf(&tmp_domain[index],"%[^:	 ][:\n]",domain_str) != EOF) && !myabort && (index < len)){
	 sanity_count++;

	 domain_construct(domain_str, domain_list, domaindb, count);

	 index += strlen(domain_str) + 1;
      }
   }

   return(A_OK);
}


/*
 * find_in_domains: see if the given hostname conforms to the given domain
 * list
 */



int find_in_domains(hostname, domain_list, count)
   hostname_t hostname;	      /* hostname to be found */
   domain_t *domain_list;     /* domain list that hostname is to be found in */
   int count;		      /* number of elements in the domain list */

{
   ptr_check(hostname, char, "find_in_domains", 0);
   ptr_check(domain_list, domain_t, "find_in_domains", -1);

   if(count <= 0)
      return(-1);

   /* if it's all domains then just return yes */

   if(strcmp(domain_list[0], DOMAIN_ALL) == 0)
      return(1);

   return !! bsearch(hostname, (char *) domain_list, count, sizeof(domain_t), strrcmp);
}
   

/*
 * get_domain_list: get the list of all domains in the domain database
 */

status_t get_domain_list(domain_set, domain_count, domaindb)
    domain_struct *domain_set;	 /* returned set of domains */
    int		  domain_count;	 /* number of elements in domain_set */
    file_info_t	  *domaindb;	 /* domain database */
{
   datum domptr;
   int count, count1;
   domain_t *domain_list;


   ptr_check(domain_set, domain_struct, "get_domain_list", ERROR);
   ptr_check(domaindb, file_info_t, "get_domain_list", ERROR);

   if((domain_list = (domain_t *) malloc(domain_count * sizeof(domain_t))) == (domain_t *) NULL){

      /* "Can't malloc() space for intermediate domain list" */

      error(A_SYSERR, "get_domain_list", GET_DOMAIN_LIST_002);
      return(ERROR);
   }

   domptr = dbm_firstkey(domaindb -> fp_or_dbm.dbm);

   if(dbm_error(domaindb -> fp_or_dbm.dbm)){

      /* "Error reading list of domains from domain database" */

      error(A_ERR, "get_domain_list", GET_DOMAIN_LIST_001);
      free(domain_list);	/*Need to free it */
      return(ERROR);
   }

   count = 0;

   if(domptr.dptr == (char *) NULL){

      /* "Empty domain database" */

      error(A_ERR, "get_domain_list", GET_DOMAIN_LIST_003);
      free(domain_list);	/*Need to free it */
      return(ERROR);
   }

   do{

      if(count < domain_count)
	 strcpy(domain_list[count++], domptr.dptr);
      else {
	 free(domain_list);	/*Need to free it */
	 return(ERROR);
      }

      domptr = dbm_nextkey(domaindb -> fp_or_dbm.fp);

   }while(domptr.dptr != (char *) NULL);

   domain_list[count][0] = '\0';

   for(count1 = 0;
       (get_dbm_entry(domain_list[count1], strlen(domain_list[count1]) + 1, &domain_set[count1], domaindb) != ERROR) && (count1 < count);
       count1++);

   domain_set[count].domain_name[0] = '\0';

   free(domain_list);	/*Need to free it */
   return(A_OK);
}


int cmp_domain_struct(a, b)
  domain_struct *a;
  domain_struct *b;
{
  return strcmp(a->domain_name, b->domain_name);
}


status_t sort_domains(domain_set)
   domain_struct domain_set[];

{
   int q;
   domain_struct *cdom;

   for(cdom = domain_set; cdom -> domain_name[0] != '\0'; cdom++);

   q = cdom - domain_set;
      
   qsort(domain_set, q, sizeof(domain_struct), cmp_domain_struct);

   return(A_OK);
}
   
