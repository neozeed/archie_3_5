#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <math.h>

#include "typedef.h"
#include "charset.h"
#include "error.h"
#include "db_files.h"
#include "parser_file.h"
#include "archstridx.h"
#include "start_db.h"
#include "files.h"

#define MIN_NUM_SITES 10
#define MAX_NUM_KEYS  500

#define MAX_KEY_LEN 50

#define MAX_KEYS  10000

static int max_keys = MAX_KEYS;

typedef struct {
  int num;
  char *key;
  char *stem;
  int in_sites;
  double w; /* weight */
} keyword_elmt_t;

extern int rec_no;

static int table_num = 0;
static int table_max = 0;
static keyword_elmt_t *table = NULL;
static keyword_elmt_t **tablelist  = NULL;

static void free_table(table,table_num)
keyword_elmt_t *table;
int *table_num;
{

  int i;
  for ( i = 0; i < *table_num; i++ ) {
    free(table[i].key);
    table[i].key =  NULL;
    free(table[i].stem);
    table[i].stem =  NULL;    
  }
  
  *table_num = 0;
}


static int get_num_sites()
{

  static int num_sites = -1;
  char **file_list;
  int i;
  
  if ( num_sites == -1 ) {


    if ( get_file_list(get_wfiles_db_dir(),&file_list,SUFFIX_EXCERPT,  &num_sites) == ERROR ) {
      num_sites = 0;
    }

    for ( i = 0; i < num_sites; i++ ) {
      free(file_list[i]);
    }
    free(file_list);
  }
  return num_sites;
  
}
static int cmptable(a,b)
keyword_elmt_t **a,**b;
{
  return strcasecmp((*a)->key,(*b)->key);
}

static int stemcmp(a,b)
keyword_elmt_t **a,**b;
{
  return strcasecmp((*a)->stem,(*b)->stem);
}


static int weightcmp(a,b)
keyword_elmt_t **a,**b;
{
  if ( (*a)->w > (*b)->w )
    return -1;
  if ( (*a)->w < (*b)->w )
    return 1;
  return 0;
}




static void compute_weight(table, table_num, num_keys, start_db, strhan)
keyword_elmt_t *table;
int table_num;
int *num_keys;
file_info_t *start_db;
struct arch_stridx_handle *strhan;
{
  int i,j,k;
  int total;
  unsigned long start[1];
  int number_of_sites;
  int sub_num;
  int sub_total;
  

  if ( table_num == 0 )
    return;

  qsort(tablelist,table_num,sizeof(keyword_elmt_t*),cmptable);


  /* Make items unique */
  tablelist[0]->num = 1;
  for ( j = 1, i = 0; j< table_num; j++ ) {
    if ( strcasecmp(tablelist[i]->key,tablelist[j]->key) == 0 ) {
      tablelist[i]->num++;
      if ( tablelist[j]->key != NULL ) {
        free(tablelist[j]->key);
        tablelist[j]->key = NULL;
      }
      tablelist[j] = NULL;
    }
    else {
      i++;
      tablelist[i] = tablelist[j];
      tablelist[i]->num = 1;
    }

  }
  *num_keys = i;

  /* Now find stems */
  total = 0;
  for ( i = 0; i < *num_keys; i++) {
    tablelist[i]->stem = (char*)stemmer(tablelist[i]->key);
    if ( tablelist[i]->stem == NULL ) {
      error(A_ERR,"compute_weight", "Unable to stem keyword %s", tablelist[i]->key);
/*      return ERROR; */
      return;
    }
    total += tablelist[i]->num;
  }

  number_of_sites = get_num_sites();
  if ( number_of_sites >= MIN_NUM_SITES ) {  
    /* Find in how many each keyword can be found */
    for ( i = 0; i < *num_keys; i++ ) {
      unsigned long state;
    
      if( !archSearchExact( strhan, tablelist[i]->key, 1, 1, &state, start) ){
        error(A_ERR, "compute_weight", "Could not perform exact search successfully.");
        /*      return(ERROR); */
        return;
      }
      if( state != 1 ){
        tablelist[i]->in_sites = 0;
      }
      else {
        int count;
        if ( count_index_start_dbs(start_db, start[1], &count ) == ERROR ) {
          error(A_INFO,"compute_weight", "Error in counting the number of sites for keywords %s",tablelist[i]->key);
        }
        tablelist[i]->in_sites = count;      
      }
    }
  }
  qsort(tablelist,*num_keys,sizeof(keyword_elmt_t*),stemcmp);
  

