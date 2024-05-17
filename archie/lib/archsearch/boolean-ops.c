/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <memory.h>
#include "protos.h"
#include "archie_dns.h"
#include "db_files.h"
#include "domain.h"
#include "db_ops.h"
#include "error.h"
#include "debug.h"
#include "master.h"
#include "site_file.h"
#include "start_db.h"
#include "lang_startdb.h"
#include "get-info.h"
#include "boolean-ops.h"


extern char *prog;

static int indexcompare(i,j)
  index_t *i, *j;
{
  return((int)(*i - *j));
}

static int siteindexcompare(i,j)
  host_table_index_t *i, *j;
{
  return((int)(*i - *j));
}

static int tuplecompare(i,j)
  site_tuple_t *i, *j;
{
  int a;
  return((int)(a=(i->file - j->file)?
               (i->file - j->file):(i->child - j->child)));
}

static int parentcompare(i,j)
  site_tuple_t *i, *j;
{
  return((int)(i->file - j->file));
}

extern void tuplecpy( a, b )
  site_tuple_t *a,*b;
{
  a->site = b->site;
  a->file = b->file;
  a->child  = b->child;
}

extern void getLongestAndWord( at, i )
  bool_list_t at;
  int *i;
{
  int k,l,max;
  *i = -1;
  l = max = 0;
  for(k=0; k<at.lnum; k++){
    l = strlen(at.lwords[k]);
    if(max<l) { max = l; *i = k; }
  }
  return;
}

extern void unique_tuple( tuple, sz )
  site_tuple_t **tuple;
  int *sz;
{
  int i, bef, aft, rep, cmp;
  if( *sz <= 0 ) return;
  for( i=0, bef=0, aft=1, rep=0; aft<*sz ; i++ ){
    cmp =  tuplecompare(&((*tuple)[bef]),&((*tuple)[aft]));
    if( (*tuple)[bef].site != (*tuple)[aft].site ){
      bef++;
      aft++;
    }else if( cmp==0 ){
      aft++;
      rep = 1;
    }else if( cmp<0 && rep==1){
      rep = 0;
      bef++;
      tuplecpy( &((*tuple)[bef]) , &((*tuple)[aft]));
    }else{
      bef++;
      aft++;
    }
  }
  if(*sz!=0)  *sz = bef+1;
}

extern void unique( list, sz )
  index_t **list;
  int *sz;
{
  int i, bef, aft, rep;
  for( i=0, bef=0, aft=1, rep=0; aft<*sz ; i++ ){
    if( (*list)[bef] == (*list)[aft] ){
      aft++;
      rep = 1;
    }else if( (*list)[bef] < (*list)[aft] && rep == 1){
      rep = 0;
      bef++;
      (*list)[bef] = (*list)[aft];
    }else{
      bef++;
      aft++;
    }
  }
  if(*sz!=0)  *sz = bef+1;
}

extern void unique_hindex( list, sz )
  host_table_index_t **list;
  int *sz;
{
  int i, bef, aft, rep, old;
  old = *sz;
  for( i=0, bef=0, aft=1, rep=0; aft<*sz ; i++ ){
    if( (*list)[bef] == (*list)[aft] ){
      aft++;
      rep = 1;
    }else if( (*list)[bef] < (*list)[aft] && rep == 1){
      rep = 0;
      bef++;
      (*list)[bef] = (*list)[aft];
    }else{
      bef++;
      aft++;
    }
  }
  if(*sz!=0)  *sz = bef+1;
  if( *sz!=old ){
    if( (*list = (host_table_index_t *)realloc(*list, sizeof(host_table_index_t)*(*sz))) == (host_table_index_t *)NULL ){
      error(A_ERR,"unique_hindex","Could not realloc memory");
    }
  }
  
}



/*
 *  the resultant site list is not unique.  but it is sorted.  I did not make
 *  it unique inorder to save time if this was called in a loop.  I make the
 *  resultant unique after I get out of the loop.
 */
