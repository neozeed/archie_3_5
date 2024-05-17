#include <stdio.h>

#include <ansi_compat.h>
#include "str.h"

char *strCopy(s)
char *s;
{
   char *t = (char*)malloc(sizeof(char)*(strlen(s)+1));
   if ( t != NULL ) 
      strcpy(t,s);
   return t;
}

char *strnCopy(s,n)
char *s;
int n;
{
   char *t = (char*)malloc(sizeof(char)*(n+1));
   if ( t != NULL ) {
      strncpy(t,s,n);
      t[n] = '\0';
   }
   return t;
}
