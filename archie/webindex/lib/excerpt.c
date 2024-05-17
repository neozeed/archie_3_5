#include <stdio.h>
#include <search.h>
#include "typedef.h"
#include "excerpt.h"


int excerpt_extract(infp,outfp)
FILE *infp,*outfp;
{
  
  char buff[8*1024];
  int ret,i;
  char *s;
  int finished = 0;
  excerpt_t text;
  char *textp;
  char last;
  int in_tag;

  
  fseek(infp,0L,0); /* Go to the beginning of the file */

  memset(text,0,sizeof(excerpt_t));
  textp = text;

  in_tag = 0;
  last = '\0';
  
  while ( (ret = fread(buff,sizeof(char),8*1024,infp)) > 0  ){

    s = buff;
    
    for ( i = 0; i < ret && !finished ; i++, s++) {

      
      if ( *s == '<' ) {
        in_tag = 1;
        continue;
      }

      if ( *s == '>' ) {
        in_tag = 0;
        continue;
      }

      if ( in_tag ) {
        continue;
      }

      if ( *s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' ) {
        last = ' ';
        continue;
      }

      if ( last == ' ' ) {
        if ( textp-text == EXCERPT_MAX_LEN ) {
          finished = 1;
          continue;
        }
        *textp = last;
        textp++;
      }


      if ( textp-text == EXCERPT_MAX_LEN ) {
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
    fprintf(stderr,"Error in reading input file \n");
    return ERROR;
  }


/*  fprintf(outfp,"\nStart Excerpt\n");   */
  fwrite(text,sizeof(excerpt_t),1,outfp);
/*  fprintf(outfp,"\nEnd Excerpt\n"); */
  
  fflush(outfp);
  
  return A_OK;
}


