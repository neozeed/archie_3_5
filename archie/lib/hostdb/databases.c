#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "typedef.h"
#include "databases.h"
#include "error.h"
#include "archie_strings.h"
#ifdef OLD_FILES
#  include "old-host_db.h"
#else
#  include "host_db.h"
#endif

#include "protos.h"


status_t compile_database_list(databases, database_list, count)
   char *databases;
   database_name_t *database_list;
   int *count;

{
   char *tmp_ptr;
   database_name_t d_name;

   if(strcmp(databases, DATABASES_ALL) == 0){

      strcpy(database_list[0], DATABASES_ALL);
      *count = 1;
      return(A_OK);
   }

   for(tmp_ptr = databases, *count = 0; *tmp_ptr != '\0';){

       if(sscanf(tmp_ptr,"%[^: ]*[: ]", d_name) == 0){
          tmp_ptr++;

	  continue;
       }
       else{

	  strcpy(database_list[(*count)++], d_name);
	  tmp_ptr += strlen(d_name);
       }
   }

   qsort(database_list, *count, sizeof(database_name_t), strcmp);

   *(database_list[*count]) = '\0';
   return(A_OK);
}
   


int find_in_databases(database, database_list,count)
   char *database;	      /* database to be found */
   domain_t *database_list;       /* database list that database is to be found in */
   int count;		      /* number of elements in the database list */

{
   ptr_check(database, char, "find_in_domains", 0);
   ptr_check(database_list, domain_t, "find_in_domains", -1);

   if(count <= 0)
      return(-1);

   /* if it's all domains then just return yes */

   if(strcmp(database_list[0], DATABASES_ALL) == 0)
      return(1);

   return !! bsearch(database, (char *) database_list, count, sizeof(database_name_t), strrcmp);
}
