#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include "protos.h" 

#include "typedef.h"

#include "web.h"
#include "sub_header.h"
#include "parser_file.h"

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
#include "site_file.h"
#include "core_entry.h"
#include "debug.h"
#include "master.h"
#include "excerpt.h"

#include "archstridx.h"

typedef struct {
  int rec_no;
  long s_sub;
  urlname_t local_url;  /* local_url of the form /port/url */
  sub_header_t shrec;
} sub_header_info_t;

int rec_no = 0;

extern date_time_t retrieve_time;

static int excerpt_recno = 0;

static sub_header_info_t **table = NULL;
static int table_num = 0;
static int table_max = 0;

/*static sub_header_info_t external_urls; */

void output(pent,str,outfp)
parser_entry_t *pent;
char *str;
FILE *outfp;
{
  char b[5] = "    ";

/*  
  int i;
  
  printf("\t--"); 
  for ( i = 0; i < pent->slen; i++ )
    printf("%c",str[i]);
  printf("--\n");
*/
  
  fwrite((char*)pent,sizeof(parser_entry_t),1,outfp);
  fwrite(str,sizeof(char),pent->slen,outfp);
  
  if ( (sizeof(parser_entry_t) + pent->slen) & 0x03) 
    fwrite(b,sizeof(char),4- ((sizeof(parser_entry_t) + pent->slen) & 0x03),outfp ) ;
  
}


static int shi_cmpr(a,b)
sub_header_info_t **a,**b;
{
  return strcasecmp((*a)->local_url,(*b)->local_url);
}

static void free_info_table()
{
  int i; 
  for ( i = 0; i < table_num; i++ ) {
    free(table[i]);
  }
  table_num = 0;
}


static sub_header_info_t **add_table(table, table_num, table_max, shi)
sub_header_info_t **table;
int *table_num,*table_max;
sub_header_info_t *shi;
{

   sub_header_info_t **tmp;

   if ( *table_num >= *table_max-1 ) {

      *table_max += 10;
      tmp = (sub_header_info_t **)malloc(sizeof(sub_header_info_t*)**table_max);
      if ( tmp == NULL ) {
        error(A_ERR, "add_table", "Unable to malloc memory");
        return table;
      }
      memcpy(tmp,table,sizeof(sub_header_info_t*)**table_num);
      if ( *table_num ) {
        free(table);
      }
      table = tmp;
   }

   table[(*table_num)] = shi;

   (*table_num)++;
   return table;
}



static int get_info(fp)
FILE *fp;
{
  sub_header_t shrec;
  long s_sub;
  sub_header_info_t *shi;
  int i,j;

  init_sub_header();
  s_sub = ftell(fp);
  while ( read_sub_header(fp,&shrec,(u32*)0,0,0) == A_OK ) {
    int size,fsize;

    s_sub = ftell(fp);    
    if(HDR_GET_SIZE_ENTRY(shrec.sub_header_flags))
      size = shrec.size;
    
    if(HDR_GET_FSIZE_ENTRY(shrec.sub_header_flags))
      fsize = shrec.fsize;


    if ( HDR_GET_STATE_ENTRY(shrec.sub_header_flags)) {
      if ( strcasecmp(shrec.state,"extern") == 0 ) {

        s_sub = ftell(fp);
        
        continue;
      }
      if (strcasecmp(shrec.state,"New") == 0 ) {

        fseek(fp,(long)fsize,1);
      }
    }

      
    shi = (sub_header_info_t*)malloc(sizeof(sub_header_info_t));
    memset(shi,0, sizeof(sub_header_info_t));
    shi->s_sub = s_sub;
    memcpy(&(shi->shrec),&shrec,sizeof(sub_header_t));
    
    sprintf(shi->local_url,"/%s/%d%s",shrec.server,(int)shrec.port,shrec.local_url);
    table = add_table(table, &table_num, &table_max,shi);

  }

  qsort(table,table_num,sizeof(sub_header_info_t*),shi_cmpr);


  for ( i = 1, j = 0; i < table_num; i++ ) {

    if ( strcmp(table[i]->local_url , table[j]->local_url ) == 0 ) {
      free(table[i]);
      table[i] = NULL;
    }
    else {
      j++;
      if ( i != j ) {
        table[j] = table[i];
        table[i] = NULL;
      }
    }
  }
  
  if ( j > 0 && j != table_num-1 ) {
    error(A_INFO,"get_info","Found duplicate urls in the parse file");
    table_num = j+1;
  }

  
  return 1; /* added return - wheelan */
}

static char *find_char(s,c,n)
char *s,c;
int n;
{
  char *t;
  t = s;
  while(*t!= '\0') {
    if ( *t == c) {
     n--;
     if ( !n )
       return t;
   }
    t++;
  }
  return t;

}


