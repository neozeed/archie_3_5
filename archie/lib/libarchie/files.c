/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#define _BSD_SOURCE

#include <unistd.h>
#include <stdio.h>
#ifdef __STDC__
#include <stdlib.h>
#endif
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dirent.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <varargs.h>

#include "archie_dbm.h"
#include "master.h"
#include "files.h"
#include "error.h"
#include "db_ops.h"
#include "lang_libarchie.h"
#include "archie_strings.h"
#include "db_files.h"
#ifdef OLD_FILES
#  include "old-host_db.h"
#else
#  include "host_db.h"
#endif
#include "archie_dns.h"
#include "archie_mail.h"

#include "protos.h"
#include "start_db.h"

/*
 * open_file: Open a file. <perms> are those defined in fcntl.h for the
 * open(2) call
 */

extern int errno;

#define COPY_FILENAME "temp_COPY_file"
#define BUFF_SIZE 8192

status_t open_file( file, perms )
   file_info_t *file;
   int perms;

{
   char perms_string[8];


   if(file == (file_info_t *) NULL){
   
      /* "NULL file info structure" */

      error(A_INTERR, "open_file", OPEN_FILE_001);
      return(ERROR);
   }

      

   if(file -> filename[0] == '\0'){

      /* "No filename in given structure" */

      error(A_INTERR, "open_file", OPEN_FILE_002);
      return(ERROR);
   }

   if( perms == O_RDWR)
     strcpy(perms_string, "r+");
   else if(perms == O_RDONLY)
     strcpy(perms_string, "r");
   else if(perms == O_WRONLY)
     strcpy(perms_string, "w+");
   else if(perms == O_APPEND)
     strcpy(perms_string, "a");
     
   if(( file -> fp_or_dbm.fp = fopen (file -> filename, perms_string)) == (FILE *) NULL){

      /* "Can't open file %s" */

      error(A_SYSERR,"open_file",OPEN_FILE_003, file -> filename);
      return(ERROR);
   }

   file -> offset = -1;

   return(A_OK);
}



/* close_file: close the file given in the file_info_t structure */

status_t close_file( file )
   file_info_t *file;		/* File info struct for file to be closed */

{

#ifdef __STDC__

   extern int fclose(FILE *);

#else

   extern int fclose();

#endif

   ptr_check(file, file_info_t, "close_file", ERROR);
   
   /* If the file is currently mmaped, munmap it first */

   if(file -> ptr)
      if(munmap_file(file) == ERROR)
         return(ERROR);

   if(file -> fp_or_dbm.fp != (FILE *) NULL){

      if( fclose( file -> fp_or_dbm.fp ) != 0){

	 /* "Can't close given file %s"; */

	 error(A_SYSERR,"close_file", CLOSE_FILE_002, file -> filename);
	 return(ERROR);
      }
   }

   file -> offset = -1;
   file -> fp_or_dbm.fp = (FILE *) NULL;

   return(A_OK);
}
   


/*
 * mmap_file: mmap a given file. The mode is one of those defined in the
 * system fcntl.h file (for the open(2) call).
 */


status_t mmap_file(mapfile, mode)
   file_info_t *mapfile;
   int mode;

{
   struct stat statbuf;	   /* file stat buffer */
   int open_mode;	   

   /* Check pointers */

   ptr_check(mapfile, file_info_t, "mmap_file", ERROR);
   ptr_check(mapfile -> fp_or_dbm.fp, FILE, "mmap_file", ERROR);

#if 0
   if(mapfile -> ptr){

      /* "non-null file information pointer. File: %s" */

      error(A_INTERR, "mmap_file", MMAP_FILE_002, mapfile -> filename);
   }
#endif

   /* Get the size of the file to mmap */

   if(fstat(fileno(mapfile -> fp_or_dbm.fp), &statbuf) == -1){

	 /* "Can't stat mapfile %s" */

	 error(A_SYSERR,"mmap_file", MMAP_FILE_003, mapfile -> filename);
	 return(ERROR);
   }

   mapfile -> size = statbuf.st_size;
   
   if(mode == O_RDONLY){
     open_mode = PROT_READ;
#ifdef AIX

     /* mark it as readonly for AIX */

     mapfile -> write = 0;
#endif
   }
   else{
     open_mode = PROT_READ | PROT_WRITE;
#ifdef AIX
     mapfile -> write = 1;
#endif
   }
     

   if((mapfile -> ptr = (char *) mmap(0, mapfile -> size, open_mode, MAP_SHARED, fileno(mapfile -> fp_or_dbm.fp), 0)) == (char *) -1){

      /* "mmap() of file %s failed" */

      error(A_SYSERR,"mmap_file", MMAP_FILE_004,  mapfile -> filename);
      return(ERROR);

   }

   /* If offset field of mapfile is not -1, then place the ptr to be offset
      bytes into the file */

   if(mapfile -> offset != -1){
      mapfile -> size -= mapfile -> offset;
      mapfile -> ptr += mapfile -> offset;
   }

      
   return(A_OK);
}


