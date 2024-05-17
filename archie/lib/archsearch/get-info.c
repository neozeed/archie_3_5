#include <stdio.h>
#include "error.h"
#include "protos.h"
#include "typedef.h"
#include "host_db.h"
#include "search.h"
#include "get-info.h"

#ifndef SOLARIS
#include "protos.h"
#ifdef __ANSI__
  extern char *quoteString PROTO(( char * ));
#else
  extern char *quoteString();
  extern char *strupr();
  extern char *strlwr();
#endif
#endif

extern char *prog;

/*
 *  Trims the trailing and preceding spaces in the input string.  Those
 *  spaces confuse the boolean search
 */

extern char *trim(str)
  char **str;
{
  char *fptr, *bptr, *string;

  if( *str==NULL || (*str)[0]=='\0' ) return *str;
  string = *str;
  fptr = string;
  if (fptr)
  {
    for ( ; *fptr == ' '; fptr++);
  }
  bptr = string + strlen(string) - 1;
  if( bptr ){
    for( ; *bptr == ' '; --bptr);
    bptr++;
    if( *bptr == ' ' )    *bptr = '\0';
  }
  string = strdup(fptr);
  free(*str);
  *str = string;

  return string;
}

/*
 *  Change given string to upper case
 */
extern char *strupr(str)
  char *str;
{
  char *string = str;

  if (str)
  {
    for ( ; *str; ++str)
    *str = toupper(*str);
  }
  return string;
}


/*
 *  Change given string to lower case
 */
extern char *strlwr(str)
  char *str;
{
  char *string = str;

  if (str)
  {
    for ( ; *str; ++str)
    *str = tolower(*str);
  }
  return string;
}


/*
 *  Counts the number of tokens being sperated by the charachter "ch".
 */
static int count_toks(str,ch)
  char *str;
  char *ch;
{
  char *s,*p;
  int i;

  if( strchr( str, ch[0] ) == (char *) NULL )
  { return 0;
  }
  
  s = strdup(str);
  for( i=0, p=strtok(s,ch) ; p!=(char *)NULL ; i++ )
  {
    p = strtok((char *)NULL,ch);
  }
  free(s);
  return(i);
}


/*
 *  Takes an integer representing type and returns the name of that
 *  type
 */
extern char * get_type( inttype )
  int inttype;
{
  char *chartype;
  int l;

  chartype=NULL;
  l=0;
  switch(inttype){
  case 0:
    l=strlen(FORM_EXACT)+1;
    chartype=(char *)malloc(sizeof(char)*l);
    strcpy(chartype,FORM_EXACT);
    break;
  case 1:
    l=strlen(FORM_SUB)+1;
    chartype=(char *)malloc(sizeof(char)*l);
    strcpy(chartype,FORM_SUB);
    break;
  case 2:
    l=strlen(FORM_REGEX)+1;
    chartype=(char *)malloc(sizeof(char)*l);
    strcpy(chartype,FORM_REGEX);
    break;
  default:
    error(A_ERR,"get_type","int type is not valid");
    return((char *)NULL);
    break;
  }
  if(l>0){
    chartype[l-1] = '\0';
  }
  return(chartype);
}


/*
 *  Takes an integer representing case and returns the name of that
 *  type
 */
extern char * get_case( intcase )
  int intcase;
{
  char *charcase;
  int l;

  charcase=NULL;
  l=0;
  switch(intcase){
  case 0:
    l=strlen(FORM_CASE_INS)+1;
    charcase=(char *)malloc(sizeof(char)*l);
    strcpy(charcase,FORM_CASE_INS);
    break;
  case 1:
    l=strlen(FORM_CASE_SENS)+1;
    charcase=(char *)malloc(sizeof(char)*l);
    strcpy(charcase,FORM_CASE_SENS);
    break;
  default:
    error(A_ERR,"get_case","int case is not valid");
    return((char *)NULL);
    break;
  }
  if(l>0){
    charcase[l-1] = '\0';
  }
  return(charcase);
}

static status_t parse_bool_more( b, ret )
  char *b;
  boolean_return_t *ret;
{
  char *s,*p;
  s = strdup(b);

  if( (p = strtok(s,","))==(char *)NULL ) return ERROR;
  ret->and_tpl = atoi(p);
  if( (p = strtok((char *)NULL,","))==(char *)NULL ) return ERROR;
  ret->site_in_list = atoi(p);
  if( (p = strtok((char *)NULL,","))==(char *)NULL ) return ERROR;
  ret->acc_res = atoi(p);

  free(s);
  return A_OK;
}

static char * get_bool_more( bent )
  boolean_return_t bent;
{
  char *ret;
  ret = (char *)malloc(sizeof(char)*MAX_STR);
  memset( ret,0,sizeof(char)*MAX_STR );
  sprintf(ret, "%d,%d,%d",
          bent.and_tpl, bent.site_in_list, bent.acc_res);
  return ((char *)ret);
}

