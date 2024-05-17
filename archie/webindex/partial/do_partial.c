#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <varargs.h>

#include "protos.h"
#include "typedef.h"
#include "archie_inet.h"
#include "header.h"
#include "files.h"
#include "host_db.h"
#include "error.h"
#include "master.h"
#include "db_files.h"
#include "archie_strings.h"
#include "parser_file.h"
#include "sub_header.h"
#include "webindexdb_ops.h"

#include "site_file.h"
#include "excerpt.h"
#include "archstridx.h"

extern int errno;
extern int compress_flag;
pathname_t compress_pgm;
extern int force;

typedef struct {
  char *path;
  int recno;
} pathEntry;



void output_padding(fp,num)
  FILE *fp;
  int num;
{
  static char tmp[] = "    ";

  if ( num ) 
  fwrite(tmp,1,num,fp);
  
}



int convert_site_parser( site_ent, curr_ent,strhan, parent, string )
  full_site_entry_t *site_ent;
  parser_entry_t *curr_ent;
  struct arch_stridx_handle *strhan;  
  index_t parent;
  char **string;
{

  if(  !(CSE_IS_SUBPATH((*site_ent)) || 
         CSE_IS_NAME((*site_ent)) ||
         CSE_IS_PORT((*site_ent)) ) ) {

    if( CSE_IS_KEY((*site_ent)) ){
      CSE_STORE_WEIGHT( (curr_ent->core) , (site_ent->core.kwrd.weight) );
    }else{
      curr_ent->core.size = site_ent -> core.entry.size;
      curr_ent->core.date = site_ent -> core.entry.date;
      curr_ent->core.rdate = site_ent -> core.entry.rdate;
      curr_ent->core.perms = site_ent -> core.entry.perms;
    }
      
    curr_ent->core.parent_idx = parent;
    curr_ent->core.child_idx = (index_t)(-1);
    curr_ent->core.flags = site_ent -> flags;
    /* Get the string associated with parser record */
    
    if( !archGetString( strhan,  site_ent -> strt_1, string) ){
      error(A_ERR,"copy_parser_to_xdr","Could not find the string using archGetString\n");
      return( ERROR );
    }

    curr_ent->slen = strlen(*string);

  }else
  {

    curr_ent->slen = 0;
      
    if( (index_t) site_ent -> core.prnt_entry.strt_2 >= 0 )
    {

      curr_ent->core.size = 0;
      curr_ent->core.date = 0;
      curr_ent->core.rdate = 0;
      curr_ent->core.perms = 0;
      curr_ent->core.parent_idx = parent;
      curr_ent->core.child_idx = (index_t)(-1);
      curr_ent->core.flags = site_ent -> flags;

      *string = NULL;
      
    }
  }
  return A_OK;
}


char *parser_build_path(offset, offset_table, array )
  int offset;
  int *offset_table;
  char *array;
{

  static char string[512];
  char tmp[512],str[512];
  parser_entry_t curr_ent;
  
  string[0] = '\0';

  while ( offset != -1 ) {
    memcpy(&curr_ent, &array[offset],  sizeof(parser_entry_t));
    strncpy(tmp,array+offset+sizeof(parser_entry_t),curr_ent.slen);
    tmp[curr_ent.slen] = '\0';
    if ( string[0] == '\0' ) {
      sprintf(string,"/%s",tmp);
    }
    else {
      strcpy(str,string);
      sprintf(string,"/%s%s",tmp,str);
    }

    if ( offset <= 0 )
      break;
    offset = offset_table[curr_ent.core.parent_idx];
  }

  return string;
}



int site_build_paths(site_file, pathList,strhan  )
  file_info_t *site_file;
  pathEntry **pathList;
  struct arch_stridx_handle *strhan;
{

  char *visited = NULL;
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
  int counter;


  t1 = s1;
  t2 = s2;
   
#if 0
  if((dbdir = (char *)set_wfiles_db_dir(files_database_dir)) == (char *) NULL){
    error(A_WARN,"init_urldb", "Error while trying to set webindex database directory");
    
  }
#endif
  
  act_size = site_file -> size / sizeof(full_site_entry_t);

  visited = (char*)malloc(sizeof(char)*act_size);
  if ( visited == NULL ) {
    error(A_ERR,"init_urldb","Unable to allocate memory for internal table");
    return ERROR;
  }
  memset(visited,0,sizeof(char)*act_size);

  *pathList = (pathEntry*)malloc(sizeof(pathEntry)*(act_size+1));
  if ( *pathList == NULL ) {
    error(A_ERR,"init_urldb","Unable to allocate memory for internal list");
    return ERROR;
  }
  memset(*pathList,0,sizeof(pathEntry)*(act_size+1));

  counter = 0;
  curr_ent = (full_site_entry_t *) site_file -> ptr + (act_size-1);
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


    curr_ptr = (full_site_entry_t *) site_file -> ptr + (curr_index);
    t1 = s1;
    t2 = s2;
    s1[0] = s2[0] = '\0';

