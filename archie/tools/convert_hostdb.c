/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <malloc.h>
#include <memory.h>
#include "protos.h"
#include "typedef.h"
#include "db_files.h"
#include "db_ops.h"
#include "files.h"
#include "host_db.h"
#include "error.h"
#include "lang_tools.h"
#include "archie_strings.h"
#include "master.h"
#include "archie_dbm.h"
#include "utils.h"
#include "old_hostdb.h"





/*
 * convert_hostdb: curses based screen-oriented management tool for archie
 * administrators to interact with the host databases


   argv, argc are used.


   Parameters:
      -M <master database pathname>
		  -h <host database pathname>

 */


extern status_t convert_hostdb(udb_config_t *config_info, file_info_t *old_hostdb,
                               file_info_t *old_hostaux_db, file_info_t *old_hostbyaddr,
                               file_info_t *domaindb, file_info_t *hostdb,
                               file_info_t *hostaux_db, file_info_t *hostbyaddr);


hostname_t *hostlist;
hostname_t *hostptr;
int hostcount;
char *prog;

int main(argc, argv)
   int argc;
   char *argv[];
{
#if 0
#ifdef __STDC__

  extern int getopt(int, char **, char *);
  extern int strcasecmp(char *, char *);

#else

  extern int strcasecmp();
  extern int getopt();
   
#endif   
#endif

  extern char *optarg;
  pathname_t master_database_dir;
  pathname_t host_database_dir;
  hostname_t hostname;
  int option;

  hostname_t source_hostname;

  pathname_t domains;

  file_info_t *old_hostdb = create_finfo();
  file_info_t *old_hostaux_db = create_finfo();
  file_info_t *old_hostbyaddr = create_finfo();
  file_info_t *hostdb = create_finfo();
  file_info_t *hostaux_db = create_finfo();
  file_info_t *hostbyaddr = create_finfo();
  file_info_t *domaindb = create_finfo();
  file_info_t *config_file = create_finfo();
  
  udb_config_t	config_info[MAX_NO_SOURCE_ARCHIES];


  char **cmdline_ptr;
  int cmdline_args;

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;

  master_database_dir[0] = '\0';
  host_database_dir[0] = '\0';
  hostname[0] = '\0';
  domains[0] = '\0';
  source_hostname[0] = '\0';
   

  while((option = (int) getopt(argc, argv, "M:h:")) != EOF){

    switch(option){

      /* Master directory */

    case 'M':
	    strcpy(master_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

      /* host database directory */

    case 'h':
	    strcpy(host_database_dir,optarg);	 
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

    default:

	    /* "Usage: convert_hostdb [-M <dir>]\n\t[-h <dir>]\n" */
	    error(A_INFO, "convert_hostdb", "Usage: convert_hostdb [-M <dir>]\n\t[-h <dir>]");
	    exit(A_OK);
	    break;

    }

  }


  /* set database directories */

  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Can't set master database directory" */

    error(A_ERR, "convert_hostdb", "Can't set master database directory" );
    exit(ERROR);
  }

  if(set_host_db_dir(host_database_dir) == (char *) NULL){

    /* "Can't set host database directory" */

    error(A_ERR, "convert_hostdb", "Can't set host database directory" );
    exit(ERROR);
  }


  /* Rename the hostdbs */

  if (rename_host_dbs(old_hostbyaddr, old_hostdb, old_hostaux_db) != A_OK ) {
    error(A_ERR,"convert_hostdb", "Unable to rename the hostdb files. Please verify the hostdb directory.");
    exit(ERROR);
  }

  if ( open_old_host_dbs(old_hostbyaddr, old_hostdb, old_hostaux_db, O_RDONLY) != A_OK ) {
    /* "Can't open old version of the host database directory" */

    error(A_ERR,"convert_hostdb", "Can't open old version of the host database directory" );
    exit(ERROR);
  }

   
  if(open_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db, O_RDWR) != A_OK){

    /* "Can't open host database directory" */

    error(A_ERR,"convert_hostdb", "Can't open host database directory" );
    exit(ERROR);
  }


  /* Read in list of domains we are responsible for */

  sprintf(config_file -> filename, "%s/%s/%s", get_archie_home(), DEFAULT_ETC_DIR, DEFAULT_RETRIEVE_CONFIG);


  if(read_arupdate_config(config_file, config_info) == ERROR){

    /* "Error reading configuration file %s" */

    error(A_ERR, "arserver", "Error reading configuration file %s", config_file);
    exit(ERROR);

  }


  if ( convert_hostdb( config_info, old_hostdb, old_hostaux_db, old_hostbyaddr,
                      domaindb, hostdb, hostaux_db, hostbyaddr) == ERROR ) {


  }
   
  close_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db);
  close_old_host_dbs(old_hostbyaddr, old_hostdb, old_hostaux_db);

  return(A_OK);
}