int send_error( err_type )
  char *err_type;
{
  fprintf(stdout,"Content-type: text/html\n\n");
  fprintf(stdout,"<html><h2>Archie Results</h2><HR>\n");
  fprintf(stdout,"<h4>Database Error</h4>\n<P><UL><B>");
  fprintf(stdout,"<i>%s</i><P>",err_type);
  fprintf(stdout,"</html>\n");
  return 1;
}

int send_error_in_plain( err_type )
  char *err_type;
{
  fprintf(stdout,"ERROR=%s\n",err_type);
  return 1;
}

static status_t parse_queries( qry, words, ops, num )
  char *qry;
  char ***words;
  bool_ops_t **ops;
  int *num;
{
  char *orig, *p;
  int j,w,o;

  *num = 0;
  
  if( (orig = strdup(qry)) == (char *)NULL ){
    fprintf(stderr,"Error: Could not allocate memory for strdup.");
    return ERROR;
  }
  *num = count_toks( orig," ");
  if( *num ){
    *words = (char **)malloc(sizeof(char *)* (*num));
    *ops = (bool_ops_t *)malloc(sizeof(bool_ops_t)*(*num));
    w = o = 0;
    for(j=0, p = strtok( orig," ") ;
        j < *num && p != (char *)NULL ;
        p = strtok((char *)NULL," "), j++)
    {
      if( !strcmp( p , "AND" ) || !strcmp( p , "OR") || !strcmp( p , "NOT" ) ){
        if( !strcmp( p , "AND" ) )  (*ops)[o] = AND_OP;
        if( !strcmp( p , "OR" ) )  (*ops)[o] = OR_OP;
        if( !strcmp( p , "NOT" ) )  (*ops)[o] = NOT_OP;
        if( j%2 == 0 ){
          error(A_ERR,"parse_queries","The boolean query is improper.");
          return ERROR;
        }
        o++;
      }else{
        (*words)[w] = (char *)malloc(sizeof(char)*(strlen(p)+1));
        strcpy( (*words)[w] , p );
        if( j%2 != 0 ){
          error(A_ERR,"parse_queries","The boolean query is improper.");
          return ERROR;
        }
        w++;
      }
    }
    (*words)[w] = NULL;
    (*ops)[o] = 0;
    *num = w;
  }
  else{
    *words = (char **)malloc(sizeof(char *)*1);
    *ops = (bool_ops_t *)malloc(sizeof(bool_ops_t)*1);
    (*words)[0] = (char *)malloc(sizeof(char)*(strlen(qry)+1));
    (*ops)[0] = NULL_OP;
    strcpy( (*words)[0], qry);
    *num = 1;
  }
  if (orig) {free(orig); orig = NULL;}
  return A_OK;
}

