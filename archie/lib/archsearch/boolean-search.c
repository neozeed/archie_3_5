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
#include "all.h"
#include "strings_index.h"
#include "get-info.h"
#include "search.h"
#include "boolean-ops.h"


extern char *prog;

#ifdef __STDC__
#else
   extern char *strlwr();
   extern status_t archSiteIndexOper();
   extern status_t host_table_find();
   extern status_t update_start_dbs();
   extern status_t close_start_dbs();
   extern status_t open_start_dbs();
   extern status_t get_index_start_dbs();
   extern int archSearchSub();
   extern status_t get_port();
   extern int archAnonftpOperOn2Lists PROTO(());
   extern status_t archAnonftpOper PROTO(());
   extern status_t getLongestAndWord PROTO(());
#endif

#define CEILING_MAX 12000

static status_t getStartsFromWord( word, strhan, type, case_sens, matches, found, start )
  char *word;
  struct arch_stridx_handle *strhan;
  int type;
  int case_sens;
  int matches;
  int *found;
  index_t **start;
{
  unsigned long hits;
  if( *start==NULL )  *start = (index_t *)malloc(sizeof(index_t)*matches);

  switch( type ){
  case EXACT:
    if( !archSearchExact( strhan, word, case_sens, matches, &(hits), *start) ){
      error(A_ERR, "getStartsFromWord","Could not perform exact search.");
      free(start);
      return(A_OK);
    }
    break;

  case SUB:
    if( !archSearchSub( strhan, word, case_sens, matches, &(hits),  *start) ){
      error(A_ERR, "getStartsFromWord", "Could not perform Sub-String search.");
      free(start);
      return(A_OK);
    }
    break;

  case REGEX:
    *found = 0;
    if( !archSearchRegex( strhan, word, case_sens, matches, &(hits),  *start) ){
      error(A_ERR, "getStartsFromWord", "Could not perform Reg-Ex search.");
      free(start);
      return(A_OK);
    }
    break;

  default:
    error(A_ERR,"getStartsFromWord", "Did not supply any search type.");
    free(start);
    return(A_OK);
    break;
  }
  *found = hits;
  return A_OK;
}


static void find_parent( file, index, tuple )
  file_info_t *file;
  int index;
  site_tuple_t *tuple;
{
  full_site_entry_t *curr_ent;
  int cnt=0;

  curr_ent = (full_site_entry_t *) file -> ptr + index;
  cnt=index;
  tuple->child= cnt;
  tuple->file = cnt;

  if( CSE_IS_KEY((*curr_ent)) ){
    while((cnt >= 0) &&
          (CSE_IS_KEY((*curr_ent))) ){
      curr_ent--;
      cnt--;
    }
    tuple->file = cnt;
  }
  /*
   *  while((cnt >= 0) && !(CSE_IS_SUBPATH((*curr_ent)) ||
   *        CSE_IS_NAME((*curr_ent)) || CSE_IS_PORT((*curr_ent))) ){
   *        curr_ent--; cnt--; } tuple->parent=cnt;
   *  
   *  opt961010
   */
  return;
}

static status_t archGetBooleanStarts(strhan, qry, type, case_sens, h,
                                     verbose, num_hits, where,
                                     more_record, outlist, outsz)
