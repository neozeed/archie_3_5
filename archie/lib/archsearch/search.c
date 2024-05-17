/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <memory.h>
#include "protos.h"
#include "typedef.h"
#include "archie_dns.h"
#include "db_files.h"
#include "domain.h"
#include "header.h"
#include "db_ops.h"
#include "webindexdb_ops.h"
#include "host_db.h"
#include "error.h"
#include "files.h"
#include "archie_strings.h"
#include "archie_dbm.h"
#include "core_entry.h"
#include "debug.h"
#include "master.h"

#include "site_file.h"
#include "start_db.h"
#include "lang_startdb.h"
#include "patrie.h"
#include "archstridx.h"

#include "strings_index.h"
#include "get-info.h"
#include "search.h"


extern char *prog;

#ifdef __STDC__
#else
   extern status_t host_table_find();
   extern status_t update_start_dbs();
   extern status_t close_start_dbs();
   extern status_t open_start_dbs();
   extern status_t get_index_start_dbs();
   extern int archSearchSub();
   extern status_t get_port();
#endif



static void copyDocInfo( ent, no_exc, no_excerpts, exc_finfo, result)
full_site_entry_t *ent;
int no_exc;
int no_excerpts;
file_info_t *exc_finfo;
query_result_t *result;
{
  if(!no_exc && CSE_IS_DOC((*ent)) && (ent->core.entry.perms < no_excerpts) ){
    memcpy(&(result->excerpt),
           ((excerpt_t *) (exc_finfo->ptr) + ent->core.entry.perms),
           sizeof(excerpt_t));
  }else{
    result->excerpt.text[0] = '\0';
  }
  result->details.type.file.perms = ent -> core.entry.perms;
  result->details.type.file.size = ent -> core.entry.size;
  result->details.type.file.date = ent -> core.entry.date;
}

/*  
 *  Check the type of document that curr_count is at.  The only way we can
 *  figure out if it was indexed or not is through checking the next
 *  entry.  If it is a keyword then the document curr_count is at is
 *  indexed otherwise it is not indexed or unidexable.
 */    
static void setIndexability( curr_count, curr_finfo, result )
  int curr_count;
  file_info_t *curr_finfo;
  query_result_t *result;
{
  full_site_entry_t *tmp;
  if( curr_count < (curr_finfo->size/sizeof(full_site_entry_t)) ){
    tmp = (full_site_entry_t *)curr_finfo->ptr + curr_count + 1;
    if( CSE_IS_KEY((*tmp)) ){
      result->type = INDX;
    }else{
      if(result->details.type.file.size==0 &&
         result->details.type.file.date==0 ){
        result->type = NOT_INDX;
      }else{
        result->type = UNINDX;
      }
    }
  }
}

/*
 *  if path_items is negative then **path may or may not contain path
 *    restriction strings and if it does the number is unknown. Otherwise
 *    path_items contain the number of strings in **path
 */
extern int check_correct_path( string, path, expath, path_rel, path_items )
char *string;
char **path;
char *expath;
int path_rel;
int path_items;
{
  int i,r;
  r = !path_rel;
  if( expath!=(char *)NULL && strstr( string, expath ) )   return 0;
  if( path_items ){
    for( i=0; i<path_items ; i++){
      /*      fprintf(stdout,"path[%d]=%s\n",i,path[i]);
       */
      if( path[i]==(char *)NULL || path[i][0]=='\0' ) return 1;
      if( strstr( string, path[i] )){
        if(path_rel) return 1;
        else r&=1;
      }else{
        if(path_rel) r|=0;
        else return 0;
      }
    }
    return r;
  }else if (path_items < 0){
    for( i=0; ( path[i]!=(char *)NULL && path[i][0]!='\0' ) ; i++){
      /*      fprintf(stdout,"path[%d]=%s\n",i,path[i]);
       */
      if( strstr( string, path[i] )){
        if(path_rel) return 1;
        else r&=1;
      }else{
        if(path_rel) r|=0;
        else return 0;
      }
    }
    return r;
  }else  return 1;
}


static int reg_table[] = {
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,1,0,0,0,1,1,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,1,1,1,0,0
};



int reg2sci( src, dest )
char src[];
char dest[];
{
  char *s = src;
  char *d = dest;

  while ( *s != '\0' ) {

    if ( *s == '[' ) {
      char a, b;

      if ( *(s+1) == '\0' || *(s+2) == '\0' || *(s+3) == '\0' ) 
      return 0;

      if ( *(s+3) != ']' ) 
      return 0;

      a = *(s+1);
      b = *(s+2);

      if ( ! isalpha(a) || ! isalpha(b) || tolower(a) != tolower(b) )
      return 0;

      *d = tolower(a);
      d++;
      s += 4;
      continue;
    }

    if ( *s == '\\' ) {
      s++;
      if ( *s == '\0' ) {
        return 0;
      }
	 
      *d = *s;
      d++;
      s++;
      continue;
    }

    if ( reg_table[*s] ) {
      return 0;
    }

    *d = *s;
    s++; 
    d++;
  }

  *d = '\0';
  return 1;
}


extern status_t strOnlyQuery( strhan, qry, type, case_sens, h, format )
struct arch_stridx_handle *strhan;
char *qry;
int type;                     /* type of search */
int case_sens;                /* case_sens */
int h;                        /* maximum hits (all in all) */
int format;
{
  int j;
  char *res;
  unsigned long hits=0;
  index_t *start;

  start = (index_t *)NULL;

  if ( type == REGEX ) {
    char output[100];
    if ( reg2sci(qry, output) ) {
      type = SUB;
      case_sens = 0;
      strcpy(qry,output);
    }
  }

  
  switch( type ){

  case EXACT:

    start = (index_t *)malloc(sizeof(index_t)*h);
    if( !archSearchExact( strhan, qry, case_sens, h, &hits, start) ){
      error(A_ERR, "strOnlyQuery","Could not perform exact search successfully.");
    }
    if( hits <= 0 ){
      error(A_INFO,"strOnlyQuery","String %s was not found in the database.",qry);
    }
    break;

  case SUB:

    start = (index_t *)malloc(sizeof(index_t)*h);
    if( !archSearchSub( strhan, qry, case_sens, h, &hits,  start) ){
      error(A_ERR, "strOnlyQuery","Could not perform Sub-String search successfully.");
    }
    if( hits <= 0 ){
      error(A_INFO,"strOnlyQuery","String %s was not found in the database.",qry);
    }
    break;

  case REGEX:

    hits = 0;
    start = (index_t *)malloc(sizeof(index_t)*h);
    if( !archSearchRegex( strhan, qry, case_sens, h, &hits,  start) ){
      error(A_ERR, "strOnlyQuery","Could not perform Reg-Ex search successfully.");
    }
    if( hits <= 0 ){
      error(A_INFO,"strOnlyQuery","String %s was not found in the database.",qry);
    }

    break;
    
  default:
    error(A_ERR,"strOnlyQuery", "Did not supply any search type for query");
    break;
  }

  if(format){
    fprintf(stdout,"Content-type: text/html\n\n");
    fprintf(stdout,"<HTML><h2>Archie Results</h2>\n");
    if(!hits){
      fprintf(stdout,"<P><H3>No hits for your query. Please check your query's paramters and try again</H3>");
      return(A_OK);
    }

    fprintf(stdout,"<h4>Found %ld Strings</h4><B>\n",hits);
    error(A_INFO,"RESULT:","Found %d Hits",hits);
  }else{
    fprintf(stdout,"HITS=%ld\n",hits);
    fprintf(stdout,"%s=0\n", PLAIN_START_STRINGS);
    error(A_INFO,"RESULT:","Found %d Hits",hits);
  }
  for(j=0 ; (j<hits) && (j<h) ; j++ ){
    if( !archGetString( strhan,  start[j], &res) ){
      error(A_ERR,"strOnlyQuery","could not get string from start");
      continue;
    }
    if(format){
      fprintf(stdout,"<i>(%d)</i>%s<P>\n",j+1,res);
    }else{
      fprintf(stdout,"%d=%s\n",j+1,res);
    }
    free(res); res = NULL;
  }
  if(format)  fprintf(stdout,"</B></HTML>");
  else   fprintf(stdout,"%s=0\n",PLAIN_END_STRINGS);

  free(start);
  return(A_OK);
}


/* More results for the previous query. */

extern status_t archQueryMore(strhan, h, m, hpm, path, expath, path_rel,
                              path_items, d_list, d_cnt, db, start_db, verbose,
                              result, strings, tot_hits, site_record, func_name,
                              no_sites_limit, format)
