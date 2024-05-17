#ifdef __STDC__
#  include <stdlib.h>
#endif
#include <string.h>
#include "lang.h"
#include "misc_ansi_defs.h"
#include "strmap.h"
#include "vars.h"

#ifndef SOLARIS
extern int strncasecmp PROTO((const char *s1, const char *s2, int n));
#endif
extern int strcasecmp PROTO((const char *s1, const char *s2));


/*  
 *
 *
 *                             Internal routines
 *
 *
 */  

static const StrMap *find_in_from PROTO((const char *s, const StrMap *map,
                                         int (*compare) PROTO((const char *s1, const char *s2,
                                                               int n)),
                                         int slen));
static const StrMap *find_in_to PROTO((const char *s, const StrMap *map,
                                       int (*compare) PROTO((const char *s1, const char *s2,
                                                             int n)),
                                       int slen));
static int strcmp_ PROTO((const char *s1, const char *s2, int n));


static int strcasecmp_(s1, s2, n)
  const char *s1;
  const char *s2;
  int n;
{
  return strcasecmp(s1, s2);
}


static int strcmp_(s1, s2, n)
  const char *s1;
  const char *s2;
  int n;
{
  return strcmp(s1, s2);
}


static const StrMap *find_in_from(s, map, compare, slen)
  const char *s;
  const StrMap *map;
  int (*compare) PROTO((const char *s1, const char *s2, int n));
  int slen;
{
  while (map->from)
  {
    if (compare(s, map->from, slen) == 0)
    {
      return map;
    }
    map++;
  }
  return (StrMap *)0;
}


static const StrMap *find_in_to(s, map, compare, slen)
  const char *s;
  const StrMap *map;
  int (*compare) PROTO((const char *s1, const char *s2, int n));
  int slen;
{
  while (map->from)
  {
    if (map->to && compare(s, map->to, slen) == 0)
    {
      return map;
    }
    map++;
  }
  return (StrMap *)0;
}


/*  
 *
 *
 *                             External routines
 *
 *
 */  


/*  
 *  If the current language is english return the `from' (english) string,
 *  otherwise return the `to' (second language string), falling back to the
 *  english value if necessary.
 */    

const char *mapLangStr(map)
  const StrMap *map;
{
  if (strcmp(get_var(V_LANGUAGE), curr_lang[307]) == 0) return map->from;
  else return map->to ? map->to : map->from;
}


const char *mapNCaseStr(s, map)
  const char *s;
  const StrMap *map;
{
  const StrMap *m;

  if ((m = find_in_to(s, map, strncasecmp, (int)strlen(s)))) return m->from;
  else if ((m = find_in_from(s, map, strncasecmp, (int)strlen(s)))) return m->from;
  else return (const char *)0;
}


/* #ifdef NEW -- commented out Thu Oct 14 18:09:40 EDT 1993 */
const char *mapStrFrom(s, map)
  const char *s;
  const StrMap *map;
{
  const StrMap *m;

  if ((m = find_in_from(s, map, strcmp_, 0))) return m->from;
  else return (const char *)0;
}


const char *mapStrTo(s, map)
  const char *s;
  const StrMap *map;
{
  const StrMap *m;

  if ((m = find_in_to(s, map, strcmp_, 0))) return m->to;
  else return (const char *)0;
}


const char *mapNCaseStrFrom(s, map)
  const char *s;
  const StrMap *map;
{
  const StrMap *m;

  if ((m = find_in_from(s, map, strncasecmp, (int)strlen(s)))) return m->from;
  else return (const char *)0;
}


const char *mapNCaseStrTo(s, map)
  const char *s;
  const StrMap *map;
{
  const StrMap *m;

  if ((m = find_in_to(s, map, strncasecmp, (int)strlen(s)))) return m->from;
  else return (const char *)0;
}
/* #endif */


const char *mapCaseStr(s, map)
  const char *s;
  const StrMap *map;
{
  const StrMap *m;

  if ((m = find_in_to(s, map, strcasecmp_, 0))) return m->from;
  else if ((m = find_in_from(s, map, strcasecmp_, 0))) return m->from;
  else return (const char *)0;
}


const char *mapStr(s, map)
  const char *s;
  const StrMap *map;
{
  const StrMap *m;

  if ((m = find_in_to(s, map, strcmp_, 0))) return m->from;
  else if ((m = find_in_from(s, map, strcmp_, 0))) return m->from;
  else return (const char *)0;
}


int mapEmpty(m)
  const StrMap *m;
{
  int r = 0;

  r = m->from == (const char *)0;
  r = r & m->to == (const char *)0;
  return r;
}


const char *mapFirstStr(m)
  const StrMap *m;
{
  if (m->to) return m->to;
  else if (m->from) return m->from;
  else return (const char *)0;
}


void freeStrMap(m)
  StrMap *m;
{
  free(m->from);
  free(m->to);
  m->from = m->to = (const char *)0;
}


StrMap *newStrMap(s, m) /* bug: should be more general; varargs? */
  const char *s;
  StrMap *m;
{
  if ( ! (m->from = strdup(s)))
  {
    return (StrMap *)0;
  }
  else if ( ! (m->to = strdup(s)))
  {
    free(m->from);
    return (StrMap *)0;
  }
  else
  {
    return m;
  }
}
