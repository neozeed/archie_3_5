#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "ansi_compat.h"
#include "typedef.h"
#include "str.h"
#include "url.h"

#define DEFAULT_HTTP_PORT "80"

char *http_method = "http";


static URL *urlAllocate()
{
   URL *u;
   
   u = (URL*)malloc(sizeof(URL));
   if ( u != NULL )  {
      u->method = NULL;
      u->server =  NULL;
      u->local_url = NULL;
      u->port = NULL;
      u->curl = NULL;
   }
   return u;
}

static int urlDeallocate(u)
URL *u;
{
   if ( u->server ) free(u->server);
   if ( u->local_url) free(u->local_url);
   if ( u->port ) free(u->port);
   if ( u->curl ) free(u->curl);
   free(u);
   return 1;

}

static void lower_str(s)
  char *s;
{

  while ( s && *s != '\0' ) {
    if ( *s >= 'A' && *s <= 'Z' )
      *s = *s - 'A' + 'a';
    s++;
  }
}

static int urlCanonical(u)
URL *u;
{
   struct hostent *h;
   char *server;

   if ( u == NULL || u->server == NULL )
      return 0;

   h = gethostbyname(u->server);
   if ( h == NULL ) {
      return 0;
   }

   server = u->server;
   u->server = h->h_name;
   lower_str(u->server);
   u->curl = urlStrBuild(u);
   u->server = server;
   lower_str(u->server);   
   return 1;
   
}


URL *urlBuild(method,server,port,local_url)
char *method, *server,*port;
char *local_url;
{

   URL *u;

   u = urlAllocate();

   if ( strcasecmp(method,http_method) == 0 ) {
      u->method = http_method;
   }
   
   u->server = strCopy(server);
   u->local_url = strCopy(local_url);
   u->port = strCopy(port);

   urlCanonical(u);
   
   return u;
}

char *urlStrBuild(u)
URL *u;
{
  int len;
  char *str;
  
  if ( u->port == NULL || strcmp(u->port,DEFAULT_HTTP_PORT) == 0  ) {
    len = strlen(u->method)+strlen(u->server)+strlen(u->local_url) + 3;
  }
  else {
    len = strlen(u->method)+strlen(u->server)+strlen(u->local_url) +
          strlen(u->port) + 4;
  }

  str = (char*)malloc((1+len)*sizeof(char));
  if ( str != NULL ){
    if ( u->port == NULL || strcmp(u->port,DEFAULT_HTTP_PORT) == 0  ) {
      sprintf(str,"%s://%s%s",u->method,u->server,u->local_url);    
    }
    else {
      sprintf(str,"%s://%s:%s%s",u->method,u->server,u->port,u->local_url);
    }
  }

  return str;

}


                
URL *urlParse(urlStr)
char *urlStr;
{

  URL *u;
  char *s,*t;

  u = urlAllocate();

  t = s = urlStr;
  while ( *s != '\0' &&  *s != ':' ) {
    s++;
  }


  if ( *s == '\0' ) {
    u->method = http_method;
    u->local_url = strCopy(t);
    urlCanonical(u);
    return u;
  } 
  else {
    if ( strncasecmp(t,http_method,s-t) || strncmp(s,"://",3) ) {
      urlDeallocate(u);
      return NULL;
    }
  }

  u->method = http_method;

  s += 3;
  t=s;

  while ( *s != '\0' &&  *s != ':'  && *s != '/' ) {
    s++;
  }

  u->server = strnCopy(t,s-t);

  t = s;
  if ( *s == ':' ) {            /* Port is specified */
    t = ++s;
    while ( *s != '\0' && *s != '/' && *s >= '0' && *s <='9' ) {
      s++;
    }

    if ( *s != '\0' && *s != '/' ) {
      urlDeallocate(u);
      return NULL;
    }      

    u->port = strnCopy(t,s-t);

  }
  else {
    u->port = strCopy(DEFAULT_HTTP_PORT);
  }

  if( *s == '\0' ) 
    u->local_url = strCopy("/");
  else
    u->local_url = strCopy(s);
  urlCanonical(u);
  return u;
}


int urlRestricted(url,rurls,path)
URL *url;
char **rurls;
char *path;
{

  if ( strncmp(url->local_url,"/cgi-bin/",9) == 0 )
  return 1;

  if ( strncmp(url->local_url,path,strlen(path)) )
    return 1;
  
  if ( rurls == NULL )
  return 0;

  while ( *rurls != NULL ) {
    if ( strncasecmp(url->local_url, *rurls, strlen(*rurls)) == 0 )
    return 1;
    rurls++;
  }


  return 0;
}



int urlFree(url)
URL *url;
{
   return urlDeallocate(url);
}


int urlLocal(server,port, u)
char *server;
char *port;
URL *u;
{
   struct hostent *h;
   hostname_t host;

   if ( u == NULL || server == NULL) 
      return 0;
   
   h = gethostbyname(u->server);
   if ( h == NULL ) {
      return 0;
   }

   strcpy(host,h->h_name);

/*   h = gethostbyname(server);

   if ( h == NULL ) {
      return 0;
   }

   return strcasecmp(host,h->h_name)==0 && strcmp(port,u->port)==0 ;
   */

   return strcasecmp(host,server) ==0 && strcmp(port,u->port) == 0 ;
}


int urlClean(u)
URL *u;
{
  char *t,*s;
  t = u->local_url;
  if ( t == '\0' )
    return 1;

  s = t+1;
  while ( *s != '\0' ) {
    if ( *t == '/' && *s == '/' ) {
      s++;
    }
    else {
      t++;
      *t = *s;
      s++;
    }
  }
  
  t = u->local_url;
  while ( *t != '\0' ) {
    if ( *t == '#' /*|| *t == '?' */) {
      *t = '\0';
      free(u->curl);
      return urlCanonical(u);
    }
    t++;
  }

  return 1;
}