#if 0

status_t mmap_file(mapfile, mode)
   file_info_t *mapfile;
   int mode;

{
   struct stat statbuf;	   /* file stat buffer */
   int open_mode;	   

   /* Check pointers */

   ptr_check(mapfile, file_info_t, "mmap_file", ERROR);

#if 0
   if(mapfile -> ptr){

      /* "non-null file information pointer. File: %s" */

      error(A_INTERR, "mmap_file", MMAP_FILE_002, mapfile -> filename);
   }
#endif

   /* Get the size of the file to mmap */

   if(fstat(fileno(mapfile -> fp_or_dbm.fp), &statbuf) == -1){

	 /* "Can't stat mapfile %s" */

	 error(A_SYSERR,"mmap_file", MMAP_FILE_003, mapfile -> filename);
	 return(ERROR);
   }

   mapfile -> size = statbuf.st_size;

   if(mapfile -> size == 0)
      return(ERROR);
   
   if(mode == O_RDONLY)
     open_mode = PROT_READ;
   else
     open_mode = PROT_READ | PROT_WRITE;


   if((mapfile -> ptr = (char *) mmap(0, mapfile -> size, open_mode, MAP_SHARED, fileno(mapfile -> fp_or_dbm.fp), 0)) == (char *) -1){

      /* "mmap() of file %s failed" */

      error(A_SYSERR,"mmap_file", MMAP_FILE_004,  mapfile -> filename);
      return(ERROR);

   }

   /* If offset field of mapfile is not -1, then place the ptr to be offset
      bytes into the file */

   if(mapfile -> offset != -1){
      mapfile -> size -= mapfile -> offset;
      mapfile -> ptr += mapfile -> offset;
   }

      
   return(A_OK);
}
#endif



/*
 * mmap_file_private: same as mmap_file but the mmap is PRIVATE, not SHARED
 * seldom used, so this is not an option to mmap_file 
 */


status_t mmap_file_private(mapfile, mode)
   file_info_t *mapfile;
   int mode;

{
    struct stat statbuf;
    int open_mode;

   /* Check pointers */

   ptr_check(mapfile, file_info_t, "mmap_file_private", ERROR);

#if 0
   if(mapfile -> ptr){

      /* "non-null file information pointer. File: %s" */

      error(A_INTERR, "mmap_file_private", MMAP_FILE_002, mapfile -> filename);
      return(ERROR);
   }
#endif

   /* Get the size of the file to mmap */

   if(fstat(fileno(mapfile -> fp_or_dbm.fp), &statbuf) == -1){

	 /* "Can't stat mapfile %s" */

	 error(A_SYSERR,"mmap_file_private", MMAP_FILE_003, mapfile -> filename);
	 return(ERROR);
   }

   mapfile -> size = statbuf.st_size;
   

   if(mode == O_RDONLY){
      open_mode = PROT_READ;
#ifdef AIX

     /* mark it as readonly for AIX */

      mapfile -> write = 0;
#endif
   }
   else{
      open_mode = PROT_READ | PROT_WRITE;
#ifdef AIX

      /* if the file is writable and mmap'd private then flag it with
         a write field of 2. This tells the mumap_file procedure not
	 to worry about truncating the file when finished */
      
      mapfile -> write = 2;
#endif
   }
     
   if((mapfile -> ptr = (char *) mmap(0, mapfile -> size, open_mode, MAP_PRIVATE, fileno(mapfile -> fp_or_dbm.fp), 0)) == (char *) -1){

      /* "mmap() of file %s failed" */

      error(A_SYSERR,"mmap_file_private", MMAP_FILE_004,  mapfile -> filename);
      return(ERROR);

   }

   /* If offset field of mapfile is not -1, then place the ptr to be offset
      bytes into the file */

   if(mapfile -> offset != -1){
      mapfile -> size -= mapfile -> offset;
      mapfile -> ptr += mapfile -> offset;
   }

   return(A_OK);


}