struct arch_stridx_handle *strhan;
int h;                        /* maximum hits (all in all) */
int m;                        /* maximum match (different unique strings) */
int hpm;                      /* maximum hits per match */
char **path;
char *expath;
int path_rel;
int path_items;
domain_t *d_list;
int d_cnt;
int db;
file_info_t *start_db;
int verbose;           /* Was hostbyaddr */
query_result_t **result;
index_t ***strings;
int *tot_hits;
start_return_t *site_record;
char *(*func_name)();
int no_sites_limit;
int format;
{
  /* File information */

  host_table_index_t inds[65536], hin;
  int indslen,j,i,l;
  unsigned long hits=0;
  hostname_t hnm;
  ip_addr_t ipaddr = (ip_addr_t)(0);
  query_result_t *res;
  char *strres;
  int at_least_in_one_site, max, flag;
  int num_hits, num_matches, num_hits_pm, tmp_stop=0;
  
  int port, site_hits;
  
  index_t *start, **strs;

  i = 0;
  hnm[0] = '\0';
  port = 0;
  num_hits = 0;
  num_matches = 0;
  num_hits_pm = 0;
  *tot_hits = 0;
  if(h>MAX_HITS || h<=0)    h= MAX_HITS;
  if(m>MAX_MATCH || m<=0)    m= MAX_MATCH;
  if(m>h) m=h;
  if(hpm<=0) hpm=1;

  *result = (query_result_t *)malloc(sizeof(query_result_t)*h);
  *strings = (index_t **)malloc(sizeof(index_t *)*h);
  if ( *strings == NULL || *result == NULL ) {
    error(A_ERR,"archQueryMore","Unable to allocate memory");
    return ERROR;
  }
  memset(*strings,0, sizeof(index_t *)*h);
  memset(*result,0, sizeof(query_result_t)*h);
  
  site_hits = 0;
  j=0;l=0;

  if(site_record->string>0  &&  site_record->stop>=0){

    get_index_start_dbs(start_db, site_record->string, inds, &indslen);

    if( indslen <= 0 ){
      if( !archGetString( strhan, site_record->string, &strres) ){
        if( verbose ){
          error(A_INFO,"archQueryMore","StrIndex %s is not in the strings database.",site_record->string);
        }
      }else{
        if( verbose ){
          error(A_INFO,"archQueryMore","String %s is not in any site in the database.",strres);
        }
      }
      free(strres);
      goto others;
    }
    /*
     *  l++;
     */
    at_least_in_one_site = 0;
/*     num_hits_pm = 0;*/
    
    for( i=site_record->stop ; (i<indslen) ; i++ ){
      site_hits = 0;
      ipaddr = (ip_addr_t)(0);
      port = 0;
      hin = inds[i];
      if( func_name != 0 ){
        if((i>(no_sites_limit-1)) && (i%no_sites_limit==0))
        func_name();
      }
      if( host_table_find( &ipaddr, hnm, &port, &hin)==ERROR ){
        error(A_ERR, "archQueryMore","Could not find host in host-table. start/host dbase corrupt.");
        goto others;
      }

      if( hnm[0]!='\0'  &&  !find_in_domains( hnm, d_list, d_cnt) ){
        error(A_INFO,"archQueryMore","Site did not match domain restriction, Ignoring.");
        goto others;
      }

      res = &(*result)[num_hits];
      strs = &((*strings)[num_hits]);
      max = (hpm-num_hits_pm < h-num_hits) ? hpm-num_hits_pm : h-num_hits;

      if( format == I_FORMAT_KEYS && db != I_ANONFTP_DB ){
        if( getStringsFromStart( site_record->string, strhan, site_record, db, (ip_addr_t)ipaddr, port, &site_hits,
                                max, path, expath, path_rel, path_items, res, ((*strings)+num_hits) )==ERROR ){
          error(A_ERR,"archQueryMore","Could not reconstruct the string from site file. site dbase corrupt.");
          goto others;
        }
      }else{
        if( getResultFromStart( site_record->string, strhan, site_record, db, (ip_addr_t)ipaddr, port, &site_hits,
                               max, path, expath, path_rel, path_items, res )==ERROR ){
          error(A_ERR,"archQueryMore","Could not reconstruct the string from site file. site dbase corrupt.");
          goto others;
        }
      }

      if( site_hits>0 ){
        at_least_in_one_site = 1;
        num_hits_pm += site_hits; /*Increase the numbers of hits for this match */
        num_hits += site_hits;  /* Increase the total number of hits */
      }

      if(num_hits>=h || num_matches>=m){
        site_record->stop = i;
        *tot_hits = num_hits;
        /*        *site_record = curr_site_record; */
        return(A_OK);
      }
    }

    if ( at_least_in_one_site ) 
       num_matches++; /* increase the match hit */      

  }

 others:
  *tot_hits = num_hits;
  
  flag = 1;  /* will let us know when maxmatch or maxhits was reached */
  start = (index_t *)malloc(sizeof(index_t)*m);
  while(flag){

    if(!archGetMoreMatches( strhan, m, &hits, start)){
      error(A_ERR, "archQueryMore","Could not perform search using archGetMoreStarts().");
      break;
    }
    if( m>0 && hits<=0 ){
      error(A_INFO,"archQueryMore","No more hits in the database.");
      break;
    }

    for(j=0 ; (j<hits) && (flag) ; j++ ){
      get_index_start_dbs(start_db, start[j], inds, &indslen);

      if( indslen <= 0 ){
        if( !archGetString( strhan, start[j], &strres) ){
          if( verbose ){
            error(A_INFO,"archQueryMore","StrIndex %s is not in the strings database.",start[j]);
          }
        }else{
          if( verbose ){
            error(A_INFO,"archQueryMore","String %s is not in any site in the database.",strres);
          }
        }
        free(strres);
        continue;
      }

      at_least_in_one_site = 0;
      tmp_stop = -1;
      for( i=0 ; flag && (i<indslen) ; i++ ){  /*  process each site which this string occurs in */
        site_hits = 0;
        ipaddr = (ip_addr_t)(0);
        port = 0;
        hin = inds[i];
        if( func_name != 0 ){
          if((i>(no_sites_limit-1)) && (i%no_sites_limit==0))
          func_name();
        }
        if( host_table_find( &ipaddr, hnm, &port, &hin)==ERROR ){
          error(A_ERR,"archQueryMore","Could not find host in host-table. start/host dbase corrupt.");
          continue;
        }

        /* fprintf(stderr,"\n   Site:%s\n",inet_ntoa( ipaddr_to_inet(ipaddr)));*/

        if( hnm[0]!='\0'  &&  !find_in_domains( hnm, d_list, d_cnt) ){
          continue;
        }

        /*      site_hits = hpm; */
        res = &(*result)[num_hits];
        strs = &((*strings)[num_hits]);
        memset(&(*site_record),'\0',sizeof(start_return_t));
/*        max = (hpm-num_hits_pm < h-num_hits) ? hpm-num_hits_pm : h-num_hits;*/
        max= h-num_hits;

        if( format == I_FORMAT_KEYS && db != I_ANONFTP_DB ){
          if( getStringsFromStart( start[j], strhan, site_record, db, (ip_addr_t)ipaddr, port, &site_hits, max, path, expath, path_rel, path_items, res, ((*strings)+num_hits) )==ERROR ){
            error(A_ERR,"archQueryMore","Could not reconstruct the string from site file. site dbase corrupt.");
            continue;
          }
        }else{
          if( getResultFromStart( start[j], strhan, site_record, db, (ip_addr_t)ipaddr, port, &site_hits, max, path, expath, path_rel, path_items, res )==ERROR ){
            error(A_ERR,"archQueryMore","Could not reconstruct the string from site file. site dbase corrupt.");
            continue;
          }
        }

        if( site_hits>0 ){
          at_least_in_one_site = 1; /* assuming that the start_db returned correct results!! */
          num_hits_pm += site_hits; /*Increase the numbers of hits for this match */
          num_hits += site_hits; /* Increase the total number of hits */
        }
        
        if ( num_hits >= h  || num_hits_pm >= hpm ){
          tmp_stop = i;
          i = indslen+1; /* that will stop the loop. we shouldn't play with flag */
        }
      
      }  /* Go through sites for one string */

      if ( at_least_in_one_site ) 
        num_matches++; /* increase the match hit */      

        
      if ( num_hits >= h )
        flag = 0;
      
    } /* Go through uniqe strings */
    
    if ( num_hits >= h /* || num_matches >= m */)
      flag = 0;
    
  }

  if( !flag ){
    if( i < indslen )   site_record->stop = i-1;
    else if( tmp_stop >= 0 )   site_record->stop = tmp_stop;
    else site_record->stop = -1;
    site_record->string = start[j-1];
  }else{
    site_record->stop = -1;
  }
  free(start);
  *tot_hits = num_hits;
  return(A_OK);
}

