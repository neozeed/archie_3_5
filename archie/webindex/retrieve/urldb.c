#include <stdio.h>
#include <unistd.h>

#include <ansi_compat.h>

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
#include "site_file.h"
#include "core_entry.h"
#include "debug.h"
#include "master.h"

#define NULL (0)


#include "typedef.h"
#include "url.h"
#include "urldb.h"
#include "core_entry.h"
#include "archstridx.h"
 
extern char *http_method;

static int table_num = 0;
static int table_max = 0;
static urlEntry **table = (urlEntry**)NULL;

static int ext_table_num = 0;
static int ext_table_max = 0;
static urlEntry **ext_table = (urlEntry**)NULL;

static int store_url PROTO((urlEntry *, int));


/* static char db_file[1024]; 
static char db_file1[1024];
static char db_file2[1024]; */

static int table_index =  0;
static int ext_table_index = 0;

static int orig_table_num;


static file_info_t *urldb = NULL;

static int table_cmp(a,b)
  urlEntry **a, **b;
{
  return (*a)->date - (*b)->date;
}

static urlEntry **addTable(t, t_num, t_max, ue)
urlEntry **t;
int *t_num,*t_max;
urlEntry *ue;
{

  urlEntry **tmp;

  if ( *t_num >= *t_max-1 ) {

    *t_max += 10;
    tmp = (urlEntry **)malloc(sizeof(urlEntry*)**t_max);
    if ( tmp == NULL ) {
      error(A_ERR,"addTable","Unable to malloc");
      return t;
    }
    memcpy(tmp,t,sizeof(urlEntry*)**t_num);
    if ( *t_num ) {
      free(t);
    }
    t = tmp;
  }

  t[(*t_num)] = ue;

  (*t_num)++;
  return t;
}


static status_t new_site(server,port,path)
  char *server,*port,*path;
{

  URL *u;
  u = urlBuild(http_method,server,port, path );
  if ( u == NULL ) {
    error(A_ERR,"new_site","Cannot create empty url database.");
    return ERROR;
  }
  
  store_url_db(u,0,0);
  orig_table_num = 0;

  return A_OK;
}







