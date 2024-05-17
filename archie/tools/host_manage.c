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
#include "files.h"
#include "host_manage.h"
#include "host_db.h"
#include "error.h"
#include "lang_tools.h"
#include "archie_strings.h"
#include "master.h"
#include "screen.h"
#include "archie_dbm.h"


static file_info_t *hostdb;
static file_info_t *hostaux_db;
static file_info_t *hostbyaddr;
static file_info_t *domaindb;



void clean_shutdown()
{
  close_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db);
}


/*
 * host_manage: curses based screen-oriented management tool for archie
 * administrators to interact with the host databases


   argv, argc are used.


   Parameters:	  -M <master database pathname>
		  -h <host database pathname>
		  -C <configuration file>
		  -D <domain list>
		  <site name> 

 */


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
   hostname_t my_hosts;

   file_info_t *dbspec_config = create_finfo();

   pathname_t domains;

   dbspec_t *dbspec_list;

   char **cmdline_ptr;
   int cmdline_args;

   cmdline_ptr = argv + 1;
   cmdline_args = argc - 1;

   master_database_dir[0] = '\0';
   host_database_dir[0] = '\0';
   hostname[0] = '\0';
   domains[0] = '\0';
   source_hostname[0] = '\0';
   
   hostdb = create_finfo();
   hostaux_db = create_finfo();
   hostbyaddr = create_finfo();
   domaindb = create_finfo();



   while((option = (int) getopt(argc, argv, "M:h:C:D:H:")) != EOF){

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

	 /* Configuration file */

	 case 'C':
	    strcpy(dbspec_config -> filename, optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* Domain list */
	 case 'D':
	    strcpy(domains, optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* set source hostname */
	 case 'H':
	    strcpy(source_hostname, optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 case 'm':
	    strcpy(my_hosts, optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 default:

	    /* "Usage: host_manage [-M <dir>]\n\t[-h <dir>]\n\t[-C <config file>]\n\t[-D <domain list>]\n\t[-H <source hostname>]\n\t[ <sitename> ]" */
	    error(A_INFO, "host_manage", HOST_MANAGE_054);
	    exit(A_OK);
	    break;

      }

   }

   /* We have an initialization site given */

   if(cmdline_args != 0)
      strcpy(hostname, *cmdline_ptr);


   /* set database directories */

   if(set_master_db_dir(master_database_dir) == (char *) NULL){

     /* "Can't set master database directory" */

     error(A_ERR, "host_manage", HOST_MANAGE_001);
     exit(ERROR);
   }

   if(set_host_db_dir(host_database_dir) == (char *) NULL){

     /* "Can't set host database directory" */

     error(A_ERR, "host_manage", HOST_MANAGE_002);
     exit(ERROR);
   }

   if(open_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db, O_RDWR) != A_OK){

      /* "Can't open host database directory" */

      error(A_ERR,"host_manage", HOST_MANAGE_003);
      exit(ERROR);
   }

   if((dbspec_list = (dbspec_t *) malloc(sizeof(dbspec_t) * 5)) == (dbspec_t *) NULL){

      /* "Can't allocated space for auxiliary host database list" */

      error(A_ERR, "host_manage", HOST_MANAGE_004);
      exit(ERROR);
   }

   if(dbspec_config -> filename[0] == '\0')
      sprintf(dbspec_config -> filename, "%s/%s/%s", get_archie_home(), DEFAULT_ETC_DIR, DEF_DBSPEC_FILE);

   if(read_dbspecs(dbspec_config, dbspec_list) == ERROR){

      /* "Error reading host_manage configuration file" */

      error(A_ERR, "host_manage", HOST_MANAGE_005);
      exit(ERROR);
   }

   /* initialize curses */
   
   if(setup_screen() == ERROR)
      exit(ERROR);

   /* Set up signal handling */

   signal(SIGTSTP, sig_handle);
   signal(SIGTERM, sig_handle);
   signal(SIGINT, sig_handle);   
   signal(SIGCONT, sig_handle);   

   if(source_hostname[0] == '\0'){

      if(get_archie_hostname(source_hostname, sizeof(source_hostname)) == (char *) NULL){

	 /* "Can't get hostname of this host" */

	 error(A_SYSERR, "host_manage", HOST_MANAGE_055);
	 exit(ERROR);
      }
   }

      
   if(host_manage(hostdb, hostaux_db, hostbyaddr, domaindb, domains, hostname, dbspec_list, source_hostname, my_hosts) == ERROR){

      /* "Error in program" */
      
      error(A_ERR,"host_manage", HOST_MANAGE_032);
      exit(ERROR);
   }

   teardown_screen();

   return(A_OK);
}

/*
 * host_manage: main routine to perform functions of program
 */


status_t host_manage(hostdb, hostaux_db, hostbyaddr, domaindb, domains, hostname, dbspec_list, source_hostname, my_hosts)
   file_info_t *hostdb;	      /* primary host database */
   file_info_t *hostaux_db;   /* auxiliary host database */
   file_info_t *hostbyaddr;   /* host by address cache */
   file_info_t *domaindb;     /* domains database */
   char	       *domains;      /* list of domains */
   hostname_t  hostname;      /* hostname of initial host, if any */
   dbspec_t    *dbspec_list;  /* Auxiliary database list */
   hostname_t  source_hostname; /* Name to use in source: field */
   hostname_t  my_hosts;      /* only list sites who match my_hosts as source */
{

#if 0 
#ifdef __STDC__

  extern int strcasecmp(char *, char *);
  extern time_t time(time_t *);
#else

  extern int strcasecmp();
  extern time_t time();

#endif
#endif   

   
  extern dbspec_t *glob_dbspec;
  extern int in_hostlist;

  action_status_t action_status;
  host_status_t host_status;

  hostdb_t orig_hostdb;
  hostdb_t new_hostdb;


  hostdb_aux_t orig_hostaux_db;
  hostdb_aux_t new_hostaux_db;

  hostaux_index_t *hit;
  
  domain_t domain_list[MAX_NO_DOMAINS];
  int retval;

  int domain_count = 0;

  flags_t flags = 0;
  int control_flags = 0;

  char **aux_dbs = (char **) NULL;
  char **aux_list = (char **) NULL;   

  char **test_list = (char **) NULL;

  ptr_check(hostdb, file_info_t, "host_manage", ERROR);
  ptr_check(hostaux_db, file_info_t, "host_manage", ERROR);
  ptr_check(hostbyaddr, file_info_t, "host_manage", ERROR);
  ptr_check(hostname, char, "host_manage", ERROR);
  ptr_check(dbspec_list, dbspec_t, "host_manage", ERROR);   


  /* Zero out the structures */

  memset(&orig_hostdb, '\0', sizeof(hostdb_t));
  memset(&new_hostdb, '\0', sizeof(hostdb_t));
  orig_hostaux_db.origin = NULL;
  new_hostaux_db.origin = NULL;
  clean_hostaux(&orig_hostaux_db);
  clean_hostaux(&new_hostaux_db);


  error(A_INFO,"host_manage","Reading host database...");


  if(domains[0] != '\0'){
    if(compile_domains(domains, domain_list, domaindb, &domain_count) == ERROR){

      /* "Can't compile domain list %s" */

      error(A_ERR, "host_manage", HOST_MANAGE_053, domains);
      return(ERROR);
    }
      
  }

  if(get_hostnames(hostdb, &hostlist, &hostcount, domain_list, domain_count) == ERROR){

    /* "Can't get list of hostnames from database" */

    error(A_ERR, "host_manage", HOST_MANAGE_007);
    return(ERROR);
  }


  strcpy(new_hostdb.primary_hostname, hostname);

  /*
   * if hostname is NULL then we go from beginning of database, otherwise
   * start with the one we've been given
   */

  if(new_hostdb.primary_hostname[0] == '\0'){
    hostptr = hostlist;
    strcpy(hostname, (char *) hostptr);
  }
  else{
    AR_DNS *dns;
    flags_t flags;
    int control_flags;


    /*
     * Look up given name in local database first, then if not found, in
     * DNS
     */

    if((dns = ar_open_dns_name(new_hostdb.primary_hostname, DNS_LOCAL_FIRST, hostdb))== (AR_DNS *) NULL){

      /* "Can't locate host '%s' in local database or DNS" */

      error(A_ERR,"host_manage", HOST_MANAGE_008, hostname);
      exit(ERROR);
    }

    make_lcase(hostname);
      
      
    /* find hostname in list of sites stored */
   
    if((hostptr = (hostname_t *) bsearch ( (char *) hostname, (char *) hostlist, (unsigned) hostcount, sizeof(hostname_t), strcasecmp)) == (hostname_t *) NULL){

      /* Not found, so must be a new entry for the database */

      strcpy(orig_hostdb.primary_hostname, hostname);

      if((host_status = check_new_hentry(hostbyaddr, hostdb, &orig_hostdb, hostaux_db, NULL, flags, control_flags)) == HOST_OK){

        /* If the host checks out then set default values */

        action_status = NEW;
        orig_hostaux_db.current_status = INACTIVE;
        orig_hostdb.primary_ipaddr = *get_dns_addr(dns);
      }
      else{

        /* Host doesn't check out. Send error message */

        send_response(get_host_error(host_status));
        new_hostdb = orig_hostdb;
        copy_hostaux(&new_hostaux_db, &orig_hostaux_db);

        /*
         * Find site in list. Note that orig_hostdb can be changed by
         * check_new_hentry
         */

        if((hostptr = (hostname_t *) bsearch ( (char *) orig_hostdb.primary_hostname, (char *) hostlist, (unsigned) hostcount, sizeof(hostname_t), strcasecmp)) == (hostname_t *) NULL){

          /* "Internal error. Can't resolve hostname in database" */


          send_response(HOST_MANAGE_009);
        }
      }

    }

    /* "close" the DNS stream */

    ar_dns_close(dns);
  }

  /* hostptr points to the site we want to give info on */


  if(hostptr != (hostname_t *) NULL){

    if(hostptr[0][0] != '\0'){

      action_status = UPDATE;

      if(get_dbm_entry(hostptr, strlen(*hostptr) +1, &orig_hostdb, hostdb) == ERROR){

        /* "Can't find %s in primary host database" */

        error(A_ERR, "host_manage", HOST_MANAGE_010, hostptr);
        return(ERROR);
      }
	 
      if(orig_hostdb.access_methods[0] != '\0'){
        pathname_t tmp_name;
        index_t index;

        if(aux_dbs == (char **) NULL)
        aux_dbs = aux_list = str_sep(orig_hostdb.access_methods,
                                     NET_DELIM_CHAR);

        if ( get_hostaux_ent(orig_hostdb.primary_hostname, *aux_list, &index,
                             NULL,NULL,&orig_hostaux_db, hostaux_db) == ERROR )
        /*                           
           sprintf(tmp_name,"%s.%s", orig_hostdb.primary_hostname, *aux_list);
           
           if(get_dbm_entry(tmp_name, strlen(tmp_name) + 1, &orig_hostaux_db, hostaux_db) == ERROR)
           */
        /* "Can't find %s in %s" */

        error(A_ERR, "host_manage", HOST_MANAGE_011 , tmp_name, hostaux_db -> filename);
        else 
        set_aux_origin(&orig_hostaux_db, *aux_list, index);
      }

      new_hostdb = orig_hostdb;
      copy_hostaux(&new_hostaux_db, &orig_hostaux_db);

      if(hostcount != 1){

        /* "%d sites in database, hostname set to %s" */

        send_response( HOST_MANAGE_012, hostcount, source_hostname);
      }
      else{

        /* "1 site in database, hostname set to %s" */

        send_response(HOST_MANAGE_013, source_hostname);
      }

    }
    else{
      send_response(HOST_MANAGE_015);
      action_status = NEW;
    }

  }

  if ( orig_hostaux_db.origin == NULL ) {
    set_aux_origin(&orig_hostaux_db, "", -1 );
  }

  if ( new_hostaux_db.origin == NULL ) {
    set_aux_origin(&new_hostaux_db, "", -1 );
  }

  /* Main controlling loop */


  do{

    index_t index;
    /* Display current record */

    if(display_records(&new_hostdb, &new_hostaux_db, dbspec_list, action_status) == ERROR){

      /* "Can't display current record!" */

      error(A_ERR,"host_manage", HOST_MANAGE_015);
      return(ERROR);
    }

    /* Read input and process it */

    retval = process_input(&new_hostdb, &new_hostaux_db, glob_dbspec, &action_status);

    get_new_acommand(new_hostaux_db.origin->access_methods, new_hostaux_db.access_command, dbspec_list);

    make_lcase(new_hostdb.primary_hostname);

    /* Check if current entry and original entry differ */

    if(((hostdb_cmp(&orig_hostdb, &new_hostdb, (action_status == NEW) ) == ERROR)
        || (hostaux_cmp(&orig_hostaux_db, &new_hostaux_db, !in_hostlist) == ERROR))
       && (retval != UPDATE_SITE)){

      /* "Lose current changes to %s ? " */

      if(get_yn_question(HOST_MANAGE_016, action_status == NEW ? new_hostdb.primary_hostname : orig_hostdb.primary_hostname) == ERROR)
	    continue;

    }

    clear_response();
      
    /* Perform requested command */

    switch(retval){

      /* leave program */

    case EXIT_SITE:
	    return(A_OK);
	    break;

      /* Error */

    case -1:
	    return(ERROR);
	    break;


      /* Goto given site */

    case GOTO_SITE:
	    strcpy(hostname, new_hostdb.primary_hostname);
	    if(strcasecmp(orig_hostaux_db.preferred_hostname, new_hostaux_db.preferred_hostname) == 0)
      new_hostaux_db.preferred_hostname[0] = '\0';
	    {
        hostname_t *hold_ptr;

        if((hold_ptr = (hostname_t *) bsearch ( (char *) hostname, (char *) hostlist, (unsigned) hostcount, sizeof(hostname_t), strcasecmp)) != (hostname_t *) NULL)
        hostptr = hold_ptr;
	    }

	    if(in_hostlist)
      in_hostlist = 0;

	    break;

      /* Goto given auxiliary database */

    case GOTO_DATABASE:

	    {
        char **aux_tmp = (char **) NULL;
        int fnd;

        if(aux_dbs == (char **) NULL){

          /* "No databases for site %s" */

          send_response(HOST_MANAGE_042,new_hostaux_db.origin->access_methods, new_hostdb.primary_hostname);
          continue;
        }

        make_lcase(new_hostaux_db.origin->access_methods);

        for(aux_tmp = aux_dbs; (*aux_tmp != (char *) NULL) && (fnd = strcasecmp(new_hostaux_db.origin->access_methods, *aux_tmp)); aux_tmp++);

        if(fnd){

          /* "No database '%s' for site %s" */

          send_response(HOST_MANAGE_034,new_hostaux_db.origin->access_methods, new_hostdb.primary_hostname);
          strcpy(new_hostaux_db.origin->access_methods, *aux_list);
        }
        else{
          pathname_t tmp_name;

          aux_list = aux_tmp;

          sprintf(tmp_name,"%s.%s", new_hostdb.primary_hostname, *aux_list);

          hit = new_hostaux_db.origin;
          if(get_dbm_entry(tmp_name, strlen(tmp_name) + 1, &new_hostaux_db, hostaux_db) == ERROR){

            /* "Can't get auxiliary database entry for host %s with non-NULL access methods" */

            send_response(HOST_MANAGE_031, new_hostdb.primary_hostname);
            hit = new_hostaux_db.origin;            
            clean_hostaux(&new_hostaux_db);

          }
          else
            hit = new_hostaux_db.origin;
          
          if(new_hostaux_db.current_status == 0)
          new_hostaux_db.current_status = DISABLED;
          copy_hostaux(&orig_hostaux_db, &new_hostaux_db);
        }
	    }

	    continue;

	    break;		      


    case HOSTLIST:

	    in_hostlist = 1;
	    display_hostlist();
	    break;

      /* Add database */

    case ADD_HOSTAUX:

      clean_hostaux( &new_hostaux_db );
      clean_hostaux( &orig_hostaux_db );
      
	    action_status = NEW;

	    new_hostaux_db.current_status = orig_hostaux_db.current_status = DISABLED;

	    strcpy(orig_hostaux_db.source_archie_hostname, source_hostname);
	    strcpy(new_hostaux_db.source_archie_hostname, source_hostname);

	    /* "Add database" */

	    send_response(HOST_MANAGE_035);
	    continue;
	    break;
	    
      /* Add a new site */

    case ADD_SITE:

	    memset(&orig_hostdb, '\0', sizeof(hostdb_t));
	    memset(&new_hostdb, '\0', sizeof(hostdb_t));

      clean_hostaux(&new_hostaux_db);
      clean_hostaux(&orig_hostaux_db);

      
	    free_opts(aux_dbs);
	    aux_dbs = (char **) NULL;
	    action_status = NEW;

	    new_hostaux_db.current_status = orig_hostaux_db.current_status = DISABLED;

	    strcpy(orig_hostaux_db.source_archie_hostname, source_hostname);
	    strcpy(new_hostaux_db.source_archie_hostname, source_hostname);

	    /* "Add site" */

	    send_response(HOST_MANAGE_017);
	    continue;
	    break;

    case UPDATE_SITE:

	    if(new_hostdb.primary_hostname[0] == '\0'){
	    
        /* "Hostname required" */

        send_response(HOST_MANAGE_018);
        continue;
	    }

	    
	    /* Check to see if the record was modified */

      if(((hostdb_cmp(&orig_hostdb, &new_hostdb, 1) == A_OK)) && (hostaux_cmp(&orig_hostaux_db, &new_hostaux_db, 1) == A_OK)){

        /* "No changes to original record" */

        send_response(HOST_MANAGE_019);
        continue;
	    }

	    if(hostdb_cmp(&orig_hostdb, &new_hostdb, 1) == ERROR){

        pathname_t tmp_string;
        hostdb_aux_t ha_ent;
		  
        /* Make sure that the information jives */
	    
        if(check_update(hostbyaddr, hostdb, &new_hostdb, hostaux_db, &new_hostaux_db, dbspec_list, flags, control_flags, action_status) == A_OK){

          /* Check that they really want to update the record */

          /* "Do you want to update %s ?" */

          if(get_yn_question(HOST_MANAGE_020, new_hostdb.primary_hostname) == ERROR)
          continue;
	      

          if(get_hostaux_ent(new_hostdb.primary_hostname,
                             new_hostaux_db.origin->access_methods, &index,
                             new_hostaux_db.preferred_hostname,
                             new_hostaux_db.access_command, &ha_ent, hostaux_db) == ERROR){

            if(addto_auxdbs(&aux_dbs, new_hostaux_db.origin->access_methods, &aux_list) == ERROR){

              /* "Database already exists for given host" */

              send_response(HOST_MANAGE_052);
              continue;
            }

            sprintf(new_hostdb.access_methods, "%s", *aux_dbs);

            aux_list = aux_dbs + 1;

            for(;*aux_list != (char *) NULL; aux_list++){

              sprintf(tmp_string, ":%s",*aux_list);
              strcat(new_hostdb.access_methods, tmp_string);
            }

            aux_list--;

          }
          /* Try to update the record */

          if(do_update(hostbyaddr, hostdb, hostaux_db, &new_hostdb, &new_hostaux_db,action_status) == ERROR){

            /*  "Can't update host" */

            error(A_ERR,"host_manage", HOST_MANAGE_021);
            return(ERROR);
          }

          /* This is hitting a small problem with a big hammer. Get
             the new list of hostnames including the one just added.
             Easier than doing the memory mgmt, sorting etc, manually */
		    
          if(get_hostnames(hostdb, &hostlist, &hostcount, domain_list, domain_count) == ERROR){

            /* "Can't get new list of hosts" */

            error(A_ERR, "host_manage", HOST_MANAGE_023);
            return(ERROR);
          }

          /* Find host that was just added in list */

          strcpy(hostname, new_hostdb.primary_hostname);

          if((hostptr = (hostname_t *) bsearch ( (char *) hostname, (char *) hostlist, (unsigned) hostcount, sizeof(hostname_t), strcasecmp)) == (hostname_t *) NULL){

            /* we've got a real problem here */

            /* "Can't find host just updated in database" */

            error(A_ERR, "host_manage", HOST_MANAGE_024);
            return(ERROR);
          }

          free_opts(aux_dbs);
          aux_dbs = (char **) NULL;
        }
        else{
		  
          continue;
        }
	    }

	    /* Check to see if the auxiliary database needs to be updated */

	    if(hostaux_cmp(&orig_hostaux_db, &new_hostaux_db, 1) == ERROR){
        pathname_t tmp_name;
        pathname_t tmp_string;
        hostdb_aux_t tmp_hostaux;
        int asked = 0;

        if(new_hostaux_db.origin->access_methods[0] == '\0')
        break;

        make_lcase(new_hostdb.primary_hostname);
        make_lcase(new_hostaux_db.origin->access_methods);
        if ( new_hostaux_db.origin->hostaux_index == -1 ) {
          index_t index;
          find_hostaux_last( new_hostdb.primary_hostname,
                             new_hostaux_db.origin->access_methods,
                             &index, hostaux_db );
          index++;
          new_hostaux_db.origin->hostaux_index = index;
        }
        sprintf(tmp_name,"%s.%s.%d", new_hostdb.primary_hostname,
                new_hostaux_db.origin->access_methods,
                (int)new_hostaux_db.origin->hostaux_index);


        if(get_dbm_entry(tmp_name, strlen(tmp_name) + 1, &tmp_hostaux, hostaux_db) == ERROR){
          if(aux_dbs == (char **) NULL)
          aux_dbs = str_sep(orig_hostdb.access_methods,NET_DELIM_CHAR);
    
          if(addto_auxdbs(&aux_dbs, new_hostaux_db.origin->access_methods, &aux_list) == ERROR){

            /* "Database already exists for given host" */

            send_response(HOST_MANAGE_052);
            continue;
          }

          /* add it to primary host database record */

          sprintf(new_hostdb.access_methods, "%s", *aux_dbs);

          aux_list = aux_dbs + 1;

          while(*aux_list != (char *) NULL){

            sprintf(tmp_string, ":%s",*aux_list);
            strcat(new_hostdb.access_methods, tmp_string);
            aux_list++;
          }

          aux_list--;


          /* "Do you want to update %s database ?" */

          if(get_yn_question(HOST_MANAGE_040, new_hostaux_db.origin->access_methods) == ERROR)
          continue;
          else
          asked = 1;

          /* update primary host database entry */

          if(put_dbm_entry(new_hostdb.primary_hostname, strlen(new_hostdb.primary_hostname) + 1, &new_hostdb, sizeof(hostdb_t), hostdb, 1) == ERROR){

            /* "Can't update primary host database entry for new database" */

            error(A_ERR, "host_manage", HOST_MANAGE_038);
            return(ERROR);
          }
        }
        else {
          tmp_hostaux.origin = NULL;
          set_aux_origin(&tmp_hostaux, new_hostaux_db.origin->access_methods,
                                       (int)new_hostaux_db.origin->hostaux_index);
          
        }

        /* "Do you want to update %s database ?" */

        if(!asked) 
        if(get_yn_question(HOST_MANAGE_040, tmp_hostaux.origin->access_methods) == ERROR)
        continue;

        /* update the auxiliary database */

        if(put_dbm_entry(tmp_name, strlen(tmp_name) + 1, &new_hostaux_db, sizeof(hostdb_aux_t), hostaux_db, 1) == ERROR){

          /* "Can't update new or modified auxilary host database entry" */

          error(A_ERR, "host_manage", HOST_MANAGE_039);
          return(ERROR);
        }
	    }

/*      orig_hostdb = new_hostdb; */
	    memcpy(&orig_hostdb, &new_hostdb, sizeof(hostdb_t) );
      copy_hostaux(&orig_hostaux_db, &new_hostaux_db);

	    /* "Host %s, database %s updated" */

	    send_response(HOST_MANAGE_022, new_hostdb.primary_hostname, new_hostaux_db.origin->access_methods);

	    continue;
	    break;


      /* Go to first site */

    case FIRST_SITE:

	    action_status = UPDATE;
	    hostptr = hostlist;
	    strcpy((char *) hostname, (char *) hostptr);

	    if(in_hostlist)
      display_hostlist();

	    break;

      /* go to last site */

    case LAST_SITE:
	    action_status = UPDATE;
	    hostptr = hostlist + hostcount - 1;
	    strcpy((char *) hostname, (char *) hostptr);

	    if(in_hostlist)
      display_hostlist();
	    
	    break;

      /* Next site in list */

    case NEXT_SITE:

	    action_status = UPDATE;

	    if(hostptr != (hostname_t *) NULL){

        if(hostptr >  hostlist + hostcount){
          hostptr = hostlist + hostcount;

          /* "Last site in database" */

          send_response(HOST_MANAGE_025);
        }
        else{

          if(hostcount <= 1){

            /* "Last site in database" */

            send_response(HOST_MANAGE_025);
            continue;
          }

          if(hostptr != hostlist + hostcount -1 )
          hostptr++;
          else{

            /* "Last site in database" */

            send_response(HOST_MANAGE_025);
          }
        }
	    }
	    else
      hostptr = hostlist;

	    strcpy((char *) hostname, (char *) hostptr);

	    free_opts(aux_dbs);
	    aux_dbs = (char **) NULL;

	    if(in_hostlist)
      display_hostlist();

	    break;


#if 0
	    if(hostptr != (hostname_t *) NULL){

        if(hostptr >= hostlist + hostcount){
          hostptr = hostlist + hostcount;

          /* "Last site in database" */

          send_response(HOST_MANAGE_025);
          continue;
        }
        else{
          if(hostptr + 1 < hostlist + hostcount){
            strcpy((char *) hostname,  (char *) ++hostptr);
          }
          else{

            /* "Last site in database" */

            send_response(HOST_MANAGE_025);
            continue;
          }
        }
	    }
	    else{
        hostptr = hostlist;
        strcpy((char *) hostname, (char *) hostptr);
	    }

	    free_opts(aux_dbs);
	    aux_dbs = (char **) NULL;

	    if(in_hostlist)
      display_hostlist();

	    break;
#endif

      /* Go to previous site in list */

    case PREVIOUS_SITE:

	    action_status = UPDATE;

	    if(hostptr != (hostname_t *) NULL){

        if(hostptr < hostlist){
          hostptr = hostlist;

          /* "First site in database" */

          send_response(HOST_MANAGE_026);
        }
        else{
          if(hostcount == 0){

            /* "First site in database" */

            send_response(HOST_MANAGE_026);
            continue;
          }


          if(hostptr != hostlist)
          hostptr--;
          else{
            /* "First site in database" */

            send_response(HOST_MANAGE_026);
          }
        }
	    }
	    else
      hostptr = hostlist;

	    strcpy((char *) hostname, (char *) hostptr);

	    free_opts(aux_dbs);
	    aux_dbs = (char **) NULL;

	    if(in_hostlist)
      display_hostlist();

	    break;


      /* Go to next access method in list */

    case NEXT_HOSTAUX:

	    action_status = UPDATE;


      index++;

      if ( get_hostaux_entry(*hostptr, *aux_list, index, &new_hostaux_db, hostaux_db) == ERROR ) { /* Last hostaux for the site */
        
        if(*(aux_list + 1) != (char *) NULL) {
          ++aux_list;
          index = 0;
        }
        else{

          /* "Last access method in site" */

          send_response(HOST_MANAGE_028);
          index--;
        }
        if ( get_hostaux_entry(*hostptr, *aux_list, index, &new_hostaux_db, hostaux_db) == ERROR ) {

          error(A_ERR, "host_manage","get_hostaux_entry() failed" );
          break;
        }
      }

      set_aux_origin(&new_hostaux_db, *aux_list, index);
      
	    if(new_hostaux_db.current_status == 0)
      new_hostaux_db.current_status = DISABLED;

      copy_hostaux(&orig_hostaux_db, &new_hostaux_db);
      set_aux_origin(&orig_hostaux_db, *aux_list, index);
	    continue;
	    break;

      /* Go to previous access method */

    case PREVIOUS_HOSTAUX:

	    action_status = UPDATE;

      index--;
      if (  get_hostaux_entry(*hostptr, *aux_list, index, &new_hostaux_db, hostaux_db) == ERROR ) { /* Last hostaux for the site */
       
        if(aux_list != aux_dbs) {
          --aux_list;
          index = 0;
        }
        else{

          /* "First access method in site" */

          send_response(HOST_MANAGE_027);
        }
        
        get_hostaux_entry(*hostptr, *aux_list, index, &new_hostaux_db, hostaux_db);

      }
      set_aux_origin(&new_hostaux_db, *aux_list, index);
	    if(new_hostaux_db.current_status == 0)
      new_hostaux_db.current_status = DISABLED;

	    copy_hostaux(&orig_hostaux_db, &new_hostaux_db);

	    continue;
	    break;


    case DELETE_HOSTAUX:


	    if((new_hostaux_db.current_status == DEL_BY_ADMIN) ||
	       (new_hostaux_db.current_status == DEL_BY_ARCHIE)){

        /* "Database already scheduled for deletion" */

        send_response(HOST_MANAGE_047, new_hostaux_db.origin->access_methods);
        continue;
	    }

	    if(new_hostaux_db.current_status == DELETED){

        /* "Database %s already deleted" */

        send_response(HOST_MANAGE_048, new_hostaux_db.origin->access_methods);
        continue;
	    }

	    /* "Do you really want to delete the current database ?" */

	    if(get_yn_question(HOST_MANAGE_044) == A_OK){
        pathname_t tmp_name;

        sprintf(tmp_name,"%s.%s.%d", make_lcase(new_hostdb.primary_hostname), make_lcase(new_hostaux_db.origin->access_methods), (int)new_hostaux_db.origin->hostaux_index);

        new_hostaux_db.current_status = DEL_BY_ARCHIE;
        new_hostaux_db.update_time = time((time_t *) NULL);

        if(put_dbm_entry(tmp_name, strlen(tmp_name) + 1, &new_hostaux_db, sizeof(hostdb_aux_t), hostaux_db, 1) == ERROR){

          /* "Can't update auxiliary host database for deletion" */

          send_response(HOST_MANAGE_045);
          continue;
        }
        else{

          /* "Host %s, database %s scheduled for deletion" */

          send_response(HOST_MANAGE_046, new_hostdb.primary_hostname, new_hostaux_db.origin->access_methods);
        }

        copy_hostaux(&orig_hostaux_db, &new_hostaux_db);
	    }

	    continue;
	    break;

    case DELETE_SITE:

	    for(test_list = aux_dbs; (test_list != (char **) NULL) && (*test_list != (char *) NULL); test_list++){
        hostdb_aux_t tmp_hostaux;
        int i,flag = 0 ;
        i = 0;
        do { 
          if(get_hostaux_entry(new_hostdb.primary_hostname, *test_list, (index_t) i, &tmp_hostaux, hostaux_db) == ERROR){

            if ( i == 0 ) {
              /* "Can't find auxiliary database %s for %s" */

              send_response(HOST_MANAGE_056, new_hostdb.primary_hostname, *test_list);
            }
            break;
          }
	       
          if(tmp_hostaux.current_status != DELETED){

            /* "Database %s not deleted. Cannot remove host %s" */

            send_response(HOST_MANAGE_057, *test_list, new_hostdb.primary_hostname);
            flag = 1;
            break;
          }
          i++;
        } while (1);
        if ( flag == 1 ) {
          break;
        }
      }

	    if((test_list != (char **) NULL) && (*test_list != (char *) NULL))
      continue;

	    /* We can delete this host */

	    /* "Delete: %s. Are you sure ?" */

	    if(get_yn_question(HOST_MANAGE_058, new_hostdb.primary_hostname) == ERROR)
      continue;

	    for(test_list = aux_dbs; (test_list != (char **) NULL) && (*test_list != (char *) NULL); test_list++){

        if ( delete_hostaux_ent(new_hostdb.primary_hostname, *test_list, hostaux_db) == ERROR ) {
/*        
        sprintf(tmps, "%s.%s", new_hostdb.primary_hostname, *test_list);

        if(delete_dbm_entry(tmps, strlen(tmps) + 1, hostaux_db) == ERROR){
*/
          /* "Error while trying to delete database %s. Continue ?" */

          if(get_yn_question(HOST_MANAGE_059, *test_list) == ERROR)
          break;
        }
	    }

	    if((test_list != (char **) NULL) && (*test_list != (char *) NULL))
      continue;

	    if(delete_dbm_entry(&new_hostdb.primary_ipaddr, sizeof(ip_addr_t), hostbyaddr) == ERROR){

        /* "Can't delete %s (%s) address cache. Continue ?" */

        if(get_yn_question(HOST_MANAGE_060, new_hostdb.primary_hostname, inet_ntoa(ipaddr_to_inet(new_hostdb.primary_ipaddr))) == ERROR)
        continue;
	       
	    }

	    if(delete_dbm_entry(new_hostdb.primary_hostname, strlen(new_hostdb.primary_hostname) + 1, hostdb) == ERROR){

        /* "Can't delete %s from host database" */

        send_response(HOST_MANAGE_061, new_hostdb.primary_hostname);
	    }
	    else
      send_response("Host %s deleted", new_hostdb.primary_hostname);

	    if(hostptr != (hostname_t *) NULL){

        if(hostptr >  hostlist + hostcount){
          hostptr = hostlist + hostcount;

          /* "Last site in database" */

          send_response(HOST_MANAGE_025);
        }
        else{

          if(hostcount <= 1){

            /* "Last site in database" */

            send_response(HOST_MANAGE_025);
            continue;
          }

          if(hostptr != hostlist + hostcount -1 )
          hostptr++;
          else{

            /* "Last site in database" */

            send_response(HOST_MANAGE_025);
          }
        }
	    }
	    else
      hostptr = hostlist;

	    strcpy((char *) hostname, (char *) hostptr);

	    /* Go to next host on list */

	    if(get_hostnames(hostdb, &hostlist, &hostcount, domain_list, domain_count) == ERROR){

        /* "Can't get new list of hosts" */

        error(A_ERR, "host_manage", HOST_MANAGE_023);
        return(ERROR);
	    }

	    if((hostptr = (hostname_t *) bsearch ( (char *) hostname, (char *) hostlist, (unsigned) hostcount, sizeof(hostname_t), strcasecmp)) == (hostname_t *) NULL){

        /* we've got a real problem here */

        /* "Can't find host just updated in database" */

        error(A_ERR, "host_manage", HOST_MANAGE_024);
        return(ERROR);
	    }

	    free_opts(aux_dbs);
	    aux_dbs = (char **) NULL;

	    break;
	 
    case REACTIVATE_DATABASE:

	    if(new_hostaux_db.current_status == ACTIVE){

        /* "Database %s is already active" */
	    
        send_response(HOST_MANAGE_062, new_hostaux_db.origin->access_methods);
        continue;
	    }

	    if(get_yn_question("Reactivate %s database?", new_hostaux_db.origin->access_methods) == A_OK){

        new_hostaux_db.current_status = ACTIVE;

        if(update_hostaux(hostaux_db, new_hostdb.primary_hostname, &new_hostaux_db, 1) == ERROR){

          /* "Can't update new or modified auxilary host database entry" */

          error(A_ERR, "host_manage", HOST_MANAGE_039);
          return(ERROR);
        }

        /* "Database %s reactivated" */

        send_response("Database %s reactivated", new_hostaux_db.origin->access_methods);

        copy_hostaux(&orig_hostaux_db, &new_hostaux_db);
	    }
	    continue;

    case FORCE_UPDATE:


	    if(HADB_IS_FORCE_UPDATE(new_hostaux_db.flags)){
	    
        /* "Early update already scheduled" */

        send_response(HOST_MANAGE_050);
        continue;
	    }

	    /* "Schedule early update ?" */

	    if(get_yn_question(HOST_MANAGE_049) == A_OK){

        HADB_SET_FORCE_UPDATE(new_hostaux_db.flags);

        new_hostaux_db.current_status = ACTIVE;

        if(update_hostaux(hostaux_db, new_hostdb.primary_hostname, &new_hostaux_db, 1) == ERROR){

          /* "Can't update new or modified auxilary host database entry" */

          error(A_ERR, "host_manage", HOST_MANAGE_039);
          return(ERROR);
        }

        /* "Early update scheduled for %s database %s" */

        send_response(HOST_MANAGE_051, new_hostdb.primary_hostname, new_hostaux_db.origin->access_methods);

        copy_hostaux(&orig_hostaux_db, &new_hostaux_db);
	    }
	    continue;

	    break;

    case FORCE_DB_DELETE:

	    if(get_yn_question("Are you sure you want to force deletion of %s?", new_hostaux_db.origin->access_methods) == ERROR)
      continue;
	    else{
        pathname_t tmps;
        pathname_t myhold;

        tmps[0] = '\0';
        myhold[0] = '\0';

        for(test_list = aux_dbs; (test_list != (char **) NULL) && (*test_list != (char *) NULL); test_list++){

          if(strcmp(*test_list, new_hostaux_db.origin->access_methods) != 0){

            if(tmps[0] == '\0')
            strcpy(tmps,*test_list);
            else{
              sprintf(myhold, ":%s", *test_list);
              strcat(tmps, myhold);
            }
          }
        }

        if(get_yn_question("This action can cause damage to the system. Continue?") == ERROR)
        continue;

        /* Remove the tmps from the access_methods list only if last
           instance of the databse */
        
        if ( new_hostaux_db.origin->hostaux_index == 0 ) {
          index_t i;
          find_hostaux_last(new_hostdb.primary_hostname,
                            new_hostaux_db.origin->access_methods,
                            &i, hostaux_db);

          if ( i == 0 ) {
            
            strcpy(new_hostdb.access_methods, tmps);

            /* update primary host database entry */

            if(put_dbm_entry(new_hostdb.primary_hostname, strlen(new_hostdb.primary_hostname) + 1, &new_hostdb, sizeof(hostdb_t), hostdb, 1) == ERROR){

              if(get_yn_question("Can't update primary hostdb record for %s.%s", new_hostdb.primary_hostname, new_hostaux_db.origin->access_methods) == ERROR)
              continue;
            }
          }
        }


      if ( delete_hostaux_entry(new_hostdb.primary_hostname, &new_hostaux_db,
                                hostaux_db) == ERROR ) {

          /* "Error while trying to delete database %s. Continue ?" */

          if(get_yn_question(HOST_MANAGE_059, new_hostaux_db.origin->access_methods) == ERROR)
          continue;
        }

        orig_hostdb = new_hostdb;

        send_response("Database %s deleted", new_hostaux_db.origin->access_methods);
    }

	    continue;
	    break;

    default:

	    /* "Unknown command code returned from command parser" */

	    error(A_ERR,"host_manage", HOST_MANAGE_033);
	    return(ERROR);
	    
    } /* switch */

    /*
     * Get the current entry from the database. Only reach here if the
     * site has not been modified
     */

    if(get_dbm_entry(hostname, strlen(hostname) +1, &orig_hostdb, hostdb) == ERROR){

      /* site not found in database */

      if((host_status = check_new_hentry(hostbyaddr, hostdb, &new_hostdb, hostaux_db, &new_hostaux_db, flags, control_flags)) != HOST_OK){

        /* Error with this site */

        /* "Site %s: %s" */

        send_response( HOST_MANAGE_029, hostname, get_host_error(host_status));

        if(host_status != HOST_UNKNOWN){
          orig_hostdb = new_hostdb;
          strcpy(hostname, orig_hostdb.primary_hostname);

          if((hostptr = (hostname_t *) bsearch ( (char *) hostname, (char *) hostlist, (unsigned) hostcount, sizeof(hostname_t), strcasecmp)) == (hostname_t *) NULL){


            /* "Can't locate host returned from error" */

            send_response(HOST_MANAGE_041);
            continue;
          }
        }
        else
        new_hostdb = orig_hostdb;


      }
      else{

        /* "Host %s not found in database" */

        send_response(HOST_MANAGE_030, hostname);
        new_hostdb = orig_hostdb;
        continue;
      }
	 
    }
    else{
      if(aux_dbs)
	    free_opts(aux_dbs);

      aux_dbs = (char **) NULL;
    }

	
    if(orig_hostdb.access_methods[0] != '\0'){
      index_t index;

      if(aux_dbs == (char **) NULL)
	    aux_dbs = aux_list = str_sep(orig_hostdb.access_methods,
                                   NET_DELIM_CHAR);
      hit = orig_hostaux_db.origin;      
      if ( get_hostaux_ent(orig_hostdb.primary_hostname, *aux_list, &index,
                           NULL,NULL,&orig_hostaux_db, hostaux_db) == ERROR ) {
          
        /* "Can't get auxiliary database entry for host %s with non-NULL access methods" */

        send_response(HOST_MANAGE_031, orig_hostdb.primary_hostname);
        clean_hostaux( &new_hostaux_db );
        index = -1;
      }
      orig_hostaux_db.origin = hit;      
      set_aux_origin(&orig_hostaux_db, *aux_list, index);
    }
    else{
      orig_hostaux_db.origin = hit;      
      aux_dbs = (char **) NULL;
      clean_hostaux( &new_hostaux_db );
      clean_hostaux( &orig_hostaux_db );

    }

    new_hostdb = orig_hostdb;
    copy_hostaux( &new_hostaux_db, &orig_hostaux_db);

  }while(1);                    /* it keeps on going, and going and going and... */

}


/*
 * check_update: verify that the given hostdb record is valid
 */
   

status_t check_update(hostbyaddr, hostdb, hostdb_rec, hostaux_db, hostaux_rec, dbspec_list, flags, control_flags, action)
   file_info_t  *hostbyaddr;	 /* host address cache */
   file_info_t  *hostdb;	 /* primary host database */
   hostdb_t     *hostdb_rec;	 /* primary host database record to be verified */
  file_info_t  *hostaux_db;	 /* primary host database */
   hostdb_aux_t *hostaux_rec;	 /* auxiliary host database record to be verified */
   dbspec_t *dbspec_list;	 /* List of fields in access commands */
   flags_t flags;
   int control_flags;
   action_status_t action;	 /* Ongoing action */

{
  host_status_t check_resp;
  hostname_t hostname;
  AR_DNS *dns;

  ptr_check(hostbyaddr, file_info_t , "check_update", ERROR);
  ptr_check(hostdb, file_info_t , "check_update", ERROR);
  ptr_check(hostdb_rec, hostdb_t , "check_update", ERROR);
  ptr_check(hostaux_rec, hostdb_aux_t , "check_update", ERROR);      
  ptr_check(dbspec_list, dbspec_t, "check_update", ERROR);
   

  strcpy(hostname, hostdb_rec -> primary_hostname);

  /* if it is a new entry then check it out */

  if(action == NEW){

    if((check_resp = check_new_hentry(hostbyaddr, hostdb, hostdb_rec, hostaux_db, hostaux_rec, flags, control_flags)) != HOST_OK){

      /* "Error: %s" */

      send_response(CHECK_UPDATE_001, get_host_error(check_resp));

      if(display_records(hostdb_rec, hostaux_rec, dbspec_list, action) == ERROR){

        /* "Can't display current record" */

        error(A_ERR, "check_update", CHECK_UPDATE_002);
        return(ERROR);
      }

      return(ERROR);

    } 

  } else if(action == UPDATE){

    /* We're updating a new site */


    if(hostaux_rec -> preferred_hostname[0] != '\0'){

      if((dns = ar_open_dns_name(hostaux_rec -> preferred_hostname, DNS_EXTERN_ONLY, (file_info_t *) NULL)) == (AR_DNS *) NULL){

        /* "Error: %s" */

        send_response(CHECK_UPDATE_001, get_host_error(HOST_CNAME_UNKNOWN));
        return(ERROR);
      }

      if(cmp_dns_name(hostdb_rec -> primary_hostname, dns) == ERROR){

        /* "Error: %s" */

        send_response(CHECK_UPDATE_001, get_host_error(HOST_CNAME_MISMATCH));
        return(ERROR);
      }

      ar_dns_close(dns);
    }
  }

  if(display_records(hostdb_rec, hostaux_rec, dbspec_list, action) == ERROR){

    /* "Can't display current record" */

    error(A_ERR, "check_update", CHECK_UPDATE_002);
    return(ERROR);
  }

  return(A_OK);
}

/*
 * do_update: perform the update on the given primary host database record.
 * It is a wrapper around the update_hostdb, update_hostaux calls
 */


status_t do_update(hostbyaddr, hostdb, hostaux_db, new_hostrec, new_hostaux, action)
   file_info_t *hostbyaddr;	 /* host address cache */
   file_info_t *hostdb;		 /* primary host database */
   file_info_t *hostaux_db;	 /* auxiliary host database */
   hostdb_t    *new_hostrec;	 /* primary host record to be updated */
   hostdb_aux_t *new_hostaux;	 /* auxiliary host record to be updated */
   action_status_t action;	 /* current action */
{
   host_status_t host_status;
   

   ptr_check(hostbyaddr, file_info_t, "do_update", ERROR);
   ptr_check(hostdb, file_info_t, "do_update", ERROR);
   ptr_check(hostaux_db, file_info_t, "do_update", ERROR);   
   ptr_check(new_hostrec, hostdb_t, "do_update", ERROR);
   ptr_check(new_hostaux, hostdb_aux_t, "do_update", ERROR);

   if((host_status = update_hostdb(hostbyaddr, hostdb, new_hostrec, action == NEW ? FALSE : TRUE)) != HOST_OK){

      /* "Failed: %s" */

      send_response(DO_UPDATE_001, get_host_error(host_status));
      return(ERROR);
   }

   
   if((host_status = update_hostaux(hostaux_db, new_hostrec -> primary_hostname, new_hostaux, action == NEW ? FALSE : TRUE))){


      /* "Failed: %s" */

      send_response(DO_UPDATE_001, get_host_error(host_status));
      return(ERROR);
   }

   return(A_OK);
}


status_t addto_auxdbs(aux_dbs, newstring, newptr)
   char ***aux_dbs;
   char *newstring;
   char ***newptr;
{
  char **aux_ptr;
  char **disp;

  if(*aux_dbs != (char **) NULL){
    int j;

    /* find end of list */

    for(aux_ptr = *aux_dbs; *aux_ptr != (char *) NULL; aux_ptr++){

      if(strcasecmp(*aux_ptr, newstring) == 0)
	    return(A_OK);
    }
   
    /* realloc for space for new database */

    if((disp = (char **) realloc(*aux_dbs, sizeof(char **) * ((j = aux_ptr - *aux_dbs) + 2))) == (char **) NULL){
/*disp*/
      /* "Can't realloc space for new auxiliary host list" */

      error(A_SYSERR, "addto_auxdbs", ADDTO_AUXDBS_001);
      return(ERROR);
    }

    *aux_dbs = disp;
    /* add new database to list */

    aux_ptr = *aux_dbs + j;

    *aux_ptr = strdup(newstring);
    *(aux_ptr + 1) = (char *) NULL;

    *newptr = aux_ptr;
  }
  else{

    /* no pre-existing database list */

    if((*aux_dbs = (char **) malloc(sizeof(char **) * 2)) == (char **) NULL){

      /* "Can't allocate space for new auxiliary host list" */

      error(A_SYSERR, "host_manage", ADDTO_AUXDBS_002);
      return(ERROR);
    }

    **aux_dbs = strdup(newstring);
    *(*aux_dbs + 1) = (char *) NULL;
    *newptr = *aux_dbs;

  }


  return(A_OK);
}