/* archQuery: takes the search parameters:
 *
 *   qry: string being searched
 *   type: type of search.. so far EXACT - SUB
 *   case_sens: case sensetivity if type is not exact of course
 *   h: maximum hits
 *   m: maximum string match (different strings when type of search is not exact)
 *   hpm: hits per match (how many hits for each distinct string)
 *   path: path restriction (if path[0]=='\0' then no path restriction applied
 *   d_list: domain list already compiled through compile_domains() function
 *   d_cnt: number of domains in domain list returned through the same function as above
 *   start_db: finfo
 *   result: a pointer to an array of results.
 *
 *  PS: reason why I did not pass the domain_db as a paramter instead of d_list and
 *      d_cnt is that compile_domains() needs to be called only once and not to every
 *      call to archQuery.
 */

extern status_t archQuery( strhan, qry, type, case_sens, h, m, hpm,
                          path, expath, path_rel, path_items, d_list,
                          d_cnt, db, start_db, verbose, result, strings,
                          tot_hits, site_record, func_name, no_sites_limit,
                          format)
struct arch_stridx_handle *strhan;
char *qry;
int type;                     /* type of search */
int case_sens;                /* case_sens */
int h;                        /* maximum hits (all in all) */
int m;                        /* maximum match (different unique strings) */
int hpm;                      /* maximum hits per match */
char **path;
char *expath;
int path_rel;
int path_items;
domain_t *d_list;
int d_cnt;
int db;
file_info_t *start_db;
int verbose;
query_result_t **result;
index_t ***strings;
int *tot_hits;
start_return_t *site_record;
char *(*func_name)();
int no_sites_limit;
int format;
{
  /* File information */

  host_table_index_t inds[65536], hin;
  int indslen,j,i,k;
  unsigned long num_found_str=0;
  hostname_t hnm;
  ip_addr_t ipaddr = (ip_addr_t)(0);
  query_result_t *res;
  index_t **strs;
  char *strres;
  int flag;
  int port, site_hits;
  int at_least_in_one_site;

  int num_hits, num_matches, num_hits_pm, tmp_stop=0;
  
  index_t *start;
  hnm[0] = '\0';
  port = 0;

  j=i=k=0;
  *tot_hits = 0;
  if(h>MAX_HITS || h<=0)    h= MAX_HITS;
  if(m>MAX_MATCH || m<=0)    m= MAX_MATCH;
  if(m>h) m=h;
  if(hpm<=0) hpm=1;
  /*
   *  If Maximum Strings Matched is higher than max hits then we might lose
   *  some strings in the following search (More Results). When we call
   *  archSearch<*> with Maxmatch as a parameter, it will return more strings
   *  than maxhits and when we call archGetMore the following call will miss
   *  all the extra strings from the previous search.  I will have to look for
   *  a better solution.
   */

  *result = (query_result_t *)malloc(sizeof(query_result_t)*h);
  *strings = (index_t **)malloc(sizeof(index_t *)*h);
  if ( *strings == NULL || *result == NULL ) {
    error(A_ERR,"archQuery","Unable to allocate memory");
    return ERROR;
  }
  memset(*strings,0, sizeof(index_t *)*h);
  memset(*result,0, sizeof(query_result_t)*h);


  num_hits = 0;
  num_matches = 0;

  flag = 1;
  start = (index_t *)malloc(sizeof(index_t)*m);

  if ( type == REGEX ) {
    char output[100];
    if ( reg2sci(qry, output) ) {
      type = SUB;
      case_sens = 0;
      strcpy(qry,output);      
    }
  }
  
  while ( flag ) {

    if ( flag > 1 ) {
      if(!archGetMoreMatches( strhan, m-num_matches, &num_found_str, start)){
        error(A_ERR, "archQuery",
              "Could not perform search using archGetMoreStarts().");
        break;
      }
    }
    else {
      switch( type ){

      case EXACT:
        if( !archSearchExact(strhan, qry, case_sens, m-num_matches,
                             &num_found_str, start) ){
          error(A_ERR, "archQuery",
                "Could not perform exact search successfully.");
          free(start);
          return(A_OK);
        }
        if( num_found_str <= 0 ){
          if( verbose ){
            error(A_INFO, "archQuery",
                  "String %s was not found in the database.",qry);
          }
          free(start);
          return(A_OK);
        }
        break;

      case SUB:

        if( !archSearchSub( strhan, qry, case_sens, m-num_matches, &num_found_str,  start) ){
          error(A_ERR, "archQuery", "Could not perform Sub-String search successfully.");
          free(start);
          return(A_OK);
        }
        if( num_found_str <= 0 ){
          if( verbose ){
            error(A_INFO, "archQuery",
                  "String %s was not found in the database.",qry);
          }
          free(start);
          return(A_OK);
        }
        break;

      case REGEX:

        *tot_hits = 0;
        num_found_str = 0;
        if( !archSearchRegex(strhan, qry, case_sens, m-num_matches,
                             &num_found_str,  start) ){
          error(A_ERR, "archQuery", "Could not perform Reg-Ex search successfully.");
          free(start);
          return(A_OK);
        }
        if( num_found_str <= 0 ){
          if( verbose ){
            error(A_INFO, "archQuery",
                  "String %s was not found in the database.",qry);
          }
          free(start);
          return(A_OK);
        }
        break;

      default:
        error(A_ERR,"archQuery", "Did not supply any search type for query.");
        free(start);
        return(A_OK);
        break;
      }
    }    
    flag++;

    if ( num_found_str == 0 ) {
      break;
    }
    
    for ( j = 0; flag && j < num_found_str; j++)  {
      /* Go through every found string */

      get_index_start_dbs(start_db, start[j], inds, &indslen);

      if( indslen <= 0 ){
        if( !archGetString( strhan,  start[j], &strres) ){
          if( verbose ){
            error(A_INFO,"archQuery",
                  "StrIndex %ld is not in the strings database.",start[j]);
          }
        }else{
          if( verbose ){
            error(A_INFO,"archQuery",
                  "String %s is not in any site in the database.",strres);
          }
        }
        free(strres);
        continue;
      }

      /* For every found string .. go through each site file */

      at_least_in_one_site = 0;
      num_hits_pm = 0;
      tmp_stop = -1;
      for ( k = 0; k < indslen ; k++ ) {
        int max;
        ipaddr = (ip_addr_t)(0);
        port = 0;
        hin = inds[k];

        if( host_table_find( &ipaddr, hnm, &port, &hin) == ERROR ) {
          error(A_ERR,"archQuery",
                "Could not find host in host-table. start/host dbase corrupt.");
          continue;
        }

        /* This is for the stupid dirsrv program ... need to call the function that
           actually checks for incoming connections ... */
        
        if( func_name != 0 ){
          if((i>(no_sites_limit-1)) && (i%no_sites_limit==0))
          func_name();
        }

        if( hnm[0]!='\0'  &&  !find_in_domains( hnm, d_list, d_cnt) ){
          continue;
        }
        
        res = &(*result)[num_hits];
        strs = &((*strings)[num_hits]);
        memset(&(*site_record),'\0',sizeof(start_return_t));
        site_record->stop = -1;
/*        max = (hpm-num_hits_pm < h-num_hits) ? hpm-num_hits_pm : h-num_hits; */
        max=h-num_hits;
        
        if( format == I_FORMAT_KEYS && db != I_ANONFTP_DB){
          if( getStringsFromStart( start[j], strhan, site_record, db,
                                  (ip_addr_t)ipaddr, port, &site_hits, max,
                                  path, expath, path_rel, path_items, res,
                                  strs )==ERROR ){
            error(A_ERR,"archQuery",
                  "Could not reconstruct the string from site file. site dbase corrupt.");
            continue;
          }
        }else{
          if( getResultFromStart( start[j], strhan, site_record, db,
                                 (ip_addr_t)ipaddr, port, &site_hits, max,
                                 path, expath, path_rel, path_items, res )==ERROR ){
            error(A_ERR,"archQuery",
                  "Could not reconstruct the string from site file. site dbase corrupt.");
            continue;
          }
        }

        if( site_hits > 0 ){
          at_least_in_one_site = 1;
          num_hits_pm += site_hits; /*Increase the numbers of hits for this match */
          num_hits += site_hits; /* Increase the total number of hits */
        }
        
        if ( num_hits >= h  || num_hits_pm >= hpm ){
          tmp_stop = k;
          k = indslen+1;          /* that will stop the loop */
        }
      }  /* Go through sites for one string */

      if ( at_least_in_one_site ) 
        num_matches++; /* increase the match hit */      

        
      if ( num_hits >= h )
        flag = 0;
      
    } /* Go through uniqe strings */
    
    if ( num_hits >= h /* || num_matches >= m */)
      flag = 0;
    
  }

  *tot_hits = num_hits;
  if( k < indslen )   site_record->stop = k-1;
  else if( tmp_stop >= 0 )   site_record->stop = tmp_stop;
  else site_record->stop = -1;
  site_record->string = start[j-1];
  free(start);
  return(A_OK);

}