extern status_t archSiteIndexOper( old, osz, new, nsz, oper, res, rsz )
  host_table_index_t **old;
  int osz;
  host_table_index_t **new;
  int nsz;
  bool_ops_t oper;
  host_table_index_t **res;
  int *rsz;
{
  int i,j,k;
  host_table_index_t prev;
  int in = 0;
  qsort( *old, osz, sizeof(host_table_index_t), siteindexcompare);
  qsort( *new, nsz, sizeof(host_table_index_t), siteindexcompare);

  *rsz = osz+nsz;
  *res = (host_table_index_t *)malloc(sizeof(host_table_index_t)*(*rsz));

  /*
   *  Take each element in "old" and compare it to each element in "new".
   */
  if( oper == OR_OP ){
    for( i=0; i<osz; i++ ){
      (*res)[i] = old[0][i];
    }
    for( j=0; j<nsz; j++,i++ ){
      (*res)[i] = new[0][j];
    }
    qsort( (*res), *rsz, sizeof(host_table_index_t), siteindexcompare);
/*
 *  unique_hindex( &(*res), &(*rsz));
 */
    return A_OK;
  }
  prev=-1;
  for( in=0, k=0, i=0; (i<osz) && (k<(*rsz)); i++ ){
    if( prev == (*old)[i] ) continue;
    for( j=0, in=0 ; j<nsz; j++ ){
      switch( oper ){
      case AND_OP:
        if( (*old)[i] == (*new)[j] ){
          (*res)[k] = (*old)[i];
          k++;
          j=nsz;
        }
        break;
      case NOT_OP:
        if( (*old)[i] == (*new)[j] ){
          in = 1;
          j=nsz;
        }
        break;
      case OR_OP:
      case NULL_OP:
        break;
      }
    }
    if( in == 0 && oper == NOT_OP){ /* only happens in NOT_OP */
      (*res)[k] = (*old)[i];
      k++;
    }
    prev = (*old)[i];
  }

  *rsz = k;
/*
 *  unique_hindex( &(*res), &(*rsz)); opt961010
 */
  if( (*res = (host_table_index_t *)realloc(*res,sizeof(host_table_index_t)*(*rsz))) == (host_table_index_t *)NULL ){
    error(A_ERR,"archSiteIndexOper","Could not realloc memory");
    return ERROR;
  }
  res[0][*rsz]=0;

  return A_OK;
}

extern status_t orSitesForStarts( start, sn, start_db,
                                  max, found, sites, d_list, d_cnt )
  index_t *start;
  int sn;
  file_info_t *start_db;
  int max;
  int *found;
  host_table_index_t **sites;
  domain_t * d_list;
  int d_cnt;
{
  int n, k, ls, total_hits, local_hits = 0;
  hostname_t hnm;
  int port;
  ip_addr_t ipaddr = (ip_addr_t)(0);
  host_table_index_t *local_sites, *total_sites, hin;
  total_sites = NULL;
  total_hits = 0;  
  for( n=0; n<sn; n++ ){
    local_sites = (host_table_index_t *)malloc(sizeof(host_table_index_t)*MAX_LIST_SIZE);
    get_index_start_dbs(start_db, start[n], local_sites, &local_hits);
    local_sites = (host_table_index_t *)realloc(local_sites ,
                                           sizeof(host_table_index_t)*local_hits);
    /*
     *  Here do some filtering of sites that do not match the domain
     *  restriction
     */
    if( local_hits > 0 ){

      ls = 0;
      /* For every found string .. go through each site file */
      for ( k = 0 ; k < local_hits ; k++ ) {
        ipaddr = (ip_addr_t)(0);
        port = 0;
        hin = local_sites[k];

        if( host_table_find( &ipaddr, hnm, &port, &hin) == ERROR ) {
          error(A_ERR,"archBooleanQueryMore","Could not find host in host-table. start/host dbase corrupt.");
          continue;
        }

        if( hnm[0]!='\0' ){
          if( !find_in_domains( hnm, d_list, d_cnt) ){
            continue;
          } else {
            local_sites[ls] = hin;
            ls++;
          }
        }
      }

      local_sites[ls] = -1;
      local_hits = ls;
      local_sites = (host_table_index_t *)realloc(local_sites ,
                     sizeof(host_table_index_t)*local_hits);
    }
    /*
     *  Done with domain restriction filtering.
     */
    if( local_hits<=0 ){
      free( local_sites );
    }else if( n==0 || total_sites == NULL ){
      total_sites = local_sites;
      total_hits = local_hits;
    }else if ( n>0 ){
      *sites = total_sites;
      *found = total_hits;
      archSiteIndexOper(&(*sites), *found, &local_sites, local_hits,
                        OR_OP, &total_sites, &total_hits );
      free( local_sites );
      free( *sites );
    }
  }
  if( total_hits > 0 ){
    unique_hindex( &total_sites, &total_hits );
  }else{
    total_sites = *sites;
    total_hits = *found;
  }
  *sites = total_sites;
  *found = total_hits;
  return A_OK;
}


