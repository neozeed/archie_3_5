#include <stdio.h>
#include <string.h>
#include "defines.h"
#include "parse.h"
#include "error.h"


int
vfprintf_path(fp, p, n)
  FILE *fp ;
  char **p ;
  int n ;
{
  ptr_check(fp, FILE, "vfprintf_path", 0) ;
  ptr_check(p, char *, "vfprintf_path", 0) ;

  if(n < 1)
  {
    return 0 ;
  }
  else
  {
    int i ;

    for(i = 0 ; i < n - 1 ; i++)
    {
      fprintf(stderr, "%s/", p[i]) ;
    }
    fprintf(stderr, "%s", p[i]) ;
    return 1 ;
  }
}

char *
nuke_nl(s)
  char *s ;
{
  char *p ;

  ptr_check(s, char, "nuke_nl", (char *)0) ;

  p = strchr(s, '\n') ;
  if(p != (char *)0)
  {
    *p = '\0' ;
  }
  return s ;
}

#if 0
char *
strdup(s)
   char *s ;
{
  char *t ;

  ptr_check(s,  char, "strdup", (char *)0) ;

  t = (char *)malloc((size_t)(strlen(s)+1)) ;
  return t == (char *)0 ? t : strcpy(t, s) ;
}
#endif

/*
    Return a pointer to the nul in a nul terminated string.
*/

char *
strend(s)
   char *s ;
{
  ptr_check(s, char, "strend", (char *)0) ;

  for( ; *s != '\0' ; s++)
    ;
  return s ;
}

static char *
findfirst(s, set)
  char *s ;
   char *set ;
{
   char *p ;

  ptr_check(s, char, "findfirst", (char *)0) ;
  ptr_check(set,  char, "findfirst", (char *)0) ;

  for( ; *s != '\0' ; s++)
  {
    for(p = set ; *p != '\0' ; p++)
    {
      if(*p == *s)
      {
        return s ;
      }
    }
  }
  return (char *)0 ;
}

/*
    Return a pointer to the first occurence in '*s' of a character in 'set'.  If none
    exists return the null pointer.

    If a character is found, set it to '\0' and set *s to point to the first character
    after the '\0'.
*/

char *
strsep(s, set)
  char **s ;
   char *set ;
{
  char *p ;
  char *start = *s ;

  ptr_check(s, char *, "strsep", (char *)0) ;
  ptr_check(*s, char, "strsep", (char *)0) ;
  ptr_check(set,  char, "strsep", (char *)0) ;

  if((p = findfirst(*s, set)) == (char *)0)
  {
    return (char *)0 ;
  }
  else
  {
    *p = '\0' ;
    *s = ++p ;
    return start ;
  }
}

#if 0
char *
tail(path)
   char *path ;
{
  char *p ;

  ptr_check(path,  char, "tail", (char *)0) ;

  p = strrchr(path, '/') ;
  return p == (char *)0 ? path : ++p ;
}

#endif
