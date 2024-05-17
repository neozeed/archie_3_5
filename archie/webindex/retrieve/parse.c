#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <malloc.h>

#include "ansi_compat.h"
#include "url.h"
#include "protos.h"
#include "error.h"

static char **table;
static int  table_num=0;
static int  table_max=0;



#define BUFFER_SIZE (8*1024)
typedef char buffer_t[BUFFER_SIZE];


static buffer_t fbuf;
static char *bptr = NULL;

struct tag_element {
   char *name;
   int end;
   int tag_num;
};

#define ANCHOR 1
#define BASE   2
#define AREA   3
#define FRAME  4

#define NO_END 0
#define END    1
#define POSSIBLE_END 2

struct tag_element tag_table[] = {
   {"A",    0, ANCHOR}, /* put 0 instead of 1 */
   {"BASE", 0, BASE},
   {"AREA", 0, AREA},
   {"FRAME", 0, FRAME},
   {NULL,   0, 0}
};


extern char *http_method;

static char **addTable(t, t_num, t_max, str)
char  **t;
int *t_num,*t_max;
char *str;
{

  char **tmp;

  if ( *t_num >= *t_max-1 ) {

    *t_max += 10;
    tmp = (char **)malloc(sizeof(char*)**t_max);
    if ( tmp == NULL ) {
      error(A_ERR,"addTable", "Unable to malloc");
      return t;
    }
    memcpy(tmp,t,sizeof(char*)**t_num);
    if ( *t_num ) {
      free(t);
    }
    t = tmp;
  }

  t[(*t_num)] = str;

  (*t_num)++;
  return t;
}

static int interesting_tag(tag)
buffer_t tag;
{
   char *tmp;
   int i;

   if ( tag[0] != '<' ) 
      return 0;

   tag++;
   if ( *tag == '/' ) 
      tag++;

   tmp = tag;
   while ( *tmp != '\0' && (isalpha(*tmp)) ) 
      tmp++;

   if ( *tmp == '\0' ) 
      return 0;

   if ( tmp-tag == 0 )
     return 0;
   
   for ( i=0; tag_table[i].name != NULL ; i++ ){
      if ( strncasecmp(tag,tag_table[i].name,tmp-tag) == 0 ){
	 return tag_table[i].tag_num;
      }
   }
   return 0;
}


static int end_tag(tag_num)
int tag_num;
{

  if ( tag_table[tag_num-1].end ) {


  }
  
  return tag_table[tag_num-1].end ;

}

static int find_a_tag(fp,buffer,copy) 
FILE *fp;
buffer_t buffer;
int copy;
{
   char *tmp;
   int ret;
   int flag;
   int tag_num = 0;

   if ( bptr == NULL ) {
      if ( (ret=fread(fbuf,sizeof(char),BUFFER_SIZE-1,fp)) == 0 ) 
        return 0;
      bptr = fbuf;
      fbuf[ret] = '\0';
   }

   flag = 1;
   if (copy) tmp = buffer;
   while ( flag ) {
      while ( *bptr != '\0'  ) {
        if ( *bptr == '<'  ) { /* Found a tag */
          copy = 1;
          tmp = buffer;
        }
        if ( copy ) {
          *tmp = *bptr;
          tmp++;
        }
        if ( copy && tmp-buffer > 8*1024 ) {
          copy = 0;
        }
        if ( *bptr == '>'  && copy ) {
          copy = 0;
          *tmp = '\0';
          if ( (tag_num = interesting_tag(buffer)) ) {
            flag = 0;
            bptr++;
            break;
          }
          flag = 1;
        }
        bptr++;
      }
      if ( flag ) {
        if ( (ret=fread(fbuf,sizeof(char),BUFFER_SIZE-1,fp)) == 0 ) 
          return 0;
        bptr = fbuf;
        fbuf[ret] = '\0';
      }
   }
   return tag_num;
}


static int find_tag(fp,tag)
FILE *fp;
buffer_t tag;
{
  int ret;
  int which_tag;
  buffer_t tag_buffer;

  /* Find start tag */
  which_tag = find_a_tag(fp,tag_buffer,0);

  if ( which_tag ) {
    strcpy(tag,tag_buffer);
    if ( end_tag(which_tag) ) { /* Find end tag */
      ret = find_a_tag(fp,tag_buffer,1);
      if ( ret  ) {
        strcat(tag,tag_buffer);
      }
      else 
	    return 0;
    }
  }
  return which_tag;
}