/*
 *  takes as input an array of strings' indices = list, and a string = word.
 *  according to the operation = oper and the case sensitivity = casesens it
 *  performs "oper" between "word" and every word in the "list".
 *  
 *  I use this instead of performing mass operations between lists in the
 *  case of NOT and AND operations.  This should be less expensive.
 */

extern status_t archAnonftpOper( list, lsz, oper, word, strhan, casesens )
  index_t **list;
  int *lsz;
  bool_ops_t oper;
  char *word;
  struct arch_stridx_handle *strhan;  
  int casesens;
{
  int i,j,in=0;
  char *res, *wd;

  wd = word;
  if( !casesens ){
    wd = (char *)strlwr( wd );
  }
  for( i=0, j=0 ; i<*lsz; i++ ){
    if( !archGetString( strhan,  (*list)[i], &res) ) {
      error(A_ERR, "archAnonftpOper", "Start %d has string index out of bounds",(*list)[i] );
      return(ERROR);
    }
    if( !casesens ){
      res = (char *)strlwr( res );
    }
    if(strstr( res, wd )!=(char *)NULL) in=1;
    else in=0;
    if( (in && oper == AND_OP) || (!in && oper == NOT_OP) ){
      (*list)[j] = (*list)[i];
      j++;
    }
    free(res);
  }
  *lsz = j;
  (*list)[j] = 0;
  return A_OK;

}

extern status_t archAnonftpOperCont(list, lsz, stopin, stopout, oper,
                                    word, strhan, casesens, outlist, outsz)
  index_t *list;
  int lsz;
  int *stopin;
  int *stopout;
  bool_ops_t oper;
  char *word;
  struct arch_stridx_handle *strhan;  
  int casesens;
  index_t **outlist;
  int *outsz;
{
  int i,j,in;
  char *res, *wd;

  wd = word;
  in = 0;
  if( !casesens ){
    wd = (char *)strlwr( wd );
  }
  if(/* *stopout == 0 ||*/ *outlist==NULL){
    *outlist = (index_t *)malloc(sizeof(index_t)*lsz);
  }
  if( *stopin < 0 ) *stopin=0;
  if( *stopout < 0 ) *stopout=0;
  for( i=*stopin, j=*stopout ; i<lsz; i++ ){
    if( !archGetString( strhan,  (list)[i], &res) ) {
      error(A_ERR, "archAnonftpOper",
            "Start %d has string index out of bounds",(list)[i] );
      return(ERROR);
    }
    if( !casesens ){
      res = (char *)strlwr( res );
    }
    if(strstr( res, wd )!=(char *)NULL) in=1;
    else in=0;
    if( (in && oper == AND_OP) || (!in && oper == NOT_OP) ){
      (*outlist)[j] = (list)[i];
      j++;
    }
    if((j+1+(*stopout))%lsz==0){
      index_t *t;
      int sz;
      sz = (j+1+(*stopout))+lsz;
      if( (t = (index_t *)realloc(*outlist, sizeof(index_t)*sz))==NULL ){
        error(A_ERR,"archAnonfpOperCont","could not realloc list");
        free( res );
        return ERROR;
      }
      *outlist = t;
    }
    free(res);
  }
  *stopin = i;
  *stopout = j;
  *outsz = j;
  (*outlist)[j] = 0;
  return A_OK;

}