struct arch_stridx_handle *strhan;
bool_query_t qry;
int type;                     /* type of search */
int case_sens;                /* case_sens */
int h;                        /* maximum hits (all in all) */
int verbose;
int num_hits;                 /* how many hits found so far in main search */
int *where;                    /* which and_tpl to start with */
boolean_return_t *more_record;
index_t **outlist;
int *outsz;
{
  int i,k,j,l,ind;
  int oldsz, newsz, tmpsz, ressz, opersz, method, osz, nsz, si, so, stopin, 
  stopout, olsz, insz, startsz;
  int stop, *and_num_starts, min, min_ind, max, max_ind, newm, flag, m;
  index_t *tmp_list, *res_list, *new_list, *old_list, *oper_list, *ol, *nl;
  index_t **and_starts, *inlist, *olist, *startptr, *start = NULL;
  char **strhan_arr;
  bool_ops_t oper=0;
  unsigned long num_found_str=0;

  inlist = olist = tmp_list = NULL; olsz = insz = 0;
  stop=ind=max=opersz=tmpsz=ressz=method=0;
  j=i=k=0;
  m=BOOL_MAX_MATCH;
  if( *where<qry.orn ){
    for(i=*where; i<qry.orn && !stop; i++ ){
      res_list=NULL;
      and_starts=NULL;
      and_num_starts=NULL;
      min = m;        max = 0;
      min_ind = 0;    max_ind = 0;
      /*
       *  See @archGetBooleanStarts#0
       */
      and_starts = (index_t **)malloc(sizeof(index_t *)*qry.and_list[i].lnum);
      and_num_starts = (int *)malloc(sizeof(int)*qry.and_list[i].lnum);
      strhan_arr = (char **)malloc(sizeof(char *) * qry.and_list[i].lnum);

      for( j=0; j<qry.and_list[i].lnum; j++ ){
        and_starts[j] = NULL;
        and_num_starts[j] = 0;
        start = NULL;
        if( getStartsFromWord(qry.and_list[i].lwords[j], strhan, type,
                              case_sens, m, &num_found_str,
                              &start) == ERROR ){
          error(A_ERR,"archGetBooleanStarts",
                "could not get starts for a single word.");
          return ERROR;
        }
        {                       /* save strhan of this last search */
          if(!archGetStateString( strhan, &(strhan_arr[j]) )){
            error(A_ERR,prog,"Could not convert strhan to string using archGetStateString()\n");
            return(0);
          }
        }
      
        if( num_found_str <= 0 ){
          if( verbose ){
            error(A_INFO, "archGetBooleanStarts","String %s was not found in the database.",qry.and_list[i].lwords[j]);
          }
          if( start ) {
            free(start);  start = NULL;
          }
          if( and_starts ){
            for(l=0; l<qry.and_list[i].lnum; l++){
              if( and_starts[l] ) free(and_starts[l]);
            }
            free( and_starts ); and_starts = NULL;
          }
          if( and_num_starts ){
            free( and_num_starts ); and_num_starts = NULL;
          }
          res_list = (index_t *)NULL;
          ressz = 0; max=0; min=0;
          break;
        }else{
          and_starts[j] = start;
          and_num_starts[j] = num_found_str;
          if( min>=num_found_str ){
            min = num_found_str;
            min_ind = j;
          }
          if( max<num_found_str ){
            max_ind = j;
            max = num_found_str;
          }
        }
      } /* for loop going through each and component */

      if(  min>0  ){
        if( max<m && min>0 ){
          /*
           *  use the archAnonftpOperOn2Lists() on all starts
           */
          method = 0;
        }else if( max==m && min<m ){
          /*
           *  get the smallest "min" and AND the other a-cs with all its
           *  starts
           */
          method = 1;
        }else if( max==m && min==m ){
          /*
           *  get the longest word of all the a-cs and take its list as the
           *  target list to and its starts with the other a-cs
           *  until we reach max_match
           */
          method = 2;
        }

        if( qry.and_list[i].lnum>1 ){  
          switch( method ){
          case 0:
            res_list = NULL; ressz = 0;
            for( l=0; l<qry.and_list[i].lnum; l++ ){
              if( res_list==NULL ){
                res_list = and_starts[l];
                ressz = and_num_starts[l];
              }else{
                ol = res_list;
                osz = ressz;
                nl = and_starts[l];
                nsz = and_num_starts[l];
                if( archAnonftpOperOn2Lists(&ol, osz, &nl, nsz, AND_OP,
                                            &res_list, &ressz )==ERROR ){
                  error(A_ERR,"archGetBooleanStarts",
                        "archAnonftpOperOn2Lists failed");
                  return ERROR;
                }else{
                  if( ol ){ free(ol); ol=NULL; }
/*       will be freed later on           if( nl ){ free(nl); nl=NULL; } */
                }
              }
            }
            break;
          case 1:
            res_list = and_starts[min_ind]; start=NULL;
            ressz = min;
            for( k=0; k<qry.and_list[i].lnum; k++){
              if( k==min_ind ) continue;
              if( archAnonftpOper(&res_list, &ressz, AND_OP,
                                  qry.and_list[i].lwords[k],
                                  strhan, case_sens ) == ERROR){
                error(A_ERR,"archGetBooleanStarts","archAnonftpOper failed");
                return ERROR;
              }
            }
            break;
          case 2:
            getLongestAndWord( qry.and_list[i], &min_ind);
            if( min_ind >= 0 ){
              flag = 1;
              res_list = and_starts[min_ind]; 
              ressz = min;
              newm=0;
              stopin = stopout = 0;
              if( olist ) {
                free(olist); olist = NULL;
              }
              olsz = 0;
              while( flag ){
                start=NULL;
                inlist = res_list; insz = ressz;
                /*                olist = NULL;      olsz = 0; */
                si = stopin; so = stopout;
                for( k=0; k<qry.and_list[i].lnum; k++){
                  if( k==min_ind ) continue;
                  /*
                   *  archAnonftpOperCont() will create a new olist (outlist)
                   *                      IFF olist=NULL originally.  Other
                   *                      wise it will add to it.  Here I
                   *                      don't want it to create a new except
                   *                      at the start.
                   */
                  if( archAnonftpOperCont(inlist, insz,
                                          &si, &so,
                                          AND_OP, qry.and_list[i].lwords[k],
                                          strhan, case_sens, &olist, &olsz ) == ERROR){
                    error(A_ERR,"archGetBooleanStarts","archAnonftpOper failed");
                    return ERROR;
                  }
                  /*  will be freed later
                     if( inlist ) {
                     free(inlist); inlist=NULL;
                     }*/
                  inlist = olist;
                  insz = olsz;
                  if( (k+1)==qry.and_list[i].lnum ){
                    stopin = ressz;  /* 1000 */
                    stopout = so;    /* 13 */
                  }
                  si = stopout;
                  so = stopout;
                }
                /*
                 *  here I put some code to go back and do some more searches
                 *  on the list targetted. UGLY as a beast.
                 */
                /* if( olsz<m ) */
                {
                  if( num_found_str < m ){
                    /* stop if we can't reach maxmatch */
                    break;
                  }
                  start = NULL;
                  if( newm==0 ){
                    if (!archSetStateFromString(strhan,strhan_arr[min_ind])){
                      free( strhan_arr[min_ind] );
                      return ERROR;
                    }
                    free( strhan_arr[min_ind] );
                    strhan_arr[min_ind] = NULL;
                    ressz = and_num_starts[min_ind];
                    res_list = and_starts[min_ind];
                  }
                  newm = m;
                  start = (index_t *)malloc(sizeof(index_t)*newm);
                  if(!archGetMoreMatches(strhan, newm,
                                         &num_found_str, start)){
                    error(A_ERR, "archQuery",
                          "Could not perform search using archGetMoreStarts().");
                    return ERROR;
                  }
                  if( num_found_str>0 ){
                    startsz = num_found_str;
                    startptr = start;
                    if(archStartIndexConcat(&res_list,ressz,
                                            &startptr, startsz,
                                            &start, &num_found_str)==A_OK){
                      if( res_list ){ free(res_list); res_list=NULL; }
                      if( startptr ){ free(startptr); startptr=NULL; }
                    }else{
                      error(A_ERR, "archQuery",
                            "archStartIndexConcat failed");
                      return(ERROR);
                    }
                  }
                  if( num_found_str == 0 ){
                    /* stop if we can't reach maxmatch */
                    flag=0;
                  }else{
                    res_list = start; ressz = num_found_str;
                    if( ressz >= CEILING_MAX ){
                      flag=0;
                    }
                  }
                }
                /* else{
                   flag = 0; 
                   }*/
              } /* while flag (need more matches)*/
              if( olist ) {
                if( res_list) free( res_list );
                res_list = olist; ressz=olsz;
                olist=NULL; olsz=0;
              }
            } /* if min_ind found */
            break;
          } /* case method=2 */
        }else{                  /* if there is more than one and word (a-cs>1) */
          res_list = and_starts[0];
          ressz = and_num_starts[0];
        }
      } /*  if( min>0 )
         *  if there were starts to operate on. In the case
         *  when there is one word in the and_tuple
         *  that returns a 0 size for a start_list, there is
         *  no need to operate as the AND operation will
         *  result in NULL any ways
         */

      /* before anything else get rid of and_lists, and_num_lists */
      if( and_starts ){
        for(l=0; l<qry.and_list[i].lnum; l++){
          if( and_starts[l] && and_starts[l]!= res_list )
          free(and_starts[l]);
        }
        free( and_starts ); and_starts = NULL;
      }
      if( and_num_starts ){
        free( and_num_starts ); and_num_starts = NULL;
      }
      if( strhan_arr ){
        for(l=0; l<qry.and_list[i].lnum; l++){
          if( strhan_arr[l] )
          free(strhan_arr[l]);
        }
        free( strhan_arr ); strhan_arr = NULL;
      }

      /* process NOT */

      if( ressz > 0 ){
        for(k=0; k<qry.not_list[i].lnum; k++){
          if( archAnonftpOper(&res_list, &ressz, NOT_OP,
                              qry.not_list[i].lwords[k],
                              strhan, case_sens) == ERROR){
            error(A_ERR,"archGetBooleanStarts","archAnonftpOper failed");
            return ERROR;
          }
        }

        if( (ressz+opersz) >= (h+more_record->acc_res+num_hits) ){
          stop = 1;
          *where = i;
        }

        /* processing OR on two and_lists */
        if( tmp_list!=NULL && res_list!=NULL ){
          old_list = tmp_list;      oldsz = tmpsz;
          new_list = res_list;      newsz = ressz;
          oper = OR_OP;
          if( archAnonftpOperOn2Lists(&old_list, oldsz,
                                      &new_list, newsz,
                                      oper, &oper_list, &opersz ) == ERROR){
            error(A_ERR,"archGetBooleanStarts","archAnonftpOperOn2Lists failed");
            return ERROR;
          }
          tmp_list = oper_list;     tmpsz = opersz;
          if( new_list ) {
            free( new_list );
            new_list = NULL;
          }
          if( old_list ) {
            free( old_list );
            old_list = NULL;
          }
        }else if( tmp_list==NULL && res_list != NULL ){
          tmp_list = res_list;
          tmpsz = ressz;
          res_list = NULL;
        }
      }
      /*
       *  before I perform any OR operation I must check if I have reached max
       *  hits yet
       */
    }
  }
  /*
   *  before I perform any OR operation I must check if I have reached max
   *  hits yet
   */
  *outlist = tmp_list;
  *outsz = tmpsz;
  if( !stop ) *where = i;
  return(A_OK);
}