    if ( CSE_IS_DOC((*curr_ptr)) || flag == 0 ) {
      curr_strings = curr_ptr -> strt_1;
      if( curr_strings < 0 || !archGetString( strhan,  curr_strings, &res) ) {
        continue;  
      }
      else {
        sprintf(t1,"/%s",res);
      }      
    }
    else {
      continue;
/*      strcpy(t1,"/"); */
    }


    while ( (curr_index >= 0) ) {
      curr_ptr = (full_site_entry_t *) site_file -> ptr + (curr_index);
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
        ent = (full_site_entry_t *)site_file -> ptr+curr_ptr->core.prnt_entry.strt_2;
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
          sprintf(t2,"/%s%s",res,t1);
      }
      visited[curr_index] = 1;
      visited[curr_ptr->core.prnt_entry.strt_2] = 1;
      curr_index = curr_ptr->strt_1;
      curr_ptr = (full_site_entry_t *)site_file ->ptr + curr_index;

      t = t2;
      t2 = t1;
      t1 = t;
        
    }

    if ( t1 != t2 ) {
      (*pathList)[counter].path = (char*)malloc(sizeof(char)*(strlen(t1)+1));
      if ( (*pathList)[counter].path == NULL ) {
        return ERROR;
      }
      strcpy((*pathList)[counter].path, t1);
      (*pathList)[counter].recno = i;
      counter++;
    }
    flag = 0;
  }


  return A_OK;
}


int find_path(path,pathList)
  char *path;
  pathEntry *pathList;
{
  int i;

  for ( i = 0; pathList[i].path != NULL ; i++ ) {
    if ( strcasecmp(path,pathList[i].path) == 0 )
       return pathList[i].recno;
  }
  return -1;
}