extern status_t handle_chain(curr_finfo, strhan, curr_prnt, curr_strings,
                             web, ip, port, curr_count, result )
  file_info_t *curr_finfo;
  struct arch_stridx_handle *strhan;
  index_t curr_prnt, curr_strings;
  int web;
  ip_addr_t ip;
  int port;
  int curr_count;
  query_result_t *result;  
{
  char *res, *tmpptr, *totptr, *flipptr;
  full_site_entry_t *prnt_ent, *curr_ent, *old_prnt_ent, *ent, *tmp;
  int key = 0;
  int document=0;
  int doc = 0;
  int max_length = MAX_PATH_LEN;

  old_prnt_ent = ent = tmp = curr_ent = prnt_ent = (full_site_entry_t *)NULL;  
  if( web ) {
    curr_ent = (full_site_entry_t *) curr_finfo -> ptr + curr_prnt;
    if ( result->details.flags & 0x08 )
         key=1;
    if (!(CSE_IS_SUBPATH((*curr_ent)) ||
          CSE_IS_PORT((*curr_ent)) || CSE_IS_NAME((*curr_ent)) || CSE_IS_KEY((*curr_ent))) )
    {
      doc=1;
      document = 1;
    }
  }
  /*
   *  tmpstr = totstr = NULL;
   */
  /*
   *  The reason I am allocating is to be able to realloc in case this length
   *    was not enough.
   */
  tmpptr = (char *)malloc(sizeof(char)*max_length);
  totptr = (char *)malloc(sizeof(char)*max_length);
  memset(totptr,'\0',max_length);
  memset(tmpptr,'\0',max_length);

  flipptr = NULL;

  while(( curr_prnt >= (index_t)(0) )&&( curr_strings >= (index_t)(0) )){
    
    if( !archGetString( strhan,  curr_strings, &res) ) {
      
      error(A_ERR, "handle_chain", "Site %s (%s) record %d has string index %d out of bounds",
            wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port), curr_count, curr_strings );
      return(ERROR);
    }
    else {
      prnt_ent = (full_site_entry_t *) curr_finfo -> ptr + curr_prnt;
      
      if( tmpptr[0] == '\0' ) {
        if ( result->qrystr[0] == '\0' )
          strcpy(result->qrystr, res);
        
        if(web && CSE_IS_SUBPATH((*prnt_ent)) && (prnt_ent->strt_1<0)){
          sprintf(tmpptr,"%s:%d",res,port);
        }else{
          strcpy(tmpptr,res);
        }
        flipptr = tmpptr;
        tmpptr = totptr;
        if( tmpptr[0]=='\0' ) tmpptr[0] = '9'; /* Applies only to the first time*/
        totptr = flipptr;
      }
      else{
        
        if( strlen(totptr)+strlen(res)+2 >= max_length){
          char *t1,*t2;
          max_length += MAX_PATH_LEN;
          t1 = (char *)realloc( tmpptr, sizeof(char)*max_length );
          t2 = (char *)realloc( totptr, sizeof(char)*max_length );

          if ( t1 == NULL || t2 == NULL ) {
            error(A_ERR, "handle_chain","Unable to reallocate memory");
            close_file(curr_finfo);
            destroy_finfo(curr_finfo);
            free(totptr);
            free(tmpptr);
            return(ERROR);
          }
          totptr = t1;
          tmpptr = t2;
        }

        /*
         *  old_prnt_ent points to the main entry corresponding to
         *  what ent point to. ie: if ent points to a record
         *  representing a directory then old_prnt_ent should be
         *  pointing to its main_entry as a sub_path! .. comprendo?!
         *  the following check is the only way I can tell if ent is a
         *  site-name.
         */

        if( (old_prnt_ent) && CSE_IS_NAME((*old_prnt_ent)) && web )
        sprintf(tmpptr,"%s:%s",res,totptr);

        /*
         *  Here I make a check on the first record since the path
         *  construction started. If it is a key then I must verify
         *  that we are still at the start of the path build.  Since
         *  old_prnt_ent is NULL at the beginning of the path
         *  construction, I check it here before I decide that this
         *  key is the last in the path
         */

        else
          if ( key && !document ) {
            strcpy(tmpptr,res);
            key=0;
          }else if((prnt_ent) && (old_prnt_ent == NULL) &&
                   CSE_IS_SUBPATH((*prnt_ent)) && (prnt_ent->strt_1<0) ) {
            sprintf(tmpptr,"%s:%s",res,totptr);
          }else
            sprintf(tmpptr,"%s/%s",res,totptr);
        
        flipptr = tmpptr;
        tmpptr = totptr;
        if( tmpptr[0]=='\0' ) tmpptr[0] = '9';
        totptr = flipptr;
      }


      free( res ); res = NULL;
      if( !(CSE_IS_SUBPATH((*prnt_ent)) ||
            CSE_IS_PORT((*prnt_ent)) || CSE_IS_NAME((*prnt_ent)) ) && !doc){
        
        error(A_ERR, "handle_chain","Corruption in site file %s",
              wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port) );
        close_file(curr_finfo);
        destroy_finfo(curr_finfo);
        free(totptr);
        free(tmpptr);
        return(ERROR);
      }
      if( doc ){
        for ( ; curr_prnt >= 0; curr_prnt-- ){
          tmp = (full_site_entry_t *)curr_finfo->ptr + curr_prnt;
          if ( CSE_IS_SUBPATH((*tmp)) || CSE_IS_NAME((*tmp)) || CSE_IS_PORT((*tmp)) )
          break;
        }
        prnt_ent = (full_site_entry_t *) curr_finfo -> ptr + curr_prnt;
        doc=0;
        old_prnt_ent = (full_site_entry_t *)NULL;
      }else{
        old_prnt_ent = prnt_ent;
      }
      curr_prnt = prnt_ent->strt_1;
      if ( curr_prnt >= (index_t)(0) ){
        ent = (full_site_entry_t *) curr_finfo -> ptr + prnt_ent->core.prnt_entry.strt_2;
        curr_strings = ent->strt_1;
      }
    }
  }
  result->str = totptr;
  if( tmpptr ) free(tmpptr);
  return A_OK;
}

extern void handle_partial_entry( exc_finfo, curr_ent, no_excerpts, result )
  file_info_t *exc_finfo;
  full_site_entry_t *curr_ent;
  int no_excerpts;
  query_result_t *result;
{

  /*
   *  fills some fields concerning a document in the result record
   */

  result->excerpt.text[0] = '\0';
  result->excerpt.title[0] = '\0';          
  
  if((exc_finfo) && CSE_IS_DOC((*curr_ent)) &&
     (curr_ent->core.entry.perms < no_excerpts) ){
    memcpy(&(result->excerpt),
           ((excerpt_t *) (exc_finfo->ptr) + curr_ent->core.entry.perms),
           sizeof(excerpt_t));
  }else{
    result->excerpt.text[0] = '\0';
  }

}

extern void handle_entry( exc_finfo, curr_ent, web, no_excerpts, result )
  file_info_t *exc_finfo;
  full_site_entry_t *curr_ent;
  int web, no_excerpts;
  query_result_t *result;
{

  result->details.flags = curr_ent -> flags;
  result->excerpt.text[0] = '\0';
  result->excerpt.title[0] = '\0';          
  result->qrystr[0] = '\0';

  if( CSE_IS_KEY((*curr_ent)) ){
    result->type = INDX;
    result->details.type.kwrd.weight = curr_ent -> core.kwrd.weight;
  }
  else{
    copyDocInfo(curr_ent, !(web && (no_excerpts>0)), no_excerpts,
                exc_finfo,  result);
  }
}