int init_url_db(curr_filename, server, ip, port,path)
char *curr_filename;
char *server;
ip_addr_t ip;
char *port;
char *path;
{
  int finished;
  pathname_t f,f1;

  char *visited = NULL;
  struct arch_stridx_handle *strhan;
  char *dbdir;
  pathname_t s1,s2;
  pathname_t files_database_dir;
  char *t1,*t2,*res;
  int i,act_size;
  int curr_index;
  full_site_entry_t *curr_ent, *ent;
  full_site_entry_t *curr_ptr;
  int flag;
  index_t curr_strings;
  int port_no = atoi(port);
  file_info_t *curr_info;

  date_time_t date;
  
  table_index = 0;  
  srand(time((time_t *) NULL));
  urldb = create_finfo();

  t1 = s1;
  t2 = s2;
  
sleep(20);   
  for(finished = 0; !finished;){

    sprintf(f,"%s-%s_%d", curr_filename, "urldb", rand() % 100);
    strcpy(f1,f);
    strcat(f1,".db");
    if(access(f1, R_OK | F_OK) == -1) 
    finished = 1;
  }

  strcpy(urldb->filename,f);
  files_database_dir[0] = '\0';
  urldb->fp_or_dbm.dbm = dbm_open(urldb->filename, O_RDWR | O_CREAT, DEFAULT_FILE_PERMS);

  if ( urldb->fp_or_dbm.dbm  == NULL ) {
    error(A_ERR,"init_url_db", "Unable to create temporary url database");
    return ERROR;
  }

  if((dbdir = (char *)set_wfiles_db_dir(files_database_dir)) == (char *) NULL){
    error(A_WARN,"init_urldb", "Error while trying to set webindex database directory");
    
  }

  if ( !(strhan = archNewStrIdx()) ) {
    error(A_WARN,"init_url_db", "Could  not create string handle");
    return ERROR;
  }
  
  if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_SEARCH  ) )
  {
    error( A_WARN, "init_url_db", "Could not find strings_idx files. " );
    archFreeStrIdx(&strhan);
    return new_site(server,port,path);
  }
  
  
  if(access((char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port_no), F_OK) < 0) {

    error( A_WARN, "init_url_db", "Site file not present .. probably new site. " );
    send_message(server);
    archCloseStrIdx(strhan);
    archFreeStrIdx(&strhan);
    return new_site(server,port,path);
  }
  else {
    pathname_t excerpt;
    
    curr_info = create_finfo();    
    strcpy(curr_info -> filename, (char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port_no));

    sprintf(excerpt,"%s.excerpt", (char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port_no));

    if ( access(excerpt,R_OK|F_OK) == -1 ) {
      error(A_ERR, "init_url_db", "There is no excerpt file for site %s",
            curr_info->filename);
      destroy_finfo(curr_info);
      archCloseStrIdx(strhan);
      archFreeStrIdx(&strhan);
      return new_site(server,port,path);      
    }

    /* Open current file */
    if(open_file(curr_info, O_RDONLY) == ERROR){
      error(A_ERR, "init_url_db", "Cannot open site %s, assuming not present",
            curr_info->filename);
      destroy_finfo(curr_info);
      archCloseStrIdx(strhan);
      archFreeStrIdx(&strhan);
      return new_site(server,port,path);
    }
  }

  /* mmap it */
  if(mmap_file(curr_info, O_RDONLY) == ERROR) {
    error(A_WARN, "init_url_db", "Cannot mmap site %s, assuming not present",
          curr_info->filename);
    close_file(curr_info);      
    destroy_finfo(curr_info);
    archCloseStrIdx(strhan);
    archFreeStrIdx(&strhan);
    return new_site(server,port,path);
  }
  
  act_size = curr_info -> size / sizeof(full_site_entry_t);

  visited = (char*)malloc(sizeof(char)*act_size);
  if ( visited == NULL ) {
    error(A_ERR,"init_urldb","Unable to allocate memory for internal table");
    close_file(curr_info);  
    destroy_finfo(curr_info);
    archCloseStrIdx(strhan);
    archFreeStrIdx(&strhan);
    return ERROR;
  }
  memset(visited,0,sizeof(char)*act_size);

  date = 0;
  curr_ent = (full_site_entry_t *) curr_info -> ptr + (act_size-1);
  flag = 0;
  for ( i = act_size-1; i>=0; i--, curr_ent--) {

    if ( CSE_IS_KEY((*curr_ent)) )  { /* Skip over the keys */
      flag = 1;
      visited[i] = 1;
      continue;
    }

    if ( visited[i] && flag == 0 ) {
      flag = 0;
      continue;
    }

    /* Build path/url */
    curr_index = i;


    curr_ptr = (full_site_entry_t *) curr_info -> ptr + (curr_index);
    t1 = s1;
    t2 = s2;
    s1[0] = s2[0] = '\0';

    if ( CSE_IS_DOC((*curr_ptr)) || flag == 0 ) {
      curr_strings = curr_ptr -> strt_1;
      if( curr_strings < 0 || !archGetString( strhan,  curr_strings, &res) ) {
        continue;  
      }
      else {
        strcpy(t1,res);
      }      
    }
    else {
      continue;
/*      strcpy(t1,"/"); */
    }

    date = curr_ptr->core.entry.rdate;

    while ( (curr_index >= 0) ) {
      curr_ptr = (full_site_entry_t *) curr_info -> ptr + (curr_index);
      if ( CSE_IS_SUBPATH((*curr_ptr))  || 
          CSE_IS_NAME((*curr_ptr)) ||
          CSE_IS_PORT((*curr_ptr)) ) {
        break;
      }
      curr_index--;
    }

    while ( curr_index > 0  ) {
      char *t;

      if ( curr_ptr->core.prnt_entry.strt_2 <0 ) {
        t1 = t2 = NULL;          
        break;
      }
      else {
        /* ent is the parent of record at curr_index */
        ent = (full_site_entry_t *)curr_info -> ptr+curr_ptr->core.prnt_entry.strt_2;
        curr_strings = ent->strt_1; /* This is the string of the parent */
      }

      if ( curr_strings < 0 ) {
        t1 = t2 = NULL;
        break;
      }
        
      if( !archGetString( strhan,  curr_strings, &res) ) {
        t1 = t2 = NULL;
        break;
      }
      else {
        if ( CSE_IS_NAME((*curr_ptr)) ) {
          sprintf(t2,"http://%s:%s",res,t1);
        }
        else {
          sprintf(t2,"%s/%s",res,t1);
        }
      }
      visited[curr_index] = 1;
      visited[curr_ptr->core.prnt_entry.strt_2] = 1;
      curr_index = curr_ptr->strt_1;
      curr_ptr = (full_site_entry_t *)curr_info ->ptr + curr_index;

      t = t2;
      t2 = t1;
      t1 = t;
        
    }

    if ( t1 != t2 ) {
      URL *u;
      urlEntry *ue;

      /* New url constructed */
/*      fprintf(stderr,"New url:  %s\n",t1);  */
      
      u = urlParse(t1);
      if ( u != NULL ) {

        ue = ( urlEntry*)malloc(sizeof(urlEntry));
        if ( ue != NULL  ) {

          ue->url = u;
          ue->recno = i;
          ue->content = (date)?1:0; /*flag; */
          ue->date = date;
          store_url(ue,0);
        }

      }
    }
    flag = 0;
  }

  orig_table_num = table_num;

  if ( table_num == 0 ) {       /* The databse is empty */ 
    URL *u;
    u = urlBuild(http_method,server,port, path );
    if ( u == NULL ) {
      archCloseStrIdx(strhan);
      archFreeStrIdx(&strhan);
      close_file(curr_info);  
      destroy_finfo(curr_info);      
      return 0;
    }
    store_url_db(u,0,0);
  }
  else {
    /* Sort the entries  so that the ones not accessed yet are done first */

    qsort(table, table_num, sizeof(urlEntry*), table_cmp);
  
  }

  
  archCloseStrIdx(strhan);
  archFreeStrIdx(&strhan);
  close_file(curr_info);  