static status_t findParentsForStarts( start, nst, site, tuples, tnum )
  index_t *start;
  int nst;
  host_table_index_t site;
  site_tuple_t **tuples;
  int *tnum;
{
  ip_addr_t ipaddr = (ip_addr_t)0;
  int port = 0;
  hostname_t hnm;
  file_info_t *index_finfo = create_finfo();
  file_info_t *curr_finfo = create_finfo();
  int i,j,recs,act_size = 0;
  int *rec_list = NULL;
  site_tuple_t *tpl=NULL;
  char curr_file_name[MAX_PATH_LEN];
  full_site_entry_t *curr_ent, *curr_endptr;
  index_t curr_count=0;
  index_t curr_strings = 0;
  index_t curr_prnt=0;
  index_t curr_file=0;
  int retval = A_OK;
  site_tuple_t *res, *new, *old;
  int resn, newn, oldn;

  memset( hnm, 0, sizeof(char)*MAX_HOSTNAME_LEN );
  if( host_table_find( &ipaddr, hnm, &port, &site) == ERROR ) {
    error(A_ERR,"findParentsForStarts","Could not find host in host-table. start/host dbase corrupt.");
    retval = ERROR;
    goto end;
  }
  
  sprintf(curr_file_name,"%s",(char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ipaddr)),port));
  curr_file_name[strlen(curr_file_name)]='\0';
  if(access(curr_file_name, F_OK) < 0){
    /* "File %s does not exist in the database. No need to look for starts in it" */
    error(A_INFO, "findParentsForStarts", "File %s does not exist in the database. No need to look for starts in it.",
          curr_file_name);
    retval = A_OK;
    goto end;
  }else{
    strcpy(curr_finfo -> filename, curr_file_name);
    /* Open current file */
    if(open_file(curr_finfo, O_RDONLY) == ERROR){
      /* "Ignoring %s" */
      error(A_ERR, "findParentsForStarts", "Ignoring %s", curr_finfo->filename);
      retval = A_OK;
      goto end;
    }
  }

  /* mmap it */
  if(mmap_file(curr_finfo, O_RDONLY) == ERROR){
    /* "Ignoring %s" */
    error(A_ERR, "findParentsForStarts", "Ignoring %s", curr_finfo ->filename);
    retval = A_OK;
    goto end;
  }
  act_size = curr_finfo -> size / sizeof(full_site_entry_t);

  sprintf(index_finfo->filename,"%s%s",curr_finfo->filename,SITE_INDEX_SUFFIX);

  if(access(index_finfo->filename, F_OK) < 0){
    destroy_finfo(index_finfo);
    index_finfo = NULL;
    /* nothing right now . No errors. */
  }else if ( open_file(index_finfo, O_RDONLY) == A_OK ) {

    /* mmap it */
    if(mmap_file(index_finfo, O_RDONLY) == ERROR){
      /* "Ignoring %s" */
      error(A_ERR, "findParentsFromStarts", "Ignoring %s", index_finfo ->filename);
      retval = A_OK;
      goto end;
    }

    new = old = res = NULL;
    newn = oldn = resn = 0;
    for(j=0; j<nst; j++){
      if ( search_site_index(index_finfo, start[j], &rec_list, &recs) == ERROR ) {
        error(A_ERR,"findParentsFromStarts","search_site_index failed.");
        continue;               /* for now end.. once it is more stable
                                   we will let it goto traditional */
      }

      *tuples = (site_tuple_t *)malloc(sizeof(site_tuple_t)*recs);
      *tnum = recs;
      for( i=0; i<recs; i++ ){
        (*tuples)[i].site = site;
        find_parent( curr_finfo, rec_list[i], &((*tuples)[i]) );
      }
      if( j==0 || newn == 0 ){
        new = (*tuples);
        newn = *tnum;
      }else{
        old = new;
        oldn = newn;
        new = (*tuples);
        newn = *tnum;
        if( archTupleOper(&old, oldn,
                          &new, newn,
                          OR_OP, &res, &resn) == ERROR ){
          error(A_ERR,"findParentsForStarts","couldn't perform Tuple operation");
        }
        if( old!=NULL ){
          free(old);
          old=NULL; oldn=0;
        }
        if( new!=NULL ){
          free(new);
          new=NULL; newn=0;
        }
        new = res; newn = resn; resn = 0;
      }
    }
    *tuples = new;
    *tnum = newn;
    retval = A_OK;
    goto end;
  }

  /* if there is no site index */

/* traditional: */

  *tuples = (site_tuple_t *)malloc(sizeof(site_tuple_t)*act_size);
  curr_prnt = curr_file = 0;
  for(curr_ent = (full_site_entry_t *) curr_finfo -> ptr,
      curr_endptr = (full_site_entry_t *) curr_finfo -> ptr + act_size,
      curr_count = 0,
      i=0;
      curr_ent < curr_endptr && i<act_size;
      (full_site_entry_t *)curr_ent++, curr_count++){

    curr_strings = curr_ent->strt_1;

    /* Check if string offset is valid */
    if(curr_strings >= (index_t)(0)){

      /*
       *  figure out the different parents.
       */
      
      if ( CSE_IS_SUBPATH((*curr_ent)) ||
          CSE_IS_PORT((*curr_ent)) ||
          CSE_IS_NAME((*curr_ent)) ){
        curr_file = curr_prnt = curr_count;
      }else{
        if(!CSE_IS_KEY((*curr_ent))){
          curr_file = curr_count;
        }
        for( j=0; j<nst; j++ ){
          if( curr_strings == start[j] ){
            (*tuples)[i].child = curr_count;
            /*
             *  (*tuples)[i].parent = curr_prnt; opt961010
             */
            (*tuples)[i].file = curr_file;
            (*tuples)[i].site = site;
            i++;
            break;
          }
        }
      }
    }
  }
  if( i>0 ){
    if( (tpl = (site_tuple_t *)realloc(*tuples, sizeof(site_tuple_t)*i))==NULL ){
      free(*tuples);
      *tuples=NULL;
      *tnum=0;
      retval = ERROR;
      goto end;
    }else{
      *tuples=tpl;
    }
  }else{
    free( *tuples );    *tuples = NULL;
  }
  *tnum = i;

  retval = A_OK;
 end:
  if( index_finfo ){
    close_file(index_finfo);
    destroy_finfo(index_finfo);
    index_finfo = NULL;
  }
  if( curr_finfo ){
    close_file(curr_finfo);
    destroy_finfo(curr_finfo);
    curr_finfo = NULL;
  }
  return retval;
}

/*
 *  - findParentsForAllStarts():
 *  
 *  Takes two lists of "and" and "not" starts, builds their corresponding
 *  parent tuples in "andtuples" and "nottuples".
 *  
 *  It is different from findParentsForStarts() because it also takes the
 *  "not" list corresponding to the "and" list and produces its tuples too.
 *  It saves time because you don't need to open the same site file again to
 *  produce the "not" tuples!
 *  
 *  The tuples contain three very important indecies: "parent", "child" and
 *  "file".  If the start belongs to a file entry then "child" and "start"
 *  point to the same entry. Otherwise if the start belongs to a keywords then
 *  the child will be contained in the file (different indecies) and both will
 *  be contained in the parent index.
 *  
 */