static void output_site_rec(site,ex_in, ex_out, site_rec_no, max_site_recno,outfp,strhan)
file_info_t *site;
FILE *ex_in, *ex_out;
int site_rec_no;
int max_site_recno;
FILE *outfp;
struct arch_stridx_handle *strhan;
{
  full_site_entry_t *curr_ptr;
  char *res;
  parser_entry_t pent;
  int excerpt_rec_no;
  excerpt_t excerpt_rec;

  if ( site == NULL ) {
    error(A_INFO,"output_site_rec", "No site ... ");
    return;
  }

  if ( ex_in == NULL ) {
    error(A_WARN, "output_site_rec", "No excerpt site ... ");
    return;
  }
  
  curr_ptr = (full_site_entry_t *) site -> ptr + (site_rec_no);
  if (  CSE_IS_DOC((*curr_ptr))  && ex_in != NULL ) { 
    excerpt_rec_no = curr_ptr->core.entry.perms;
    fseek(ex_in, excerpt_rec_no*sizeof(excerpt_t), 0);
    fread((char*)&excerpt_rec,sizeof(excerpt_t),1,ex_in);
    fwrite((char*)&excerpt_rec,sizeof(excerpt_t),1,ex_out);
  }

  
  for ( site_rec_no++;  site_rec_no < max_site_recno;  site_rec_no++ ) {
    
    pent.core.size = 0;
    pent.core.date = 0;
    pent.core.perms = 0;
    pent.core.flags = 0;
    pent.core.parent_idx = 0;
    pent.core.child_idx  = 0;
    
    curr_ptr = (full_site_entry_t *) site -> ptr + (site_rec_no);
    if ( ! CSE_IS_KEY((*curr_ptr)) ) { /* Done ! */
      return;
    }

    pent.core.flags = curr_ptr->flags;
    
    CSE_STORE_WEIGHT((pent.core),(curr_ptr->core.kwrd.weight));

    archGetString( strhan,  curr_ptr->strt_1, &res);
    pent.slen = strlen(res);
    output(pent,res,outfp);
    
    rec_no++;
  }
}


static void get_site_rec(site, site_rec_no, pent)
file_info_t *site;
int site_rec_no;
parser_entry_t *pent;
{
  full_site_entry_t *curr_ptr;

  if ( site == NULL ) {
    error(A_INFO,"output_site_rec", "No site ... ");
    return;
  }

  curr_ptr = (full_site_entry_t *) site -> ptr + (site_rec_no);

  pent->core.size = curr_ptr->core.entry.size;
  pent->core.date = curr_ptr->core.entry.date;
  pent->core.rdate = curr_ptr->core.entry.rdate;
  pent->core.perms = curr_ptr->core.entry.perms;
  
  pent->core.flags = curr_ptr->flags;

}