  for ( j = 1, i = 0; j< *num_keys; j++ ) {
    if ( strcasecmp(tablelist[i]->stem,tablelist[j]->stem) ) {
      sub_total = 0;
      for ( k = i; k <= j-1; k++ ) {
        sub_total += tablelist[k]->num;
      }
      if ( number_of_sites >= MIN_NUM_SITES ) {
        sub_num = 0;
        for ( k = i; k <= j-1; k++ ) {
          sub_num += tablelist[k]->in_sites;
        }
      }

      for ( k = i; k <= j-1; k++ ) {
        if ( number_of_sites >= MIN_NUM_SITES )
        tablelist[k]->w = ((double)sub_total / (double)total) *
                      log10((double)number_of_sites/(double)sub_num);
        else
        tablelist[k]->w = ((double)sub_total / (double)total);
      }
      i = j;
    }
  }

  /* Do the last section */
  sub_total = 0;
  for ( k = i; k <= j-1; k++ ) {
    sub_total += tablelist[k]->num;
  }
  if ( number_of_sites >= MIN_NUM_SITES ) {
    sub_num = 0;
    for ( k = i; k <= j-1; k++ ) {
      sub_num += tablelist[k]->in_sites;
    }
  }

  for ( k = i; k <= j-1; k++ ) {
    if ( number_of_sites >= MIN_NUM_SITES )
    tablelist[k]->w = ((double)sub_total / (double)total) *
                      log10((double)number_of_sites/(double)sub_num);
    else
    tablelist[k]->w = ((double)sub_total / (double)total);
  }
  
  qsort(tablelist,*num_keys,sizeof(keyword_elmt_t*),weightcmp);    

    
}



static keyword_elmt_t *add_keyword(table, table_num, table_max, str)
keyword_elmt_t *table;
int *table_num,*table_max;
char *str;
{

   keyword_elmt_t *tmp;
/*   keyword_elmt_t **t2;*/

   if ( *table_max == 0 ) {
     *table_max += 100;
     table = (keyword_elmt_t *)malloc(sizeof(keyword_elmt_t)**table_max);
     if ( table == NULL ) {
        error(A_ERR, "add_keyword", "Unable to malloc memory");
        return NULL;
     }
   }
   
   if ( *table_num >= *table_max-1 ) {
     
     if ( *table_max+10 > max_keys ) {
       return table;
     }
     
     *table_max += 500;

     tmp = (keyword_elmt_t *)realloc(table,sizeof(keyword_elmt_t)**table_max);
/*     error(A_INFO,"add_keyword", "Reallocating table of size: %d",sizeof(keyword_elmt_t)**table_max); */
     if ( tmp == NULL ) {
       error(A_ERR, "add_keyword", "Unable to realloc memory");
       return table;
     }
     table = tmp;

   }

   table[(*table_num)].key = (char*)malloc(strlen(str)+1);
   strcpy(table[(*table_num)].key,str);
   table[(*table_num)].stem = NULL;
   
   (*table_num)++;
   return table;
}


int valid_keyword(char *key)
{
  int i, counter;
  
  for( i = 0, counter = 0; key[i] != '\0'; i++ ) {
    if ( key[i] & 0x80 )
      counter++;
  }
  
  /* Do not keep if the keyword is long */
  /* Do not keep if more than 25% are 8 bit chars */
  /* maybe the keyword is binary .. */
  if ( i > MAX_KEY_LEN || 4*counter > i )  
    return 0;
  
  return 1;
}

status_t keyword_extract(infp,outfp, strhan)
FILE *infp,*outfp;
struct arch_stridx_handle *strhan;
{
  
  char buff[16*1024+1];
  int ret,i;
  char *s,*t,*u;
  parser_entry_t pent;
  int num_keys = 0;
  static file_info_t *start_db = NULL;
  static file_info_t *domain_db = NULL;
  int s_exist, d_exist;

  memset(&pent,0,sizeof(parser_entry_t));
  