extern status_t archTupleOper( old, osz, new, nsz, oper, res, rsz )
  site_tuple_t **old;
  int osz;
  site_tuple_t **new;
  int nsz;
  bool_ops_t oper;
  site_tuple_t **res;
  int *rsz;
{
  int i,j,k,r,max,c1,c2,c3;
  int in = 0;
  site_tuple_t *prev, *rem;
  
  qsort( *old, osz, sizeof(site_tuple_t), tuplecompare);
  qsort( *new, nsz, sizeof(site_tuple_t), tuplecompare);

  *rsz = osz+nsz;
  *res = (site_tuple_t *)malloc(sizeof(site_tuple_t)*(*rsz));
  r = max = c1 = c2 = c3 = 0;
  rem = NULL;

  prev = (site_tuple_t *)malloc(sizeof(site_tuple_t)*(1));
  memset( prev, 0, sizeof(site_tuple_t));

/*
 *  if( osz>0 ) prev=old[0]; else if( nsz>0 ) prev=new[0]; else prev=NULL;
 */

  /*
   *  Take each element in "old" and compare it to each element in "new".
   */

  if( oper == OR_OP ){
    for(i=0,j=0,k=0; i<osz && j<nsz && k<*rsz; ){
      c1 = tuplecompare(&(old[0][i]), &(new[0][j]));
      
      if( c1<0 && (c2=tuplecompare(&(old[0][i]), prev))!=0 ){
        prev = &(old[0][i]);
        tuplecpy(&(res[0][k]),&(old[0][i]));
        i++; k++;
      }else if( c1>0 && (c3=tuplecompare(&(new[0][j]),(prev)))!=0 ){
        prev = &(new[0][j]);
        tuplecpy(&(res[0][k]),&(new[0][j]));
        j++; k++;
      }else if( c1==0 && c2!=0 ){
        prev = &(old[0][i]);
        tuplecpy(&(res[0][k]),&(old[0][i]));
        i++; j++; k++;
      }else if( c2==0 ){
        i++;
      }else if( c3==0 ){
        j++;
      }
    }
    if( i>=osz && j<nsz ){
      rem = (*new);
      r = j;
      max = nsz;
    }else if( j>=nsz && i<osz ){
      rem = (*old);
      r = i;
      max = osz;
    }
    for( ; r<max && k<*rsz ; r++,k++ ){
      tuplecpy(&(res[0][k]),&(rem[r]));
    }
    goto end;
  }

  for( k=0, i=0; (i<osz) && (k<(*rsz)); i++ ){
    for( j=0, in=0 ; j<nsz; j++ ){
      if( parentcompare(&((*old)[i]) , &((*new)[j])) == 0 ){
        in = 1;
        break;
      }
    }
    if( (in && oper == AND_OP) || (!in && oper == NOT_OP) ){
      tuplecpy(&((*res)[k]) , &((*old)[i]));
      k++;      
    }
  }

 end:
  *rsz = k;
/*   unique_tuple( &(*res), &(*rsz)); */
  if( (*res = (site_tuple_t *)realloc(*res,sizeof(site_tuple_t)*(*rsz))) == (site_tuple_t *)NULL ){
    error(A_ERR,"archTupleOper","Could not realloc memory");
    return ERROR;
  }

  return A_OK;
}