static status_t findParentsForAllStarts( andstart, nast, notstart, nnst, site, andtuples, atnum, nottuples, ntnum )
  index_t *andstart;
  int nast;
  index_t *notstart;
  int nnst;
  host_table_index_t site;
  site_tuple_t **andtuples;
  int *atnum;
  site_tuple_t **nottuples;
  int *ntnum;
{
  ip_addr_t ipaddr = (ip_addr_t)0;
  int port = 0;
  hostname_t hnm;
  file_info_t *index_finfo = create_finfo();
  file_info_t *curr_finfo = create_finfo();
  int i,j,k,recs,act_size = 0;
  int *rec_list = NULL;
  site_tuple_t *tpl=NULL;
  char curr_file_name[MAX_PATH_LEN];
  full_site_entry_t *curr_ent, *curr_endptr;
  index_t curr_count=0;
  index_t curr_strings = 0;
  index_t curr_prnt=0;
  index_t curr_file=0;
  int retval = A_OK;
  site_tuple_t *res, *new, *old;
  int resn, newn, oldn;

  memset( hnm, 0, sizeof(char)*MAX_HOSTNAME_LEN );
  if( host_table_find( &ipaddr, hnm, &port, &site) == ERROR ) {
    error(A_ERR,"findParentsForAllStarts","Could not find host in host-table. start/host dbase corrupt.");
    retval = ERROR;
    goto end;
  }
  
  sprintf(curr_file_name,"%s",(char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ipaddr)),port));
  curr_file_name[strlen(curr_file_name)]='\0';
  if(access(curr_file_name, F_OK) < 0){
    /* "File %s does not exist in the database. No need to look for starts in it" */
    error(A_INFO, "findParentsForAllStarts", "File %s does not exist in the database. No need to look for starts in it.",
          curr_file_name);
    retval = A_OK;
    goto end;
  }else{
    strcpy(curr_finfo -> filename, curr_file_name);
    /* Open current file */
    if(open_file(curr_finfo, O_RDONLY) == ERROR){
      /* "Ignoring %s" */
      error(A_ERR, "findParentsForAllStarts", "Ignoring %s", curr_finfo->filename);
      retval = A_OK;
      goto end;
    }
  }

  /* mmap it */
  if(mmap_file(curr_finfo, O_RDONLY) == ERROR){
    /* "Ignoring %s" */
    error(A_ERR, "findParentsForAllStarts", "Ignoring %s", curr_finfo ->filename);
    retval = A_OK;
    goto end;
  }
  act_size = curr_finfo -> size / sizeof(full_site_entry_t);

  sprintf(index_finfo->filename,"%s%s",curr_finfo->filename,SITE_INDEX_SUFFIX);

  if(access(index_finfo->filename, F_OK) < 0){
    destroy_finfo(index_finfo);
    index_finfo = NULL;
    /* nothing right now . No errors. */
  }else if ( open_file(index_finfo, O_RDONLY) == A_OK ) {

    /* mmap it */
    if(mmap_file(index_finfo, O_RDONLY) == ERROR){
      /* "Ignoring %s" */
      error(A_ERR, "findParentsForAllStarts", "Ignoring %s", index_finfo ->filename);
      retval = A_OK;
      goto end;
    }

    /* processing AND list */
    new = old = res = NULL;
    newn = oldn = resn = 0;
    for(j=0; j<nast; j++){
      if ( search_site_index(index_finfo, andstart[j], &rec_list, &recs) == ERROR ) {
        /*
         *  error(A_ERR,"findParentsForAllStarts","search_site_index failed.");
         */
        continue;       /* for now end.. once it is more stable
                           we will let it goto traditional */
      }

      *andtuples = (site_tuple_t *)malloc(sizeof(site_tuple_t)*recs);
      *atnum = recs;
      for( i=0; i<recs; i++ ){
        (*andtuples)[i].site = site;
        find_parent( curr_finfo, rec_list[i], &((*andtuples)[i]) );
      }
      if( j==0 || newn == 0 ){
        new = (*andtuples);
        newn = *atnum;
      }else{
        old = new;
        oldn = newn;
        new = (*andtuples);
        newn = *atnum;
        if( archTupleOper(&old, oldn,
                          &new, newn,
                          OR_OP, &res, &resn) == ERROR ){
          error(A_ERR,"findParentsForAllStarts","couldn't perform Tuple operation");
        }
        if( old!=NULL ){
          free(old);
          old=NULL; oldn=0;
        }
        if( new!=NULL ){
          free(new);
          new=NULL; newn=0;
        }
        new = res; newn = resn; resn = 0;
      }
    }
    if( newn>0 ){
      unique_tuple( &(new), &(newn));
      *andtuples = new;
    }else{
      if( new!=NULL ) {
        free( new );
        new = NULL;
      }
    }
    *atnum = newn;

    /* processing NOT list */
    new = old = res = NULL;
    newn = oldn = resn = 0;
    for(j=0; j<nnst; j++){
      if ( search_site_index(index_finfo, notstart[j], &rec_list, &recs) == ERROR ) {
        /*
         *  error(A_ERR,"findParentsForAllStarts","search_site_index failed.");
         */
        continue;               /* for now end.. once it is more stable
                                   we will let it goto traditional */
      }

      *nottuples = (site_tuple_t *)malloc(sizeof(site_tuple_t)*recs);
      *ntnum = recs;
      for( i=0; i<recs; i++ ){
        (*nottuples)[i].site = site;
        find_parent( curr_finfo, rec_list[i], &((*nottuples)[i]) );
      }
      if( j==0 || newn == 0 ){
        new = (*nottuples);
        newn = *ntnum;
      }else{
        old = new;
        oldn = newn;
        new = (*nottuples);
        newn = *ntnum;
        if( archTupleOper(&old, oldn,
                          &new, newn,
                          OR_OP, &res, &resn) == ERROR ){
          error(A_ERR,"findParentsForAllStarts","couldn't perform Tuple operation");
        }
        if( old!=NULL ){
          free(old);
          old=NULL; oldn=0;
        }
        if( new!=NULL ){
          free(new);
          new=NULL; newn=0;
        }
        new = res; newn = resn; resn = 0;
      }
    }
    if( newn>0 ){
      unique_tuple( &(new), &(newn));
      *nottuples = new;
    }else{
      if( new!=NULL ) {
        free( new );
        new = NULL;
      }
    }
    *ntnum = newn;
    /* finished both AND and NOT lists */
    
    retval = A_OK;
    goto end;
  }

  /* if there is no site index */

 traditional:

  *andtuples = (site_tuple_t *)malloc(sizeof(site_tuple_t)*act_size);
  *nottuples = (site_tuple_t *)malloc(sizeof(site_tuple_t)*act_size);
  curr_prnt = curr_file = 0;
  for(curr_ent = (full_site_entry_t *) curr_finfo -> ptr,
      curr_endptr = (full_site_entry_t *) curr_finfo -> ptr + act_size,
      curr_count = 0,
      i=0, k=0;
      curr_ent < curr_endptr && i<act_size && k<act_size;
      (full_site_entry_t *)curr_ent++, curr_count++){

    curr_strings = curr_ent->strt_1;

    /* Check if string offset is valid */
    if(curr_strings >= (index_t)(0)){

      /*
       *  figure out the different parents.
       */
      
      if ( CSE_IS_SUBPATH((*curr_ent)) ||
          CSE_IS_PORT((*curr_ent)) ||
          CSE_IS_NAME((*curr_ent)) ){
        curr_file = curr_prnt = curr_count;
      }else{
        if(!CSE_IS_KEY((*curr_ent))){
          curr_file = curr_count;
        }
        for( j=0; j<nast; j++ ){
          if( curr_strings == andstart[j] ){
            (*andtuples)[i].child = curr_count;
            /*
             *  (*andtuples)[i].parent = curr_prnt; opt961010
             */
            (*andtuples)[i].file = curr_file;
            (*andtuples)[i].site = site;
            i++;
            break;
          }
        }
        for( j=0; j<nnst; j++ ){
          if( curr_strings == notstart[j] ){
            (*nottuples)[k].child = curr_count;
            /*
             *  (*nottuples)[k].parent = curr_prnt; opt961010
             */
            (*nottuples)[k].file = curr_file;
            (*nottuples)[k].site = site;
            k++;
            break;
          }
        }
      }
    }
  }
  if( i>0 ){
    if( (tpl = (site_tuple_t *)realloc(*andtuples, sizeof(site_tuple_t)*i))==NULL){
      free(*andtuples);
      *andtuples=NULL;
      *atnum=0;
      retval = ERROR;
      goto end;
    }else{
      *andtuples = tpl;
    }
  }else{
    free( *andtuples );    *andtuples = NULL;
  }
  if( k>0 ){
    if( (tpl = (site_tuple_t *)realloc(*nottuples, sizeof(site_tuple_t)*k))==NULL ){
      free(*nottuples);
      *nottuples=NULL;
      *ntnum=0;
      retval = ERROR;
      goto end;
    }else{
      *nottuples = tpl;
    }
  }else{
    free( *nottuples );    *nottuples = NULL;
  }
  *atnum = i;
  *ntnum = k;

  retval = A_OK;
 end:
  if( index_finfo ){
    close_file(index_finfo);
    destroy_finfo(index_finfo);
    index_finfo = NULL;
  }
  if( curr_finfo ){
    close_file(curr_finfo);
    destroy_finfo(curr_finfo);
    curr_finfo = NULL;
  }
  return retval;
}