extern void handle_key_prnt_entry( exc_finfo, curr_ent, web, no_excerpts, result )
  file_info_t *exc_finfo;
  full_site_entry_t *curr_ent;
  int web, no_excerpts;
  query_result_t *result;
{
  result->details.type.file.perms = curr_ent -> core.entry.perms;
  result->details.type.file.size = curr_ent -> core.entry.size;
  result->details.type.file.date = curr_ent -> core.entry.date;
    
  if(web && (exc_finfo) && CSE_IS_DOC((*curr_ent)) &&
     (curr_ent->core.entry.perms < no_excerpts) ){
    memcpy(&(result->excerpt),
           ((excerpt_t *)(exc_finfo->ptr)+curr_ent->core.entry.perms),
           sizeof(excerpt_t));
  }else{
    result->excerpt.text[0] = '\0';
  }
}


/*
 *  getResultFromStart: Given,
 *  
 *  start: Start (string index returned by archSearch<>)
 *  
 *  strhan: A file handler for the strings files.
 *  
 *  db: I_ANONFTP_DB=0 or I_WEBINDEX_DB=1
 *  
 *  ip: ip address of the site file.
 *  
 *  port: port of the site.
 *  
 *  hits: max hits all in all.
 *  
 *  path: array of strings corresponding to the different path restrictions.
 *  
 *  expath: One string that must determine which paths to exclude from the
 *  result.
 *  
 *  path_rel: for the array of path strings.. this determines what type of
 *  relation to be peformed on them OR,AND
 *  
 *  path_items: how many strings in path array.
 *  
 *  Returns,
 *  
 *  result: an array of returned paths constructed from the site file. The
 *  array ends with NULL and this is how you know when to stop.
 */


extern status_t getResultFromStart( start, strhan, site_record, db, ip, port, hits, max_hits, path, expath, path_rel, path_items, result )
index_t start;
struct arch_stridx_handle *strhan;
start_return_t *site_record;
int db;
ip_addr_t ip;
int port;
int *hits;
int max_hits;
char **path;
char *expath;
int path_rel;
int path_items;
query_result_t *result;
{
  /* File information */
  char *res;

  int act_size = 0;
  int cnt = 0;
  int web = 0;
  index_t curr_strings;
  index_t curr_count=0;
  index_t cp, cf;
  index_t curr_file=0;
  index_t curr_prnt=0;
  index_t st = 0;

  full_site_entry_t *curr_ent, *ent;
  full_site_entry_t *curr_endptr;
  full_site_entry_t *old_prnt_ent;
  char exc_file_name[MAX_PATH_LEN];
  char curr_file_name[MAX_PATH_LEN];
  int no_exc = 1;
  int no_excerpts = -1;
  int key=0;
  int recno, i, *rec_list = NULL;
  int retval = A_OK;

  file_info_t *index_file = create_finfo();
  file_info_t *curr_finfo = create_finfo();
  file_info_t *exc_finfo = create_finfo();

  res = (char *)(0);
  *hits = cnt;

  /*
   *  The reason I am allocating is to be able to realloc in case this length
   *    was not enough. 
   */

  if(db == I_WEBINDEX_DB){
    sprintf(curr_file_name,"%s",(char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port));
    curr_file_name[strlen(curr_file_name)]='\0';
    sprintf(exc_file_name,"%s%s",(char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port),EXCERPT_SUFFIX);
    exc_file_name[strlen(exc_file_name)]='\0';
    web = 1;    
    if(access(exc_file_name, F_OK) < 0){
      /* "File %s does not exist in the database. No need to look for starts in it" */
      no_exc = 1;
    }else{
      strcpy(exc_finfo -> filename, exc_file_name);
      no_exc = 0;
      /* Open current file */
      if(open_file(exc_finfo, O_RDONLY) == ERROR){
        /* "Ignoring %s" */
        /* error(A_ERR, "getResultFromStart", "Ignoring %s", exc_finfo->filename);*/
        no_exc = 1;
      }
      if( mmap_file(exc_finfo, O_RDONLY) == ERROR){
        /* "Ignoring %s" */
        error(A_ERR, "getResultFromStart", "Ignoring %s", exc_finfo ->filename);
        no_exc = 1;
      }
      if( !no_exc ) no_excerpts = exc_finfo->size/sizeof(excerpt_t);
    }
  }else if(db == I_ANONFTP_DB){
    sprintf(curr_file_name,"%s",(char *)files_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port));
    curr_file_name[strlen(curr_file_name)]='\0';
    exc_file_name[0]='\0';
  }
   
  if(access(curr_file_name, F_OK) < 0){
    /* "File %s does not exist in the database. No need to look for starts in it" */
    error(A_INFO, "getResultFromStart", "File %s does not exist in the database. No need to look for starts in it.",
          curr_file_name);
    retval = A_OK;
    goto end1;
  }else{
    strcpy(curr_finfo -> filename, curr_file_name);
    /* Open current file */
    if(open_file(curr_finfo, O_RDONLY) == ERROR){
      /* "Ignoring %s" */
      error(A_ERR, "getResultFromStart", "Ignoring %s", curr_finfo->filename);
      retval = A_OK;
      goto end1;
    }
  }

  /* mmap it */
  if(mmap_file(curr_finfo, O_RDONLY) == ERROR){
    /* "Ignoring %s" */
    error(A_ERR, "getResultFromStart", "Ignoring %s", curr_finfo ->filename);
    retval = A_OK;
    goto end1;
  }
  act_size = curr_finfo -> size / sizeof(full_site_entry_t);

  sprintf(index_file->filename,"%s%s",curr_finfo->filename,SITE_INDEX_SUFFIX);

  if( site_record->stop>=0 && site_record->site_stop >= 0  &&  site_record->site_stop < act_size ){
    if( site_record->site_file >= 0  &&  site_record->site_file < act_size )   curr_file = site_record->site_file;
    if( site_record->site_prnt >= 0  &&  site_record->site_prnt < act_size )   curr_prnt = site_record->site_prnt;
    st = site_record->site_stop;
  }else{
    st = 0;
  }

  if(access(index_file->filename, F_OK) < 0){
    destroy_finfo(index_file);
    index_file = NULL;
    /* nothing right now . No errors. */
  }else if ( open_file(index_file, O_RDONLY) == A_OK ) {

    /* mmap it */
    if(mmap_file(index_file, O_RDONLY) == ERROR){
      /* "Ignoring %s" */
      error(A_ERR, "getResultFromStart", "Ignoring %s", index_file ->filename);
      retval = A_OK;
      goto end1;
    }
    
    if ( search_site_index(index_file, start, &rec_list, &recno) == ERROR ) {
      error(A_ERR,"getResultFromStart","search_site_index failed.");
      close_file(index_file);
      destroy_finfo(index_file);
      index_file = NULL;
      free(rec_list);
      goto traditional;
    }

    for ( i = 0; i < recno; i++ ) {
      full_site_entry_t *tmp;

      curr_count =  rec_list[i];
      if( st > 0 && curr_count < st ) continue;
      curr_ent = (full_site_entry_t *) curr_finfo -> ptr + curr_count;
      if (web && !(CSE_IS_DOC((*curr_ent)) || CSE_IS_FILE((*curr_ent))
            || CSE_IS_KEY((*curr_ent)))){
        continue;
      }
      curr_strings = curr_ent -> strt_1;
      handle_entry(exc_finfo, curr_ent, web, no_excerpts, &result[cnt]);
      result[cnt].qrystr[0]='\0';
      /*
       *  here was the block 0
       */
      if( !archGetString( strhan,  curr_strings, &res) )
      {                         /* "Site %s (%s) record %d has string index %d out of bounds" */
        error(A_ERR, "getResultFromStart", "Site %s (%s) record %d has string index %d out of bounds",
              wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port), curr_count, curr_strings );
        retval = ERROR;
        goto end1;
      }else{
        strcpy(result[cnt].qrystr, res);
        free(res); res = NULL;
      }
      /*
       *  block 0 starts
       */
      if ( web && CSE_IS_KEY((*curr_ent)) ) {
        result[cnt].type = INDX;
        for ( curr_prnt = curr_count; curr_prnt >= 0; curr_prnt-- ){
          tmp = (full_site_entry_t *)curr_finfo->ptr + curr_prnt;
          if ( !CSE_IS_KEY((*tmp)) )
          break;
        }
        curr_ent = (full_site_entry_t *) curr_finfo -> ptr + curr_prnt;
        curr_file = curr_prnt;
        /*
         *  used to be handle_partial_entry() But changed. Why you ask?  There
         *            is no real use of the weight field, therefore we
         *            overwrite it with the information of the document
         *            containing the keyword.
         */
        handle_key_prnt_entry(exc_finfo, curr_ent, web, no_excerpts, &result[cnt]);
        curr_strings = curr_ent -> strt_1;
      }
      else {
        /*
         *  get the type of the document first
         */
        setIndexability( curr_count, curr_finfo, &(result[cnt]) );

        curr_file = curr_count;
        for ( curr_prnt = curr_count; curr_prnt >= 0; curr_prnt-- ){
          tmp = (full_site_entry_t *)curr_finfo->ptr + curr_prnt;
          if ( CSE_IS_SUBPATH((*tmp)) || CSE_IS_NAME((*tmp)) || CSE_IS_PORT((*tmp)) )
          break;
        }
      }
      /*
       *  block 0 ends
       */
      if ( handle_chain(curr_finfo, strhan, curr_prnt, curr_strings,web,
                        ip, port, curr_count, &(result[cnt])) == ERROR ) {
        
        retval = ERROR;
        goto end1;
      }
          
      if( check_correct_path(result[cnt].str, path, expath, path_rel, path_items) ){
        result[cnt].ipaddr = ip;
        cnt++;
      }

      if( cnt >= max_hits){
        break;
      }
    }
        
    *hits = cnt;
    if( site_record->site_stop == curr_count )   site_record->stop = -1;
    else   site_record->site_stop = curr_count;
    site_record->site_file = curr_file;
    site_record->site_prnt = curr_prnt;
  end1:
    close_file( index_file );
    close_file( curr_finfo );
    close_file( exc_finfo );
    destroy_finfo( index_file );
    destroy_finfo( curr_finfo );
    destroy_finfo( exc_finfo );
    if( rec_list != NULL ) free( rec_list );
    return retval;
  }

