/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include "protos.h"
#include "typedef.h"
#include "host_db.h"
#include "header.h"
#include "db_files.h"
#include "error.h"
#include "files.h"
#include "lang_tools.h"
#include "master.h"
#include "archie_dbm.h"
#include "archie_strings.h"
#include "times.h"

/*
 * parse_anonftp: handles the parsing of an anonftp listing, spawning the
 * appropriate parser

   argv, argc are used.


   Parameters:	  
*/


#define	 DUMP_ALL    2
#define	 DUMP_SEMI   1
#define	 DUMP_PERM   0

static int verbose = 0;
#if 0
host_status_t rhandle_unknown_host(hostname, dbname, hostdb_rec, hostaux_entry, hostdb, hostaux_db, hostbyaddr)
   char *hostname;
   char *dbname;
   hostdb_t *hostdb_rec;
   hostdb_aux_t *hostaux_entry;
   file_info_t *hostdb;
   file_info_t *hostaux_db;
   file_info_t *hostbyaddr;
{

  host_status_t host_status;

  ptr_check(hostname, char, "handle_unknown_host", HOST_ERROR);
  ptr_check(dbname, char, "handle_unknown_host", HOST_ERROR);
  ptr_check(hostdb_rec, hostdb_t, "handle_unknown_host", HOST_ERROR);
  ptr_check(hostaux_entry, hostdb_aux_t, "handle_unknown_host", HOST_ERROR);
  ptr_check(hostdb, file_info_t, "handle_unknown_host", HOST_ERROR);
  ptr_check(hostaux_db, file_info_t, "handle_unknown_host", HOST_ERROR);
  ptr_check(hostbyaddr, file_info_t, "handle_unknown_host", HOST_ERROR);

  /* The host has disappeared since the last time it was
     updated */

  memset(hostaux_entry, '\0', sizeof(hostdb_aux_t));

  if(get_dbm_entry(hostname, strlen(hostname) + 1, hostdb_rec, hostdb) == ERROR){

    /* we don't currently have the host stored */

    hostdb_rec -> primary_ipaddr = 0;

    strcpy(hostdb_rec -> access_methods, dbname);

  }
  else{

    /* we do have the host */

    get_hostaux_ent(hostname, dbname, hostaux_entry, hostaux_db);

  }


  strcpy(hostaux_entry -> access_methods, dbname);
  hostaux_entry -> update_time = (date_time_t) time((time_t *) NULL);
  hostaux_entry -> retrieve_time = (date_time_t) time((time_t *) NULL);

  hostaux_entry -> current_status = DISABLED;
   

  if(((host_status = update_hostdb(hostbyaddr, hostdb, hostdb_rec, 1)) == ERROR)
     || ((host_status = update_hostaux(hostaux_db, hostname, hostaux_entry, 1)) == ERROR))
  return(host_status);

  return(HOST_OK);
}
#endif


