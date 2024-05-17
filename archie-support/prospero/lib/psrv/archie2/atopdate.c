#include <stdio.h>
#include <sys/types.h>

#include <string.h>

#include <defines.h>
#include <structs.h>
#include <database.h>

extern datestruct unpack_date();

char *atopdate(entry_db)
	db_date entry_db;
{

   static char result[20] ;
   datestruct entry;

   entry = unpack_date(entry_db);

   if(entry.hour == MAX_HOUR) {
       entry.hour = 0;
       entry.min = 0;
   }

   (void) sprintf(result,"%04d%02d%02d%02d%02d00Z",entry.year,entry.month+1,
		  entry.day,entry.hour,entry.min);

   return(result);
     
}