extern status_t archStartIndexConcat( old, osz, new, nsz, res, rsz )
  index_t **old;
  int osz;
  index_t **new;
  int nsz;
  index_t **res;
  int *rsz;
{
  int i,j,k;
  index_t *rem;
  
/*  qsort( *old, osz, sizeof(index_t), indexcompare);
  qsort( *new, nsz, sizeof(index_t), indexcompare);
*/
  *rsz = osz+nsz;
  *res = (index_t *)malloc(sizeof(index_t)*(*rsz));
  rem = NULL;
  for( i=0,k=0; i<osz; i++,k++ ){
    (*res)[k]=(*old)[i];
  }
  for( j=0; j<nsz; j++,k++ ){
    (*res)[k]=(*new)[j];
  }
  /*
   *  qsort( *res, k, sizeof(index_t), indexcompare); unique( &(*res), &(k));
   */
  *rsz = k;
  return A_OK;
}



extern status_t archTupleConcat( old, osz, new, nsz, res, rsz )
  site_tuple_t **old;
  int osz;
  site_tuple_t **new;
  int nsz;
  site_tuple_t **res;
  int *rsz;
{
  int i,j,k;
  site_tuple_t *rem;
  
  qsort( *old, osz, sizeof(site_tuple_t), tuplecompare);
  qsort( *new, nsz, sizeof(site_tuple_t), tuplecompare);

  *rsz = osz+nsz;
  *res = (site_tuple_t *)malloc(sizeof(site_tuple_t)*(*rsz));
  rem = NULL;
  for( i=0,k=0; i<osz; i++,k++ ){
    tuplecpy(&((*res)[k]) , &((*old)[i]));    
  }
  for( j=0; j<nsz; j++,k++ ){
    tuplecpy(&((*res)[k]) , &((*new)[j]));
  }

  *rsz = k;
  return A_OK;
}


/*
   Takes two starts lists and performs the operation "oper" on them.
   The result is placed in the "res" list and the size in rsz.
   Different from the above archAnonftpOper() because it deals with
   two different list while archAnonftpOper() applies an operation on
   a single word with a list.
   */
extern status_t archAnonftpOperOn2Lists( old, osz, new, nsz, oper, res, rsz )
  index_t **old;
  int osz;
  index_t **new;
  int nsz;
  bool_ops_t oper;
  index_t **res;
  int *rsz;
{
  int i,j,k;
  int in = 0;
  qsort( *old, osz, sizeof(index_t), indexcompare);
  qsort( *new, nsz, sizeof(index_t), indexcompare);

  *rsz = osz+nsz;
  *res = (index_t *)malloc(sizeof(index_t)*(*rsz));

  /*
   *  Take each element in "old" and compare it to each element in "new".
   */
  if( oper == OR_OP ){
    for( i=0; i<osz; i++ ){
      (*res)[i] = old[0][i];
    }
    for( j=0; j<nsz; j++,i++ ){
      (*res)[i] = new[0][j];
    }
    qsort( (*res), *rsz, sizeof(index_t), indexcompare);
    unique( &(*res), &(*rsz));
    return A_OK;
  }

  for( in=0, k=0, i=0; (i<osz) && (k<(*rsz)); i++ ){
    for( j=0, in=0 ; j<nsz; j++ ){
      switch( oper ){
      case AND_OP:
        if( (*old)[i] == (*new)[j] ){
          (*res)[k] = (*old)[i];
          k++;
          j=nsz;
        }
        break;
      case NOT_OP:
        if( (*old)[i] == (*new)[j] ){
          in = 1;
          j=nsz;
        }
        break;
      case OR_OP:
      case NULL_OP:
        break;
      }
    }
    if( in == 0 && oper == NOT_OP){ /* only happens in NOT_OP */
      (*res)[k] = (*old)[i];
      k++;
    }
  }

  *rsz = k;
  unique( &(*res), &(*rsz));
  if( (*res = (index_t *)realloc(*res,sizeof(index_t)*(*rsz+1))) == (index_t *)NULL ){
    error(A_ERR,"archAnonftpOperOn2Lists","Could not realloc memory");
    return ERROR;
  }
  res[0][*rsz]=0;

  return A_OK;
}