static void fix_access_methods( access, attrib_ptr )
  access_methods_t access;
  udb_attrib_t *attrib_ptr;
{

  int dbcount;
  int i;
  char **av;
  access_methods_t tmp;

  tmp[0] = '\0';
  av = str_sep( access, ':' ) ;
  
  for(dbcount = 0;attrib_ptr -> db_name[0] != '\0';attrib_ptr++) {
    
    for ( i = 0; av[i] != NULL && av[i][0] != '\0'; i++ ) {
      if ( strcasecmp(av[i], attrib_ptr -> db_name) == 0 ) {
        if ( tmp[0] == '\0' ) {
          strcpy(tmp, av[i]);
        }
        else {
          strcat(tmp,":");
          strcat(tmp,av[i]);
        }
      }
    }
  }

  strcpy(access,tmp);
  
}

status_t convert_hostdb(config_info, old_hostdb, old_hostaux_db,old_hostbyaddr,
                        domaindb, hostdb, hostaux_db, hostbyaddr)
  udb_config_t	*config_info;
  file_info_t *old_hostdb;
  file_info_t *old_hostaux_db;
  file_info_t *old_hostbyaddr;
  file_info_t *domaindb;
  file_info_t *hostdb;
  file_info_t *hostaux_db;
  file_info_t *hostbyaddr;
{

  int count, dbcount, writable;
  udb_attrib_t *attrib_ptr;

  datum key, result;

  old_hostdb_t old_hostdb_entry;
  old_hostdb_aux_t old_hostaux_entry;
  hostbyaddr_t hostbyaddr_rec;
  hostdb_t hostdb_entry;
  hostdb_aux_t hostaux_entry;
   
  tuple_t **tuple_list;         /* points to list of tuples */
  int tuple_count;
  int i,x;
  domain_t domain_list[MAX_NO_DOMAINS];
  int domain_count;
  char out_domain[2048];

  for(count = 0;
      config_info[count].source_archie_hostname[0] != '\0';
      count++){


    /*
     * Make sure that we need to connect to this host by going through
     * all the databases that it has and checking for the correct
     * permissions
     */

    attrib_ptr = &config_info[count].db_attrib[0];

    for(dbcount = 0, writable = 0 ;attrib_ptr -> db_name[0] != '\0';attrib_ptr++){
	    
      if((attrib_ptr -> perms[0] == 'w') || (attrib_ptr -> perms[1] == 'w')){
        writable = 1;
        break;
      }
    }

    if(!writable)
    continue;

    if(config_info[count].source_archie_hostname[0] == '*')
    continue;

    attrib_ptr = &config_info[count].db_attrib[0];

    for(dbcount = 0;attrib_ptr -> db_name[0] != '\0';attrib_ptr++) {

      if ( strcasecmp(attrib_ptr->db_name, "gopherindex") == 0 ) {
        error(A_INFO, "convert_hostdb", "The database gopherindex is not yet supported\n");
        continue;
      }

      /*	    strcpy(params[0], attrib_ptr -> db_name); */

      /*
       * If you don't get information about this database from this
       * server go to next database
       */

      if((attrib_ptr -> perms[0] != 'w') && (attrib_ptr -> perms[1] != 'w'))
      continue;

      
      if(compile_domains(attrib_ptr -> domains, domain_list, domaindb, &domain_count) == ERROR){

        /* "Can't expand domains '%s' in output" */

        error(A_INTERR, "do_client","Can't expand domains '%s' in output" , attrib_ptr -> domains);

        /* "Ignoring %s, %s" */

        error(A_INTERR, "do_client", "Ignoring %s, %s", config_info[count].source_archie_hostname, attrib_ptr -> db_name);
        continue;
      }

      out_domain[0] = '\0';

      strcpy(out_domain, domain_list[0]);

      for(x = 1; x < domain_count; x++){
        char hold_str[MAX_DOMAIN_LEN];

        sprintf(hold_str, ":%s", domain_list[x]);
        strcat(out_domain, hold_str);
      }

      /*      strcpy(params[3], out_domain); */


      if((tuple_list = old_compose_tuples( attrib_ptr -> db_name , ">", 
                                          cvt_to_inttime("19700101000000",0),
                                          cvt_to_domainlist(out_domain),
                                          old_hostbyaddr, old_hostdb, domaindb,
                                          old_hostaux_db, &tuple_count)) == (tuple_t **) NULL){

        /* "Can't compose tuples" */

        error(A_FATAL,"convert_hostdb", "Can't compose tuple list");
        return ERROR;
      }

      
      for ( i = 0; i < tuple_count; i++ ) {

        hostname_t source_archie_host;
        hostname_t primary_hostname;
        hostname_t preferred_hostname;
        char date_str[EXTERN_DATE_LEN + 1];
        char	ip_string[18];
        database_name_t db_name;
        pathname_t tmp_str;

        
        if ( str_decompose(tuple_list[i],NET_DELIM_CHAR,source_archie_host,
                           date_str,primary_hostname, preferred_hostname,
                           ip_string, db_name) == ERROR ) {
          fprintf(stderr,"cannot decompose tuple\n");
          continue;
        }
    
        if(get_dbm_entry(make_lcase(primary_hostname), strlen(primary_hostname)+1,
                         &old_hostdb_entry, old_hostdb) == ERROR){
          fprintf(stderr,"cannot get entry \n");
          continue;
        }

        sprintf(tmp_str,"%s.%s", make_lcase(primary_hostname), db_name);
      
        if(get_dbm_entry(tmp_str, strlen(tmp_str)+1,
                         &old_hostaux_entry, old_hostaux_db) == ERROR){
          fprintf(stderr,"cannot get entry \n");
          continue;
        }

        memset(&hostdb_entry, 0, sizeof(hostdb_t));
        memset(&hostaux_entry, 0, sizeof(hostdb_aux_t));

        strcpy(hostdb_entry.primary_hostname,old_hostdb_entry.primary_hostname);

        fix_access_methods( old_hostdb_entry.access_methods, &config_info[count].db_attrib[0] );
    
        
        strcpy(hostdb_entry.access_methods,old_hostdb_entry.access_methods);
        hostdb_entry.primary_ipaddr = old_hostdb_entry.primary_ipaddr;
        hostdb_entry.os_type = old_hostdb_entry.os_type;
        hostdb_entry.timezone = old_hostdb_entry.timezone;
        hostdb_entry.flags = old_hostdb_entry.flags;


        hostaux_entry.generated_by = old_hostaux_entry.generated_by;
        strcpy(hostaux_entry.source_archie_hostname,old_hostaux_entry.source_archie_hostname);
        strcpy(hostaux_entry.access_command,old_hostaux_entry.access_command);
        strcpy(hostaux_entry.comment,old_hostaux_entry.comment);
        strcpy(hostaux_entry.preferred_hostname,old_hostdb_entry.preferred_hostname);      
        hostaux_entry.retrieve_time = old_hostaux_entry.retrieve_time ;
        hostaux_entry.parse_time = old_hostaux_entry.parse_time ;
        hostaux_entry.update_time = old_hostaux_entry.update_time ;
        hostaux_entry.no_recs = old_hostaux_entry.no_recs ;
        hostaux_entry.flags = old_hostaux_entry.flags ;

        hostaux_entry.fail_count = old_hostaux_entry.fail_count ;
        hostaux_entry.current_status = old_hostaux_entry.current_status ;      

      
        if ( put_dbm_entry(hostdb_entry.primary_hostname,
                           strlen(hostdb_entry.primary_hostname) + 1,
                           &hostdb_entry, sizeof(hostdb_entry),hostdb, 1) == ERROR ) {
          fprintf(stderr, "Error adding site %s\n",hostdb_entry.primary_hostname);
        }

        strcat(tmp_str,".0");
        if ( put_dbm_entry(tmp_str,strlen(tmp_str) + 1,
                           &hostaux_entry, sizeof(hostaux_entry),hostaux_db, 1) == ERROR ) {
          fprintf(stderr, "Error adding aux entry for site %s\n",hostdb_entry.primary_hostname);
        }

        hostbyaddr_rec.primary_ipaddr = hostdb_entry.primary_ipaddr;
        strcpy(hostbyaddr_rec.primary_hostname, hostdb_entry.primary_hostname);

        if(put_dbm_entry(&hostbyaddr_rec.primary_ipaddr, sizeof(ip_addr_t), &hostbyaddr_rec, sizeof(hostbyaddr_t), hostbyaddr, FALSE) == ERROR) {
          fprintf(stderr, "Error adding aux entry for site %s\n",hostdb_entry.primary_hostname);
        }

        fprintf(stderr,"Found server %s\n",old_hostdb_entry.primary_hostname);

      }
    }
  }
   
  return A_OK;
}




