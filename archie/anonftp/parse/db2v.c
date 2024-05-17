#include <time.h>
#include <stdio.h>
#include "defines.h"
#include "parser_file.h"
#include "error.h"

/*
  These routines take as arguments a pointer to a parser_entry_t and a
  pointer to a string on which to write the conversions.  All routines write
  a nul terminator and return a pointer to the terminator.
*/


/*
  'v' points to the first character of the date string.

  E.g. "15-JUL-1991"
*/

char *
db2v_date(pe, s, slen)
  parser_entry_t *pe ;
  char *s ;
  int slen ;
{
  static char *months[13] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL",
                              "AUG", "SEP", "OCT", "NOV", "DEC", (char *)0 } ;
  char *p = s ;
  int dm ;
  int m ;
  int y ;
  struct tm *bd_time ;

  ptr_check(pe, parser_entry_t, "db2v_date", (char *)0) ;
  ptr_check(s, char, "db2v_date", (char *)0) ;

  if((bd_time = gmtime(&pe->core.date)) == (struct tm *)0)
  {
    error(A_ERR, "db2v_date", "error from gmtime() with [%ld]",
            (long)pe->core.date) ;
    return (char *)0 ;
  }
#ifdef AIX
  if(strftime(s, slen, "%d-%h-%Y %H:%M", bd_time) == 0)
#else
  if(strftime(s, slen, "%d-%h-%Y %R", bd_time) == 0)
#endif
  {
    error(A_ERR, "db2v_date", "error from strftime()") ;
    return (char *)0 ;
  }
  return s ;
}


char *
db2v_links(pe, s)
  parser_entry_t *pe ;
  char *s ;
{
  ptr_check(pe, parser_entry_t, "db2v_links", (char *)0) ;
  ptr_check(s, char, "db2v_links", (char *)0) ;

  *s = '\0' ;
  return s ;
}


char *
db2v_name(pe, s)
  parser_entry_t *pe ;
  char *s ;
{
  ptr_check(pe, parser_entry_t, "db2v_name", (char *)0) ;
  ptr_check(s, char, "db2v_name", (char *)0) ;

  *s = '\0' ;
  return s ;
}


char *
db2v_perms(pe, s)
  parser_entry_t *pe ;
  char *s ;
{
  char *p_grp ;
  char *p_own ;
  char *p_sys ;
  char *p_wrl ;
  static char *pstr[16] =
  {	"",
	"D",
	"E", "ED",
	"W", "WD", "WE", "WED",
	"R", "RD", "RE", "RED", "RW", "RWD", "RWE", "RWED"
  } ;

  ptr_check(pe, parser_entry_t, "db2v_perms", (char *)0) ;
  ptr_check(s, char, "db2v_perms", (char *)0) ;

  p_sys = pstr[(pe->core.perms >> 12) & 0x0f] ;
  p_own = pstr[(pe->core.perms >> 8) & 0x0f] ;
  p_grp = pstr[(pe->core.perms >> 4) & 0x0f] ;
  p_wrl = pstr[(pe->core.perms >> 0) & 0x0f] ;

  sprintf(s, "(%s,%s,%s,%s)", p_sys, p_own, p_grp, p_wrl) ;
  return s ;
}


#define OWNER "[owner,owner]"

char *
db2v_owner(pe, s)
  parser_entry_t *pe ;
  char *s ;
{
  ptr_check(pe, parser_entry_t, "db2v_owner", (char *)0) ;
  ptr_check(s, char, "db2v_owner", (char *)0) ;

  strcpy(s, OWNER) ;
  return s ;
}


char *
db2v_size(pe, s)
  parser_entry_t *pe ;
  char *s ;
{
  ptr_check(pe, parser_entry_t, "db2v_size", (char *)0) ;
  ptr_check(s, char, "db2v_size", (char *)0) ;

  sprintf(s, "%ld", (long)pe->core.size) ;
  return s ;
}


char *
db2v_type(pe, s)
  parser_entry_t *pe ;
  char *s ;
{
  ptr_check(pe, parser_entry_t, "db2v_type", (char *)0) ;
  ptr_check(s, char, "db2v_type", (char *)0) ;

  *s = '\0' ;
  return s ;
}
