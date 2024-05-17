#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "charset.h"


typedef struct {
  int index;
  char *name;
} char_elmt;


static char_elmt iso8859_char_table[] = {
  {34,"quot"},  {38,"amp"},   {60,"lt"},   {62,"gt"},
  {192,"Agrave"},  {193,"Aacute"},   {194,"Acirc"},   {195,"Atilde"},
  {196,"Auml"},  {197,"Aring"},   {198,"AElig"},   {199,"Ccedil"},
  {200,"Egrave"},  {201,"Eacute"},   {202,"Ecirc"},   {203,"Euml"},
  {204,"Igrave"},  {205,"Iacute"},   {206,"Icirc"},   {207,"Iuml"},
  {209,"Ntilde"},  {210,"Ograve"},   {211,"Oacute"},   {212,"Ocirc"},
  {213,"Otilde"},  {214,"Ouml"},   {216,"Oslash"},   {217,"Ugrave"},
  {218,"Uacute"},  {219,"Ucirc"},   {220,"Uuml"},   {221,"Yacute"},
  {222,"THORN"},  {223,"szlig"},   {224,"agrave"},   {225,"aacute"},
  {226,"acirc"},  {227,"atilde"},   {228,"auml"},   {229,"aring"},
  {230,"aelig"},  {231,"ccedil"},   {232,"egrave"},   {233,"eacute"},
  {234,"ecirc"},  {235,"euml"},   {236,"igrave"},   {237,"iacute"},
  {238,"icirc"},  {239,"iuml"},   {240,"eth"},   {241,"ntilde"},
  {242,"ograve"},  {243,"oacute"},   {244,"ocirc"},   {245,"otilde"},
  {246,"ouml"},  {248,"oslash"},   {249,"ugrave"},   {250,"uacute"},
  {251,"ucirc"},  {252,"uuml"},   {253,"yacute"},   {254,"thorn"},
  {-1,NULL}
};


static int iso8859_alnum[] = {
  /* #00 -> #47 */
  0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,1,0,0,
  /* '0' -> '9' */
  1,1,1,1,1,1,1,1,1,1,
  /* #58 -> #64 */
  0,0,0,0,0,0,0,
  /* 'A' -> 'Z' */
  1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,
  /* #91 -> #96 */
  0,0,0,0,0,0,
  /* 'a' -> 'z' */
  1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,
  /*  #123 -> #191 */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,
  /* #192 -> #255 */
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,1,1,1,1,1,1,1,1
};


static char_elmt **iso8859_char_list = NULL;
static int iso8859_list_num = 0;



static int char_cmp(a,b)
char **a,**b;
{
  return (strcmp((*(char_elmt**)a)->name,(*(char_elmt**)b)->name));
}



static int find_char( name )
char *name;
{
  char_elmt **res,*ch,key;
  
  if ( iso8859_char_list == NULL ) {
    int i;

    for ( i = 0; iso8859_char_table[i].name  != NULL;   )
      i++;
    i = iso8859_list_num = i;
    iso8859_char_list = (char_elmt**)malloc(sizeof(char_elmt*)*i);
    if ( iso8859_char_list == NULL ) {
      return UNDEFINED_CHAR;
    }

    for (i = 0; i < iso8859_list_num; i++ ) {
      iso8859_char_list[i] = &iso8859_char_table[i];
    }
      
    qsort( iso8859_char_list, iso8859_list_num, sizeof(char_elmt*), char_cmp);
    
  }


  key.name = name;
  ch = &key;
  res = (char_elmt**)bsearch(&ch,iso8859_char_list,iso8859_list_num, sizeof(char_elmt**), char_cmp);
  
  if ( res == NULL ) {
    return UNDEFINED_CHAR;
  }

  return (*res)->index;

}




int html_char(s)
char *s;
{

  char *t;
  char buff[256];
  
  if ( s == NULL ) {
    return INVALID_CODED_CHAR;
  }

  if ( *s != '&' ) {
    return INVALID_CODED_CHAR;
  }
  
  t = ++s;
  while ( *s != '\0' ) {

    if ( *s == ';' ) { /* Reached end of coded character */
      if ( *t == '#' ) { /* this is coded as per number */
        return atoi(t+1);
      }
      strncpy(buff,t,s-t);
      buff[s-t] = '\0';
      return find_char(buff);
    }
    if ( isspace(*s) )
      return INVALID_CODED_CHAR;
    s++;
  }

  return INVALID_CODED_CHAR;
      
}


int html_isalnum(c)
char c;
{
  return iso8859_alnum[(int)c];
}