static status_t parse_boolean_query( inqry, outqry, type  )
  char **inqry;
  bool_query_t *outqry;
  int *type;
{
  char *orig, **ops, *p, *q, *g;
  int j,ands,ors,nots,num,max,length,k,i,ok,tmp,val;
  bool_ops_t op;
  char **words, **nwords;
  int len,*l,*ordered;

  num = 0;
  ands = 0; /* per and_list it counts how many words */
  ors = 0; /* counts how many and_lists per outqry */
  nots = 0; /* counts how many not words per outqry */
  words = (char **)NULL;
  ordered = NULL;

  outqry->not_list = (bool_list_t *)NULL;
  outqry->and_list = (bool_list_t *)NULL;
  outqry->orn = 0;
  *type = 1;

  *inqry = trim(&(*inqry));
  if( (orig = strdup(*inqry)) == (char *)NULL ){
    fprintf(stderr,"Error: Could not allocate memory for strdup.");
    return ERROR;
  }
  num = count_toks( orig," ");
  if( num ){
    words = (char **)malloc(sizeof(char *)* (num));
    op=NULL_OP;
    i=0;
    p=strtok( orig," ") ;
    ops = (char **)malloc(sizeof(char *)*num);
    while( p!=NULL ){
      q=strdup(p);
      q=strlwr(q);
      ops[i] = q;
      if(strcmp(q,"and") && strcmp(q,"or") && strcmp(q,"not")){
        /* try to get the quoted words together */

        if( p[0]=='"' ){
          p++;
          g = p;

          len=strlen(g);
          if( (g+len) && (g+len+1) && g[len]=='\0' && g[len+1]=='"' ){
            g[len]=' ';
            p = strtok((char *)NULL," ");
            if( p && p[0]=='"' )   p = strtok((char *)NULL," ");
            g[len+1]='\0';
            if( g!= NULL ){
              sprintf(g,"%s",g);
              words[ands]=(char *)malloc((strlen(g)+1)*sizeof(char));
              strcpy(words[ands],g);
              ands++;
            }
            continue;
          }

          p = strtok(NULL,"\"");
          if( p!= NULL ){
            sprintf(g,"%s %s",g,p);
            p=g;
          }else{
            len=strlen(g);
            if( (g+len) && (g+len+1) && g[len]=='\0' && g[len+1]=='"' ){
              g[len]=' ';
              p = g;
              g[len+1]='\0';
            }else{
              send_error_in_plain("The query sent is not a valid boolean query. Check the parameters.");
              return ERROR;
            }
          }
        }
        /* end of quoted words */

        if( op == NOT_OP ){
          if(outqry->not_list == (bool_list_t *)NULL){
            outqry->not_list = (bool_list_t *)malloc(sizeof(bool_list_t)*num);
            memset(outqry->not_list,0,sizeof(bool_list_t)*num);
          }
          if( outqry->not_list[ors].lwords == (char **)NULL){
            outqry->not_list[ors].lwords = (char **)malloc(sizeof(char *)* (num));
            outqry->not_list[ors].lnum = 0;
          }
          outqry->not_list[ors].lwords[nots]=(char *)malloc((strlen(p)+1)*sizeof(char));
          strcpy(outqry->not_list[ors].lwords[nots],p);
          nots++;
          op=AND_OP;
        }else{
          words[ands]=(char *)malloc((strlen(p)+1)*sizeof(char));
          strcpy(words[ands],p);
          ands++;
        }
      }else{
        if(!strcmp(q,"and")){
          op=AND_OP;
        }else if(!strcmp(q,"or")){
          op=OR_OP;
          words = (char **)realloc(words,sizeof(char *)*(ands));
          if(outqry->and_list == (bool_list_t *)NULL){
            outqry->and_list = (bool_list_t *)malloc(sizeof(bool_list_t)*num);
          }
          if(outqry->not_list == (bool_list_t *)NULL){
            outqry->not_list = (bool_list_t *)malloc(sizeof(bool_list_t)*num);
            memset(outqry->not_list,0,sizeof(bool_list_t)*num);
          }
          outqry->and_list[ors].lwords = words;
          outqry->and_list[ors].lnum = ands;
          words = (char **)malloc(sizeof(char *)* (num));
          words[0] = (char *)NULL;
          outqry->not_list[ors].lnum = nots;
          outqry->not_list[ors].lwords = (char **)realloc(outqry->not_list[ors].lwords, sizeof(char *)* (nots));
          ors++;
          ands=0;
          nots=0;
        }else if(!strcmp(q,"not")){
          op=NOT_OP;
        }
      }
      p = strtok((char *)NULL," ");
      i++;
    }

    for( i=0; i<num; i++ ){
      free( ops[i] );
    }
    free(ops);
    
    if( words && words[0]!=(char *)NULL ){
      words = (char **)realloc(words,sizeof(char *)*(ands));
      if(outqry->not_list == (bool_list_t *)NULL){
        outqry->not_list = (bool_list_t *)malloc(sizeof(bool_list_t)*num);
        memset(outqry->not_list,0,sizeof(bool_list_t)*num);
      }
      if(outqry->and_list == (bool_list_t *)NULL){
        outqry->and_list = (bool_list_t *)malloc(sizeof(bool_list_t)*num);
      }
      outqry->and_list[ors].lwords = words;
      outqry->and_list[ors].lnum = ands;
      outqry->not_list[ors].lwords = (char **)realloc(outqry->not_list[ors].lwords, sizeof(char *)* (nots));
      outqry->not_list[ors].lnum = nots;
      ors++;
      ands=0;
      nots=0;
    }else{
      if(words) free(words);
    }
    if( ors==0 && ands>0 ) ors++;

    /*
     *  outqry->not_list[ands].lnum = nots; outqry->not_list.lwords = (char
     *  **)realloc(outqry->not_list.lwords, sizeof(char *)* (nots));
     */

    if( ors ){
      outqry->and_list = (bool_list_t *)realloc(outqry->and_list, sizeof(bool_list_t)*ors);
      outqry->orn = ors;
      outqry->not_list = (bool_list_t *)realloc(outqry->not_list, sizeof(bool_list_t)*ors);
    }
  }
  else{
    words = (char **)malloc(sizeof(char *)*1);
    words[0] = (char *)malloc(sizeof(char)*(strlen(*inqry)+1));
    strcpy( words[0], *inqry);
    outqry->and_list = (bool_list_t *)malloc(sizeof(bool_list_t)*1);
    outqry->and_list[0].lwords = words;
    outqry->and_list[0].lnum = 1;  
    outqry->orn = 1;
    *type = 0;
  }


  for(i=0; i<outqry->orn; i++){
    nwords=(char **)NULL;
    ok = 0;
    num = outqry->and_list[i].lnum;
    ordered = (int *)malloc(sizeof(int)*num);
    l = (int *)malloc(sizeof(int)*num);
    max=0;
    nwords=(char **)malloc(sizeof(char *)*num);
    for(j=0; j<num; j++){
      length = strlen( outqry->and_list[i].lwords[j] );
      l[j] = length;
      if( max < length ) {
        for( k=ok-1 ; k>=0 && k<(num-1) ; k-- ){
          ordered[k+1] = ordered[k];
        }
        ordered[0] = j;
        ok++;
        max = length;
      }else{
        for(k=0;k<ok && l[ordered[k]]>length;k++);
        tmp = j;
        for(;k<=ok;k++){
          val = ordered[k];
          ordered[k] = tmp;
          tmp = val;
        }
        ok++;
      }
    }
    free(l);
    l=NULL;
    if( num ){
      for(j=0; j<num; j++){
        nwords[j] =  outqry->and_list[i].lwords[ordered[j]];
      }
      free( outqry->and_list[i].lwords );
      outqry->and_list[i].lwords = nwords;
    }
  }

  if (ordered) { free(ordered); ordered=NULL; }
  if (orig) { free(orig); orig = NULL; }
  return A_OK;
}