host_status_t rhandle_paddr_mismatch(hostname, dbname, hostdb_rec, hostdb, hostbyaddr)
   char *hostname;
   char *dbname;
   hostdb_t *hostdb_rec;
   file_info_t *hostdb;
   file_info_t *hostbyaddr;
{
  host_status_t host_status;
  hostdb_t local_hostdb_rec;
  hostbyaddr_t hbyaddr_rec1,hbyaddr_rec2;

  ptr_check(hostname, char, "rhandle_paddr_mismatch", HOST_PTR_NULL);
  ptr_check(dbname, char, "rhandle_paddr_mismatch", HOST_PTR_NULL);
  ptr_check(hostdb_rec, hostdb_t, "rhandle_paddr_mismatch", HOST_PTR_NULL);
  ptr_check(hostdb, file_info_t, "rhandle_paddr_mismatch", HOST_PTR_NULL);
  ptr_check(hostbyaddr, file_info_t, "rhandle_paddr_mismatch", HOST_PTR_NULL);

  if(get_dbm_entry(hostname, strlen(hostname) + 1, &local_hostdb_rec, hostdb) == ERROR){


    error(A_INTERR, "rhandle_paddr_mismatch", "Can't find hostname %s in primary host database", hostname);
    return(HOST_DOESNT_EXIST);
  }

  if(get_dbm_entry(&(local_hostdb_rec.primary_ipaddr), sizeof(ip_addr_t), &hbyaddr_rec1, hostbyaddr) == ERROR){

    error(A_INTERR, "rhandle_paddr_mismatch", "Can't find ipaddr record for hostname %s ipaddr %s", hostname, inet_ntoa(ipaddr_to_inet(local_hostdb_rec.primary_ipaddr)));
    return(HOST_DB_ERROR);
  }

  strcpy(hbyaddr_rec2.primary_hostname, hostname);
  hbyaddr_rec2.primary_ipaddr = hostdb_rec -> primary_ipaddr;

  if(put_dbm_entry(&(hostdb_rec -> primary_ipaddr), sizeof(ip_addr_t), &hbyaddr_rec2, sizeof(hostbyaddr_t), hostbyaddr, 0) == ERROR){


    error(A_ERR, "rhandle_paddr_mismatch", "Can't insert new IP address %s into host address cache", inet_ntoa(ipaddr_to_inet(hostdb_rec -> primary_ipaddr)));
    return(HOST_PADDR_EXISTS);
  }

  if(delete_dbm_entry(&(hbyaddr_rec1.primary_ipaddr), sizeof(ip_addr_t), hostbyaddr) == ERROR){


    error(A_ERR, "rhandle_paddr_mismatch", "Can't delete old IP address %s from host address cache", inet_ntoa(ipaddr_to_inet(hbyaddr_rec1.primary_ipaddr)));
    return(HOST_DB_ERROR);
  }

  error(A_INFO, "rhandle_paddr_mismatch", "Primary IP address for %s has changed from %s to %s", hostname,
        inet_ntoa(ipaddr_to_inet(local_hostdb_rec.primary_ipaddr)),
        inet_ntoa(ipaddr_to_inet(hostdb_rec -> primary_ipaddr)));

  local_hostdb_rec.primary_ipaddr = hostdb_rec -> primary_ipaddr;

  if((host_status = update_hostdb(hostbyaddr, hostdb, hostdb_rec, 1)) != HOST_OK)
  return(host_status);

  return(HOST_OK);
}