#if 0

status_t mmap_file_private(mapfile, mode)
   file_info_t *mapfile;
   int mode;

{
    struct stat statbuf;
    int open_mode;

   /* Check pointers */

   ptr_check(mapfile, file_info_t, "mmap_file_private", ERROR);

#if 0
   if(mapfile -> ptr){

      /* "non-null file information pointer. File: %s" */

      error(A_INTERR, "mmap_file_private", MMAP_FILE_002, mapfile -> filename);
      return(ERROR);
   }
#endif

   /* Get the size of the file to mmap */

   if(fstat(fileno(mapfile -> fp_or_dbm.fp), &statbuf) == -1){

	 /* "Can't stat mapfile %s" */

	 error(A_SYSERR,"mmap_file_private", MMAP_FILE_003, mapfile -> filename);
	 return(ERROR);
   }

   mapfile -> size = statbuf.st_size;

   if(mapfile -> size == 0)
      return(ERROR);
   
   if(mode == O_RDONLY)
     open_mode = PROT_READ;
   else
     open_mode = PROT_READ | PROT_WRITE;


   if((mapfile -> ptr = (char *) mmap(0, mapfile -> size, open_mode, MAP_PRIVATE, fileno(mapfile -> fp_or_dbm.fp), 0)) == (char *) -1){

      /* "mmap() of file %s failed" */

      error(A_SYSERR,"mmap_file_private", MMAP_FILE_004,  mapfile -> filename);
      return(ERROR);

   }

   /* If offset field of mapfile is not -1, then place the ptr to be offset
      bytes into the file */

   if(mapfile -> offset != -1){
      mapfile -> size -= mapfile -> offset;
      mapfile -> ptr += mapfile -> offset;
   }

      
   return(A_OK);


}
#endif

/* munmap_file: unmap a previously mmapp'd file */

status_t munmap_file(mapfile)
   file_info_t *mapfile;   /* file info struct for file */

{
   if((mapfile -> ptr != (char *) NULL) && (mapfile -> size != 0)){

       /* Reset pointer */

       if(mapfile -> offset != -1){
	  mapfile -> size += mapfile -> offset;
	  mapfile -> ptr -= mapfile -> offset;
       }

      if(munmap(mapfile -> ptr, mapfile -> size) != 0){

	 /* "Can't munmap() file %s" */

	 error(A_SYSERR,"munmap_file", MUNMAP_FILE_001, mapfile -> filename);
	 return(ERROR);
      }
   }
   else

      /* "Attempt to munmap non mmaped, or zero length file %s" */

      error(A_WARN, "munmap_file", MUNMAP_FILE_002, mapfile -> filename);
      
   /* AIX is brain-damaged and always writes an extra page of NULLs to the
      end of the file it is unmapping. This code corrects for that */

#ifdef AIX

   /*
    * if write == 2 then it is a private map... don't truncate
    */

   if(mapfile -> write == 1){

      if(mapfile -> fp_or_dbm.fp == (FILE *) NULL){

	 if(truncate(mapfile -> filename, mapfile -> size) == -1){

	    /* "Error while trying to truncate unmmaped file %s to correct length %d" */

	    error(A_SYSERR, "munmap_file", MUNMAP_FILE_004, mapfile -> filename, mapfile -> size);
	    return(ERROR);
	 }
      }
      else{
      
	 if(ftruncate(fileno(mapfile -> fp_or_dbm.fp), mapfile -> size) == -1){

	    /* "Error while trying to truncate unmmaped file %s to correct length %d" */

	    error(A_SYSERR, "munmap_file", MUNMAP_FILE_004, mapfile -> filename, mapfile -> size);
	    return(ERROR);
	 }
      }

      mapfile -> write = 0;

   }
#endif

   mapfile -> ptr = (char *) NULL;
   mapfile -> size = 0;
   mapfile -> offset = -1;   

   return(A_OK);
}