  if ( start_db == NULL ) {
    
    start_db = create_finfo();
    domain_db = create_finfo();

    if ( exist_start_dbs(start_db, domain_db, &s_exist, &d_exist ) == ERROR ) {
      /* "Can't stat start/host database" */

      error(A_ERR, "keyword_extract", "Can't stat start/host database");
      exit(A_OK);
    }

    if ( s_exist == 0 ) {
      start_db->fp_or_dbm.dbm = NULL;
      domain_db->fp_or_dbm.dbm = NULL;
    }
    else 
    if ( open_start_dbs(start_db,domain_db,O_RDONLY ) == ERROR ) {

      /* "Can't open start/host database" */

      error(A_ERR, "keyword_extract", "Can't open start/host database");
      exit(A_OK);
    }
    
  }
  
  free_table(table,&table_num);
  
  while ( (ret = fread(buff,sizeof(char),16*1024,infp)) > 0  ){
    int in_tag, in_word, in_comment;

    in_tag = in_word = in_comment = 0;
    u = t = s = buff;
    for ( i = 0; i < ret; i++, s++) {

      if ( *s == '<' ) {
        if ( strncasecmp(s,"<!--",4) == 0 ) {
          in_comment = 1;
        }
        else {
          in_tag = 1;
        }
        if ( in_word ) {
          *s = '\0';
          *u = '\0';
          if ( stoplist_keyword(&t) == 0 && valid_keyword(t) ) {
            table = add_keyword(table,&table_num,&table_max,t);
          }
          in_word = 0;
        }
        continue;
      }

      if ( *s == '-' && strncasecmp(s,"-->",3) == 0 ) {
        in_comment = 0;
        s+=3;
        i+=3;
        continue;
      }
      if ( *s == '>' && in_tag  ) {
        in_tag = 0;
        continue;
      }

      if ( in_tag || in_comment ) {
        continue;
      }
      
      if ( *s == '&' ) {
        int cret = html_char(s);
        if ( cret >= 0 ) {
          if ( html_isalnum(cret) ) {          
            if ( in_word == 0 ) {
              u = t = s;
              in_word = 1;
            }
            *u = cret;
            u++;
          } 
          for ( ; i < ret && *s != '\0'; s++,i++ )
            if ( *s == ';' )
              break;
          continue;
        }
        else {
          switch ( cret ) {
            case UNDEFINED_CHAR:
/*              printf("%c",*s); */
              break;
          
            case INVALID_CODED_CHAR: /* Cannot find the ; character ... so skip over & */
              break;
            
            }
        }
      }

      
      if ( *s == '-' ) { /* Assume same word .. */
        if ( in_word ) {
          *u = *s;
          u++;
        }
        continue;
      }

      
      if ( !html_isalnum(*s) ) {
        if ( in_word ) {
          *s = '\0';
          *u = '\0';
          if ( stoplist_keyword(&t) == 0 && valid_keyword(t) ) {
            table = add_keyword(table,&table_num,&table_max,t);
          }
        }
        in_word = 0;
        continue;
      }
      
      if ( in_word == 0 ) {
        u = t = s;
        u++;
        in_word = 1;
      }
      else {
        *u = *s;
        u++;
      }
    }
  }

  if ( ret == -1 ) {
    error(A_ERR, "keyword_extract","Error in reading input file");
    return ERROR;
  }

  tablelist = (keyword_elmt_t**)malloc(sizeof(keyword_elmt_t*)*(table_num));
  if ( tablelist == NULL ) {
    error(A_ERR, "compute_weight", "Cannot allocate memory");
    return ERROR;
  }

  for ( i=0; i< table_num; i++)
    tablelist[i] = &table[i];
  
  
  /* At this point need to compute the weight of each keyword */
  compute_weight(table,table_num, &num_keys, start_db, strhan);
  
  /* At this point need to spit out the keywords and weights */
#if 0
  for ( i = 0; i < num_keys; i++ ){
    fprintf(outfp,"key: %s %f\n",table[i].key,table[i].w);
  }
#endif

  pent.core.flags = 0;
  CSE_SET_KEY(pent.core);

  for ( i = 0; i < MAX_NUM_KEYS && i < num_keys; i++ ){ 

    pent.slen = strlen(tablelist[i]->key);
    CSE_STORE_WEIGHT(pent.core,tablelist[i]->w);
    output(&pent,tablelist[i]->key,outfp);
    rec_no++;
  }
  free_table(table,&table_num);
  
  fflush(outfp);

  free(tablelist);

  return A_OK;
}