static int extract_href(tag_buff,href_buff )
char *tag_buff;
char *href_buff;
{

  char *tmp;
  tmp = tag_buff;

  while ( *tmp != '\0' ) {
    if ( *tmp == 'h' || *tmp == 'H' ) {
      if ( strncasecmp(tmp,"href",4) == 0 ){
        tmp+=4;
        break;
      }
    }
    tmp++;
  }
  
  while ( *tmp != '\0' ) {
    if ( *tmp == '=' ) {
      tmp++;
      break;
    }
    tmp++;
  }

  if ( *tmp == '\0' ) 
  return 1;
   
  while ( *tmp != '\0' ) {
    if ( isspace(*tmp) ){
      tmp++;
      continue;
    }
     
    if ( *tmp == '"' ) {
      if ( *(tmp+1) == '\0' || *(tmp+1) == '"' )
      return 1;
      else {
        tmp++;
        break;
      }
    }
    else
    break;
    tmp++;
  }
     
  if ( *tmp == '\0' ) 
  return 1;

  while ( *tmp != '\0' && *tmp != '"' && *tmp != ' ' && *tmp != '>' ) {
    if ( isspace(*tmp) ) {
      tmp++;
      continue;
    }
    *href_buff = *tmp;
    tmp++;
    href_buff++;
  }

/*  if ( *tmp == '\0' ) 
  return 1;
*/
  
  *href_buff = '\0';

  return 0;
}


static int extract_src(tag_buff,href_buff )
char *tag_buff;
char *href_buff;
{

  char *tmp;
  tmp = tag_buff;

  while ( *tmp != '\0' ) {
    if ( *tmp == 's' || *tmp == 'S' ) {
      if ( strncasecmp(tmp,"src",3) == 0 ){
        tmp+=3;
        break;
      }
    }
    tmp++;
  }
  
  while ( *tmp != '\0' ) {
    if ( *tmp == '=' ) {
      tmp++;
      break;
    }
    tmp++;
  }

  if ( *tmp == '\0' ) 
  return 1;
   
  while ( *tmp != '\0' ) {
    if ( isspace(*tmp) ){
      tmp++;
      continue;
    }
     
    if ( *tmp == '"' ) {
      if ( *(tmp+1) == '\0' || *(tmp+1) == '"' )
      return 1;
      else {
        tmp++;
        break;
      }
    }
    else
    break;
    tmp++;
  }
     
  if ( *tmp == '\0' ) 
  return 1;

  while ( *tmp != '\0' && *tmp != '"' && *tmp != ' ' && *tmp != '>' ) {
    if ( isspace(*tmp) ) {
      tmp++;
      continue;
    }
    *href_buff = *tmp;
    tmp++;
    href_buff++;
  }

/*  if ( *tmp == '\0' ) 
  return 1;
*/
  
  *href_buff = '\0';

  return 0;
}


int urlscan(url,method,server,port,path)
char *url,*method,*server;
int *port;
char *path;
{

   char *t,*s;
   int ret = 0;

   s = url;

   *method = *server = *path = '\0';
   *port = -1;

   /* Get method */
   t = strchr(s,':');
   if ( t == NULL ) {
      if ( *s != '\0' ) {
	 strcpy(path,s);
	 ret++;
      }
      return ret;
   }
   else {
      if ( strncmp(t,"://",3) == 0 ) {
	 strncpy(method,s,t-s);
	 method[t-s] = '\0';
	 ret++;
      }
      else 
	 return ret;
   }


   /* Get server */
   t += 3;   s = t;
 
   t = strchr(s,':'); 
   if ( t == NULL ) { /* No port specified */
      t = strchr(s,'/');
      if ( t == NULL ) { /* No trailing / */
	 strcpy(server,s);
      }
      else {
	 strncpy(server,s,t-s);
	 server[t-s] = '\0';
	 t++;
	 strcpy(path,t);
	 ret++;
      }
      return ++ret;
   }
   else {
      strncpy(server,s,t-s);
      server[t-s] = '\0';
      ret++;
   }
   t++;
   s = t;

   *port = atoi(s);
   ret++;

   t = strchr(s,'/');
   
   if ( t != NULL )  {
      t++;
      strcpy(path,t);
      ret++;
   }

   return ret;
}



static int clean_url(url)
buffer_t url;
{

  buffer_t method;
  buffer_t server;
  buffer_t path;
  int port = -1;
  int ret;
  char *t,*s;


  path[0] = server[0] = method[0] = '\0';

  ret = urlscan(url,method,server,&port,path);

  if ( strcasecmp(method,"http") )
  return 0;


  if ( path[0] == '\0')  {
    path[0] = '/';
    path[1] = '\0';
  }

  s = t = path;

  if ( strncmp(path,"..",2) == 0 ) {
    strcpy(path,&path[2]);  
  }
  
  if ( strncmp(path,".",1) == 0 ) {
    strcpy(path,&path[1]);  
  }
  
  while ( *s != '\0' ) {

    if ( *s == ' ' ) {          /* Jump spaces */
      s++;
      continue;
    }

    if ( *s == '.' ) {
      if ( strncmp(s-1,"/../",4) == 0 ) { /* Case blah/blah/../blah */

        if ( t-path > 2 ) 
          t-=2;
        while ( *t != '/' && t != path ) 
          t--;
        if ( *t == '/' ) 
        t++;
        s+=3;
        continue;
      }
      else 
	    if ( strncmp(s-1,"/./",3) == 0 ) { /* case blah/blah/./blah */
        s+= 2;                  /* Jump over ./ */
        continue;
	    }


    }

    *t = *s;
    s++;
    t++;
  }
  *t = '\0';
  if ( port == -1 || port == 80 ) 
  sprintf(url,"%s://%s",method,server);
  else
  sprintf(url,"%s://%s:%d",method,server,port);
  if ( path[0] == '/' )
  strcat(url,path);
  else {
    strcat(url,"/");
    strcat(url,path);
  }
  return 1;
}



