#include <stdio.h>
#include <string.h>
#include <search.h>
#include "typedef.h"
#include "error.h"
#include "excerpt.h"



static int title_extract(infp, title)
  FILE *infp;
  char *title;
{
  char *textp = title;
  char buff[8*1024];
  char last;
  int ret,i,finished;
  int in_tag, in_title;
  char *s;
  
  memset(title,0,TITLE_SIZE);
  
  in_tag = 0;
  last = ' ';
  in_title = 0;
  finished = 0;
  while ( (ret = fread(buff,sizeof(char),8*1024,infp)) > 0  ){

    s = buff;
    
    for ( i = 0; i < ret && !finished ; i++, s++) {

      
      if ( *s == '<' ) {
        in_tag = 1;
        if ( in_title && strncasecmp(s,"</TITLE>",8) == 0 ) {
          long pos;
          in_title = 0;
          *textp = '\0';
          s+=8;
          pos = ftell(infp);
          if ( pos < 8*1024 )
            fseek(infp, (long)(s-buff), 0 );
          else 
            fseek(infp, ftell(infp)-8*1024 + (s-buff), 0);
          
          finished = 1;
          break;

        }
        
        if ( strncasecmp(s,"<TITLE>",7) == 0 ) {
          in_title = 1;
        }
        continue;
      }

      if ( *s == '>' ) {
        in_tag = 0;
        continue;
      }

      if ( in_tag || in_title == 0 ) {
        continue;
      }

      if ( *s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' ) {
        last = ' ';
        continue;
      }

      if ( last == ' ' ) {
        if ( textp-title == TITLE_SIZE-1 ) {
          finished = 1;
          continue;
        }
        *textp = last;
        textp++;
      }


      if ( textp-title == TITLE_SIZE-1 ) {
        finished = 1;
        continue;
      }
      
      *textp = *s;
      textp++;
      last = *s;

    }

    
    if ( finished )
      break;
  }

  *textp = '\0';
  if ( ret == -1 ) {
    error(A_ERR, "title_extract","Error in reading input file");
    return ERROR;
  }

  if ( *title == ' ' ){
    char *t1, *t2;
    t1 = t2 = title;
    while ( *t2 == ' ' )
    t2++;

    while (*t2 != '\0' ){
      *t1++ = *t2++;
    }
    *t1 = '\0';
  }

  return A_OK;
}



static int text_extract(infp,text)
  FILE *infp;
  char *text;
{

  char buff[8*1024];
  int ret,i;
  char *s;
  int finished = 0;

  char *textp;
  char last;
  int in_tag, in_comment;

  
  memset(text,0,TEXT_SIZE);
  
  textp = text;

  in_tag = in_comment = 0;
  last = '\0';
  
  while ( (ret = fread(buff,sizeof(char),8*1024,infp)) > 0  ){

    s = buff;
    
    for ( i = 0; i < ret && !finished ; i++, s++) {

      
      if ( *s == '<' ) {
        if ( strncasecmp(s,"<!--",4) == 0 ) {
          in_comment = 1;
          s+=3; /* s++ in the for loop */
          continue;
        }
        in_tag = 1;
        continue;
      }
      if ( *s == '-' && strncasecmp(s,"-->",3) == 0 ) {
        in_comment = 0;
        last = ' ';
        s+=2; /* s++ in the for loop */
        continue;
      }
      
      if ( *s == '>'  && in_tag ) {
        in_tag = 0;
        last = ' ';
        continue;
      }

      if ( in_tag || in_comment ) {
        continue;
      }

      if ( *s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' ) {
        last = ' ';
        continue;
      }

      if ( last == ' ' ) {
        if ( textp-text == TEXT_SIZE-1 ) {
          finished = 1;
          continue;
        }
        *textp = last;
        textp++;
      }


      if ( textp-text == TEXT_SIZE-1 ) {
        finished = 1;
        continue;
      }
      *textp = *s;
      textp++;
      last = *s;

    }
    if ( finished )
      break;
  }

  *textp = '\0';
  
  if ( ret == -1 ) {
    error( A_ERR, "text_extract", "Error in reading input file");
    return ERROR;
  }
  return A_OK;
}

status_t  excerpt_extract(infp,outfp)
FILE *infp,*outfp;
{
  
  
  excerpt_t excerpt;

  fseek(infp,0L,0); /* Go to the beginning of the file */

  if ( title_extract(infp,excerpt.title) == ERROR ) {
    error(A_ERR, "excerpt_extract", "Error while searching for title");
    return ERROR;
  }

  if ( excerpt.title[0] == '\0' ) {
    fseek(infp,0L,0); /* Go to the beginning of the file */    
  }


  if ( text_extract(infp,excerpt.text) == ERROR ) {
    error(A_ERR,"excerpt_extract", "Error while searching for text");
    return ERROR;
  }

  fwrite(&excerpt,sizeof(excerpt_t),1,outfp);
  
  fflush(outfp);
  
  return A_OK;
}