extern int parse_entries( entries, qry, db, type, case_sens, max_hits,
                         hpm, match, path, expath, path_rel, path_items,
                         domains, surl, gurl, han, format, booltype,
                         start_tbl, more_tbl )
  entry *entries;
  bool_query_t *qry;/*  char **qry;*/
  int *db;
  int *type;
  int *case_sens;
  int *max_hits;
  int *hpm;
  int *match;
  char ***path;   /* an array of different paths */
  char **expath;
  int *path_rel;
  int *path_items;
  char *domains;
  char **surl;
  char **gurl;
  char **han;
  int *format;
  int *booltype;
  start_return_t *start_tbl;
  boolean_return_t *more_tbl;
{
  int x,j,array_sz, stronly=1;
  char *p;
  char *oldqry;
  domains[0]='\0';
  start_tbl->stop = -1;
  start_tbl->string = -1;
  start_tbl->site_stop = -1;
  start_tbl->site_file = -1;
  start_tbl->site_prnt = -1;
  *format = I_FORMAT_EXC;
  *hpm = MAX_HPM;
  *max_hits = MAX_HITS;
  *match = MAX_MATCH;
  *booltype = 0;
  oldqry = (char *)NULL;
  memset(&(*qry),0,sizeof(bool_query_t));  

  for(x=0; (x <= MAX_ENTRIES) && (entries[x].name != (char *)NULL) ;x++)
  {
    if(!strcmp( entries[x].name, FORM_QUERY ))
    {
      qry->string = (char *) malloc (sizeof(char)*(strlen(entries[x].val)+1));
      strcpy( qry->string, entries[x].val );
      if( parse_boolean_query( &(qry->string), &(*qry), &(*booltype) ) == ERROR){
        send_error_in_plain("The query sent is not a valid boolean query. Check the parameters.");
        return 0;
      }  /*  &(qry->words), &(qry->ops), &(qry->now) */
    }
    else if(!strcmp( entries[x].name, FORM_OLD_QUERY ))
    {
      oldqry = dequoteString(entries[x].val);
/*
 *  oldqry = (char *) malloc (sizeof(char)*(strlen(entries[x].val)+1));
 *  strcpy( oldqry, entries[x].val );
 */
    }
    else if(!strcmp( entries[x].name, FORM_STRINGS_ONLY ))
    {
      if( !strcmp(entries[x].val,FORM_STRINGS_YES ))
        stronly = SEARCH_STRINGS_ONLY;
    }
    else if(!strcmp( entries[x].name, FORM_MORE_SEARCH ))
    {
      if( !strcmp(entries[x].val,FORM_STRINGS_YES ))
        stronly = SEARCH_MORE_STRINGS;
    }
    else if(!strcmp( entries[x].name, FORM_STR_HANDLE ))
    {
      if( entries[x].val[0] != '\0' ){
        *han = (char *) malloc (sizeof(char)*(1+strlen(entries[x].val)));
        strcpy(*han,entries[x].val);
/*
 *  if (!archSetStateFromString(*strhan,han_str )){ fprintf(stdout,"Could not
 *    convert the state string into strhan."); }
 */
      }
    }
    else if(!strcmp( entries[x].name, FORM_SERV_URL ))
    {
      *surl = (char *)NULL;
      if( entries[x].val[0] != '\0' ){
        *surl = (char *) malloc (sizeof(char)*(1+strlen(entries[x].val)));
        strcpy( *surl, entries[x].val );
      }
    }
    else if(!strcmp( entries[x].name, FORM_GIF_URL ))
    {
      *gurl = (char *)NULL;
      if( entries[x].val[0] != '\0' ){
        *gurl = (char *) malloc (sizeof(char)*(1+strlen(entries[x].val)));
        strcpy( *gurl, entries[x].val );
      }
    }
    else if(!strcmp( entries[x].name, FORM_DB ))
    {
      if( !strcmp(entries[x].val,FORM_ANONFTP_DB ))
        *db = I_ANONFTP_DB;
      else if( !strcmp(entries[x].val,FORM_WEB_DB ))
        *db = I_WEBINDEX_DB;
    }
    else if(!strcmp( entries[x].name, FORM_FORMAT ))
    {
      if( !strcmp(entries[x].val,FORM_FORMAT_KEYS ))
        *format = I_FORMAT_KEYS;
      else if( !strcmp(entries[x].val,FORM_FORMAT_EXC ))
        *format = I_FORMAT_EXC;
      else if( !strcmp(entries[x].val,FORM_FORMAT_LINKS ))
        *format = I_FORMAT_LINKS;
      else if( !strcmp(entries[x].val,FORM_FORMAT_STRINGS_ONLY ))
        stronly = SEARCH_STRINGS_ONLY;
    }
    else if(!strcmp( entries[x].name, FORM_TYPE ))
    {
      if( !strcmp(entries[x].val,FORM_EXACT ))
      *type = EXACT;
      else if( !strcmp(entries[x].val,FORM_SUB ))
      *type = SUB;
      else if( !strcmp(entries[x].val, FORM_REGEX ))
      *type = REGEX;
      else
      *type = EXACT;
    }
    else if(!strcmp( entries[x].name, FORM_CASE ))
    {
      if( !strcmp(entries[x].val,FORM_CASE_SENS ))
      *case_sens = CMP_CASE_SENS;
      else
      *case_sens = 0;

      *case_sens |= CMP_ACCENT_SENS;
    }
    else if(!strcmp( entries[x].name, FORM_MAX_HITS ))
    {
      if( entries[x].val[0] != '\0' )
      *max_hits = atoi( entries[x].val );
      else
      *max_hits = MAX_HITS;
    }
    else if(!strcmp( entries[x].name, FORM_MAX_HPM ))
    {
      if( entries[x].val[0] != '\0' )
      *hpm = atoi( entries[x].val );
      else
      *hpm = MAX_HPM;
    }
    else if(!strcmp( entries[x].name, FORM_MAX_MATCH ))
    {
      if( entries[x].val[0] != '\0' )
      *match = atoi( entries[x].val );
      else
      *match = MAX_MATCH;
    }
    else if(!strcmp( entries[x].name, FORM_PATH_REL ))
    {
      if( !strcmp( entries[x].val, FORM_OR ) )
      {  *path_rel = PATH_OR; }
      else
      {  *path_rel = PATH_AND; }
    }
    else if(!strcmp( entries[x].name, FORM_EXCLUDE_PATH ))
    {
      *expath = (char *)NULL;
      if( entries[x].val[0] != '\0' ){
        *expath = (char *) malloc (sizeof(char)*(strlen(entries[x].val)+1));
        strcpy( *expath, entries[x].val );
      }
    }
    else if(!strcmp( entries[x].name, FORM_PATH ))
    {
      if( entries[x].val[0] != '\0' )
      {
        array_sz = count_toks(entries[x].val," ");
        if(array_sz){
          *path = (char **)malloc(sizeof(char *)*array_sz);

          for(j=0,p=strtok(entries[x].val," ") ; j<array_sz && p!=(char *)NULL ; p=strtok((char *)NULL," "),j++){
            (*path)[j] = (char *)malloc(sizeof(char)*(strlen(p)+1));
            strcpy((*path)[j],p);
          }
          *path_items = array_sz;        
        }
        else{
          *path = (char **)malloc(sizeof(char *)*1);
          (*path)[0] = (char *)malloc(sizeof(char)*(strlen(entries[x].val)+1));
          strcpy( (*path)[0], entries[x].val);
          *path_items = 1;
        }
      }
      else{
        *path = (char **)NULL;
        *path_items = 0;
      }
    }
    else if(!strcmp( entries[x].name, FORM_DOMAINS ))
    {
      if( entries[x].val[0] != '\0' )
      {
/*        *domains = (char *)malloc(sizeof(char)*(1+strlen(entries[x].val)));*/
        strcpy( domains, entries[x].val);
      }
      else
      domains[0] = (char)'\0';
    }
    else if(!strcmp( entries[x].name, "domain1" )||!strcmp( entries[x].name, "domain2" )||!strcmp( entries[x].name, "domain3" )||!strcmp( entries[x].name, "domain4" )||!strcmp( entries[x].name, "domain5" ))
    {
      if( entries[x].val[0] != '\0' )
      {
        if( domains[0] != '\0' )   sprintf( domains, "%s:%s", domains, entries[x].val);
        else      sprintf( domains, "%s", entries[x].val);
      }
    }      
    else if(!strcmp( entries[x].name, FORM_BOOLEAN_MORE_ENT ))
    {
      if( entries[x].val[0] != '\0' )
      parse_bool_more( entries[x].val, more_tbl );
      else
      more_tbl->and_tpl = -1;
    }
    else if(!strcmp( entries[x].name, FORM_START_STRING ))
    {
      if( entries[x].val[0] != '\0' )
      start_tbl->string = atoi( entries[x].val );
      else
      start_tbl->string = -1;
    }
    else if(!strcmp( entries[x].name, FORM_START_STOP ))
    {
      if( entries[x].val[0] != '\0' )
      start_tbl->stop = atoi( entries[x].val );
      else
      start_tbl->stop = -1;
    }
    else if(!strcmp( entries[x].name, FORM_START_SITE_STOP ))
    {
      if( entries[x].val[0] != '\0' )
      start_tbl->site_stop = atoi( entries[x].val );
      else
      start_tbl->site_stop = -1;
    }
    else if(!strcmp( entries[x].name, FORM_START_SITE_PRNT ))
    {
      if( entries[x].val[0] != '\0' )
      start_tbl->site_prnt = atoi( entries[x].val );
      else
      start_tbl->site_prnt = -1;
    }
    else if(!strcmp( entries[x].name, FORM_START_SITE_FILE ))
    {
      if( entries[x].val[0] != '\0' )
      start_tbl->site_file = atoi( entries[x].val );
      else
      start_tbl->site_file = -1;
    }
  }
  if( oldqry != (char *)NULL && qry->string != (char *)NULL ){
    if( strcmp( oldqry, qry->string ) ){
      stronly = 1;
    }
    free(oldqry);
  }
  return stronly;
}