static void process_list(table,beg,end,which,infp,outfp, ex_in, ex_out, site, strhan, max_site_recno, parent)
sub_header_info_t **table;
int beg;
int end,which;
FILE *infp,*outfp, *ex_in, *ex_out;
file_info_t *site;
struct arch_stridx_handle *strhan;
int max_site_recno;
int parent;
{
  int i,l;
  char *t,*s = NULL;
  int first;
  char buff[1024];
  char bb[1024];
  char cse_buff[100],*tmp;
  parser_entry_t pent;
  int disp,init_rec_no;
  
  memset(&pent,0,sizeof(parser_entry_t));

  if ( strlen(table[beg]->shrec.local_url) != 1 &&
      table[beg]->local_url[strlen(table[beg]->local_url)-1] == '/' ) {
    table[beg]->local_url[strlen(table[beg]->local_url)-1] = '\0';
  }
  
  /*
     if which == 2 then we are looking at the server name
     if which == 3 then we are looking at the port number
     */

  t = find_char(table[beg]->local_url,'/',which);
  if ( *t == '\0' ) {
    if ( beg == end )
    return;
  }
  else 
  t++;

  /* output the directory like listing */

  l = t-table[beg]->local_url;
#if 0
  if ( beg == end ) {           /* Check if we have more than one sub */
    s = find_char(table[beg]->local_url+l,'/',1);
    if ( *s == '\0' ) 
    return;
  }
#endif
  strncpy(bb,table[beg]->local_url,l);
  bb[l] = '\0';

  if ( which > 1 ) {
    tmp = find_char(table[beg]->local_url,'/',which  - 1);
    if ( t == tmp ) {
      strcpy(cse_buff,tmp+1);
    }
    else
    if ( *t == '\0' && *(t-1) != '/' ) {
      strncpy(cse_buff,tmp+1,t-tmp-1);
      cse_buff[t-tmp-1] = '\0';
    }
    else {
      strncpy(cse_buff,tmp+1,t-tmp-2);
      cse_buff[t-tmp-2] = '\0';
    }
  }

  pent.core.size = 0;
  pent.core.date = 0;
  pent.core.perms = 0;
  pent.core.flags = 0;
  pent.core.parent_idx = 0;
  pent.core.child_idx  = 0;

  

  switch ( which ) {
  case 1:
    break;
    
  case 2:                       /* Found the '/' after the server */
    CSE_SET_NAME(pent.core);
    break;
    
  case 3:                       /* Found the '/' after the port */
    CSE_SET_PORT(pent.core);
    break;
    
  default:
    CSE_SET_SUBPATH(pent.core);
    break;
  }
  
  if ( which != 1 ) {
    if ( *t == '\0' ) {
      pent.slen = t-tmp-1;
    }
    else  {
      pent.slen = t-tmp-2;
    }
    
    pent.core.parent_idx = parent;
    output(&pent,cse_buff,outfp);
/*    printf("%s: (%d,%d)\n",bb,rec_no,parent); */ /* Directory def */ 
    rec_no++;
  }

         
  /* Required as if the  first element of the sequence
     is something like /a/b/c and the next is /a/b/c/d then
     the table[beg]->local_url+l points to the '\0' after c
     
     */

  if ( *(table[beg]->local_url+l) == '\0' && l > 0 &&
      *(table[beg]->local_url+l-1) != '/' ) {
    disp = 1;
  }
  else {
    disp = 0; 
  }

  init_rec_no = rec_no;

  for ( i = beg; i <= end; i++ ) {
    
    memset(&pent,0,sizeof(parser_entry_t));
    
    if ( *(table[i]->local_url+l) == '\0'  ) {

#if 0
      if ( table[i]->shrec.size ) {
        CSE_SET_DOC(pent.core);
        pent.core.size = table[i]->shrec.size;
        pent.core.date = table[i]->shrec.date;
        pent.core.perms = excerpt_recno++;

        /* Output keys */
        if ( HDR_GET_RECNO_ENTRY(table[i]->shrec.sub_header_flags) ) {
          output_site_rec(site, ex_in, ex_out, table[i]->shrec.recno,
                          max_site_recno, outfp,strhan );
        }
        else {
          parse_rec(infp,outfp, ex_out, &(table[i]->shrec),table[i]->s_sub,strhan);
          /*        printf("\t\tkeys\n"); */
        }
      }
#endif
    }
    else {
      s = find_char(table[i]->local_url+l+disp,'/',1);
      if ( s-(table[i]->local_url+l+disp) > 1 ) {

        strncpy(buff,table[i]->local_url+l+disp,s-(table[i]->local_url+l+disp));
        buff[s-(table[i]->local_url+l+disp)] = '\0';

        if ( strcmp(buff,bb) ) {
          pent.core.flags = 0;
          if ( which > 2 ) {
            if ( *s == '/' ) {
              CSE_SET_DIR(pent.core);
              if ( *(s+1) == '\0' )
                CSE_SET_DOC(pent.core);
            }
            else {
/*              if ( table[i]->shrec.size != 0 || table[i]->shrec.recno != 0 )  */
              if ( strcasecmp(table[i]->shrec.state, "New") == 0 ||
                   strcasecmp(table[i]->shrec.state, "Old") == 0 ) 
                CSE_SET_DOC(pent.core);
            }
          }
          else {
            CSE_SET_DIR(pent.core);
              if ( *(s+1) == '\0' )
                CSE_SET_DOC(pent.core);            
          }

          pent.core.size = table[i]->shrec.size;
          pent.core.date = table[i]->shrec.date;
          
          if ( CSE_IS_DOC(pent.core) ) {  /* This is the record of the exerpt */

            pent.core.perms = excerpt_recno++;
            pent.core.rdate = retrieve_time;
            if ( HDR_GET_RECNO_ENTRY(table[i]->shrec.sub_header_flags) ) {
              parser_entry_t tmp_pent;
              
              get_site_rec(site, table[i]->shrec.recno, &tmp_pent);
              pent.core.size = tmp_pent.core.size;
              pent.core.date = tmp_pent.core.date;
              pent.core.rdate = tmp_pent.core.rdate;
            }

          }
          
          pent.slen= strlen(buff);
          pent.core.parent_idx = parent;
          table[i]->rec_no = rec_no;
          
          output(&pent,buff,outfp);
          rec_no++;

/*          printf("\t%s (%d,%d) excerpt_recno = %d\n",buff,rec_no-1,(int)pent.core.parent_idx, excerpt_recno); */ /* File element */


          /* Need to output keys */
/*          if ( which > 2 && *s != '/' ) { */
          if ( CSE_IS_DOC(pent.core) ) {
            if ( HDR_GET_RECNO_ENTRY(table[i]->shrec.sub_header_flags) ) {
              output_site_rec(site, ex_in, ex_out, table[i]->shrec.recno,
                              max_site_recno, outfp,strhan );
            }
            else {
              parse_rec(infp,outfp, ex_out, &(table[i]->shrec),table[i]->s_sub,strhan);
/*              printf("\t\tkeys\n"); */
            }
          }
          strcpy(bb,buff);
        }
      }
    }
  }

/*  printf("\n"); */
  /* Recurse */

  if ( beg == end ) {           /* Check if we have more than one sub */
    s = find_char(table[beg]->local_url+l+disp,'/',1);
    if ( *s == '/' && *(s+1) != '\0' ) {
      process_list(table,beg,end,which+1,infp,outfp,ex_in, ex_out,site, strhan,
                   max_site_recno, table[beg]->rec_no);
      return;
    }
  }

  first = beg;
  s = find_char(table[beg]->local_url+l+disp,'/',1);
  strncpy(bb,table[beg]->local_url+l+disp,s-(table[beg]->local_url+l+disp));
  bb[s-(table[beg]->local_url+l+disp)] = '\0';
  
  for ( i = beg+1; i <= end; i++ ) {
    s = find_char(table[i]->local_url+l+disp,'/',1);
    strncpy(buff,table[i]->local_url+l+disp,s-(table[i]->local_url+l+disp));
    buff[s-(table[i]->local_url+l+disp)] = '\0';
    if ( strcmp(buff,bb) ) {
      if ( bb[0] != '\0' ) {
        process_list(table,first,i-1,which+1,infp,outfp, ex_in, ex_out, site,
                     strhan, max_site_recno, table[first]->rec_no);
      } 
      first = i;
      strcpy(bb,buff);
    }
  }

  if ( beg != end  )
  process_list(table,first,end,which+1,infp,outfp,ex_in, ex_out, site,strhan,
               max_site_recno, table[first]->rec_no);

}