static status_t archGetResultFromTuple( tpl, result, strings, num_results, format, strhan, path, expath, path_rel, path_items)
  site_tuple_t tpl;
  query_result_t *result;
  index_t **strings;
  int *num_results;
  int format;
  struct arch_stridx_handle *strhan;
  char **path;
  char *expath;
  int path_rel;
  int path_items;
{
  char *res;
  int act_size = 0;
  int cnt = 0;
  int web = 1;
  full_site_entry_t *tmp;
  index_t curr_strings;
  index_t curr_count=0;
  index_t curr_file=0;
  index_t curr_prnt=0;

  full_site_entry_t *curr_ent, *ent;
  full_site_entry_t *ptr;
  char exc_file_name[MAX_PATH_LEN];
  char curr_file_name[MAX_PATH_LEN];
  int no_exc = 1;
  int no_excerpts = 0;  
  int p=0;
  int *rec_list = NULL;
  int retval = A_OK;
  int port=0;
  hostname_t hnm;
  ip_addr_t ipaddr=(ip_addr_t)0;

  file_info_t *curr_finfo = create_finfo();
  file_info_t *exc_finfo = create_finfo();

  res = (char *)(0);
  *num_results = cnt;

  if( host_table_find( &ipaddr, hnm, &port, &(tpl.site)) == ERROR ) {
    error(A_ERR,"archGetResultFromTuple",
          "Could not find host in host-table. start/host dbase corrupt.");
    retval = ERROR;
    goto end1;
  }

  sprintf(curr_file_name,"%s",
          (char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ipaddr)),port));
  curr_file_name[strlen(curr_file_name)]='\0';
  
  sprintf(exc_file_name,"%s%s",(char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ipaddr)),port),EXCERPT_SUFFIX);
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
  
  if(access(curr_file_name, F_OK) < 0){
    /* "File %s does not exist in the database. No need to look for starts in it" */
    error(A_INFO,"archGetResultFromTuple","File %s does not exist in the database. No need to look for starts in it.",
          curr_file_name);
    retval = A_OK;
    goto end1;
  }else{
    strcpy(curr_finfo -> filename, curr_file_name);
    /* Open current file */
    if(open_file(curr_finfo, O_RDONLY) == ERROR){
      /* "Ignoring %s" */
      error(A_ERR, "archGetResultFromTuple", "Ignoring %s", curr_finfo->filename);
      retval = A_OK;
      goto end1;
    }
  }

  /* mmap it */
  if(mmap_file(curr_finfo, O_RDONLY) == ERROR){
    /* "Ignoring %s" */
    error(A_ERR, "archGetResultFromTuple", "Ignoring %s", curr_finfo ->filename);
    retval = A_OK;
    goto end1;
  }

  act_size = curr_finfo -> size / sizeof(full_site_entry_t);

  curr_count =  tpl.child;
/*
 *  if( st > 0 && curr_count < st ) continue;
 */

  if( curr_count <= 0 && curr_count >= act_size ) goto end1;
  curr_ent = (full_site_entry_t *) curr_finfo -> ptr + curr_count;
  if ( CSE_IS_SUBPATH((*curr_ent)) || CSE_IS_NAME((*curr_ent)) || CSE_IS_PORT((*curr_ent)) ){
    error(A_ERR,"archGetResultFromTuple","search_site_index() returned an ivalid pointer to a subpath in file-index %s",curr_finfo->filename);
    goto end1;
  }
  curr_strings = curr_ent -> strt_1;        
  handle_entry((file_info_t *)exc_finfo, curr_ent, web, no_excerpts, &result[cnt]);

  if ( web && CSE_IS_KEY((*curr_ent)) ) {
    result[cnt].type = INDX;
    /*
     *  for ( curr_prnt = tpl.child; curr_prnt >= 0; curr_prnt-- ){ tmp =
     *    (full_site_entry_t *)curr_finfo->ptr + curr_prnt; if (
     *    !CSE_IS_KEY((*tmp)) ) break; }
     */
    curr_prnt = tpl.file;
    curr_ent = (full_site_entry_t *) curr_finfo -> ptr + curr_prnt;
    handle_key_prnt_entry((file_info_t *)exc_finfo, curr_ent, web,
                          no_excerpts, &result[cnt]);
    curr_file = curr_prnt;
  }
  else {
    /*
     *  get the type of the document first
     */
    if( curr_count<(act_size-1) ){
      tmp = (full_site_entry_t *)curr_finfo->ptr + curr_count + 1;
      if( CSE_IS_KEY((*tmp)) ){
        result[cnt].type = INDX;
      }else{
        if( result[cnt].details.type.file.size==0 &&
           result[cnt].details.type.file.date==0 ){
          result[cnt].type = NOT_INDX;
        }else{
          result[cnt].type = UNINDX;
        }
      }
    }
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
  {                             /* "Site %s (%s) record %d has string index %d out of bounds" */
    error(A_ERR, "archGetResultFromTuple", "Site %s (%s) record %d has string index %d out of bounds",
          wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ipaddr)),port), curr_count, curr_strings );
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
      error(A_ERR,"archGetResultFromTuple","Can't allocate memory\n");
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
     */

    ent = (full_site_entry_t *) curr_finfo -> ptr + curr_file;
    curr_strings = ent->strt_1;
    if(!no_exc && CSE_IS_DOC((*ent)) && (ent->core.entry.perms <= no_excerpts) ){
      memcpy(&(result[cnt].excerpt), ((excerpt_t *) (exc_finfo->ptr) + ent->core.entry.perms), sizeof(excerpt_t));
    }else{
      result[cnt].excerpt.text[0] = '\0';
    }
  }
    /*changed*/

  if ( handle_chain(curr_finfo, strhan, curr_prnt, curr_strings,web,
                    ipaddr, port, curr_count, &(result[cnt])) == ERROR ) {
    error(A_ERR,"archGetResultFromTuple","Could not handle chain for file %s",curr_finfo->filename);
    retval = ERROR;
    goto end1;
  }
          
  if( check_correct_path(result[cnt].str, path, expath, path_rel, path_items) ){
    result[cnt].ipaddr = ipaddr;
    cnt++;
  }

  *num_results = cnt;        
  /*    if( more_record->site_stop == curr_count )   more_record->stop = -1;
        else   more_record->site_stop = curr_count;
        more_record->site_file = curr_file;
        more_record->site_prnt = curr_prnt;
        */
 end1:
  close_file( curr_finfo );
  close_file( exc_finfo );
  destroy_finfo( curr_finfo );
  destroy_finfo( exc_finfo );
  if( rec_list != NULL ) free( rec_list );
  return retval;

}

extern status_t archWebBooleanQueryMore( strhan, qry, type, case_sens, h,
                                        m, hpm, path, expath, path_rel,
                                        path_items, d_list, d_cnt, db,
                                        start_db, verbose, result,
                                        strings, tot_hits, more_record,
                                        func_name, no_sites_limit, format)
  struct arch_stridx_handle *strhan;
  bool_query_t qry;
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
  boolean_return_t *more_record;
  char *(*func_name)();
  int no_sites_limit;
  int format;
{
  /* File information */

  int j,i,k,l,stop,orn,port,num_results;
  unsigned long starts_found=0;
  int sites_found = 0;
  hostname_t hnm;
  query_result_t *res;
  index_t **strs;
  host_table_index_t **sites, *prev_sites, *curr_sites, *total_sites;
  int prev_sites_found, curr_sites_found, *num_sites, total_sites_found;
  index_t ***starts, ***not_starts, *notstarts, *andstarts;
  int **num_starts, **num_not_starts, num_notstarts, num_andstarts;
  site_tuple_t **tuples, *notstarts_tuples, *andstarts_tuples;
  site_tuple_t *and_tuples, *or_tuples;
  site_tuple_t *old_and_tuples, *old_or_tuples;
  site_tuple_t *new_and_tuples, *new_or_tuples;  /* for each site there is a list of tuples */
  site_tuple_t *not_tuples, *old_not_tuples, *new_not_tuples;

  int *num_tuples = NULL;
  int num_notstarts_tuples, num_andstarts_tuples,
      num_and_tuples, num_or_tuples;
  int num_old_and_tuples, num_old_or_tuples;
  int num_new_and_tuples, num_new_or_tuples;
  int num_not_tuples, num_old_not_tuples, num_new_not_tuples;
  int acc_num_tuples=0;
  boolean_return_t tmp_record;

  index_t *start = NULL;
  hnm[0] = '\0';
  port = 0;

  j=i=k=0;
  *tot_hits = 0;
  if(h>BOOL_MAX_HITS || h<=0)    h= BOOL_MAX_HITS;
  if(m>BOOL_MAX_MATCH || m<=0)    m= BOOL_MAX_MATCH;
/*   if(m>h) m=h; */
  if(hpm<=0) hpm=1;
  if( more_record->and_tpl<0 ){
    total_sites=NULL;
    goto end;
  }
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
    error(A_ERR,"archWebBooleanQueryMore","Unable to allocate memory");
    return ERROR;
  }
  memset(*strings,0, sizeof(index_t *)*h);
  memset(*result,0, sizeof(query_result_t)*h);