traditional:
  
  for(curr_ent = (full_site_entry_t *) curr_finfo -> ptr + st ,
      curr_endptr = (full_site_entry_t *) curr_finfo -> ptr + act_size,
      curr_count = st;
      curr_ent < curr_endptr;
      (full_site_entry_t *)curr_ent++, curr_count++){
     
    curr_strings = curr_ent -> strt_1;

    /* Check if string offset is valid */
    if(curr_strings >= (index_t)(0)){

      /*
       *  figure out the different parents.
       */
      
      if ( CSE_IS_SUBPATH((*curr_ent)) ||
          CSE_IS_PORT((*curr_ent)) ||
          CSE_IS_NAME((*curr_ent)) ){
        curr_prnt = curr_count;
      }else if(!web || CSE_IS_DOC((*curr_ent)) || CSE_IS_FILE((*curr_ent)) ||
               CSE_IS_KEY((*curr_ent))){

        /*
         *  only check the starts of entries other than main entries
         */

        if( !CSE_IS_KEY((*curr_ent)) ){
          curr_file = curr_count;
        }

        /*
         *  memorize the current parents before going into the loop for
         *  rebuilding the path
         */


        if( curr_strings == start ){
          cf = curr_file;
          cp = curr_prnt;

          handle_entry(exc_finfo, curr_ent, web, no_excerpts, &(result[cnt]));
          
          key = 0;
          old_prnt_ent = (full_site_entry_t *)NULL;

          /*
           *  only when curr_file is greater than curr_prnt is the parent a
           *  file and not a dir. The current entry must be a keyword 
           *  otherwise we only consider the directory as a parent
           */
          if(CSE_IS_KEY((*curr_ent))){
            result[cnt].type = INDX;
            if(web && ( curr_file > curr_prnt) ) {
              if( curr_file >= 0 ){
                ent = (full_site_entry_t *) curr_finfo -> ptr + curr_file;
                curr_strings = ent->strt_1;
                copyDocInfo( ent, no_exc, no_excerpts,
                             exc_finfo, &(result[cnt]) );
              }
              /*
               *  it seems that curr_prnt must be curr_file
               */
              curr_prnt = curr_file;
            }
          }else{
            /*
             *  get the type of the document first
             */
            setIndexability( curr_count, curr_finfo, &(result[cnt]) );
          }
          if ( handle_chain(curr_finfo, strhan, curr_prnt, curr_strings,web,
                           ip, port, curr_count, &(result[cnt])) == ERROR ) {
            retval = ERROR;
            goto end2;
          }
          
          if( check_correct_path(result[cnt].str, path, expath, path_rel, path_items) ){

            if( archGetString( strhan, start, &res) ){
              strcpy(result[cnt].qrystr, res);
              free(res); res = NULL;
            }else{
              error(A_ERR,"search","Could not get the string from strings file");
              retval = ERROR;
              goto end2;
            }

            result[cnt].ipaddr = ip;
            cnt++;
          }
          curr_file = cf;
          curr_prnt = cp;
        }
      }
      if( cnt >= max_hits ){
        break;
      }
    }
  }
  *hits = cnt;
  if(site_record->site_stop == curr_count)   site_record->stop = -1;
  else   site_record->site_stop = curr_count;
  site_record->site_file = curr_file;
  site_record->site_prnt = curr_prnt;
 end2:
  close_file( curr_finfo );
  close_file( exc_finfo );
  destroy_finfo( curr_finfo );
  if( exc_finfo )  destroy_finfo( exc_finfo );
  if( rec_list != NULL ) free( rec_list );
  return retval;
}



extern status_t getStringsFromStart(start, strhan, site_record, db, ip,
                                    port, hits, max_hits, path, expath,
                                    path_rel, path_items, result, strings )