int recurse(infp,outfp, ex_out, no_recs,ip, port_no)
FILE *infp, *outfp, *ex_out;
int *no_recs;
ip_addr_t ip;
int port_no;
{
  struct arch_stridx_handle *strhan = NULL;
  char *dbdir = NULL;
  pathname_t files_database_dir;
  pathname_t excerpt_name;

  file_info_t *site_info = NULL;
  FILE *ex_in;
  int act_size = 0;

  files_database_dir[0] = excerpt_name[0] = '\0';
  
  if((dbdir = (char *)set_wfiles_db_dir(files_database_dir)) == (char *) NULL){
    error(A_WARN,"recurse", "Error while trying to set webindex database directory");
    
  }


  if ( !(strhan = archNewStrIdx() ) ) {
    error(A_WARN,"recurse", "Could not create string handler");
    return ERROR;
  }
  
  if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_SEARCH  ) )
  {
    error(A_WARN,"recurse", "Error while trying to set webindex index database");
/*    return ERROR; */
  }
  
  
  if(access((char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port_no), F_OK) < 0) {
    error(A_INFO,"recurse", "The Site is not present");
    ex_in = NULL;
  }
  else {
    site_info = create_finfo();    
    strcpy(site_info -> filename, (char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port_no));

    /* Open current file */
    if(open_file(site_info, O_RDONLY) == ERROR){
      error(A_ERR, "recurse", "Cannot open site %s, assuming not present",
            site_info->filename);
      destroy_finfo(site_info);
    }
    /* mmap it */
    if(mmap_file(site_info, O_RDONLY) == ERROR) {
      error(A_WARN, "recurse", "Cannot mmap site %s, assuming not present",
            site_info->filename);
      destroy_finfo(site_info);
    }
    act_size = site_info -> size / sizeof(full_site_entry_t);

    sprintf(excerpt_name, "%s.excerpt",site_info -> filename);
    ex_in = fopen(excerpt_name, "r");    
  }



  rec_no = 0;
  if ( get_info(infp) == 0 ) {
    return ERROR;
  }

  if ( table_num <= 0  ) {
    error(A_ERR,"recurse", "There is no data to be parsed");
    return ERROR;
  }
  
  process_list(table,0,table_num-1,1,infp,outfp,ex_in, ex_out, site_info,strhan,act_size, -1);

  *no_recs = rec_no;

  free_info_table();
  
  archCloseStrIdx(strhan);
  archFreeStrIdx(&strhan);

  if ( site_info != NULL ) {
    munmap_file(site_info);
    destroy_finfo(site_info);
  }
  
  return A_OK;
}