#if 0
status_t munmap_file(mapfile)
   file_info_t *mapfile;   /* file info struct for file */

{

#ifdef __STDC__

   extern int munmap(caddr_t, int);

#else

   extern int munmap();

#endif
   

   if((mapfile -> ptr != (char *) NULL) && (mapfile -> size != 0)){

       /* Reset pointer */

       if(mapfile -> offset != -1){
	  mapfile -> size += mapfile -> offset;
	  mapfile -> ptr -= mapfile -> offset;
       }

      if(munmap(mapfile -> ptr, mapfile -> size) != 0){

	 /* "Can't munmap() file %s" */

	 error(A_SYSERR,"munmap_file", MUNMAP_FILE_001, mapfile -> filename);
	 return(ERROR);
      }
   }
   else

      /* "Attempt to munmap non mmaped, or zero length file %s" */

      error(A_WARN, "munmap_file", MUNMAP_FILE_002, mapfile -> filename);
      

   mapfile -> ptr = (char *) NULL;
   mapfile -> size = 0;
   mapfile -> offset = -1;   

   return(A_OK);
}

#endif

/*
 * create_finfo: allocate space and initialize a new file_info_t structure
 */



file_info_t *create_finfo()

{
   file_info_t *finfo;

   if((finfo = (file_info_t *) malloc(sizeof(file_info_t))) == (file_info_t *) NULL){

      /* "Can't allocate new file_info_t structure" */

      error(A_INTERR,"create_finfo", CREATE_FINFO_001);
      return((file_info_t *) NULL);
   }
   else{

      finfo -> filename[0] = '\0';
      finfo -> fp_or_dbm.fp = (FILE *) NULL;
      finfo -> ptr = (char *) NULL;
      finfo -> size = 0;
      finfo -> offset = -1;
#ifdef AIX
      finfo -> write = 0;
#endif      
   }

   return(finfo);
}



/*
 * destroy_finfo: destroy a file_info_t strucuture. Must have been allocated
 * by create_finfo or directly by malloc(3)
 */

void destroy_finfo( finfo_ptr )
   file_info_t *finfo_ptr;

{
   if(!finfo_ptr)
     return;

   free(finfo_ptr);
}


/*
 * tail: get the last component of a pathname
 */


      
char *tail( path )
   char *path ;
{
   char *slash ;

   if(path == (char *) NULL)
     return((char *) NULL);

   if(( slash = strrchr( path, '/' )) == (char *)NULL )
      return path ;
   else
      return ++slash ;
}
      

/*
 * get_file_list: find a list of files in <tmp_dir> with the suffix <suffix>
 * return an array of pointers to filenames that match and a count of how
 * many matched
 */


status_t get_file_list(tmp_dir, file_list, suffix, count)
   pathname_t tmp_dir;	/* directory in which to look for files */
   char ***file_list;	/* array of pointers to filenames matched */
   char *suffix;	/* filename suffix to be matched to */
   int  *count;		/* number of filenames matched */

{

   char **tmp_list;
   char **mylist;
   DIR *direct;
   struct dirent *dirptr;
   int max_count, curr_count;
   pathname_t tmp_name;
   int suff_len;

   if((direct = opendir(tmp_dir)) == (DIR *) NULL){

      /* "Can't open directory %s" */

      error(A_SYSERR,"get_file_list", GET_FILE_LIST_001, tmp_dir);
      return(ERROR);
   }

   if((tmp_list = (char **) malloc(DEFAULT_NO_FILES * sizeof(char *))) == (char **) NULL){

      /* "Can't malloc() space for file list" */

      error(A_INTERR,"get_file_list", GET_FILE_LIST_002);
      return(ERROR);
   }

   max_count = DEFAULT_NO_FILES;
   curr_count = 0;

   if(suffix != (char *) NULL)
      suff_len = strlen(suffix);
   else
      suff_len = 0;

   /* Go through directory, one name at a time */
   
   while((dirptr = readdir(direct)) != (struct dirent *) NULL){

      tmp_name[0] = '\0';

      if(suffix == (char *) NULL)
         strcpy(tmp_name, dirptr -> d_name);
      else
         if(strrncmp(suffix,dirptr -> d_name, suff_len) == 0)
	    strcpy(tmp_name, dirptr -> d_name);
       
      if(tmp_name[0] != '\0'){

	 if(curr_count < max_count)
	    tmp_list[curr_count] = strdup(dirptr -> d_name);
	 else{

	    if((mylist = (char **) realloc( tmp_list, (max_count + NO_FILES_INC) * sizeof(char *))) == (char **) NULL){

	       /* "Can't realloc() space for file list" */

	       error(A_SYSERR,"get_file_list", GET_FILE_LIST_003);
	       return(ERROR);
	    }
	    tmp_list = mylist;

	    max_count += NO_FILES_INC;
	    tmp_list[curr_count] = strdup(dirptr -> d_name);
	 }

	 curr_count++;
      }
   }

   *count = curr_count;
   tmp_list[curr_count] = (char *) NULL;
   *file_list = tmp_list;

   if(closedir(direct) == -1){

      /* "Can't close directory %s" */

      error(A_SYSWARN, get_file_list, GET_FILE_LIST_004, tmp_dir);
   }

   return(A_OK);
}