host_status_t rdo_hostdb_update(hostbyaddr, hostdb, hostaux_db, header_rec, overwrite, host_only)
   file_info_t *hostbyaddr;
   file_info_t *hostdb;
   file_info_t *hostaux_db;   
   header_t    *header_rec;
   int	       overwrite;
   int	       host_only;
{

  hostdb_t hostdb_rec;
  hostdb_t hostdb_entry;
  hostdb_aux_t hostaux_rec;
  hostdb_aux_t hostaux_entry;
  
  host_status_t host_status = 0;
  int p1;
  pathname_t aux_name;
  hostbyaddr_t hostbyaddr_rec;

  int last,j;

  memset(&hostdb_rec, '\0', sizeof(hostdb_t));
  memset(&hostaux_rec, '\0', sizeof(hostdb_aux_t));

  /* Perform the pointer checks */
   
  ptr_check(hostbyaddr,file_info_t, "rdo_hostdb_update", ERROR);
  ptr_check(hostdb, file_info_t, "rdo_hostdb_update", ERROR);
  ptr_check(hostaux_db,file_info_t, "rdo_hostdb_update", ERROR);
  ptr_check(header_rec, header_t, "rdo_hostdb_update", ERROR);

  header_rec -> action_status = UPDATE;
  HDR_SET_ACTION_STATUS(header_rec -> header_flags);

  /* create a hostdb record from the header */

  make_hostdb_from_header(header_rec, &hostdb_rec, 0);
  make_hostaux_from_header(header_rec, &hostaux_rec, 0);

  hostaux_rec.update_time = header_rec -> update_time;

  set_aux_origin(&hostaux_rec, header_rec->access_methods, -1);


  
  if ( get_dbm_entry(hostdb_rec.primary_hostname,
                     strlen(hostdb_rec.primary_hostname)+1,
                     &hostdb_entry, hostdb) == ERROR ) {

    if(put_dbm_entry(hostdb_rec.primary_hostname, strlen(hostdb_rec.primary_hostname) + 1, &hostdb_rec, sizeof(hostdb_t), hostdb, FALSE) != A_OK)
      return(HOST_INS_FAIL);

    hostbyaddr_rec.primary_ipaddr = hostdb_rec.primary_ipaddr;
    strcpy(hostbyaddr_rec.primary_hostname, hostdb_rec.primary_hostname);

    if(put_dbm_entry(&hostbyaddr_rec.primary_ipaddr, sizeof(ip_addr_t), &hostbyaddr_rec, sizeof(hostbyaddr_t), hostbyaddr, FALSE) == ERROR)
      return(HOST_PADDR_FAIL);

  }


  if(host_only)
  return(HOST_OK);

  if(get_dbm_entry(hostdb_rec.primary_hostname, strlen(hostdb_rec.primary_hostname) + 1, &hostdb_entry, hostdb) == ERROR){


    error(A_ERR,"rdo_hostdb_update", "Entry for %s not in host database", hostdb_rec.primary_hostname);
    return(ERROR);
  }

  hostdb_rec.primary_ipaddr = hostdb_entry.primary_ipaddr;

  /* Host already in database */

  find_hostaux_last(hostdb_rec.primary_hostname,
                    header_rec->access_methods, (index_t*)&last, hostaux_db);

  if ( get_port(header_rec->access_command, header_rec->access_methods,
                &p1 ) == ERROR )  {

    error(A_ERR,"restore_hostdb", "Unable to extract port number from header info.");
    return ERROR;
  }

  for ( j = 0; j <= last; j++ ) {
    /* Get the access method list */
    int port;
    
    if ( get_hostaux_entry(hostdb_rec.primary_hostname,
                           header_rec->access_methods,j,
                           &hostaux_entry, hostaux_db) == A_OK) {
      if ( get_port(hostaux_entry.access_command, header_rec->access_methods,
                    &port ) == ERROR )  {
        continue;
      }
      if ( p1 == port ) {
        break;
      }
    }
  }
  
  if ( last == -1 || j > last) {

    pathname_t tmp_string;
    char **aux_dbs;
    char **aux_list;
    int unfound;
    char **newlist;
    int displacement;
      


    /* First break up the list */

    aux_dbs = aux_list = str_sep(hostdb_entry.access_methods, NET_DELIM_CHAR);

    unfound = 0;


    /* Try to find the current database on the list */

    while((*aux_list != (char *) NULL) && (unfound = strcasecmp(*aux_list,header_rec -> access_methods )))
    aux_list++;

    if(unfound){


      /* Can't find it so add it */

      if((newlist = (char **) realloc(aux_dbs, sizeof(char **) * (displacement = (aux_list - aux_dbs)) + 2)) == (char **) NULL){
        error(A_ERR, "Can't malloc space for new auxiliary list");
        return(HOST_MALLOC_ERROR);
      }

      aux_dbs = newlist;
      aux_list = aux_dbs + displacement;
	 

      *aux_list = (char *) strdup(header_rec -> access_methods);
      *(aux_list +1) = (char *) NULL;
    }

    sprintf(hostdb_rec.access_methods, "%s", *aux_dbs);

    aux_list = aux_dbs + 1;

    while((*aux_list != (char *) NULL) && (*aux_list[0] != '\0')){

      sprintf(tmp_string, ":%s",*aux_list);
      strcat(hostdb_rec.access_methods, tmp_string);
      aux_list++;

    }

    /*    strcpy(hostaux_entry.access_methods, header_rec -> access_methods); */

    /* Insert it into the datababase */

    if(put_dbm_entry(hostdb_rec.primary_hostname, strlen(hostdb_rec.primary_hostname) + 1, &hostdb_rec, sizeof(hostdb_t), hostdb, TRUE) != A_OK){


      error(A_ERR,"rdo_hostdb_update", "Can't insert %s into primary host database", hostdb_rec.primary_hostname);
      return(HOST_INS_FAIL);
    }

    sprintf(tmp_string,"%s.%s.%d", hostdb_rec.primary_hostname,
            header_rec->access_methods, j );
    
    if(put_dbm_entry(tmp_string, strlen(tmp_string)+1, &hostaux_rec,
                     sizeof(hostdb_aux_t),hostaux_db, FALSE) != A_OK ) {

      error(A_ERR,"rdo_hostdb_update", "Can't insert %s into aux host database", hostdb_rec.primary_hostname);
      return(HOST_INS_FAIL);
    }
  }

  return(HOST_OK);
}



