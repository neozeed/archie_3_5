#include <stdio.h>
#include <string.h>
#include <time.h>
#if defined(AIX) || defined(SOLARIS)
#include <stdlib.h>
#include <ctype.h>
#endif
#include "defines.h"
#include "parse.h"
#include "site_file.h"
#include "unix2.h"
#include "error.h"
#include "lang_parsers.h"

/*
  'mon' points to the first character of the month string.

  E.g. "Mar"

  'day' points to the first character of the day string.

  E.g. "25"

  't_y' points to the first character of the "time or year" string.

  E.g. "13:51" or "1992"

  Assume: time_t is the number of seconds since 00:00:00 Jan 1, 1970 GMT.  ANSI does not
	  guarantee this.
*/

int
u2db_time(mon, day, t_y, cse)
   const char *mon ;
   const char *day ;
   const char *t_y ;
  core_site_entry_t *cse ;
{
  extern int local_timezone;

  static char *months[13] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
                              "Aug", "Sep", "Oct", "Nov", "Dec", (char *)0 } ;
  static struct tm *mytime;
  int dm ;
  int hr ;
  int m = 0 ;
  int min ;
  int y ;
  struct tm bd_time ;           /* broken down time */
  time_t ltime;
  time_t the_time ;

#if defined(AIX) || defined(SOLARIS)

  extern long timezone;

#else

  extern time_t time();
  struct tm *our_time ;           /* broken down time */

#endif

  ptr_check(mon, char, "u2db_time", 0) ;
  ptr_check(day, char, "u2db_time", 0) ;
  ptr_check(t_y, char, "u2db_time", 0) ;
  ptr_check(cse, core_site_entry_t, "u2db_time", 0) ;

  if(mytime == (struct tm *) NULL){
     the_time = time((time_t *) NULL);


     if((mytime = (struct tm *) malloc(sizeof(struct tm))) == (struct tm *) NULL){

       error(A_SYSERR, "u2db_time", "Can't malloc space for time structure");
       return 0;
     }

     memcpy(mytime, gmtime(&the_time), sizeof(struct tm));
  }

  while(months[m] != (char *)0)
  {
    if(strncmp(mon, months[m], 4) == 0)
    {
      break ;
    }
    else
    {
      m++ ;
    }
  }
  if(m == 12)
  {
    return 0 ;
  }

  if(sscanf(day, "%d", &dm) != 1)
  {

  /* "Error in date '%s'" */

    error(A_ERR, "u2db_time", U2DB_TIME_001, day) ;
    return 0 ;
  }
  if(t_y[2] == ':')
  {
    if(sscanf(t_y, "%2d:%2d", &hr, &min) != 2)
    {

      /* "Error in time '%s'" */

      error(A_ERR, "u2db_time", U2DB_TIME_002, t_y) ;
      return 0 ;
    }


   if(mytime -> tm_mon - m < 6 ){
      if(mytime -> tm_mon - m < 0 )
         y = mytime -> tm_year + 1900 - 1;
      else
         y = mytime -> tm_year + 1900;
   }
   else
      y = mytime -> tm_year + 1900;

  }
  else
  {
    if(sscanf(t_y, "%d", &y) != 1)
    {

    /* "Error in year '%s'" */

      error(A_ERR, "u2db_time", U2DB_TIME_003, t_y) ;
      return 0 ;
    }
    hr = 0 ;
    min = 0 ;
  }
  if(dm < 1 || dm > 31 || y < 1900)
  {


/* "Day-of-month or year value error in '%s %s %s'" */

    error(A_ERR, "u2db_time", U2DB_TIME_004, mon, day, t_y) ;
    return 0 ;
  }

#if defined(AIX) || defined(SOLARIS)

  /* AIX can't handle dates less than 1970, so reset them to that */

  if(y < 1970)
     y = 1970;

