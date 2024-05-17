#include <stdio.h>
#include <stdlib.h>
#include "typedef.h"
#include "archie_dbm.h"
#include "archie_strings.h"
#include "error.h"


long add_host_to_hash(hname, curr_hostoff, host_hash_finfo, host_finfo)
   char *hname;
   int *curr_hostoff;
   file_info_t *host_hash_finfo;
   file_info_t *host_finfo;
{
   static ftime;
   long hid;
   hostname_t hnm;


   if(!ftime){
      srand(time((time_t *) NULL));
      ftime = 1;
   }

   ptr_check(hname, char, "add_host_to_hash", 0);
   ptr_check(host_hash_finfo, file_info_t, "add_host_to_hash", 0);   

   if(host_hash_finfo -> fp_or_dbm.dbm == (DBM *) NULL){

      error(A_INTERR, "add_host_to_hash", "Passed NULL file pointer");
      return -1;
   }

   make_lcase(hname);

try_again:

   if(get_dbm_entry(hname, strlen(hname) +1, &hid, host_hash_finfo) == ERROR){

      /* Not found */

      hid = rand();

      if(put_dbm_entry(hname, strlen(hname) + 1, &hid, sizeof(hid), host_hash_finfo, 0)){

	 error(A_ERR, "add_host_to_hash", "Error trying to put %s into host hash database", hname);
	 return -1;
      }

      if(fwrite(hname, strlen(hname) + 1, 1, host_finfo -> fp_or_dbm.fp) == 0){

	 error(A_INTERR, "add_host_to_hash", "Error while writing '%s' to host strings file", hname);
	 return -1;
      }

   }
   else{

      /* Make sure that it's the same host */

      if(get_dbm_entry(&hid, sizeof(hid), hnm, host_rhash_finfo) == ERROR){

	 error(A_INTERR, "add_host_to_hash", "Can't find hostid %d in reverse host hash database", hid);
	 return -1;
      }

      if(strcasecmp(hnm, hname) != 0){

	 /* Not the same host ! */

	 goto try_again;
      }

   }
	 
   return hid;

}
