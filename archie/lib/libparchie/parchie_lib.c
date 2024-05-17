#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "pfs.h"
#include "typedef.h"
#include "ar_search.h"
#include "error.h"
#include "archie_dns.h"
#include "files.h"
#include "db_ops.h"

status_t cvt_token_to_internal(token_list, internal, separator)
   struct token *token_list;
   char *internal;
   int separator;

{
   pathname_t mytmp, tptr;
   struct token *tokenptr;
   char sep = (char) separator;

   ptr_check(internal, char, "cvt_token_to_internal", ERROR);   

   

   if(token_list == (struct token *) NULL){
      internal[0] = '\0';
      return(A_OK);
   }

   tokenptr = token_list;
      
   tokenptr = token_list;
   strcpy(mytmp, tokenptr -> token);

   tokenptr = tokenptr -> next;

   for(; tokenptr != (struct token *) NULL; tokenptr = tokenptr -> next){

      sprintf(tptr, "%c%s", sep, tokenptr -> token);
      strcat(mytmp, tptr);
   }

   strcpy(internal, mytmp);

   return(A_OK);
}

   

status_t check_comp_restrict( restrict, pathname)
   char **restrict;
   char **pathname;
{
   char **chptr;
   char **mypath;
   int found = FALSE;

   for(chptr = restrict; (*chptr != (char *) NULL) && !found; chptr++){

      for(mypath = pathname;(*mypath != (char *) NULL) && !found; mypath++){


	 /* XXX for pathame restrictions, this is where
	    the comparison goes. */

	 if(strstr(*mypath, *chptr))
          found = TRUE;
      }
   }

   return( found ? A_OK : ERROR);
}



/*
 * Find the site file associated with given site name
 */

int find_sitefile_in_db(site_name, sitefile, hostdb)
   hostname_t  site_name;	/* site to be found in database		*/
   file_info_t *sitefile;	/* file info structure to be filled in	*/
   file_info_t *hostdb;

{

   AR_DNS *host_entry;
   ip_addr_t **addr;
   int i;
   int finished = 0;

   /* Get ip address of given site name, checking the local hostdb first */
   
   if((host_entry =  ar_open_dns_name(site_name, DNS_LOCAL_FIRST, hostdb)) == (struct hostent *) NULL )
      return(PRARCH_SITE_NOT_FOUND);

   for(addr = (ip_addr_t **) host_entry -> h_addr_list, i = 0;
       addr[i] != (ip_addr_t *) NULL && ! finished;
       i++) {
     if(access(files_db_filename(inet_ntoa(ipaddr_to_inet(*addr[i])), 0), F_OK) != -1) {
       finished = 1;
     } else {
       error(A_SYSERR, "find_sitefile_in_db", "Error accessing site file %s",
             files_db_filename(inet_ntoa(ipaddr_to_inet(*addr[i])), 0));
       return(PRARCH_CANT_OPEN_FILE);
     }
   }

   if( addr[i] == (ip_addr_t *) NULL )
      return(PRARCH_SITE_NOT_FOUND);

   strcpy(sitefile->filename, files_db_filename(inet_ntoa(ipaddr_to_inet(*addr[i])), 0));

   ar_dns_close(host_entry);

   return(A_OK);
}