/*
 *  for each OR[i]
 *  
 *    for each OR[i].AND[j]
 *       Get the list of starts[j]
 *         Get the andSITES[j][k]
 *    andSITES[j] = andSITES[j][0-k] (ored)
 *    orSITES[i] = Perform AND on andSITES[0-j]
 *  
 *  for each orSITES[i]
 *  
 *    for each OR[i].AND[j]
 *       create parentsPERSITE[j]
 *    GOODparents[i] = all parentsPERSITE[0-j] (ored)
 *  
 *    for each OR[i].NOT[j]
 *       create parentsPERSITEforNOT[j]
 *    BADparents[i] = all parentsPERSITEforNOT[0-j] (ored)
 *  
 *    GOODparents[i] = GOODparents[i] - BADparents[i]
 *  
 *  GOODparents = GOODparents[0-i] (ored)
 *  
 */

  curr_sites=NULL; curr_sites_found=0;
  prev_sites=NULL; prev_sites_found=0;
  total_sites=NULL; total_sites_found=0;
  sites = (host_table_index_t **)malloc(sizeof(host_table_index_t *)*qry.orn);
  num_sites = (int *)malloc(sizeof(int )*qry.orn);
  starts = (index_t ***)malloc(sizeof(index_t **)*(qry.orn));
  not_starts = (index_t ***)malloc(sizeof(index_t **)*(qry.orn));
  num_starts = (int **)malloc(sizeof(int *)*(qry.orn));
  num_not_starts = (int **)malloc(sizeof(int *)*(qry.orn));
  for( i=0; i<qry.orn; i++ ){
    sites[i] = NULL;
    sites_found = 0;
    prev_sites = NULL;
    prev_sites_found = 0;
    starts[i] = (index_t **)malloc(sizeof(index_t *)*qry.and_list[i].lnum);
    not_starts[i] = (index_t **)malloc(sizeof(index_t *)*qry.not_list[i].lnum);
    num_starts[i] = (int *)malloc(sizeof(int)*qry.and_list[i].lnum);
    num_not_starts[i] = (int *)malloc(sizeof(int)*qry.not_list[i].lnum);
    for( j=0; j<qry.and_list[i].lnum; j++ ){
      start=NULL;
      starts_found = 0;
      if( getStartsFromWord( qry.and_list[i].lwords[j], strhan, type,
                            case_sens, m, &starts_found, &start) == ERROR ){
        error(A_ERR,"archWebBooleanQueryMore","could not get starts for a single word.");
        /*   if( start) free(start); */
        return ERROR;
      }
      starts[i][j] = start;
      num_starts[i][j] = starts_found;
      /* accumulating site indecies for all starts of one word */
      if( orSitesForStarts( start, starts_found, start_db, m, &curr_sites_found,
                           &curr_sites, d_list, d_cnt ) == ERROR ){
        error(A_ERR,"archWebBooleanQueryMore","could not get sites for a single start.");
        if( start) free(start);
        if( curr_sites) free(curr_sites);
        return ERROR;
      }

      if( curr_sites_found <= 0 ){  /* AND will result in NULL anyway so why
                                     bother continuing! */
        if( curr_sites ) {
          free( curr_sites );
          curr_sites = NULL;
        }
        if( prev_sites ) {
          free( prev_sites );
          prev_sites = NULL;
        }
        prev_sites_found = 0;
        break;
      }

      if( j>0 && prev_sites != NULL ){
        sites[i] = NULL;
        sites_found = 0;
        archSiteIndexOper(&prev_sites, prev_sites_found,
                          &curr_sites, curr_sites_found, AND_OP,
                          &(sites[i]), &sites_found );
        if( curr_sites ) {
          free( curr_sites );
          curr_sites = NULL;
        }
        if( prev_sites ) {
          free( prev_sites );
        }
        prev_sites = sites[i];
        prev_sites_found = sites_found;
        sites_found = 0;
        sites[i] = NULL;        
      }else if( j==0 ){
        prev_sites = curr_sites;
        prev_sites_found = curr_sites_found;
      }else{
        prev_sites_found = 0;
        break;
      }
    }
    if(  prev_sites_found > 0 ){
      unique_hindex( &prev_sites, &prev_sites_found );
    }
    
    sites[i] = prev_sites;  prev_sites = NULL;
    num_sites[i] = prev_sites_found;  prev_sites_found = 0;

    if( num_sites[i]>0 && total_sites!=NULL && total_sites_found>0){
      prev_sites = total_sites;
      prev_sites_found = total_sites_found;
      total_sites = NULL;
      total_sites_found = 0;
      archSiteIndexOper(&prev_sites, prev_sites_found,
                        &(sites[i]), num_sites[i], OR_OP,
                        &(total_sites), &total_sites_found );
      /*   if( prev_sites ) { free( prev_sites ); prev_sites = NULL; }*/
    }else if( num_sites[i]>0 ){
      total_sites = sites[i];
      total_sites_found = num_sites[i];
    }

    for( j=0; j<qry.not_list[i].lnum; j++ ){
      /*
       *  Get the NOT list of starts for this j-th not_list word
       */
      start=NULL;
      starts_found = 0;
      if( getStartsFromWord( qry.not_list[i].lwords[j], strhan, type, case_sens, m, &starts_found, &start) == ERROR ){
        error(A_ERR,"archWebBooleanQueryMore","could not get starts for a single word.");
        /*   if( start) free(start); */
        return ERROR;
      }
      not_starts[i][j] = start;
      num_not_starts[i][j] = starts_found;
    }
  }
  

  /* total_sites carries all the sites containing possible hits */

  if( total_sites_found <= 0 ){
    error(A_INFO,"archWebBooleanQueryMore","no sites contain any hits for this boolean expression on webindex.");
    goto end;
  }

  tuples = (site_tuple_t **)malloc(sizeof(site_tuple_t *)*qry.orn);
  num_tuples = (int *)malloc(sizeof(int)*qry.orn);
  memset( tuples, 0, sizeof(site_tuple_t *)*qry.orn );
  memset( num_tuples, 0, sizeof(int)*qry.orn );
  orn = qry.orn;
  if( more_record->and_tpl<=qry.orn &&  more_record->and_tpl>=0 ){
    j=tmp_record.and_tpl=more_record->and_tpl;
    i=tmp_record.site_in_list=more_record->site_in_list;
    tmp_record.acc_res=more_record->acc_res;
  }else{
    j=tmp_record.and_tpl=0;
    i=tmp_record.site_in_list=0;
    tmp_record.acc_res=0;
  }

  for( stop=0; j<qry.orn; j++ ){  /* go through each and list */

    or_tuples=old_or_tuples=new_or_tuples=NULL;
    num_or_tuples=num_old_or_tuples=num_new_or_tuples=0;
    /*
     *  assuming "i" has been set previously from the more_record
     */
    for(; i<num_sites[j]; i++ ){  /* process only the sites of the i-th
                                     and_list , those tuples should be
                                     ORed with the rest */

      and_tuples = old_and_tuples = new_and_tuples = NULL;
      num_and_tuples = num_old_and_tuples = num_new_and_tuples = 0;

      old_not_tuples = new_not_tuples = not_tuples = NULL;
      num_old_not_tuples = num_new_not_tuples = num_not_tuples = 0;

      for(k=0, l=0; ;
          k++, l++ ){
        /*
         *  go through each start of the j-th word in the i-th and_group
         */

        andstarts_tuples = notstarts_tuples = NULL;
        num_andstarts_tuples = num_notstarts_tuples = 0;

        if( k>=qry.and_list[j].lnum && l>=qry.not_list[j].lnum ){
          break;  /* fix this */
        }

        if( k>=qry.and_list[j].lnum ){
          andstarts = NULL; num_andstarts = 0;
        }else{
          andstarts = starts[j][k]; num_andstarts = num_starts[j][k];
        }

        if( l>=qry.not_list[j].lnum ){
          notstarts = NULL; num_notstarts = 0;
        }else{
          notstarts = not_starts[j][l]; num_notstarts = num_not_starts[j][l];
        }

        /*
         *  OR all the parents for one all starts of one word of one site
         */

        if( findParentsForAllStarts(andstarts, num_andstarts,
                                    notstarts, num_notstarts,
                                    sites[j][i],
                                    &(andstarts_tuples), &num_andstarts_tuples,
                                    &(notstarts_tuples), &num_notstarts_tuples)==ERROR ){
          error(A_ERR,"archWebBooleanQueryMore",
                "couldn't construct parent tuples for AND and/or NOT lists");
        }

        if( andstarts!=NULL ){
          if( num_andstarts_tuples <= 0 ){
            error(A_ERR,"archWebBooleanQueryMore",
                  "no parents could be constructed. No results");
            num_and_tuples = 0;
            k = qry.and_list[j].lnum;
            /* no need to AND anymore */
          }else if( k==0){
            and_tuples = andstarts_tuples;
            num_and_tuples = num_andstarts_tuples;
          }else{
            old_and_tuples = and_tuples;
            num_old_and_tuples = num_and_tuples;
            new_and_tuples = andstarts_tuples;
            num_new_and_tuples = num_andstarts_tuples;
            if( archTupleOper(&old_and_tuples, num_old_and_tuples,
                              &new_and_tuples, num_new_and_tuples,
                              AND_OP, &and_tuples, &num_and_tuples) == ERROR ){
              error(A_ERR,"archWebBooleanQueryMore",
                    "couldn't perform Tuple operation");
              goto end;
            }
            if( old_and_tuples!=NULL ){
              free(old_and_tuples);            old_and_tuples = NULL;
            }
            if( new_and_tuples!=NULL ){
              free(new_and_tuples);            new_and_tuples = NULL;
            }
          }
        }

        if( num_notstarts_tuples == 0 ) continue;
        if( l==0 || num_not_tuples==0 ){
          not_tuples = notstarts_tuples;
          num_not_tuples = num_notstarts_tuples;
        }else{
          old_not_tuples = not_tuples;
          num_old_not_tuples = num_not_tuples;
          new_not_tuples = notstarts_tuples;
          num_new_not_tuples = num_notstarts_tuples;
          if( archTupleOper(&old_not_tuples, num_old_not_tuples,
                            &new_not_tuples, num_new_not_tuples,
                            OR_OP, &not_tuples, &num_not_tuples) == ERROR ){
            error(A_ERR,"archWebBooleanQueryMore","couldn't perform Tuple operation");
            goto end;
          }
          if( old_not_tuples!=NULL ){
            free(old_not_tuples);            old_not_tuples = NULL;
          }
          if( new_not_tuples!=NULL ){
            free(new_not_tuples);            new_not_tuples = NULL;
          }
        }
      } /* for k=0,l=0 k++,l++ */
      

      if( num_and_tuples <= 0 ){ /* no need to OR or NOT this */
        continue;
      }
      /* ifdef #2 was here */

      if( num_not_tuples>0){
        old_and_tuples = and_tuples;
        num_old_and_tuples = num_and_tuples;
        if( archTupleOper(&old_and_tuples, num_old_and_tuples,
                          &not_tuples, num_not_tuples,
                          NOT_OP, &and_tuples, &num_and_tuples) == ERROR ){
          error(A_ERR,"archWebBooleanQueryMore","couldn't perform Tuple operation");
          goto end;
        }
      }

      if( num_and_tuples <= 0 ){ /* no need to OR this */
        continue;
      }

      if( i==0 || num_or_tuples==0 ){
        or_tuples = and_tuples;
        num_or_tuples = num_and_tuples;
      }else{
        old_or_tuples = or_tuples;
        num_old_or_tuples = num_or_tuples;
        new_or_tuples = and_tuples;
        num_new_or_tuples = num_and_tuples;
        if( archTupleOper(&old_or_tuples, num_old_or_tuples,
                          &new_or_tuples, num_new_or_tuples,
                          OR_OP, &or_tuples, &num_or_tuples) == ERROR ){
          error(A_ERR,"archWebBooleanQueryMore","couldn't perform Tuple operation");
          goto end;
        }
        if( old_or_tuples!=NULL ){
          free(old_or_tuples);
          old_or_tuples = NULL;
        }
        if( new_or_tuples!=NULL ){
          free(new_or_tuples);
          new_or_tuples = NULL;
        }
      }

      /*
       *  check if num_and_tuples>maxhits if yes then we must stop and save
       *  other information.
       *  
       *  The following holds position of
       *  
       *   j= the number of and/not tuples we're at.
       *  
       *   i= the site in the site list site[j] we're at (each tuple has its
       *      own site list site[j]).
       */

      /*
       *  I do the check for maxhits here since the above guarantees a VALID
       *  result array of tuples.
       */

      if( (num_or_tuples+acc_num_tuples) >= (h+more_record->acc_res) ){
        more_record->and_tpl = j;
        more_record->site_in_list = i;
        if( j!=tmp_record.and_tpl ){
          more_record->acc_res = h-(acc_num_tuples-more_record->acc_res);
        }else if( num_or_tuples == (h+more_record->acc_res) ){
          more_record->site_in_list++;
          more_record->acc_res = 0;
        }else  more_record->acc_res = h+more_record->acc_res;
        /*
         *  the results that will be printed (where we have stopped and should
         *  continue from).  I think I prefer to just print the rest and
         *  force the users to accept it :)
         */
        stop = 1;
        break;
      }
      /*  end of maxhits checking */
    } /* end of loop going through the sites of one and-group */
    if( sites[j]!=NULL ){
      free( sites[j] );  sites[j]=NULL;
    }

    /*
     *  Here I SHOULD do an OR operation to get rid of redundancy we'll see
     *  when I will get to do this 961024
     */
    tuples[j] = or_tuples;
    num_tuples[j] = num_or_tuples;
    acc_num_tuples+=num_or_tuples;
    if( stop ){  /* we have reached maxhits */
      orn = j+1;
      break;
    }
    i=0;
    /*
     *  since we will be going through a new list of sites and previously "i"
     *  started from where it left off in the old search.
     */
  } /* end of loop going through each and-group */

  if( sites!=NULL ){
    free( sites ); sites=NULL;
  }

  /* building the results records */

  i=tmp_record.and_tpl;
  j=tmp_record.acc_res;

  for( k=0; i<orn && k<h; i++ ){
    for( ; j<num_tuples[i] && k<h ; j++){
      res = &(*result)[k];
      strs = &((*strings)[k]);
      archGetResultFromTuple( tuples[i][j], res, strs, &num_results, format,
                             strhan, path, expath, path_rel, path_items);
      k+=num_results;
    }
    j=0;
    if( tuples[i]!=NULL ){
      free( tuples[i] ); tuples[i]=NULL;
    }
  }
  if( tuples!=NULL ){
    free( tuples ); tuples=NULL;
  }
  if(!stop){
    more_record->and_tpl = -1;
  }
 end:

  *tot_hits = k;
  if( total_sites ) free(total_sites);
  return A_OK;

}

