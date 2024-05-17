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
#include "error.h"
#include "lang_parsers.h"

/*
    'd' points to the first character of the date string.

    E.g. "15-JUL-1991"

    't' points to the first character of the time string.

    E.g. "13:51"

    Assume: time_t is the number of seconds since 00:00:00 Jan 1, 1970 GMT.  ANSI does not
	    guarantee this.
*/

int
v2db_time(d, t, cse)
  char *d ;
  char *t ;
  core_site_entry_t *cse ;
{
  extern int local_timezone;

  static char *months[13] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL",
                              "AUG", "SEP", "OCT", "NOV", "DEC", (char *)0 } ;
  char mstr[4] ;                /* error waiting to happen... */
  date_time_t dt = 0 ;
  int dm ;
  int hr ;
  int m = 0 ;
  int min ;
  int y ;
  struct tm bd_time ;           /* broken down time */
  time_t the_time ;
  time_t ltime ;

#if defined(AIX) || defined(SOLARIS)

  extern long timezone;

#else

  struct tm *our_time ;           /* broken down time */

#endif

  ptr_check(d, char, "v2db_time", 0) ;
  ptr_check(t, char, "v2db_time", 0) ;
  ptr_check(cse, core_site_entry_t, "v2db_time", 0) ;

  if(sscanf(d, "%d-%[^-]-%d", &dm, mstr, &y) != 3)
  {

    /* "Error in date '%s'" */

    error(A_ERR, "v2db_date", V2DB_DATE_001, d) ;
    return 0 ;
  }
  if(sscanf(t, "%2d:%2d", &hr, &min) != 2)
  {

    /* "Error in time '%s'" */

    error(A_ERR, "v2db_date", V2DB_DATE_002, t) ;
    return 0 ;
  }
  if(dm > 31 || y < 1970)
  {


    /* "Day-of-month or year value error in '%s'" */

    error(A_ERR, "v2db_date", V2DB_DATE_003, d) ;
    return 0 ;
  }
  if(hr > 23 || min > 59)
  {


    /* "Hour or minute value error in '%s'" */

    error(A_ERR, "v2db_date", V2DB_DATE_004, t) ;
    return 0 ;
  }
  else
  {
    time_t timenow;


    while(months[m] != (char *)0)
    {
      if(strncmp(mstr, months[m], 3) == 0)
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

      /* "Error from timegm()" */

      error(A_ERR, "v2db_time", V2DB_DATE_005 ) ;
      return 0 ;
    }

#else

    ltime = timezone + local_timezone;

    if((the_time = mktime(&bd_time)) == -1)
    {

      /* "Error from timegm()" */

      error(A_ERR, "v2db_time", V2DB_DATE_005 ) ;
      return 0 ;
    }

    
#endif


    cse->date = the_time + ltime;
  }
  return 1 ;
}

int
v2db_links(v, cse)
  char *v ;
  core_site_entry_t *cse ;
{
  ptr_check(v, char, "v2db_links", 0) ;
  return 1 ;
}

int
v2db_name(v, cse)
  char *v ;
  core_site_entry_t *cse ;
{
  ptr_check(v, char, "v2db_name", 0) ;
  return 1 ;
}

static int
v2db_one_set_perm(in, perm)
  char *in ;
  perms_t *perm ;
{
  char *iptr = in ;
  perms_t p = 0 ;

  ptr_check(in, char, "v2db_one_set_perm", 0) ;
  ptr_check(perm, perms_t, "v2db_one_set_perm", 0) ;

  while(*iptr != ',' && *iptr != ')')
  {
    switch(*iptr++)
    {
    case 'R':	p |= 0x08 ; break ;
    case 'W':	p |= 0x04 ; break ;
    case 'E':	p |= 0x02 ; break ;
    case 'D':   p |= 0x01 ; break ;

    default:

      /* "Unexpected character ('%c') in permissions '%s'" */

      error(A_ERR, "v2db_one_set_perm", V2DB_ONE_SET_PERM_001, *(iptr - 1), in) ;
      return 0 ;
    }
  }

  *perm = p ;
  return 1 ;
}

int
v2db_owner(v, cse)
  char *v ;
  core_site_entry_t *cse ;
{
  ptr_check(v, char, "v2db_owner", 0) ;
  ptr_check(cse, core_site_entry_t, "v2db_owner", 0) ;
  return 1 ;
}

int
v2db_perm(v, cse)
  char *v ;
  core_site_entry_t *cse ;
{
  perms_t p = 0 ;
  perms_t perms = 0 ;

  ptr_check(v, char, "v2db_perm", 0) ;
  ptr_check(cse, core_site_entry_t, "v2db_perm", 0) ;

  /* Probably should do some error checking in here. */

  ++v ;
  v2db_one_set_perm(v, &p) ;
  perms = p << 12 ;

  while(*v++ != ',') ;

  v2db_one_set_perm(v, &p) ;
  perms |= p << 8 ;

  while(*v++ != ',') ;

  v2db_one_set_perm(v, &p) ;
  perms |= p << 4 ;

  while(*v++ != ',') ;

  v2db_one_set_perm(v, &p) ;
  perms |= p ;

  cse->perms |= perms ;
  return 1 ;
}

int
v2db_size(v, cse)
  char *v ;
  core_site_entry_t *cse ;
{
  ptr_check(v, char, "v2db_size", 0) ;
  ptr_check(cse, core_site_entry_t, "v2db_size", 0) ;

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


#if 0 /* Directory or non-directory is determined higher up */
int
v2db_type(v, cse)
  char *v ;
  core_site_entry_t *cse ;
{
  char *p ;

  ptr_check(v, char, "v2db_type", 0) ;
  ptr_check(cse, core_site_entry_t, "v2db_type", 0) ;

  if((p = strrchr(v, ';')) == (char *)0)
  {
    return 0 ;
  }
  else
  {
    if(*--p == 'R' && *--p == 'I' && *--p == 'D' && *--p == '.')
    {
      cse->perms |= 01000 ;
    }
    else
    {
      cse->perms &= ~01000 ;
    }
    return 1 ;
  }
}
#endif