/*
 * get_pre_file_list: find a list of files in <tmp_dir> with the prefix<prefix>
 * return an array of pointers to filenames that match and a count of how
 * many matched
 */


status_t get_pre_file_list(tmp_dir, file_list, prefix, count)
   pathname_t tmp_dir;	/* directory in which to look for files */
   char ***file_list;	/* array of pointers to filenames matched */
   char *prefix;	/* filename prefix to be matched to */
   int  *count;		/* number of filenames matched */

{

   char **tmp_list;
   char **mylist;
   DIR *direct;
   struct dirent *dirptr;
   int max_count, curr_count;
   pathname_t tmp_name;
   int suff_len;

   if((direct = opendir(tmp_dir)) == (DIR *) NULL){

      /* "Can't open directory %s" */

      error(A_SYSERR,"get_pre_file_list", GET_FILE_LIST_001, tmp_dir);
      return(ERROR);
   }

   if((tmp_list = (char **) malloc(DEFAULT_NO_FILES * sizeof(char *))) == (char **) NULL){

      /* "Can't malloc() space for file list" */

      error(A_INTERR,"get_pre_file_list", GET_FILE_LIST_002);
      return(ERROR);
   }

   max_count = DEFAULT_NO_FILES;
   curr_count = 0;

   if(prefix != (char *) NULL)
      suff_len = strlen(prefix);
   else
      suff_len = 0;

   /* Go through directory, one name at a time */
   
   while((dirptr = readdir(direct)) != (struct dirent *) NULL){

      tmp_name[0] = '\0';

      if(prefix == (char *) NULL)
         strcpy(tmp_name, dirptr -> d_name);
      else
         if(strncmp(prefix,dirptr -> d_name, suff_len) == 0)
    	    strcpy(tmp_name, dirptr -> d_name);
       
      if(tmp_name[0] != '\0'){

	 if(curr_count < max_count)
	    tmp_list[curr_count] = strdup(dirptr -> d_name);
	 else{

	    if((mylist = (char **) realloc( tmp_list, (max_count + NO_FILES_INC) * sizeof(char *))) == (char **) NULL){

	       /* "Can't realloc() space for file list" */

	       error(A_SYSERR,"get_pre_file_list", GET_FILE_LIST_003);
	       return(ERROR);
	    }
	    tmp_list = mylist;

	    max_count += NO_FILES_INC;
	    tmp_list[curr_count] = strdup(dirptr -> d_name);
	 }

	 curr_count++;
      }
   }

   *count = curr_count;
   tmp_list[curr_count] = (char *) NULL;
   *file_list = tmp_list;

   if(closedir(direct) == -1){

      /* "Can't close directory %s" */

      error(A_SYSWARN, get_pre_file_list, GET_FILE_LIST_004, tmp_dir);
   }

   return(A_OK);
}

	    
char *get_tmp_filename(tmp_dir)
   char *tmp_dir;

{
   static pathname_t tmp_filename;
   pathname_t hold_str;
   char *tmpname;

   if((tmp_dir == (char *) NULL) || (tmp_dir[0] == '\0'))
      sprintf(hold_str, "%s/%s", get_master_db_dir(), DEFAULT_TMP_DIR);
   else
      strcpy(hold_str, tmp_dir);
   

   if((tmpname = tempnam(hold_str,DEFAULT_TMP_PREFIX)) == (char *) NULL){

         error(A_INTERR,"get_tmp_filename","can't allocate temporary filename in directory %s", DEFAULT_TMP_DIR);
	 return((char *) NULL);
   }

   strcpy(tmp_filename, tmpname);
   free(tmpname);
   return((char *) tmp_filename);
}


