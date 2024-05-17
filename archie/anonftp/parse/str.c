#include <strings.h>

extern char *malloc() ;


char *
strdup(s)
  char *s ;
{
  char *t = malloc((unsigned)(strlen(s)+1)) ;

  return t == (char *)0 ? t : strcpy(t, s) ;
}


static char *
findfirst(s, set)
  char *s ;
  char *set ;
{
  char *p ;

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


char *
strsep(s, set)
  char **s ;
  char *set ;
{
  char *p ;
  char *start = *s ;

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