extern status_t archBooleanQueryMore(strhan, qry, type, case_sens,h,m, hpm,
                                     path, expath, path_rel, path_items,
                                     d_list, d_cnt, db, start_db, verbose,
                                     result, strings, tot_hits, more_record,
                                     func_name, no_sites_limit, format)
struct arch_stridx_handle *strhan;
bool_query_t qry;
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
boolean_return_t *more_record;
char *(*func_name)();
int no_sites_limit;
int format;
{
  /* File information */

  host_table_index_t inds[MAX_LIST_SIZE], hin;
  int indslen,j,i,k,ind;
  hostname_t hnm;
  ip_addr_t ipaddr = (ip_addr_t)(0);
  query_result_t *res;
  index_t **strs, *res_list;
  char *strres;
  int flag;
  int port, site_hits, ressz;
  int at_least_in_one_site;
  start_return_t site_record;

  int num_hits, num_matches, num_hits_pm;
  int max_hits_reached, where;
  int stop, max;
  index_t *start = NULL;

  hnm[0] = '\0';
  stop=ind=max=port=max_hits_reached=where=0;
  j=i=k=0;
  *tot_hits = 0;
  num_hits = 0;
  num_matches = 0;
  if(h>BOOL_MAX_HITS || h<=0)    h= BOOL_MAX_HITS;
  /*  if(m>BOOL_MAX_MATCH || m<=0) */   m= BOOL_MAX_MATCH;
  /*  if(m>h) m=h; */
  if(hpm<=0) hpm=1;
  if( more_record->and_tpl<0 ){
    *result=NULL;
    *strings=NULL;
    goto end;
  }

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
    error(A_ERR,"archBooleanQueryMore","Unable to allocate memory");
    return ERROR;
  }
  memset(*strings,0, sizeof(index_t *)*h);
  memset(*result,0, sizeof(query_result_t)*h);

  max_hits_reached = 0;
  res_list = NULL;
  ressz=0;

  if( more_record->and_tpl<qry.orn && more_record->and_tpl>=0 ){
    where=more_record->and_tpl;
  }else{
    where=0;
  }
  j = more_record->acc_res;
  while(!max_hits_reached){
    if(archGetBooleanStarts(strhan, qry, type, case_sens, h,
                            verbose, num_hits, &where,
                            more_record, &res_list, &ressz)==ERROR ){
      error(A_ERR,"archBooleanQueryMore",
            "Could not get starts for all and tuples");
      return ERROR;
    }

    if( ressz<=0 ){
      goto end;
    }

    start = res_list;
    flag=1;
    for ( ind=0 ; flag &&  j < ressz ; j++,ind++)  {
      /* Go through every found string */

      get_index_start_dbs(start_db, start[j], inds, &indslen);

      if( indslen <= 0 ){
        if( !archGetString( strhan,  start[j], &strres) ){
          if( verbose ){
            error(A_INFO,"archBooleanQueryMore","StrIndex %ld is not in the strings database.",start[j]);
          }
        }else{
          if( verbose ){
            error(A_INFO,"archBooleanQueryMore","String %s is not in any site in the database.",strres);
          }
        }
        free(strres);
        continue;
      }

      /* For every found string .. go through each site file */

      at_least_in_one_site = 0;
      num_hits_pm = 0;
      if( ind==0 ) k = more_record->site_in_list;
      else k = 0;
      for ( ; k < indslen ; k++ ) {
        ipaddr = (ip_addr_t)(0);
        port = 0;
        hin = inds[k];

        if( host_table_find( &ipaddr, hnm, &port, &hin) == ERROR ) {
          error(A_ERR,"archBooleanQueryMore","Could not find host in host-table. start/host dbase corrupt.");
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
        memset(&(site_record),'\0',sizeof(start_return_t));
        site_record.stop = -1;
/*        max = (hpm-num_hits_pm < h-num_hits) ? hpm-num_hits_pm : h-num_hits; */
        max = h-num_hits;
        
        if( format == I_FORMAT_KEYS && db != I_ANONFTP_DB){
          if( getStringsFromStart( start[j], strhan, &site_record, db,
                                  (ip_addr_t)ipaddr, port, &site_hits, max,
                                  path, expath, path_rel, path_items,
                                  res, strs )==ERROR ){
            error(A_ERR,"archBooleanQueryMore","Could not reconstruct the string from site file. site dbase corrupt.");
            continue;
          }
        }else{
          if( getResultFromStart( start[j], strhan, &site_record, db,
                                 (ip_addr_t)ipaddr, port, &site_hits, max,
                                 path, expath, path_rel, path_items, res )==ERROR ){
            error(A_ERR,"archBooleanQueryMore","Could not reconstruct the string from site file. site dbase corrupt.");
            continue;
          }
        }

        if( site_hits > 0 ){
          at_least_in_one_site = 1;
          /*     num_hits_pm += site_hits;*/ /*Increase the numbers of hits for this match */
          num_hits += site_hits; /* Increase the total number of hits */
        }

        if ( num_hits >= h /* || num_hits_pm >= hpm */ ){
          more_record->site_in_list = k;
          more_record->acc_res = j;
          k = indslen+1;        /* that will stop the loop */
          max_hits_reached=1;
        }
      } /* Go through sites for one string */

      if ( at_least_in_one_site ) 
      num_matches++;            /* increase the match hit */      

      if ( num_hits >= h )
      flag = 0;
      
    } /* Go through uniqe strings */
    j=0;

    more_record->and_tpl = 0;
    if( (!max_hits_reached && (where+1)>=qry.orn) ){
      max_hits_reached = 1;
      more_record->and_tpl = -1;
    }else if(!max_hits_reached){ /* if "last" the loop will automatically stop */
      where = i+1;
      more_record->acc_res = j;
    }
  } /* while (max_hits_reached!=0) */

  /* process the list of index  */

 end:
  *tot_hits = num_hits;
  if(start) { free(start); }
  if(res_list!=start) { free(res_list); res_list=NULL; }
  return(A_OK);

}