status_t get_port( acc, db, port )
 access_comm_t acc;
 char *db;
 int *port;
{
  char *p = NULL;
  char *a = NULL;

  if ( db == NULL ) {
    error(A_ERR,"get_port","Unspecified database name");
    return ERROR;    
  }
  
  if (strcmp(db,ANONFTP_DB_NAME) == 0 ){
    *port = (int)atoi(ANONFTP_DEFAULT_PORT);
    return A_OK;
  }

  
  if(acc != (char *)NULL){
    a = (char *)malloc(sizeof(char)*(strlen(acc)+1));
    strcpy(a,acc);
    p = (char *)strtok(a,NET_DELIM_STR);
  }else{
    p = (char *)NULL;
  }

  if (p == (char *)NULL || !atoi(p)){
    if (!strcmp(db,WEBINDEX_DB_NAME)){
      *port = (int)atoi(WEBINDEX_DEFAULT_PORT);
    }else{
      error(A_ERR,"get_port","The database %s has no default port value",db);
      if ( a != NULL ) 
        free(a);
      return ERROR;
    }
  }else{
    *port = atoi(p);
  }

  if ( a != NULL ) 
    free(a);
  return A_OK;
}


/*
 * get_input_file: figure out what the input file for the give hostname
 */


status_t get_input_file(in_hostname, dbname, index, file_funct, input_finfo,
                        hostdb_rec, hostaux_rec, hostdb, hostaux_db, hostbyaddr)
  hostname_t in_hostname;	   /* input hostname */
  char *dbname;
  index_t index;
#if 1
  char *(*file_funct)(); /* wheelan */
#else
  int (*file_funct)();
#endif
  file_info_t *input_finfo;  /* file information for input file */
  hostdb_t *hostdb_rec;      /* primary host database record for site */
  hostdb_aux_t *hostaux_rec; /* auxiliary host database record for site/database */
  file_info_t *hostdb;	     /* primary host database */
  file_info_t *hostaux_db;   /* auxiliary host database */
  file_info_t *hostbyaddr;   /* host address cache */
{
  AR_DNS *dns;
  char *hostname;
  int tmp_val;
  pathname_t tmp_string;
  int port;

  ptr_check(input_finfo, file_info_t, "get_input_file", ERROR);
  ptr_check(hostdb_rec, hostdb_t, "get_input_file", ERROR);
  ptr_check(hostaux_rec, hostdb_aux_t, "get_input_file", ERROR);
  ptr_check(hostdb, file_info_t, "get_input_file", ERROR);
  ptr_check(hostaux_db, file_info_t, "get_input_file", ERROR);
  ptr_check(hostbyaddr, file_info_t, "get_input_file", ERROR);      

  /* If it is an IP address */

  if (sscanf(in_hostname, "%*d.%*d.%*d.%d", &tmp_val) == 1)
  {
    if ((dns = ar_open_dns_addr(inet_addr(in_hostname), DNS_LOCAL_FIRST, hostbyaddr))
        != (AR_DNS *)NULL)
    {
      /* bug! file_funct is a pointer to a function returning INT!!! -wheelan */
/*      strcpy(input_finfo->filename, file_funct(in_hostname,port)); */
    }
    else
    {
      /* "Host %s unknown" */
      error(A_ERR, "get_input_file", GET_INPUT_FILE_001, in_hostname);
      return ERROR;
    }
  }
  else
  {
    if ((dns = ar_open_dns_name(in_hostname, DNS_LOCAL_FIRST, hostdb)) != (AR_DNS *)NULL)
    {
      /* bug! file_funct is a pointer to a function returning INT!!! -wheelan */
/*      strcpy(input_finfo->filename, file_funct(inet_ntoa(ipaddr_to_inet(*get_dns_addr(dns))),port)); */
    }
    else
    {
      /* "Host %s unknown" */
      error(A_ERR, "get_input_file", GET_INPUT_FILE_001, in_hostname);
      return ERROR;
    }
  }

  hostname = get_dns_primary_name(dns);
  if (get_dbm_entry(hostname, strlen(hostname) + 1, hostdb_rec, hostdb) == ERROR)
  {
    /* "Can't find host %s in primary database" */
    error(A_ERR, "get_input_file", GET_INPUT_FILE_002, hostname);
    return ERROR;
  }

  sprintf(tmp_string, "%s.%s.%d", hostname, dbname, (int)index);
  if (get_dbm_entry(tmp_string, strlen(tmp_string) + 1, hostaux_rec, hostaux_db) == ERROR)
  {
    /* "Can't find database %s host %s in local databases" */
    error(A_ERR, "get_input_file", GET_INPUT_FILE_003, hostname, dbname);
    return ERROR;
  }

  if( get_port(hostaux_rec->access_command, dbname, &port ) == ERROR ){
    error(A_ERR,"get_port","");
    return ERROR;
  }

  strcpy(input_finfo->filename, file_funct(inet_ntoa(ipaddr_to_inet(*get_dns_addr(dns))),port));  
         
  ar_dns_close(dns);
  return A_OK;
}