#endif

  if(hr < 0 || hr > 23 || min < 0 || min > 59)
  {


    /* "Hour or minute value error in '%s'" */

    error(A_ERR, "u2db_time", U2DB_TIME_005, t_y) ;
    return 0 ;
  }
  else
  {
    time_t timenow;
    
    memset((void *)&bd_time, 0, sizeof bd_time) ;
    bd_time.tm_sec = 0 ;
    bd_time.tm_min = min ;
    bd_time.tm_hour = hr ;
    bd_time.tm_mday = dm ;
    bd_time.tm_mon = m ;
    bd_time.tm_year = y - 1900 ;
    bd_time.tm_isdst = -1 ;

#if !defined(AIX) && !defined(SOLARIS)

    timenow = time((time_t *) NULL);

    our_time = localtime(&timenow);

    ltime = our_time -> tm_gmtoff + local_timezone;


    if((the_time = timelocal(&bd_time)) == -1)
    {

      /* "Error from timelocal()" */

      error(A_ERR, "u2db_time", U2DB_TIME_006) ;

      /* reset the time to "now" */

      the_time = time((time_t) NULL);
    }

#else

    ltime = (timezone * timezone_sign()) + local_timezone;


    if((the_time = mktime(&bd_time)) == -1)
    {

      /* "Error from timegm()" */

      error(A_ERR, "u2db_time", U2DB_TIME_007) ;
      return 0 ;
    }

#endif


    cse->date = the_time + ltime;
  }
  return 1 ;
}


int
u2db_links(v, cse)
  const char *v ;
  core_site_entry_t *cse ;
{
  ptr_check(v, char, "u2db_links", 0) ;
  ptr_check(cse, core_site_entry_t, "u2db_links", 0) ;
  return 1 ;
}


int
u2db_name(v, cse)
  const char *v ;
  core_site_entry_t *cse ;
{
  ptr_check(v, char, "u2db_name", 0) ;
  ptr_check(cse, core_site_entry_t, "u2db_name", 0) ;
  return 1 ;
}


int
u2db_owner(u, v, cse)
  const char *u ;
  const char *v ;
  core_site_entry_t *cse ;
{
  ptr_check(u, char, "u2db_owner", 0) ;
  /* v (group) may be null */
  ptr_check(cse, core_site_entry_t, "u2db_owner", 0) ;
  return 1 ;
}


int
u2db_perm(v, cse)
  const char *v ;
  core_site_entry_t *cse ;
{
  int i, j;
  perms_t perms = 0 ;

  ptr_check(v, char, "u2db_perm", 0) ;
  ptr_check(cse, core_site_entry_t, "u2db_perm", 0) ;

  for(i=0, j = 9 ; i < 9 ;i++, j--)
  {
    switch(v[j])
    {
    case 'R':
      perms |= (0x01 << 2) |  (0x01 << 5) |  (0x01<< 8); /* like r--r--r-- */
      break;
    case 'W':
      perms |= (0x01 << 1) | (0x01 << 4) | (0x01<<7); /* like -w--w--w- */
      break;
    case 'C':
      perms |= (0x01 << 2) |  (0x01 << 5) |  (0x01<< 8); /* like r--r--r-- */
      perms |= (0x01 << 1) | (0x01 << 4) | (0x01<<7); /* like -w--w--w- */
      break ;

    case 'F':
      if ( CSE_IS_DIR((*cse)) )  {
	 perms |=( 0x01 << 0 | 0x01 << 3 | 0x01 << 6 );
      }
      break;
    case 'E':
    case 'M':
    case 'A':
    case '-':
    case '[':
    case ']':
      break;

    default:

      /* "Unknown permission character %c, line %s" */

      error(A_ERR, "u2db_perm", U2DB_PERM_001, v[j], v) ;
      return 0 ;
    }
  }

  cse->perms = perms ;
  return 1 ;
}


int
u2db_size(v, cse)
  const char *v ;
  core_site_entry_t *cse ;
{
  ptr_check(v, char, "u2db_size", 0) ;
  ptr_check(cse, core_site_entry_t, "u2db_size", 0) ;

  if( ! isdigit((int)*v))
  {
    return 0 ;
  }
  else
  {
    cse->size = atoi(v) ;
    return 1 ;
  }
}
