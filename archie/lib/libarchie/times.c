#include <time.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>

#include "typedef.h"
#include "times.h"
#include "lang_libarchie.h"
#include "error.h"
#include "protos.h"

/*
 * cvt_to_usertime: convert the internal format (seconds from 1 Jan 1970) to
 * "<hours>:<minutes>:<seconds> <day> <month> <year> <timezone>" format.
 * Result stored in static area
 */


char *cvt_to_usertime(internal_time, tiz)
   date_time_t internal_time;	    /* time in internal format */
   int tiz;			    /* If nonzero, give result in local time
				       not UTC */

{
   static pathname_t result_string; /* result */
   struct tm *timest;		    /* value returned from system conversion */

#if defined(AIX) || defined(SOLARIS)
   extern char *tzname[];
#endif

   if(tiz)
      timest = localtime(&internal_time);
   else
      timest = gmtime(&internal_time);


#if !defined(AIX) && !defined(SOLARIS)
   if(!strftime(result_string,sizeof(result_string),"%H:%M:%S %d %h %Y %Z",timest)){
#else
   if(!strftime(result_string,sizeof(result_string),"%H:%M:%S %d %h %Y ",timest)){
#endif
      /* "Can't convert given time %u to string" */

      error(A_ERR,"cvt_to_usertime",CVT_TO_USERTIME_001,internal_time);
      return((char *) NULL);
   }

#if defined(AIX) || defined(SOLARIS)
   strcat(result_string, tiz ? tzname[1] : "GMT");
#endif

   return(result_string);
}

#if defined(AIX) || defined(SOLARIS)

int timezone_sign()

{
   int diff;
   time_t mytime = time((time_t *) NULL);
   int sign = 1;
   struct tm *gmt_tm;
   struct tm *local_tm;

   local_tm = localtime(&mytime);
   gmt_tm = gmtime(&mytime);

   if((diff = local_tm -> tm_hour - gmt_tm -> tm_hour) < 0)
      sign = -1;
   else{
      if((diff == 0) && (local_tm -> tm_min - gmt_tm -> tm_min < 0))
	sign = -1;
   }

   return(sign);
}

#endif

   
/*
 * cvt_to_inttime: convert a string of the form YYYYMMDDHHMMSS to internal
 * format (seconds since 1 Jan 1970).
 */
   
time_t cvt_to_inttime(datestring, islocal)
   char *datestring;	   /* String to be converted */
   int islocal;		   /* non-zero if the time being given is in the
			      local time, not UTC */
{

#if defined(AIX) || defined(SOLARIS)
   extern long int timezone;
#endif

   struct tm input_time;   /* structure to pass to conversion routines */
   long int the_tz;
#ifndef AIX
   struct tm *local_tm;
#endif

   memset(&input_time, 0, sizeof(struct tm));

   if(!islocal){
#if defined(AIX) || defined(SOLARIS)
      tzset();
      the_tz = timezone * timezone_sign();
#else
      time_t mytime = time((time_t *) NULL);
      local_tm = localtime(&mytime);
      the_tz = local_tm -> tm_gmtoff;
#endif
   }
   else
      the_tz = 0;


   if(sscanf(datestring,"%4u%2u%2u%2u%2u%2u",&input_time.tm_year,
					       &input_time.tm_mon,
					       &input_time.tm_mday,
					       &input_time.tm_hour,
					       &input_time.tm_min,
					       &input_time.tm_sec) != 6){

      /* "Given string %s not in YYYYMMDDHHMMSS format" */

      error(A_INTERR,"cvt_to_inttime", CVT_TO_INTTIME_001, datestring);
      return((time_t) NULL);
   }
					       

   /* timegm() expects the year field to be year - 1900 */

   if(input_time.tm_year != 0)
      input_time.tm_year -= 1900;
#if defined(AIX) || defined(SOLARIS)
   else
      input_time.tm_year = 70;	    /* 1970, the epoch */
#endif      

   /* Months are counted from 0 */

   if(input_time.tm_mon != 0)
      input_time.tm_mon--;

   if(input_time.tm_mday == 0)
      input_time.tm_mday = 1;

#if !defined(AIX) && !defined(SOLARIS)
   if(islocal)
      return(timelocal(&input_time));
   else
      return(timelocal(&input_time) + the_tz);
#else
   if(islocal)
      return(mktime(&input_time));
   else
      return(mktime(&input_time) + the_tz);
#endif
}


/*
 * cvt_from_inttime: Convert from internal format to YYYYMMDDHHMMSS format
 * (UTC). Time returned is in UTC.
 */


char *cvt_from_inttime(internal_time)
   date_time_t internal_time;	 /* Internal time to be converted */

{
   static char result_string[EXTERN_DATE_LEN +1];
   struct tm *timest;

   timest = gmtime(&internal_time);

   if(!strftime(result_string,sizeof(result_string),"%Y%m%d%H%M%S",timest)){

      /* "Can't convert from %u to YYYYMMDDHHMMSS format" */

      error(A_INTERR,"cvt_from_inttime",CVT_FROM_INTTIME_001, internal_time);
      return((char *) NULL);
   }

   return(result_string);
}