status_t unlock_db(lock_file)
   file_info_t *lock_file;

{
   ptr_check(lock_file, file_info_t, "unlock_db", ERROR);

   if(unlink(lock_file -> filename) == -1){

      /* "Can't remove lock file %s !!" */

      error(A_SYSERR,"update_gopherindex", UNLOCK_DB_001, lock_file -> filename);
      return(ERROR);
   }

   return(A_OK);
}



status_t archie_fpcopy(infp, outfp, size)
FILE *infp,*outfp;
int size;
{
  char buff[8*1024];
  int count = 0;
  int finished = 0;
  int nbytes, bytes = 8 *1024;

  while (!finished) {

    if ( size != -1 && size - count < 1024*8 )
        bytes = size-count;


    nbytes = fread(buff,sizeof(char),bytes,infp);
    
    if ( size == -1 && nbytes == 0 ) {
      return A_OK;
    }
    
    if ( size != -1  && nbytes != bytes ) {
      error(A_ERR,"archie_fpcopy","Problem reading from input file, errno = %d",errno);
      return ERROR;
    }
    
    if ( fwrite(buff,sizeof(char),nbytes,outfp) != nbytes ) {
      error(A_ERR,"archie_fpcopy","Problem writing to output file, errno = %d",errno);
	    return ERROR;
    }

    count += bytes;
    if ( count == size )
      finished = 1;
  }
  return A_OK;
}

                       
status_t archie_rename(orig,copy)
pathname_t orig, copy;
{
  pathname_t tmpname;
  char *t;
  int fdi,fdo;
  int nbytes;

  char buff[BUFF_SIZE];

  if ( copy == NULL || orig == NULL || copy[0] == '\0' || copy[0] == '\0' ) {
    return -1;
  }

  if(rename(orig,copy) != 0 ) {

    if ( errno == EXDEV ) {

      strcpy(tmpname,copy);
      t = strrchr(tmpname,'/');
      if ( t == NULL ) 
        t = tmpname;
      else 
        t++;                          /* skip over '/' */

      strcpy(t,COPY_FILENAME);

      if ( (fdo = open(tmpname,O_WRONLY|O_CREAT|O_TRUNC,0644)) == -1 ) {
        error(A_ERR, "archie_rename", RENAME_001,tmpname,errno);
        return -1;
      }

      if ( (fdi = open(orig,O_RDONLY)) == -1 ) {
        error(A_ERR, "archie_rename", RENAME_002,orig,errno);
        close(fdo);
        return -1;
      }   
   
      while ( (nbytes = read(fdi,buff,BUFF_SIZE)) > 0 ) {
        if ( write(fdo,buff,nbytes) != nbytes ) {
          error(A_ERR, "archie_rename", RENAME_003,orig,tmpname,errno);
          close(fdo);
          close(fdi);
          return -1;	 
        }
      }

      close(fdo);
      close(fdi);

      if ( nbytes == -1 ) {
        error(A_ERR, "archie_rename", RENAME_003, orig, tmpname, errno);	 
      }


      if ( rename(tmpname,copy) != 0 ) 
      return -1;

      if ( unlink(orig) != 0 )
      return -1;

    }
    else {
      return -1;
    }

  }
  return 0;
}
