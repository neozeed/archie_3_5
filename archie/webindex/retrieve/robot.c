
#include <stdio.h>
#include <unistd.h>

#include <ansi_compat.h>
#include "typedef.h"
#include "str.h"
#include "url.h"
#include "http.h"
#include "web.h"

#define BUFF_SIZE 1024

#define ANY 1
#define SPECIFIED 2

#define ROBOT_URL "/robots.txt"



static char *skip_sep(s,l)
char *s,*l;
{
   char *c;

   while (*s != '\0' ) {
      for ( c=l; *c != '\0'; c++ ) {
	 if ( *s == *c ) {
	    s++;
	    break;
	 }
      }
      if ( *c == '\0' )
	 break;
   }
   return s;
}

/*
 * Given a list of (field seps)characters ``l'' separate a string ``s'' 
 * into distinct fields.
 *
 * The last element of the array is pointing to NULL
 *
 */

static char **str_sep(s,l)
char *s,*l;
{
   char **tmp,**av = NULL;
   int i,max;
   char *c;

   if ( s == NULL ) 
      return NULL;

   s = skip_sep(s,l);
   if ( *s == '\0' ) 
      return NULL;

   if ( (av = (char**)malloc(sizeof(char*)*256)) == NULL ) 
      return NULL;

   i = 0;
   max = 256;

   tmp = av;
   av[i++] = s;
   while ( *s != '\0' ) {
      for ( c = l; *c != '\0'; c++ ) {
	 if ( *c == *s && i > 0 && av[i-1] != s ) {
	    if ( i == max ) {
	       max+=256;
	       if ( (tmp = (char**)malloc(sizeof(char*)*max)) == NULL) {   
		  free(av);
		  return NULL;
	       }
	       memcpy(tmp,av,sizeof(char*)*i);
	       av = tmp;
	    }

	    *s = '\0';
	    if (*(s = skip_sep(s+1,l)) != '\0')
	       av[i++] = s;
	    break;
	 }
      }
      if ( *s != '\0' )
	 s++;
   }
   av[i] = NULL;
   return av;
}



static char **update_table(table,num,max,str)
char **table;
int *num,*max;
char *str;
{
  char **tmp;

  if ( *num <= *max ) {
    *max += 10;
    tmp = (char**)malloc(sizeof(char*)**max);
    if ( tmp == NULL ) 
    return NULL;
    memcpy(tmp,table,sizeof(char*)**num);
    if (*num)
    free(table);
    table = tmp;
  }
  table[(*num)++] = strCopy(str);

  return table;
}

static char **parse_robot_file(fp,agent)
FILE *fp;
char *agent;
{
  char buff[BUFF_SIZE];
  char **av;

  static char  *empty_robot[] = {NULL};
  int i,counter;

  char **rlist = NULL;
  int rlistnum = 0,rlistmax = 0;

  char **olist = NULL;
  int olistnum = 0,olistmax = 0;


  int rflag = 0;
  int oflag = 0;
  
  int end_record = 1;
  int which_agent = 0;

  if ( fp == NULL ) 
  return NULL;

  /*
   * The format of each line of the robot.txt file is as follows 
   *
   *  <field>:[space]<value>[space]
   *
   *  Where field may be one of "User-agent", "Disallow"
   *  Note that a 
   */

  counter = 0;
  while ( fgets(buff,BUFF_SIZE,fp) != NULL ) {
    av = str_sep(buff,": \n");
      
    if ( av == NULL ) 
    continue;

    counter++;
    if ( strcasecmp(av[0],"User-agent") == 0 ) {
      if ( end_record ) {
        end_record = 0;
        which_agent = 0;
      }

      if ( agent != NULL && strcasecmp(av[1],agent) == 0 ) {
        which_agent |= SPECIFIED;
      }
      else 
	    if ( av[1][0] == '*' ) {
        which_agent |= ANY;
	    }
    }
    else 
    if ( which_agent && strcasecmp(av[0],"Disallow") == 0 ) {
	    end_record = 1;

	    if ( which_agent & ANY ) {
        if ( av[1] != NULL ) 
          olist = update_table(olist,&olistnum,&olistmax,av[1]);
        oflag = 1;
	    }

	    if ( which_agent & SPECIFIED ) {
        if ( av[1] != NULL ) 
          rlist = update_table(rlist,&rlistnum,&rlistmax,av[1]);
        rflag = 1;
	    }

    }

  }
  if ( counter == 0 ) return empty_robot;
  if ( rflag ) {
    if ( oflag ) {
      for (i = 0; i < olistnum; i++ )
        if ( olist[i] != NULL ) 
  	    free(olist[i]);
      
      if ( olist != NULL ) 
        free(olist);
    }
    if ( rlist  == NULL )
      rlist = (char**)malloc(sizeof(char*));
    if ( rlist != NULL ) 
      rlist[rlistnum] = NULL;
    return rlist;
  }
  else {
    if ( oflag ) {
      if ( olist == NULL )
        olist = (char**)malloc(sizeof(char*));
      if ( olist != NULL ) 
        olist[olistnum] = NULL;
    }
    return olist;
  }
      
}



char **check_robot_file(curr_filename,server,port,agent)
char *curr_filename;
char *server;
char *port;
char *agent;
{

   FILE *fp;
   char **table = NULL;
   URL *u = NULL;
   int ret;
   pathname_t tmp;
   int finished;
   HTTPReqHead Rq;
   char *accept = "text/plain; text/html";
   
   u = urlBuild("http",server,port,ROBOT_URL);


   /* Generate a "random" file name */

   srand(time((time_t *) NULL));

   for(finished = 0; !finished;){

      sprintf(tmp,"%s-%s_%d",curr_filename, "robot", rand() % 100);

      if(access(tmp, R_OK | F_OK) == -1)
         finished = 1;

   }

   fp = fopen(tmp,"w+");
   if ( fp == NULL )  {
      urlFree(u);
      return NULL;
   }
   
   Rq.accept = accept;
   Rq.uagent = WEB_AGENT_NAME;
   Rq.uaver = WEB_AGENT_VERSION;
   Rq.from = NULL;
   
   if ( httpGetfp(u,&Rq,60,&ret,fp,NULL) == 0 )  {
      fclose(fp);
      unlink(tmp);
      urlFree(u);
      return NULL;
   }


   if ( ret == 200 ) {
      fseek(fp,0L,0);
      table = parse_robot_file(fp,agent);
   }
   else {
     table = (char**)malloc(sizeof(char*));
     if ( table != NULL )
       table[0] = NULL;
   }

   fclose(fp);

   unlink(tmp);
   urlFree(u);
   return table;
   
}

#ifdef MAIN
char *prog = "robot";
int debug = 1;
main()
{
   char **av;
   av = check_robot_file("www.cs.mcgill.ca","80","test");


   for(;av != NULL && *av != NULL ; av++ )
      printf("%s\n",*av);

}


#endif