int main(argc, argv)
   int argc;
   char *argv[];

{
   extern int opterr;
   extern char *optarg;

   char **cmdline_ptr;
   int cmdline_args;

   int option;


   /* Directory pathnames */

   pathname_t master_database_dir;
   pathname_t host_database_dir;

   domain_t domain_list[MAX_NO_DOMAINS];

   pathname_t domains;
   int domain_count;

   int dump_level = -1;
   int tmpdump = 0;
   header_t in_header;
   host_status_t host_ret;
   int host_only;
   hostname_t last_host;
   int overwrite = 0;
   int count;

   pathname_t dumpdate;

   pathname_t inbuf;

   file_info_t *hostbyaddr = create_finfo();
   file_info_t *hostdb = create_finfo();
   file_info_t *hostaux_db = create_finfo();
   file_info_t *domaindb = create_finfo();

   file_info_t *input_file = create_finfo();

   opterr = 0;

   domains[0] = host_database_dir[0] = master_database_dir[0] = '\0';
   last_host[0] = '\0';

   /* ignore argv[0] */

   cmdline_ptr = argv + 1;
   cmdline_args = argc - 1;

   while((option = (int) getopt(argc, argv, "h:M:D:i:e:Ov")) != EOF){

      switch(option){

	 /* Domains list */

	 case 'D':
	    strcpy(domains,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;


	 /* master database directory */

	 case 'M':
	    strcpy(master_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 case 'O':
	    overwrite = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

	 /* input file */

	 case 'i':
	    strcpy(input_file -> filename,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* dump level */

	 case 'e':
	    dump_level = atoi(optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;
	    

	 /* host database directory */

	 case 'h':
	    strcpy(host_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* verbose */

	 case 'v':
	    verbose = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

	 default:
	    fprintf(stderr, "Usage: %s [ -D <domains> ]\n", argv[0]);
	    fprintf(stderr, "[ -v ] [ -e <dump level> ] [ -M <dir> ]");
	    fprintf(stderr, "[ -h <dir> ] [ -i <input file> ] [ -O ]"); 
	    break;
      }

   }


   if(set_master_db_dir(master_database_dir) == (char *) NULL){

      /* "Error while trying to set master database directory" */

      error(A_ERR, "dump_hostdb", "Error while trying to set master database directory" );
      exit(ERROR);
   }


   if(set_host_db_dir(host_database_dir) == (char *) NULL){

      /* "Error while trying to set host database directory" */

      error(A_ERR,"dump_hostdb", "Error while trying to set host database directory");
      exit(A_OK);
   }

   if(open_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db, O_RDWR) != A_OK){

      /* "Error while trying to open host databases" */

      error(A_ERR,"dump_hostdb", "Error while trying to open host databases" );
      exit(A_OK);
   }


   if(input_file -> filename[0] == '\0')
      input_file -> fp_or_dbm.fp = stdin;
   else{

      if(open_file(input_file, O_RDONLY) == ERROR){

	 /* "Can't open input file %s" */

	 error(A_ERR, "dump_hostdb",  "Can't open input file %s", input_file -> filename);
	 exit(ERROR);
      }
   }

   if(domains[0] != '\0'){

      if(compile_domains(domains, domain_list, domaindb, &domain_count) == ERROR){

	 /* "Can't compile given domain list %s" */

	 error(A_ERR, "dump_hostdb", "Can't compile given domain list %s" , domains);
	 exit(ERROR);
      }
   }

   fgets(inbuf, sizeof(inbuf), input_file -> fp_or_dbm.fp);

   if(sscanf(inbuf, "%d %s %d", &count, dumpdate, &tmpdump) != 3){
      error(A_ERR, "Can't read dump header from %s", inbuf);
      exit(ERROR);
   }

   if(dump_level == -1)
      dump_level = tmpdump;
   else if(dump_level > tmpdump){
      error(A_ERR, "restore_hostdb", "One may not restore at a level (%d) greater than the original dump (%d). Aborting.", dump_level, tmpdump);
      exit(ERROR);
   }
	 
   
      

   /* read header on dump file */

   error(A_INFO, "restore_hostdb", "Dump done %s. %d sites, level %d", cvt_to_usertime(cvt_to_inttime(dumpdate, 0), 0), count, dump_level);

   while(1){
     header_flags_t hflags;

     memset(&in_header, '\0', sizeof(header_t));

     if(read_header(input_file -> fp_or_dbm.fp, &in_header, (u32 *) NULL, 0, 0) == ERROR)
     if(count == 0) {
       close_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db);
       exit(A_OK);
     }
     else {
       close_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db);
       exit(ERROR);
     }

     if(!find_in_domains(in_header.primary_hostname, domain_list, domain_count)){

       if(verbose)
       error(A_INFO, "restore_hostdb", "%s not in desired domain. Ignoring.", in_header.primary_hostname);

       continue;
     }


     hflags = in_header.header_flags;

     in_header.header_flags = 0;

     HDR_SET_PRIMARY_HOSTNAME(hflags);
     HDR_SET_ACCESS_METHODS(hflags);

     switch(dump_level){

     case DUMP_ALL:
       in_header.header_flags = hflags;
       break;

     case DUMP_SEMI:

       HDR_SET_PRIMARY_HOSTNAME(in_header.header_flags);
       HDR_SET_ACCESS_METHODS(in_header.header_flags);

       if(HDR_GET_PREFERRED_HOSTNAME(hflags))
       HDR_SET_PREFERRED_HOSTNAME(in_header.header_flags);

       if(HDR_GET_PRIMARY_IPADDR(hflags))
       HDR_SET_PRIMARY_IPADDR(in_header.header_flags);

       if(HDR_GET_OS_TYPE(hflags))
       HDR_SET_OS_TYPE(in_header.header_flags);

       if(HDR_GET_TIMEZONE(hflags))
       HDR_SET_TIMEZONE(in_header.header_flags);

       if(HDR_GET_PROSPERO_HOST(hflags))
       HDR_SET_PROSPERO_HOST(in_header.header_flags);

       if(HDR_GET_SOURCE_ARCHIE_HOSTNAME(hflags))
       HDR_SET_SOURCE_ARCHIE_HOSTNAME(in_header.header_flags);

       if(HDR_GET_ACCESS_COMMAND(hflags))
       HDR_SET_ACCESS_COMMAND(in_header.header_flags);

       if(HDR_GET_CURRENT_STATUS(hflags))
       HDR_SET_CURRENT_STATUS(in_header.header_flags);
       if(HDR_GET_PRIMARY_IPADDR(hflags))
       HDR_SET_PRIMARY_IPADDR(in_header.header_flags);

       if (HDR_GET_RETRIEVE_TIME(hflags))
       HDR_SET_RETRIEVE_TIME(in_header.header_flags);
       
       if (HDR_SET_PARSE_TIME(hflags))
       HDR_SET_PARSE_TIME(in_header.header_flags);

       if(HDR_SET_UPDATE_TIME(hflags))
       HDR_SET_UPDATE_TIME(in_header.header_flags);
       
       break;

     case DUMP_PERM:
     default:
       break;
     }

     host_only = strcasecmp(last_host, in_header.primary_hostname);
     host_only = 0;
     strcpy(last_host, in_header.primary_hostname);

     if((host_ret = rdo_hostdb_update(hostbyaddr, hostdb, hostaux_db, &in_header, overwrite, host_only)) != HOST_OK){

       error(A_ERR, "update_hostdbs", "Can't update host databases for %s.%s name: %s", in_header.primary_hostname, in_header.access_methods, get_host_error(host_ret));
       continue;
     }


     if(verbose){
       if(host_only)
       error(A_INFO, "restore_hostdb", "Adding %s", in_header.primary_hostname);
       else
       error(A_INFO, "restore_hostdb", "Adding %s, %s", in_header.primary_hostname, in_header.access_methods);
     }

     if(host_only)
     count--;
   }

      
}