/*  munmap(curr_info); */
  destroy_finfo(curr_info);
  return 1;
}


int close_url_db()
{
/*   int i;*/
   pathname_t f;
/*   
   for ( i =0; i < table_num; i++ )  {
     urlFree(table[i]->url);
     free(table[i]);
   }

   table_num = 0;
   orig_table_num = 0;
*/
   dbm_close(urldb->fp_or_dbm.dbm);

   strcpy(f,urldb->filename);
   strcat(f,".db");
   unlink(f);

   return  1;
}


int print_url_db()
{
   int i;

   for(i=0; i < table_num; i++ ) {
      printf("%s\n",table[i]->url->curl);
   }
   return 1;
}

static int cmpstr(a,b)
urlEntry **a,**b;
{
   return strcasecmp(CANONICAL_URL((*a)->url),CANONICAL_URL((*b)->url));
}


int output_url_db(fp)
FILE *fp;
{
   int i;

   /* sort the table */
   qsort((char*)table,table_num,sizeof(URL*),cmpstr );

   /* output the table */
   
   for ( i = 0; i < table_num; i++ ) {
      fprintf(fp,"url:%s\n",CANONICAL_URL(table[i]->url));
   }
   return 1;
}




int output_extern_url_db(fp)
FILE *fp;
{
   int i;

   /* sort the ext_table */
   qsort(ext_table,ext_table_num,sizeof(char*),cmpstr );

   /* output the ext_table */
   
   for ( i = 0; i < ext_table_num; i++ ) {
      fprintf(fp,"url:%s\n",CANONICAL_URL(ext_table[i]->url));     
   }
   return 1;
}