extern int read_form_entries(entries)
  entry **entries;
{
  register int x;
  int cl;
  char *in;

  *entries = (entry *)malloc(sizeof(entry)*MAX_ENTRIES);
  if(strcmp(getenv("REQUEST_METHOD"),"GET")) {
    printf("This script should be referenced with a METHOD of GET.\n");
    printf("If you don't understand this, see this ");
    printf("<A HREF=\"http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/Docs/fill-out-forms/overview.html\">forms overview</A>.%c",10);
    return(0);
  }

  if(!(in = getenv("QUERY_STRING"))) {
    printf("No query information to decode.\n");
    return(0);
  }

  for(x=0;in[0] != '\0';x++) {
    (*entries)[x].val=(char *)malloc(sizeof(char)*MAX_STR);
    (*entries)[x].name= (char *)malloc(sizeof(char)*MAX_STR);
    getword((*entries)[x].val,in,'&');
    plustospace((*entries)[x].val);
    unescape_url((*entries)[x].val);
    getword((*entries)[x].name,(*entries)[x].val,'=');
  }

  cl = x;
  (*entries)[x++].val = (char *)NULL;
  (*entries)[x].name = (char *)NULL;

  return(1);

}


extern int read_form_entries_post(entries, request)
  entry **entries;
  char **request;
{
  register int x;
  int cl;

  *entries = (entry *)malloc(sizeof(entry)*MAX_ENTRIES);
  if(!(getenv("REQUEST_METHOD") && getenv("CONTENT_TYPE") && getenv("CONTENT_LENGTH")) ) {
    fprintf(stdout,"Content-type: text/html%c%c",10,10);
    fprintf(stdout,"<html><h2>Error</h2><HR>\n");
    fprintf(stdout,"This http server does not return one of the following environment variables: REQUEST_METHOD - CONTENT_TYPE - CONTENT_LENGTH.\n");
    fprintf(stdout,"If you don't understand this, see this ");
    fprintf(stdout,"<A HREF=\"http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/Docs/fill-out-forms/overview.html\">forms overview</A>.%c",10);
    return(0);
  }

  if(strcmp(getenv("REQUEST_METHOD"),"POST")) {
    fprintf(stdout,"Content-type: text/html%c%c",10,10);
    fprintf(stdout,"<html><h2>Error</h2><HR>\n");
    fprintf(stdout,"This script should be referenced with a METHOD of POST.\n");
    fprintf(stdout,"If you don't understand this, see this ");
    fprintf(stdout,"<A HREF=\"http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/Docs/fill-out-forms/overview.html\">forms overview</A>.%c",10);
    return(0);
  }

  if(strcmp(getenv("CONTENT_TYPE"),"application/x-www-form-urlencoded")) {
    fprintf(stdout,"Content-type: text/html%c%c",10,10);
    fprintf(stdout,"<html><h2>Error</h2><HR>\n");
    fprintf(stdout,"This script can only be used to decode form results. \n");
    return(0);
  }

  cl = atoi(getenv("CONTENT_LENGTH"));
  *request = (char *)malloc(sizeof(char)*cl);
  (*request)[0] = '\0';

  for(x=0;cl && (!feof(stdin));x++) {

    (*entries)[x].val = fmakeword(stdin,'&','#',&cl);
    if( (*request)[0]!='\0' )    *request = strcat(*request,"&");
    plustospace((*entries)[x].val);

    if( strstr((*entries)[x].val,FORM_STR_HANDLE) == (char *)NULL ){
      unescape_url((*entries)[x].val);
      *request = strcat(*request,(*entries)[x].val);
    }
        
    (*entries)[x].name = makeword((*entries)[x].val,'=');
  }

  (*entries)[x].val = (char *)NULL;
  (*entries)[x].name = (char *)NULL;

  return(1);

}