static int compose_url(base,href,url)
URL *base;
buffer_t href, url;
{

   char *t;
   URL *u,*v = base;
   buffer_t tmp;

   u = urlParse(href);

   if ( u == NULL ) 
      return 0;


   if ( u->server != NULL ) { /* absolute url */
      strcpy(url,href);
      urlFree(u);
      return 1;
   }

/*   v = urlParse(base); */


   /* At this point we are considering a relative URL */
   if ( u->local_url[0] == '/' )  { /* Absolute path */
      if ( strcasecmp(v->method,http_method) == 0 ) 
        if (v->port == NULL ||  strcmp(v->port,"-1") == 0 || strcmp(v->port,"80") == 0 )
          sprintf(url,"http://%s%s",v->server,u->local_url);
        else 
          sprintf(url,"http://%s:%s%s",v->server,v->port,u->local_url);      
      urlFree(u);
/*      urlFree(v);* */
      return 1;
    }


   /* Relative path */

   if (  u->local_url[0]  == '#' ) {
     strcpy(tmp,v->local_url);
     if ( tmp[strlen(tmp)-1] == '/' ) {
       tmp[strlen(tmp)-1] = '\0';
     }
   }
   else { 
     t = strrchr(v->local_url,'/');
     t++;
     strncpy(tmp,v->local_url,t-v->local_url);
     tmp[t-v->local_url] = '\0';
   }

   if ( strcasecmp(v->method,http_method) == 0) {
      if (v->port == NULL ||  strcmp(v->port,"-1") == 0 ||  strcmp(v->port,"80") == 0   )	
        sprintf(url,"http://%s%s%s",v->server,tmp,u->local_url);
      else 
	      sprintf(url,"http://%s:%s%s%s",v->server,v->port,tmp,u->local_url);      	 
   }

   urlFree(u);
/*   urlFree(v); */
   return 1;
}

char **find_urls( fp, base ) 
FILE *fp;
URL *base;
{

  buffer_t tag_buff;
  buffer_t href_buff;
  buffer_t url_buff;

  int tag_num;

  table_num = 0;
  /*   if ( base != NULL ) 
       strcpy(base_url,base);
       else
       base_url[0] = '\0';
       */
  bptr = NULL;
  while ( (tag_num = find_tag(fp,tag_buff)) ) {

    switch ( tag_num ) {

    case AREA:
    case ANCHOR: 
	    
	    if ( extract_href(tag_buff,href_buff) ) { /* Error */
	       
	    }
	    else {
        char *us;
        if ( compose_url(base,href_buff,url_buff) ) {
          clean_url(url_buff);
          /* Add to table Now */
          /*		  printf("Found url : %s\n",url_buff);  */

          us = (char*)malloc(strlen(url_buff)+1);
          if ( us == NULL )  {
            exit(1);
          }
          strcpy(us,url_buff);
          table = addTable(table,&table_num,&table_max,us);
        }
	    }
	    break;

    case FRAME: 
	    
	    if ( extract_src(tag_buff,href_buff) ) { /* Error */
	       
	    }
	    else {
        char *us;
        if ( compose_url(base,href_buff,url_buff) ) {
          clean_url(url_buff);
          /* Add to table Now */
          /*		  printf("Found url : %s\n",url_buff);  */

          us = (char*)malloc(strlen(url_buff)+1);
          if ( us == NULL )  {
            exit(1);
          }
          strcpy(us,url_buff);
          table = addTable(table,&table_num,&table_max,us);
        }
	    }
	    break;
      

    case BASE:
	    if ( extract_href(tag_buff,href_buff) ) { /* Error */
	       
	    }
	    else {
        URL *tmp;
        tmp = urlParse(href_buff);
        /*	       strcpy(base_url,href_buff);
                   clean_url(base_url);
                   */
        /*	       printf("Found base url%s\n",base_url); */
        if ( tmp != NULL ) {
          base = tmp;
        }
	    }
	    break;

    default:                    /* Error */
	    break;
    }
  }

  if ( table != NULL ) 
  table[table_num] = NULL;
  return  table;
}


#ifdef PARSE_MAIN
main(argc,argv) 
int argc;
char **argv;
{


   FILE *fp;

   if ( argc != 3 ) {
      fprintf(stderr,"usage: a.out file base_url\n");
      exit(1);
   }
   fp = fopen(argv[1],"r");

   find_urls(fp,argv[2]);
   fclose(fp);

}

#endif