extern status_t strOnlyBooleanQuery(strhan, qry, type, case_sens,h,
                                    verbose, more_record,
                                    format)
struct arch_stridx_handle *strhan;
bool_query_t qry;
int type;                     /* type of search */
int case_sens;                /* case_sens */
int h;                        /* maximum hits (all in all) */
int verbose;
boolean_return_t *more_record;
int format;
{
  /* File information */

  int j,i,k;
  hostname_t hnm;
  char *res;
  index_t *res_list;
  int flag,num_hits;
  int ressz;

  int max_hits_reached, where;
  index_t *start = NULL;

  hnm[0] = '\0';
  max_hits_reached=where=0;
  j=i=k=0;
  num_hits = 0;
  if(h>BOOL_MAX_HITS || h<=0)    h= BOOL_MAX_HITS;
  /*  if(m>BOOL_MAX_MATCH || m<=0) m= BOOL_MAX_MATCH; */
  /*  if(m>h) m=h; */
  /*  if(hpm<=0) hpm=1; */

  /*
   *  If Maximum Strings Matched is higher than max hits then we might lose
   *  some strings in the following search (More Results). When we call
   *  archSearch<*> with Maxmatch as a parameter, it will return more strings
   *  than maxhits and when we call archGetMore the following call will miss
   *  all the extra strings from the previous search.  I will have to look for
   *  a better solution.
   */

  if(format){
    fprintf(stdout,"Content-type: text/html\n\n");
    fprintf(stdout,"<HTML><h2>Archie Results</h2>\n");
  }else{
/*    fprintf(stdout,"HITS=%d\n",ressz);
    fprintf(stdout,"%s=0\n", PLAIN_START_STRINGS);*/
  }
/*
   if( more_record->and_tpl<0 ){
    goto end;
  }
*/
  max_hits_reached = 0;
  res_list = NULL;
  ressz=0;

  if( more_record->and_tpl<qry.orn && more_record->and_tpl>=0 ){
    where=more_record->and_tpl;
  }else{
    where=0;
  }

  if( more_record->acc_res>0 ) j = more_record->acc_res;
  while(!max_hits_reached){
    if(archGetBooleanStarts(strhan, qry, type, case_sens, h,
                            verbose, num_hits, &where,
                            more_record, &res_list, &ressz)==ERROR ){
      error(A_ERR,"strOnlyBooleanQuery",
            "Could not get starts for all and tuples");
      return ERROR;
    }

    if(format){
      if(!ressz){
        fprintf(stdout,"<P><H3>No hits for your query. Please check your query's paramters and try again</H3>");
        return(A_OK);
      }
      fprintf(stdout,"<h4>Found %d Strings</h4><B>\n",ressz);
    }else{
      fprintf(stdout,"HITS=%d\n",ressz);
      fprintf(stdout,"%s=0\n", PLAIN_START_STRINGS);
    }


    error(A_INFO,"RESULT:","Found %d Hits",ressz);

    if( ressz<=0 ){
      goto end;
    }
    num_hits+=ressz;
    start = res_list;

    flag=1;
    for ( ; flag &&  j < ressz ; j++)  {
      if( !archGetString( strhan,  start[j], &res) ){
        error(A_ERR,"strOnlyQuery","could not get string from start");
        continue;
      }
      if(format){
        fprintf(stdout,"<i>(%d)</i>%s<P>\n",j+1,res);
      }else{
        fprintf(stdout,"%d=%s\n",(j+1),res);
      }
      free(res); res = NULL;
    } /* Go through uniqe strings */

/*    if( j>=h ) max_hits_reached=1; */

    more_record->and_tpl = 0;
    if( (!max_hits_reached && (where+1)>=qry.orn) ){
      max_hits_reached = 1;
      more_record->and_tpl = -1;
    }else if( !max_hits_reached){ /* if "last" the loop will automatically stop */
      where = i+1;
    }
    more_record->acc_res = j;
    j=0;
  } /* while (max_hits_reached!=0) */

  /* process the list of index  */

 end:
  if(format)  fprintf(stdout,"</B></HTML>");
  else   fprintf(stdout,"%s=0\n",PLAIN_END_STRINGS);
  if(start) { free(start); }
  if(res_list!=start) { free(res_list); res_list=NULL; }
  return(A_OK);

}