void send_hidden_values_in_plain( han_str, entries, start_r, more_r )
  char *han_str;
  entry *entries;
  start_return_t start_r;
  boolean_return_t more_r;
{
  int x;
  char *quoted_str, *bool_more;

  for(x=0; (x <= MAX_ENTRIES) && (entries[x].name != (char *)NULL) ;x++){
    if(!strcmp( entries[x].name, FORM_QUERY )){
      fprintf(stdout,"%s=%s\n", FORM_ORIG_QUERY, (entries[x].val));
      fprintf(stdout,"%s=%s\n", FORM_OLD_QUERY, quoteString(entries[x].val));
    }else if( strcmp( entries[x].name, FORM_STR_HANDLE )&&
             /*        strcmp( entries[x].name, FORM_TYPE )&&*/
             strcmp( entries[x].name, FORM_BOOLEAN_MORE_ENT )&&
             strcmp( entries[x].name, FORM_START_STRING )&&
             strcmp( entries[x].name, FORM_START_STOP )&&
             strcmp( entries[x].name, FORM_START_SITE_STOP )&&
             strcmp( entries[x].name, FORM_START_SITE_FILE )&&
             strcmp( entries[x].name, FORM_START_SITE_PRNT )&&
             strcmp( entries[x].name, FORM_MORE_SEARCH )&&
             strcmp( entries[x].name, FORM_ORIG_QUERY )&&
             strcmp( entries[x].name, FORM_OLD_QUERY ) ){
             /* strcmp( entries[x].name, FORM_CASE )*/
      fprintf(stdout,"%s=%s\n",entries[x].name,entries[x].val);
    }
  }
  if(han_str!=NULL){
    quoted_str = (char *) quoteString(han_str);
    fprintf(stdout,"%s=%s\n", FORM_STR_HANDLE, quoted_str);
  }

  bool_more = (char *) get_bool_more(more_r);
  fprintf(stdout,"%s=%s\n", FORM_MORE_SEARCH, FORM_STRINGS_YES);
  fprintf(stdout,"%s=%s\n", FORM_BOOLEAN_MORE_ENT, bool_more);
  if( start_r.stop >= 0 ){
    fprintf(stdout,"%s=%d\n", FORM_START_STOP, start_r.stop);
    fprintf(stdout,"%s=%li\n", FORM_START_STRING, start_r.string);
    fprintf(stdout,"%s=%li\n", FORM_START_SITE_STOP, start_r.site_stop);
    fprintf(stdout,"%s=%li\n", FORM_START_SITE_PRNT, start_r.site_prnt);
    fprintf(stdout,"%s=%li\n", FORM_START_SITE_FILE, start_r.site_file);
  }
  free(quoted_str);
  free(bool_more);
}