index_t start;
struct arch_stridx_handle *strhan;
start_return_t *site_record;
int db;
ip_addr_t ip;
int port;
int *hits;
int max_hits;
char **path;
char *expath;
int path_rel;
int path_items;
query_result_t *result;
index_t **strings;
{
  /* File information */
  char *res, *tmpptr, *totptr, *flipptr;

  int l, act_size = 0;
  int cnt = 0;
  int web = 0;
  index_t curr_strings;
  index_t curr_count=0;
  index_t cp, cf;
  index_t curr_file=0;
  index_t curr_prnt=0;
  index_t st=0;

  full_site_entry_t *curr_ent, *ent;
  full_site_entry_t *curr_endptr, *ptr;
  full_site_entry_t *prnt_ent,*old_prnt_ent;
  char exc_file_name[MAX_PATH_LEN];
  char curr_file_name[MAX_PATH_LEN];
  int no_exc = 1;
  int no_excerpts = -1;  
  int key=0;
  int p=0;
  int recno, i, *rec_list = NULL;
  int retval = A_OK;
  int max_length = MAX_PATH_LEN;

  file_info_t *index_file = create_finfo();
  file_info_t *curr_finfo = create_finfo();
  file_info_t *exc_finfo = create_finfo();

  res = (char *)(0);
  *hits = cnt;

  /*
   *  The reason I am allocating is to be able to realloc in case this length
   *    was not enough. 
   */

  if(db == I_WEBINDEX_DB){
    sprintf(curr_file_name,"%s",(char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port));
    curr_file_name[strlen(curr_file_name)]='\0';
    web = 1;    
    sprintf(exc_file_name,"%s%s",(char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port),EXCERPT_SUFFIX);
    exc_file_name[strlen(exc_file_name)]='\0';
    if(access(exc_file_name, F_OK) >= 0){
      strcpy(exc_finfo -> filename, exc_file_name);
      no_exc = 0;
      /* Open current file */
      if(open_file(exc_finfo, O_RDONLY) == ERROR){
        /* "Ignoring %s" */
        error(A_ERR, "getStringsFromSTart", "Can't open %s. Ignoring.", exc_finfo->filename);
        no_exc = 1;
      }else{
        if( mmap_file(exc_finfo, O_RDONLY) == ERROR){
          /* "Ignoring %s" */
          error(A_ERR, "getStringsFromSTart", "Can't mmap %s. Ignoring.", exc_finfo ->filename);
          no_exc = 1;
        }
        no_excerpts = exc_finfo->size/sizeof(excerpt_t);
      }
    }
  }else if(db == I_ANONFTP_DB){
    error(A_INFO,"getStringsFromStart","Looking for keywords in anonftp database. No keywords in anonftp database!");
    retval = A_OK;
    goto end1;
  }
   
  if(access(curr_file_name, F_OK) < 0){
    /* "File %s does not exist in the database. No need to look for starts in it" */
    error(A_INFO,"getStringsFromStart","File %s does not exist in the database. No need to look for starts in it.",
          curr_file_name);
    retval = A_OK;
    goto end1;
  }else{
    strcpy(curr_finfo -> filename, curr_file_name);
    /* Open current file */
    if(open_file(curr_finfo, O_RDONLY) == ERROR){
      /* "Ignoring %s" */
      error(A_ERR, "getStringsFromStart", "Ignoring %s", curr_finfo->filename);
      retval = A_OK;
      goto end1;
    }
  }

  /* mmap it */
  if(mmap_file(curr_finfo, O_RDONLY) == ERROR){
    /* "Ignoring %s" */
    error(A_ERR, "getStringsFromStart", "Ignoring %s", curr_finfo ->filename);
    retval = A_OK;
    goto end1;
  }

  act_size = curr_finfo -> size / sizeof(full_site_entry_t);

  sprintf(index_file->filename,"%s%s",curr_finfo->filename,SITE_INDEX_SUFFIX);

  if( site_record->stop >=0 && site_record->site_stop >= 0  &&  site_record->site_stop < act_size ){
    if( site_record->site_file >= 0  &&  site_record->site_file < act_size )   curr_file = site_record->site_file;
    if( site_record->site_prnt >= 0  &&  site_record->site_prnt < act_size )   curr_prnt = site_record->site_prnt;
    st = site_record->site_stop;
  }else{
    st = 0;
  }

  if(access(index_file->filename, F_OK) < 0){
    destroy_finfo(index_file);
    index_file = NULL;
    /* nothing right now . No errors. */
  }else if ( open_file(index_file, O_RDONLY) == A_OK ) {

    /* mmap it */
    if(mmap_file(index_file, O_RDONLY) == ERROR){
      /* "Ignoring %s" */
      error(A_ERR, "getStringFromStart", "Ignoring %s", index_file ->filename);
      retval = A_OK;
      goto end1;
    }
    
    if ( search_site_index(index_file, start, &rec_list, &recno) == ERROR ) {
      error(A_ERR,"getStringsFromStart","Could not search site index. search_site_index failed for %s!", index_file ->filename);
      close_file(index_file);
      destroy_finfo(index_file);
      index_file = NULL;
      free(rec_list);
      goto traditional;
    }

    for ( i = 0; i < recno; i++ ) {
      full_site_entry_t *tmp;

      curr_count =  rec_list[i];
      if( st > 0 && curr_count < st ) continue;
      curr_ent = (full_site_entry_t *) curr_finfo -> ptr + rec_list[i];
      if (CSE_IS_SUBPATH((*curr_ent)) || CSE_IS_NAME((*curr_ent)) ||
          CSE_IS_PORT((*curr_ent)) ){
        error(A_ERR,"getStringsFromStart","search_site_index() returned an ivalid pointer to a subpath in file-index %s",index_file ->filename);
        continue;
      }
      if (web && !(CSE_IS_DOC((*curr_ent)) || CSE_IS_FILE((*curr_ent))
            || CSE_IS_KEY((*curr_ent)))){
        continue;
      }
      curr_strings = curr_ent -> strt_1;        
      handle_entry((file_info_t *)exc_finfo, curr_ent, web, no_excerpts,
                   &result[cnt]);

      if ( CSE_IS_KEY((*curr_ent)) ) {
        result[cnt].type = INDX;
        for ( curr_prnt = rec_list[i]; curr_prnt >= 0; curr_prnt-- ){
          tmp = (full_site_entry_t *)curr_finfo->ptr + curr_prnt;
          if ( !CSE_IS_KEY((*tmp)) )
          break;
        }
        curr_ent = (full_site_entry_t *) curr_finfo -> ptr + curr_prnt;
        curr_file = curr_prnt;
        /*
         *  used to be handle_partial_entry() But changed. Why you ask?  There
         *            is no real use of the weight field, therefore we
         *            overwrite it with the information of the document
         *            containing the keyword.
         */
        handle_key_prnt_entry(exc_finfo, curr_ent, web, no_excerpts, &result[cnt]);
        curr_file = curr_prnt;
      }
      else {
        setIndexability( curr_count, curr_finfo, &(result[cnt]) );

        curr_file = curr_count;
        for ( curr_prnt = curr_count; curr_prnt >= 0; curr_prnt-- ){
          tmp = (full_site_entry_t *)curr_finfo->ptr + curr_prnt;
          if ( CSE_IS_SUBPATH((*tmp)) || CSE_IS_NAME((*tmp)) || CSE_IS_PORT((*tmp)) )
          break;
        }
      }

      /*changed*/
      result[cnt].qrystr[0]='\0';
      if( !archGetString( strhan,  curr_strings, &res) )
      { /* "Site %s (%s) record %d has string index %d out of bounds" */
        error(A_ERR, "getStringsFromStart", "Site %s (%s) record %d has string index %d out of bounds",
              wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port), curr_count, curr_strings );
        retval = ERROR;
        goto end1;
      }else{
        strcpy(result[cnt].qrystr, res);
        free(res); res = NULL;
      }
      if( curr_file >= 0 ){
        p=0;
        ent = ptr = (full_site_entry_t *) curr_finfo -> ptr + curr_file + 1;
        while( CSE_IS_KEY((*ptr)) ){
          ptr++;
          p++;
        }
        if( ((*(strings+cnt)) = (index_t *)malloc(sizeof(index_t)*(p+1))) == (index_t *)NULL){
          error(A_ERR,"getStringsFromStart","Can't allocate memory\n");
          retval = ERROR;
          goto end1;
        }
        (*(strings+cnt))[0] = (index_t)END_CHAIN;
        p=0;
        while( CSE_IS_KEY((*ent)) ){
          (*(strings+cnt))[p] = ent->strt_1;
          ent++; p++;
        }
        (*(strings+cnt))[p] = (index_t)END_CHAIN;

        /*
         *  I must get the strings before I start sorting.
         *  
         *  qsort((char **)&(strings[cnt]),p,sizeof(char *), strcmp);
         */

        ent = (full_site_entry_t *) curr_finfo -> ptr + curr_file;
        curr_strings = ent->strt_1;
        /*
         *  copy over the excerpt information as well as other document
         *  informationhere
         */
        copyDocInfo( ent, no_exc, no_excerpts, exc_finfo, &(result[cnt]) );
      }
      /*changed*/

      if ( handle_chain(curr_finfo, strhan, curr_prnt, curr_strings, web,
                        ip, port, curr_count, &(result[cnt])) == ERROR ) {
        error(A_ERR,"getStringsFromStart","Could not handle chain for file %s",curr_finfo->filename);
        retval = ERROR;
        goto end1;
      }

      if( check_correct_path(result[cnt].str, path, expath, path_rel, path_items) ){
        result[cnt].ipaddr = ip;
        cnt++;
      }

      if( cnt >= max_hits){
        break;
      }
    }

    *hits = cnt;        
    if( site_record->site_stop == curr_count )   site_record->stop = -1;
    else   site_record->site_stop = curr_count;
    site_record->site_file = curr_file;
    site_record->site_prnt = curr_prnt;
  end1:
    close_file( index_file );
    close_file( curr_finfo );
    close_file( exc_finfo );
    destroy_finfo( index_file );
    destroy_finfo( curr_finfo );
    destroy_finfo( exc_finfo );
    if( rec_list != NULL ) free( rec_list );
    return retval;

  }

 traditional:

  tmpptr = (char *)malloc(sizeof(char)*max_length);
  totptr = (char *)malloc(sizeof(char)*max_length);
  memset(totptr,'\0',max_length);
  memset(tmpptr,'\0',max_length);
  tmpptr[0] = totptr[0] = '\0';
  flipptr = NULL;

  if(site_record->stop >=0 && site_record->site_stop >= 0  &&
     site_record->site_stop < act_size ){

    if(site_record->site_file >= 0  &&
       site_record->site_file < act_size )   curr_file = site_record->site_file;
    if(site_record->site_prnt >= 0  &&
       site_record->site_prnt < act_size )   curr_prnt = site_record->site_prnt;
    st = site_record->site_stop;

  }else{
    st = 0;
  }

  for(curr_ent = (full_site_entry_t *) curr_finfo -> ptr + st ,
      curr_endptr = (full_site_entry_t *) curr_finfo -> ptr + act_size,
      curr_count = st;
      curr_ent < curr_endptr;
      (full_site_entry_t *)curr_ent++, curr_count++){
     
    curr_strings = curr_ent -> strt_1;

    /* Check if string offset is valid */
    if(curr_strings >= (index_t)(0)){

      /*
       *  figure out the different parents.
       */
      
      if ( CSE_IS_SUBPATH((*curr_ent)) ||
          CSE_IS_PORT((*curr_ent)) ||
          CSE_IS_NAME((*curr_ent)) ){
        curr_prnt = curr_count;
      }else if(!web || CSE_IS_DOC((*curr_ent)) || CSE_IS_FILE((*curr_ent)) ||
               CSE_IS_KEY((*curr_ent))){

        /*
         *  only check the starts of entries other than main entries
         */

        if( !CSE_IS_KEY((*curr_ent)) ){
          curr_file = curr_count;
        }

        /*
         *  memorize the current parents before going into the loop for
         *  rebuilding the path
         */

        cf = curr_file;
        cp = curr_prnt;
        tmpptr[0] = '\0';
        l = 0;

        if( curr_strings == start ){

          /*
           *  old_prnt_ent used for format reasons of the output. In the case
           *  of ports and site names, they should be seperated by a colon
           */

          old_prnt_ent = (full_site_entry_t *)NULL;
          key = 0;
          result[cnt].details.flags = curr_ent -> flags;
          result[cnt].excerpt.text[0] = '\0';
          result[cnt].qrystr[0] = '\0';

          if( CSE_IS_KEY((*curr_ent)) ){
            result[cnt].type = INDX;
            result[cnt].details.type.kwrd.weight = curr_ent -> core.kwrd.weight;
          }
          else{
            full_site_entry_t *tmp;
            copyDocInfo( curr_ent, no_exc, no_excerpts,
                        exc_finfo, &(result[cnt]) );
            /*
             *  get the type of the document
             */
            setIndexability( curr_count, curr_finfo, &(result[cnt]));
          }

          /*
           *  only when curr_file is greater than curr_prnt is the parent a
           *  file and not a dir. The current entry must be a keyword otherwise we
           *  only consider the directory as a parent
           */

          *(strings+cnt) = (index_t *)NULL;

          if(( curr_file > curr_prnt) && (CSE_IS_KEY((*curr_ent))) ) {
            key=1;
            if( !archGetString( strhan,  curr_strings, &res) )
            {                   /* "Site %s (%s) record %d has string index %d out of bounds" */
              error(A_ERR, "getStringsFromStart", "Site %s (%s) record %d has string index %d out of bounds",
                    wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port), curr_count, curr_strings );
              retval = ERROR;
              goto end2;
            }else{
              strcpy(result[cnt].qrystr, res);
              tmpptr[0] = '9';
              free(res); res = NULL;
            }
            if( curr_file >= 0 ){
              p=0;
              ent = ptr = (full_site_entry_t *) curr_finfo -> ptr + curr_file + 1;
              while( CSE_IS_KEY((*ptr)) ){
                ptr++;
                p++;
              }
              if( ((*(strings+cnt)) = (index_t *)malloc(sizeof(index_t)*(p+1))) == (index_t *)NULL){
                error(A_ERR,"getStringsFromStart","Can't allocate memory\n");
                retval = ERROR;
                goto end2;
              }
              (*(strings+cnt))[0] = (index_t)END_CHAIN;
              p=0;
              while( CSE_IS_KEY((*ent)) ){
                (*(strings+cnt))[p] = ent->strt_1;
                ent++; p++;
              }
              (*(strings+cnt))[p] = (index_t)END_CHAIN;

              /*
               *  Must provide strings and not indecies.
               *  
               *    qsort((char **)&(strings[cnt]),p,sizeof(char *), strcmp);
               */
              
              ent = (full_site_entry_t *) curr_finfo -> ptr + curr_file;
              curr_strings = ent->strt_1;
              /* copyDocInfo() */
              copyDocInfo( ent, no_exc, no_excerpts, exc_finfo,
                          &(result[cnt]) );
            }
          }

          while(( curr_prnt >= (index_t)(0) )&&( curr_strings >= (index_t)(0) )){
            if( !archGetString( strhan,  curr_strings, &res) )
            { /* "Site %s (%s) record %d has string index %d out of bounds" */
              error(A_ERR, "getStringsFromStart", "Site %s (%s) record %d has string index %d out of bounds",
                    wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port), curr_count, curr_strings );
              retval = ERROR;
              goto end2;
            }else{
              prnt_ent = (full_site_entry_t *) curr_finfo -> ptr + curr_prnt;
              if( tmpptr[0] == '\0' ) {
                if( CSE_IS_KEY((*curr_ent)) )  key=1;
                else   strcpy(result[cnt].qrystr, res);
                if(CSE_IS_SUBPATH((*prnt_ent)) && (prnt_ent->strt_1<0)){
                  sprintf(tmpptr,"%s:%d",res,port);
                }else{
                  strcpy(tmpptr,res);
                }
                flipptr = tmpptr;
                tmpptr = totptr;
                if( tmpptr[0]=='\0' ) tmpptr[0] = '9';
                totptr = flipptr;
              }
              else{

                if( strlen(totptr)+strlen(res)+2 >= max_length){
                  char *t1,*t2;
                  max_length += MAX_PATH_LEN;
                  t1 = (char *)realloc( tmpptr, sizeof(char)*max_length );
                  t2 = (char *)realloc( totptr, sizeof(char)*max_length );

                  if ( t1 == NULL || t2 == NULL ) {
                    error(A_ERR, "getStringsFromStart","Unable to reallocate memory");
                    retval = ERROR;
                    goto end2;
                  }
                  totptr = t1;
                  tmpptr = t2;
                }
                /*
                 *  old_prnt_ent points to the main entry corresponding to
                 *  what ent point to. ie: if ent points to a record
                 *  representing a directory then old_prnt_ent should be
                 *  pointing to its main_entry as a sub_path! .. comprendo?!
                 *  the following check is the only way I can tell if ent is a
                 *  site-name.
                 */

                if( (old_prnt_ent) && CSE_IS_NAME((*old_prnt_ent)) )
                sprintf(tmpptr,"%s:%s",res,totptr);

                /*
                 *  Here I make a check on the first record since the path
                 *  construction started. If it is a key then I must verify
                 *  that we are still at the start of the path build.  Since
                 *  old_prnt_ent is NULL at the beginning of the path
                 *  construction, I check it here before I decide that this
                 *  key is the last in the path
                 */

                else if (CSE_IS_KEY((*curr_ent)) && (key)){
                  strcpy(tmpptr,res);
                  key=0;
                }
                else
                  sprintf(tmpptr,"%s/%s",res,totptr);

                flipptr = tmpptr;
                tmpptr = totptr;
                if( tmpptr[0]=='\0' ) tmpptr[0] = '9';
                totptr = flipptr;
              }
              l = strlen(totptr);

              free( res ); res = NULL;
              if( !(CSE_IS_SUBPATH((*prnt_ent)) ||
                    CSE_IS_PORT((*prnt_ent)) ||
                    CSE_IS_NAME((*prnt_ent)) ) ){
                error(A_ERR,"getStringsFromStart","Corruption in site file %s",
                      wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port) );
                
                retval = ERROR;
                free(totptr);
                free(tmpptr);
                goto end2;
              }
              old_prnt_ent = prnt_ent;
              curr_prnt = prnt_ent->strt_1;
              if ( curr_prnt >= (index_t)(0) ){
                ent = (full_site_entry_t *) curr_finfo -> ptr + prnt_ent->core.prnt_entry.strt_2;
                curr_strings = ent->strt_1;
              }
            }
          }
          if( check_correct_path(totptr, path, expath, path_rel, path_items) ){
            result[cnt].str = totptr;
            result[cnt].ipaddr = ip;
            totptr = (char *)malloc(sizeof(char)*MAX_PATH_LEN);
            cnt++;
          }
          tmpptr[0] = '\0';
          curr_file = cf;
          curr_prnt = cp;
        }
      }
      if( cnt >= max_hits){
        break;
      }
    }
  }
  *hits = cnt;
  if(site_record->site_stop == curr_count)  site_record->site_stop = -1;
  else site_record->site_stop = curr_count;
  site_record->site_file = curr_file;
  site_record->site_prnt = curr_prnt;
 end2:
  close_file( curr_finfo );
  close_file( exc_finfo );
  destroy_finfo( index_file );
  destroy_finfo( curr_finfo );
  destroy_finfo( exc_finfo );
  if( rec_list != NULL ) free( rec_list );
  if( tmpptr != NULL ) free( tmpptr );
  if( totptr != NULL ) free( totptr );  
  if( res != NULL ) free( res );
  return retval;
}
