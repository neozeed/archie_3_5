#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "typedef.h"
#include "error.h"
#include "db_files.h"

#define STOPLIST "stoplist"

static char **table = NULL;
static int table_num = 0;
static int table_max = 0;


static int strcomp(a,b)
char **a,**b;
{
  return strcasecmp(*a,*b);
}

static char **add_table(table, table_num, table_max, str)
char **table;
int *table_num,*table_max;
char *str;
{

   char **tmp;

   if ( *table_num >= *table_max-1 ) {

      *table_max += 1000;
      if ( table == NULL ) {
        tmp = (char**)malloc( sizeof(char*)**table_max);
      }
      else {
        tmp = (char **)realloc( table, sizeof(char*)**table_max);
      }
      if ( tmp == NULL ) {
        error(A_ERR, "add_table", "Unable to malloc memory");
        return table;
      }

      table = tmp;
   }

   table[(*table_num)] = str;

   (*table_num)++;
   return table;
}



int stoplist_keyword(str)
char **str;
{
  if ( table_num == 0 ) {
    return 0;
  }
  return( bsearch(str,table,table_num,sizeof(char*),strcomp) != NULL);
}


int stoplist_setup()
{
  FILE *fp;
  char buff[80];
  pathname_t stoplist_path;

  sprintf(stoplist_path,"%s/%s/%s", get_archie_home(), DEFAULT_ETC_DIR,STOPLIST);
  fp = fopen(stoplist_path,"r");
  if ( fp == NULL ) {
    return ERROR;
  }

  while (fgets(buff,80,fp) != NULL ) {
    char *str;
    buff[strlen(buff)-1] = '\0';
    str = (char*)malloc(sizeof(char)*(strlen(buff)+1));
    strcpy(str,buff);
    table = add_table(table,&table_num, &table_max,str);
  }

  fclose(fp);

  qsort(table,table_num, sizeof(char*),strcomp);
  return A_OK;
}