int send_result_in_plain(result, strs, hits, start_ret,
                         more_ret, db, strhan, entries, format )
  query_result_t *result;
  index_t **strs;
  int hits;
  start_return_t start_ret;
  boolean_return_t more_ret;
  int db;
  struct arch_stridx_handle *strhan;
  entry *entries;
  int format;
{
  int i,k;
  char url[MAX_STR];
  char *han_str, *site, *orig, *junk;
  int p=0;
  char *res;
  
  han_str=NULL;
  fprintf(stdout,"HITS=%d\n",hits);
  error(A_INFO,"RESULT:","Found %d Hits",hits);

  if(hits>0)
  if(!archGetStateString( strhan, &han_str)){
    error(A_ERR,prog,"Could not convert strhan to string using archGetStateString()\n");
    return(0);
  }

  /*
   *  To process possible continuation of the search using the strings stack,
   *      we need to pass the strhan information to the resultant page to be
   *      able to pass them back to the CGI program
   */

  send_hidden_values_in_plain( han_str, entries, start_ret, more_ret);

  free( han_str );
  if (db == I_ANONFTP_DB){
    strcpy(url,"ftp://");
  }else{
    strcpy(url,"http://");
  }
  for( i=0, k=0 ; i<hits ; i++ ){

    site = (char *)NULL;
    fprintf(stdout,"%s=%d\n",PLAIN_START,i);
    fprintf(stdout,"%s=%s%s\n",PLAIN_URL,url, result[i].str);
    fprintf(stdout,"%s=%s\n", PLAIN_STRING, result[i].qrystr);
    if(result[i].excerpt.title[0] != '\0'){
      fprintf(stdout,"%s=%s\n", PLAIN_TITLE, result[i].excerpt.title);
    }else{
      fprintf(stdout,"%s=%s%s\n", PLAIN_NO_TITLE, url, result[i].str);
    }
    if( (orig = strdup(result[i].str)) == (char *)NULL ){
      fprintf(stderr,"Error: Could not allocate memory.");
      return 0;
    }
    if( (site = strtok( orig,"/")) == (char *)NULL ){
      if( (site = strtok( orig,"#")) == (char *)NULL )
      site = result[i].str;
    }
    fprintf(stdout,"%s=%s\n", PLAIN_SITE, site);

    if (orig) {free(orig); orig = NULL;}
    if (CSE_IS_KEY(result[i].details) && (junk = strrchr( result[i].str, '#')) != (char *)NULL ){
      junk[0] = '\0';
    }else if ((junk = strrchr( result[i].str, '/')) != (char *)NULL ){
      junk[0] = '\0';
    }
    if ((junk = strchr( result[i].str, '/')) != (char *)NULL ){
      fprintf(stdout,"%s=%s\n", PLAIN_PATH,  junk);
    }else{
      fprintf(stdout,"%s=/\n", PLAIN_PATH);
    }
    if (!CSE_IS_KEY(result[i].details)){
      fprintf(stdout,"%s=%s\n",PLAIN_TYPE, PLAIN_FILE);
      if( result[i].type==NOT_INDX ){
        fprintf(stdout,"%s=%s\n",PLAIN_FTYPE, PLAIN_FTYPE_NOT_INDX);
      }else if( result[i].type==UNINDX ){
        fprintf(stdout,"%s=%s\n",PLAIN_FTYPE, PLAIN_FTYPE_UNINDX);
        if(result[i].details.type.file.size>0 &&
           result[i].details.type.file.date>0 ){
          fprintf(stdout,"%s=%ld\n", PLAIN_SIZE,
                  result[i].details.type.file.size);
          fprintf(stdout,"%s=%s\n", PLAIN_DATE,
                  (char *) cvt_to_usertime(result[i].details.type.file.date,0));
        }
      }else if( result[i].type==INDX ){
        fprintf(stdout,"%s=%s\n",PLAIN_FTYPE, PLAIN_FTYPE_INDX);
        fprintf(stdout,"%s=%ld\n", PLAIN_SIZE, result[i].details.type.file.size);
        fprintf(stdout,"%s=%s\n", PLAIN_DATE, (char *) cvt_to_usertime(result[i].details.type.file.date,0));
      }
    }else{
      fprintf(stdout,"%s=%s\n", PLAIN_FTYPE, PLAIN_FTYPE_INDX);
      fprintf(stdout,"%s=%s\n", PLAIN_TYPE, PLAIN_KEY);
/*
 *  fprintf(stdout,"%s=%f\n", PLAIN_WEIGHT,
 *  (double)result[i].details.type.kwrd.weight);
 */
      fprintf(stdout,"%s=%ld\n", PLAIN_SIZE, result[i].details.type.file.size);
      fprintf(stdout,"%s=%s\n", PLAIN_DATE, (char *) cvt_to_usertime(result[i].details.type.file.date,0));
    }
    if( result[i].str )  free(result[i].str);
    if( db == I_ANONFTP_DB ){
      fprintf(stdout,"%s=%s\n", PLAIN_PERMS, unix_perms_itoa(result[i].details.type.file.perms, CSE_IS_DIR(result[i].details), CSE_IS_LINK(result[i].details)));
    }else{
      if( (format == I_FORMAT_EXC)  &&  (result[i].excerpt.text[0]!='\0') ){
        fprintf(stdout,"%s=%s ...\n", PLAIN_TEXT, result[i].excerpt.text);
      }else if( (format == I_FORMAT_KEYS) ){
        p = 0;
        if( strs[k] != (index_t *)NULL){
          while(strs[k][p]!=(index_t)END_CHAIN){
            if (!archGetString( strhan, strs[k][p], &res )){
              error(A_ERR, prog, " String index %d out of bounds", strs[k][p]);
              return 0;
            }
            if (!p )  fprintf(stdout,"%s=%s ",PLAIN_TEXT,res);
            else fprintf(stdout,"- %s ",res);
            fflush(stdout);
            p++;
            free(res);
          }
          if(strs[k][0]!=(index_t)END_CHAIN)      fprintf( stdout, "\n");
        }
        k++;
      }
    }
    fprintf(stdout,"%s=%d\n", PLAIN_END,i);

  }
  return(1);
}