static int free_table(tbl,tbl_num,tbl_max)
char **tbl;
int *tbl_num, *tbl_max;
{

   int i;

   /* output the tbl */

   if ( tbl != NULL )  {
     for ( i = 0; i < *tbl_num; i++ ) {
       free(tbl[i]);
     }
     free(tbl);
     *tbl_num = 0;
     *tbl_max = 0;
   }
   return 1;

}



int free_url_db()
{

  free_table(table,&table_num,&table_max);
  table = NULL;
  free_table(ext_table,&ext_table_num,&ext_table_max);
  ext_table = NULL;

  return 1;
}


int check_url_db(url)
char *url;
{
   datum key;

   if ( url == NULL )
     return 0;
   
   key.dptr = url;
   key.dsize = strlen(url)+1;
   key = dbm_fetch(urldb->fp_or_dbm.dbm,key);

   if ( key.dsize == 0 ) 
      return 0;

   return 1;
}

static int store_url(ue,ext)
urlEntry *ue;
int ext;
{  

  datum db;
  datum dv;
   
  if ( ue == NULL || ue->url == NULL )
  return 0;

  if ( CANONICAL_URL(ue->url) == 0 )
    return 0;

  if ( check_url_db(CANONICAL_URL(ue->url)) == 0 )   {
     
    if ( ext )
    ext_table = addTable(ext_table,&ext_table_num,&ext_table_max, ue );
    else 
    table = addTable(table,&table_num,&table_max, ue );

    db.dptr = CANONICAL_URL(ue->url);
    db.dsize = strlen(CANONICAL_URL(ue->url))+1;
    dv.dptr = "1";
    dv.dsize = 2;

    return (dbm_store(urldb->fp_or_dbm.dbm,db,dv,DBM_REPLACE)==0) ? 1 : 0 ;
  }

  return 0;
}

int store_url_db(url,ext,redirect)
URL *url;
int ext;
int redirect;
{

  urlEntry *ue;

  ue = (urlEntry*) malloc(sizeof(urlEntry));
  if ( ue == NULL ) {
    return 0;
  }
  if ( ! redirect ) {
    if ( url->local_url != NULL ) {
      int len = strlen(url->local_url);
      if ( len > 1 ) {
        if (url->local_url[len-1] == '/' ) {
          char *tmp;

          url->local_url[len-1] = '\0';
          if ( url->curl != NULL ) 
          url->curl[strlen(url->curl)-1] = '\0';

#if 0 
          tmp = urlStrBuild(url);
          if ( check_url_db(tmp) == 0) {
            free(tmp);
            url->local_url[len-1] = '/';
          }
          else {
            free(tmp);
            return 1;
          }
#endif
        }
      }
    }
  }
  ue->recno = -2;
  ue->content = 0;
  ue->url = url;
  
  return store_url(ue,ext);
}

urlEntry *get_extern_url()
{

  if ( ext_table_index >= ext_table_num )
    return NULL;

  return ext_table[ext_table_index++];
}

urlEntry *get_unvisited_url(known)
int *known;
{
   if ( table_index == table_num ) 
      return NULL;

/*   *known = table_index < orig_table_num; */
   *known = table[table_index]->content; 
/*
   if ( strcmp(table[table_index]->url->local_url,"/") == 0 ) {
     free(table[table_index]->url->local_url);
     table[table_index]->url->local_url = malloc(100);
     strcpy(table[table_index]->url->local_url,"/~maquelin/");
     sprintf(table[table_index]->url->curl,"http://%s%s",
             table[table_index]->url->server,
             table[table_index]->url->local_url);
   }
*/
   return table[table_index++];
}

