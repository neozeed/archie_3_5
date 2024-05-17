#include <stdio.h>
#include <signal.h>
#include <ansi_compat.h>
#include <signal.h>
#include <setjmp.h>

#include "url.h"
#include "http.h"
#include "protos.h"
#define BUFF_SIZE 1024

extern int errno;

static int counter = 0;
static jmp_buf return_pt;

static void alarm_func( )
{
  static int last_counter = 0;
  
  if ( last_counter == counter ) {
    longjmp(return_pt,1);
  }
  last_counter = counter;
  signal(SIGALRM, alarm_func);
  alarm(900);
  
}



static int httpSendHead(ofp,param)
FILE *ofp;
HTTPReqHead *param;
{
#if 0
   char date[80];
   char *s;
   if ( param->IfModSince != NULL ) {
      strcpy(date,asctime(param->IfModSince));
      s = strchr(date,'\n');
      if ( s != NULL ) 
	 *s = '\0';
      (void)fprintf(ofp,"If-Modified-Since: %s\r\n",date);
   }
#endif

   if ( param->uagent != NULL ) {
      (void)fprintf(ofp,"User-Agent: %s/%s\r\n",param->uagent,param->uaver);
   }

   if ( param->from != NULL ) {
      (void)fprintf(ofp,"From: %s\r\n",param->from);
   }

   if ( param->accept != NULL ) {
     (void)fprintf(ofp,"Accept: %s\r\n",param->accept);
   }
   
   return 1;
}



int httpGetfp(url,params,timeout,ret,html_fp,http_fp)
URL *url;
HTTPReqHead *params;
int timeout;
int *ret;
FILE *html_fp,*http_fp;
{

   char buff[BUFF_SIZE];
   FILE *ifp,*ofp;
   
   if ( openTCP(url->server,url->port,timeout,&ifp,&ofp) ) {

      /* Send the GET packet */
      (void)fprintf(ofp,"GET %s HTTP/1.0\r\n",url->local_url);

      if ( params != NULL ) 
        (void)httpSendHead(ofp,params);

      (void)fprintf(ofp,"\r\n");


      signal(SIGALRM, alarm_func);
      alarm(900);
      counter = 0;
      
      if ( setjmp(return_pt) ) { /* Return from longjmp */
        fclose(ifp);
        fclose(ofp);
        *ret = -1;
        return 0;
      }
      
      /* Get the status line  */

      if (fgets(buff,BUFF_SIZE,ifp) == NULL ) {
        *ret = ( ferror(ifp) ) ? errno : 0;
        return 0;
      }

      /* Should check the entire line ... but ... */
      if  ( strncasecmp(buff,"HTTP/",5) == 0 ) {
         *ret = atoi(buff+8);
         while ( fgets(buff,BUFF_SIZE,ifp) != NULL ) {
           if ( strncasecmp(buff,"<HTML>",6) == 0 ||
               (buff[0] == '\r' && buff[1] == '\n' ) || buff[0] == '\n' ) {
             break;
           }
           else if ( http_fp != NULL ) {
             (void)fprintf(http_fp,"%s",buff);
           }
         }


         if ( ferror(ifp) ) {
           *ret = errno;
           return 0;
         }
       }

      if ( (buff[0] != '\r' && buff[1] != '\n' ))
      (void)fprintf(html_fp,"%s",buff);

      while ( fgets(buff,BUFF_SIZE,ifp) != NULL ) {
        counter++;
        (void)fprintf(html_fp,"%s",buff);
      }

      alarm(0);
      
      if ( ferror(ifp) ) {
        *ret = errno;
        return 0;
      }
      fclose(ifp);
      fclose(ofp);
      return 1;
    } 
   *ret = -1;
   return 0;
}
   

int httpHeadfp(url,params,timeout,ret,html_fp,http_fp)
URL *url;
HTTPReqHead *params;
int timeout;
int *ret;
FILE *html_fp,*http_fp;
{

  char buff[BUFF_SIZE];
  FILE *ifp,*ofp;
   

  /*   fprintf(stderr, "httpHeadfp: server `%s', port `%s', timeout %d.\n",
       url->server, url->port, timeout); */ /* wheelan */
  if ( openTCP(url->server,url->port,timeout,&ifp,&ofp) ) {

    /* Send the GET packet */
    (void)fprintf(ofp,"HEAD %s HTTP/1.0\r\n",url->local_url);

    if ( params != NULL ) 
    (void)httpSendHead(ofp,params);

    (void)fprintf(ofp,"\r\n");

    /* Get the statur line  */

    if (fgets(buff,BUFF_SIZE,ifp) == NULL ) {
      *ret = ( ferror(ifp) ) ? errno : 0;
      return 0;
    }

    /* Should check the entire line ... but ... */
    if  ( strncasecmp(buff,"HTTP/",5) == 0 ) {
      *ret = atoi(buff+8);
      while ( fgets(buff,BUFF_SIZE,ifp) != NULL ) {
        (void)fprintf(http_fp,"%s",buff);
      }
    }
    if ( ferror(ifp) ) {
      *ret = errno;
      return 0;
    }


    fclose(ifp);
    fclose(ofp);
    return 1;
  }
  *ret = errno;
  return 0;
}
   