extern void free_list( list,sz )
  bool_list_t **list;
  int sz;
{
  int i,j;

  if ( *list == NULL )
    return;
  
  for ( i = 0; i<sz && (*list) != NULL ; i++ ) {
    for( j=0; (*list)[i].lwords!=NULL && j<(*list)[i].lnum; j++){
      if((*list)[i].lwords[j]) free((*list)[i].lwords[j]);
    }
    free((*list)[i].lwords);
  }

  free(*list);
  *list = NULL;
  
}

extern void free_entries( ents )
    entry **ents;
{
  int i;

  if ( *ents == NULL )
    return;
  
  for ( i = 0; (*ents)[i].name != NULL ; i++ ) {
    free((*ents)[i].name);
    free((*ents)[i].val);
  }

  free(*ents);
  *ents = NULL;
  
}


extern void free_result( res )
    query_result_t **res;
{
  int c;

  if ( *res == NULL )
    return;
  
  for ( c = 0; (*res)[c].str != NULL ; c++ ) {
    free((*res)[c].str);
    (*res)[c].str = NULL;
  }

  free(*res);
  *res = NULL;
  
}


extern void free_strings( strs, max )
  index_t ***strs;
  int max;
{
  int i;
  
  if ( *strs == NULL )
    return;
  
  for (i=0; (*strs)[i] != NULL && i<max; i++ ) {
    free((*strs)[i]);
    (*strs)[i] = NULL;
  }

  free(*strs);
  *strs = NULL;
}