status_t do_partial(input_file, ein_file, tmp_file, etmp_file, no_recs,header_rec, eheader_rec, ip)
  file_info_t *input_file,*ein_file;
  file_info_t *tmp_file;
  file_info_t *etmp_file;
  int *no_recs;
  header_t *header_rec;
  header_t *eheader_rec;
  ip_addr_t ip;
{
  file_info_t *site_file = create_finfo();
  file_info_t *esite_file = create_finfo();
  char *dbdir;
  pathname_t files_database_dir;
  unsigned long  eoffset;
  parser_entry_t curr_ent;
  full_site_entry_t *site_ent;
  char *in;
  int excerpt_index, i;
  int byte_offset;
  int *offset_table;
  int port;
  pathEntry *pathList;
  struct arch_stridx_handle *strhan;
  int *rec_numbers;
  
  files_database_dir[0] = '\0';  

  *no_recs = 0;
  
  if((dbdir = (char *)set_wfiles_db_dir(files_database_dir)) == (char *) NULL){
    error(A_WARN,"recurse", "Error while trying to set webindex database directory");
  }

  /*  Read in headers */

  get_port(header_rec->access_command, WEBINDEX_DB_NAME, &port);



  if(mmap_file(input_file, O_RDONLY) == ERROR){

    /* "Can't mmap() input file %s" */

    error(A_SYSERR,"insert_webindex", "Can't mmap() input file %s", input_file -> filename);
    exit(A_OK);
  }

  if(mmap_file(ein_file, O_RDONLY) == ERROR){

    /* "Can't mmap() input file %s" */
    error(A_SYSERR,"insert_webindex", "Can't mmap() input file %s", ein_file -> filename);
    exit(A_OK);
  }


  strcpy(site_file->filename, (char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port));
  sprintf(esite_file->filename,"%s%s", (char *)wfiles_db_filename(inet_ntoa( ipaddr_to_inet(ip)),port),SUFFIX_EXCERPT);

  
  if ( open_file(site_file,O_RDONLY) == ERROR ) {
    /* "Can't open input file %s" */

    error(A_ERR, "partial_webindex", "Can't open site file %s", site_file -> filename);
    exit(ERROR);
  }

  if(mmap_file(site_file, O_RDONLY) == ERROR){

    /* "Can't mmap() input file %s" */
    error(A_SYSERR,"insert_webindex", "Can't mmap() site  file %s", site_file -> filename);
    exit(ERROR);
  }



  if ( open_file(esite_file,O_RDONLY) == ERROR ) {
    /* "Can't open input file %s" */

    error(A_ERR, "partial_webindex", "Can't open excerpt file %s", esite_file -> filename);
    exit(ERROR);
  }

  if(mmap_file(esite_file, O_RDONLY) == ERROR){

    /* "Can't mmap() input file %s" */
    error(A_SYSERR,"insert_webindex", "Can't mmap() site  file %s", site_file -> filename);
    exit(ERROR);
  }
  
  offset_table = (int*)malloc(sizeof(int)*header_rec->no_recs);
  if ( offset_table == NULL ) {
    error(A_SYSERR,"do_partial","Cannot allocate memory" );
    exit(ERROR);
  }

  if ( !(strhan = archNewStrIdx()) ) {
    error(A_WARN, "check_webindex","Could not create string handler");
    exit(A_OK);
  }

  if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_SEARCH ) )
  {
    error( A_WARN, "net_webindex", "Could not find strings_idx files, will create them.\n" );
    exit(A_OK);
  }

  
  if ( site_build_paths(site_file, &pathList, strhan) == ERROR ) {

    return ERROR;
  }
  
  excerpt_index = 0;
  in = (char *)input_file->ptr;
  rec_numbers = (int*)malloc(sizeof(int)*header_rec->no_recs);
  
  for ( i = 0; i<header_rec->no_recs; i++ ) {

    offset_table[i] = in-(char*)input_file->ptr;
    
    memcpy(&curr_ent,in,sizeof(parser_entry_t));
    rec_numbers[i] = (*no_recs);
    (*no_recs)++;
    if ( CSE_IS_DOC((curr_ent.core)) ) {
      char *path = parser_build_path(offset_table[i],offset_table,input_file->ptr);
      int index = find_path(path,pathList);
      int max_index = site_file->size / sizeof(full_site_entry_t);
      
      site_ent = ((full_site_entry_t*)site_file->ptr)+index;
      
      if ( curr_ent.core.rdate <= site_ent->core.entry.rdate ) { /* Old */

        char *string;
        parser_entry_t new_ent;
        excerpt_t *excerpt_ptr;

        if ( index == -1 ) {
          error(A_ERR,"partial_webindex","Error find path");
          return ERROR;
        }

        excerpt_ptr = ((excerpt_t*)esite_file->ptr)+site_ent->core.entry.perms;
        if ( fwrite(excerpt_ptr, sizeof(excerpt_t), 1, etmp_file->fp_or_dbm.fp) != 1 ) {
          error(A_ERR, "partial_webindex", "Unable to output excerpt");
          return ERROR;
        }

        
        do {
          convert_site_parser(site_ent,&new_ent,strhan,curr_ent.core.parent_idx ,&string);
          new_ent.core.parent_idx = /*curr_ent.core.parent_idx; */ rec_numbers[curr_ent.core.parent_idx];
          if ( CSE_IS_DOC(new_ent.core) ) {
            new_ent.core.perms = excerpt_index++;
          }
          fwrite(&new_ent,sizeof(parser_entry_t),1,tmp_file->fp_or_dbm.fp);          
          if (string == NULL ) {
            fwrite(in+sizeof(parser_entry_t), 1,curr_ent.slen, 
                   tmp_file->fp_or_dbm.fp);
            if ( curr_ent.slen&0x3 )             
            output_padding(tmp_file->fp_or_dbm.fp,4-(curr_ent.slen&0x3));
          }
          else {
            int size;
            
            fwrite(string,strlen(string),1, tmp_file->fp_or_dbm.fp);
            size = strlen(string)+sizeof(parser_entry_t);
            if ( size&0x3 ) 
            output_padding(tmp_file->fp_or_dbm.fp,4-(size&0x3));
            
          }
          (*no_recs)++;
          site_ent++;
          index++;
        }while ( index < max_index && CSE_IS_KEY((*site_ent)) );

        /* Need to substract cause already added before the  check
           if (CSE_IS_DOC())
           */
        (*no_recs)--; 

      }
      else {                    /* New */
        /* Need to copy the corresponding excerpt in the output file */

        excerpt_t *excerpt;

        excerpt = (excerpt_t*)ein_file->ptr+curr_ent.core.perms;
        fwrite(excerpt, sizeof(excerpt_t),1,etmp_file->fp_or_dbm.fp);
        
        curr_ent.core.perms = excerpt_index++; 
        curr_ent.core.parent_idx = rec_numbers[curr_ent.core.parent_idx];
        /* Output the record */
        fwrite(&curr_ent,sizeof(parser_entry_t),1,tmp_file->fp_or_dbm.fp);
        fwrite(in+sizeof(parser_entry_t), 1,curr_ent.slen, 
                   tmp_file->fp_or_dbm.fp);
        if ( curr_ent.slen&0x3 ) 
        output_padding(tmp_file->fp_or_dbm.fp,4-(curr_ent.slen&0x3));
        
      }
    }
    else {
      curr_ent.core.parent_idx = rec_numbers[curr_ent.core.parent_idx];
      fwrite(&curr_ent,sizeof(parser_entry_t),1,tmp_file->fp_or_dbm.fp);

      fwrite(in+sizeof(parser_entry_t), 1,curr_ent.slen, 
             tmp_file->fp_or_dbm.fp);
      if ( curr_ent.slen&0x3 )       
      output_padding(tmp_file->fp_or_dbm.fp,4-(curr_ent.slen&0x3));
        
    }
    
    byte_offset =  sizeof(parser_entry_t) + curr_ent.slen;    

    /* Align it on the next 4 byte boundary. May be NON-PORTABLE*/
    if( byte_offset & 0x3 )
    byte_offset += (4 - (byte_offset & 0x3));

    in +=byte_offset;
    
  }

  eheader_rec->no_recs = excerpt_index;
  HDR_SET_NO_RECS(eheader_rec->header_flags);

  close_file(site_file);  
  destroy_finfo(site_file);

  archCloseStrIdx(strhan);
  archFreeStrIdx(&strhan);


  return(A_OK);


}


