/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#ifdef AIX
#include <sys/select.h>
#endif
#include "protos.h"
#include "typedef.h"
#include "db_files.h"
#include "listd.h"
#include "error.h"
#include "archie_inet.h"
#include "archie_strings.h"
#include "lang_exchange.h"
#include "times.h"
#include "master.h"
#include "files.h"
#include "archie_mail.h"
#include "db_ops.h"
#include "hinfo.h"

/*
 * do client: perform the actions required for the client process
 */

#define MAX_RETRY_SEQ 2
int signal_set = 0;

int archie_port = ARCHIE_PORT;

extern int verbose;


char *get_retrieve_time( file_info_t *, hostname_t, char *, char *);


status_t do_client(hostbyaddr, hostdb, hostaux_db, domaindb, config_info, force_hosts, databases, ret_manager, justlist, fromhost, timeout, expand_domains, compress_trans)
   file_info_t *hostbyaddr;   /* host address cache */
   file_info_t *hostdb;	      /* primary host database */
   file_info_t *hostaux_db;   /* auxiliary host database */
   file_info_t *domaindb;     /* domain database */
   udb_config_t *config_info;
   char	       *force_hosts;  /* colon separated list of hosts to be retrieved */
   char	       *databases;    /* colon separated list of databases */
   int	       ret_manager;   /* non-zero when in retrieval manage mode */
   int	       justlist;      /*
			       * non-zero when only listing is required in
			       * retrieval manager mode
			       */
   char	       *fromhost;      /* Only contact the server at this host */
   int	       timeout;
   int	       expand_domains; /* expand domains on output */
   int	       compress_trans;

{

  extern int verbose;

#if __STDC__

  extern time_t time(time_t *);

#else

  extern time_t time();
  extern fclose();

#endif   

  int count, dbcount;
  udb_attrib_t *attrib_ptr;
  time_t curr_time = 0;

  int command_conn;
  FILE *command_ifp;
  FILE *command_ofp;


  net_scommand_t params[MAX_PARAMS];
  command_v_t *command_v;       /* command vector */
  tuple_t *tuple_list;
  tuple_t *new_tlist;
  index_t tuple_num;
  int writable = 0;
  int found_fromhost = 0;
  int max_date = 0;
  date_time_t orig_tuple_num = 0;
  domain_t domain_list[MAX_NO_DOMAINS];
  int domain_count;
  char out_domain[2048];
  int x;
  char **fromhost_list = (char **) NULL;
  char **databases_list = (char **) NULL;

  ptr_check(hostbyaddr, file_info_t, "do_client", ERROR);
  ptr_check(hostdb, file_info_t, "do_client", ERROR);
  ptr_check(hostaux_db, file_info_t, "do_client", ERROR);
  ptr_check(domaindb, file_info_t, "do_client", ERROR);
  ptr_check(config_info, udb_config_t, "do_client", ERROR);
   

  if(fromhost[0] != '\0'){

    if((fromhost_list = str_sep(fromhost, ':')) == (char **) NULL){

      /* "Can't determine set of hosts from '%s'" */

      error(A_ERR, "do_client", DO_CLIENT_025, fromhost);
      return(ERROR);
    }
  }

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

    if(fromhost[0] != '\0'){
      char **curr_fh;

      for(curr_fh = fromhost_list; *curr_fh != (char *) NULL; curr_fh++){

        if(strcasecmp(*curr_fh, config_info[count].source_archie_hostname) == 0){
          found_fromhost = 1;
          break;
        }
      }

      if(found_fromhost == 0){

        /* "Configuration host %s not in command line host list %s" */

        error(A_WARN, "do_client", DO_CLIENT_019, config_info[count].source_archie_hostname, fromhost);
        continue;
      }
      else
	    found_fromhost = 0;

    }


    if(config_info[count].source_archie_hostname[0] == '*')
    continue;

    if(verbose){

      /* "Attempting to connect to server %s" */

      error(A_INFO, "do_client", DO_CLIENT_020, config_info[count].source_archie_hostname);
    }

    {
      char *env_port;
      if ( (env_port = getenv("ARCH_PORT")) != NULL ) {
        archie_port = atoi(env_port);
      }
    }
    
    if(cliconnect(config_info[count].source_archie_hostname, archie_port, &command_conn) != A_OK){

      config_info[count].fails++;
      error(A_ERR,"do_client",
            DO_CLIENT_001,
            config_info[count].source_archie_hostname,
            config_info[count].fails);

      continue;
    }


    if((command_ifp = fdopen(command_conn, "r")) == (FILE *) NULL){

      /* "Can't open file I/O for command connection" */

      error(A_ERR,"do_client", DO_CLIENT_002);
      continue;
    }

    if((command_ofp = fdopen(command_conn, "w")) == (FILE *) NULL){

      /* "Can't open file I/O for command connection" */

      error(A_ERR,"do_client", DO_CLIENT_002);
      continue;
    }


    if(verbose){

      /* "Connected to %s" */

      error(A_INFO, "do_client", DO_CLIENT_021, config_info[count].source_archie_hostname);
    }

    attrib_ptr = &config_info[count].db_attrib[0];

    if(databases[0] != '\0'){

      if((databases_list = str_sep(databases, ':')) == (char **) NULL){

        /* "Can't determine set of databases from '%s'" */

        error(A_ERR, "do_client", DO_CLIENT_026, databases);
        return(ERROR);
      }
    }

    for(dbcount = 0;attrib_ptr -> db_name[0] != '\0';attrib_ptr++){

      max_date = 0;
      orig_tuple_num = 0;

      if(databases[0] != '\0'){
        char **pdb;
        char **config_dbs;
        char **pcdbs;
        int found_dbs = 0;

        if((config_dbs = str_sep(attrib_ptr -> db_name, ':')) == (char **) NULL){

          /* "Can't extract list of databases '%s' from configuration file" */

          error(A_ERR, "do_client", DO_CLIENT_027, databases);
          continue;
        }

        for(pdb = databases_list; *pdb != (char *) NULL; pdb++){

          for(pcdbs = config_dbs; *pcdbs != (char *) NULL; pcdbs++){

            if(strcasecmp(*pdb, *pcdbs) == 0){

              /* Found the database we are looking for */

              strcpy(params[0], *pcdbs);
              found_dbs = 1;
              break;
            }
          }

          if(found_dbs)
          break;
        }

        if((*pdb == (char *) NULL) || (*pcdbs == (char *) NULL)){
          if(config_dbs)
          free_opts(config_dbs);
          continue;
        }

        if(config_dbs)
        free_opts(config_dbs);

      }
      else
	    strcpy(params[0], attrib_ptr -> db_name);

      if(force_hosts[0] == '\0'){

        /*
         * If you don't get information about this database from this
         * server go to next database
         */

        if((attrib_ptr -> perms[0] != 'w') && (attrib_ptr -> perms[1] != 'w'))
        continue;

        if((curr_time = time((time_t *) NULL)) == -1){

          /* "Can't get current time!" */

          error(A_ERR, "do_client", DO_CLIENT_003);
          return(ERROR);
        }
		
        if(attrib_ptr -> fails > MAX_FAILS){

          /* "Number of retries (%u) exceeded for %s" */

          error(A_INFO,"do_client",DO_CLIENT_004, attrib_ptr -> fails, config_info[count].source_archie_hostname);
          continue;
        }

        /* Check to see if this archie site/database should be updated */
	     

        if((attrib_ptr -> update_time + attrib_ptr -> update_freq * 60+
            (attrib_ptr -> fails * attrib_ptr -> update_freq * 60 )) > curr_time){

          /* "Server %s database %s not scheduled for update" */

          error(A_INFO, "do_client", DO_CLIENT_022, config_info[count].source_archie_hostname,attrib_ptr -> db_name);
          continue;
        }
	  
        /* Set up the request to send to the server */

        strcpy(params[2], cvt_from_inttime(attrib_ptr -> update_time));

        if(expand_domains){

          /* Expand the domains in the output */

          if(compile_domains(attrib_ptr -> domains, domain_list, domaindb, &domain_count) == ERROR){

            /* "Can't expand domains '%s' in output" */

            error(A_INTERR, "do_client", DO_CLIENT_023, attrib_ptr -> domains);

            /* "Ignoring %s, %s" */

            error(A_INTERR, "do_client", DO_CLIENT_024, config_info[count].source_archie_hostname, attrib_ptr -> db_name);
            continue;
          }

          out_domain[0] = '\0';

          strcpy(out_domain, domain_list[0]);

          for(x = 1; x < domain_count; x++){
            char hold_str[MAX_DOMAIN_LEN];

            sprintf(hold_str, ":%s", domain_list[x]);
            strcat(out_domain, hold_str);
          }

          strcpy(params[3], out_domain);
        }
        else
        strcpy(params[3], attrib_ptr -> domains);
	    
      }
      else{

        /* force hosts */

        strcpy(params[3], force_hosts);

        /*
         * if retrieval manager, then force the site to be retrieved by
         * getting "all sites not updated since current time",
         * otherwise force "sites updated since time 0"
         */

        if(ret_manager)
        strcpy(params[2], cvt_from_inttime(time((time_t *) NULL)));
        else
        strcpy(params[2], cvt_from_inttime(0));
      }
      
      if(ret_manager)
      strcpy(params[1],"<");
      else
      strcpy(params[1],">");


      if(write_net_command(C_LISTSITES, params,command_ofp) == ERROR){

        /* "Error writing command C_LISTSITES to remote server %s" */

        error(A_ERR, "do_client", DO_CLIENT_005, config_info[count].source_archie_hostname);
        goto proto_err;
      }

      /* The response from the server should be TUPLELIST <number of tuples> */

      if((command_v = read_net_command(command_ifp)) == (command_v_t *) NULL){

        /* "Error reading C_TUPLELIST response from remote server %s" */

        error(A_ERR, "do_client", DO_CLIENT_006, config_info[count].source_archie_hostname);
        goto proto_err;
      }

      if(command_v -> command == C_AUTH_ERR){

        /* "This client not authorize to connect to server %s" */

        error(A_ERR,"do_client", DO_CLIENT_018, config_info[count].source_archie_hostname);
        goto proto_err;
      }
	    

      if(command_v -> command != C_TUPLELIST){

        /* "Unexpected response from remote server %s. Expecting C_TUPLELIST. Got %d" */

        error(A_ERR, "do_client", DO_CLIENT_007, config_info[count].source_archie_hostname, command_v -> command);
        goto proto_err;
      }


      /* If there are no sites, then go on */

      if((tuple_num = atoi(command_v -> params[0])) == 0){
        if(ret_manager)
        attrib_ptr -> update_time = curr_time;

        continue;
      }

      orig_tuple_num = tuple_num;
	 

      if((tuple_list = (tuple_t *) malloc( tuple_num * sizeof(tuple_t))) == (tuple_t *) NULL){

        /* "Error while trying to allocate space for tuplelist" */

        error(A_SYSERR, "do_client", DO_CLIENT_008);
        goto proto_err;
      }

      if(get_tuple_list(command_ifp, tuple_list, tuple_num) == ERROR){

        /* "Error while trying to read tuplelist from remote server %s" */

        error(A_ERR, "do_client", DO_CLIENT_009, config_info[count].source_archie_hostname);
        goto proto_err;
      }

      /* if only listing requested print results to stdout and return */

      if(justlist){
        int i;

        for(i = 0; i < tuple_num; i++)
        fprintf(stdout, "%s\n", tuple_list[i]);

        continue;
      }

      /* if maxno is not zero then pay attention to it */
      /* tuple_num = less_of(maxno, tuple_num)	  */

      if(attrib_ptr -> maxno != 0)
	    if(tuple_num > attrib_ptr -> maxno)
      tuple_num = attrib_ptr -> maxno;
	       
      error(A_INFO,"do_client",DO_CLIENT_010, tuple_num);

      if(!ret_manager){

        if((new_tlist = (tuple_t *) malloc( tuple_num * sizeof(tuple_t))) == (tuple_t *) NULL){

          /* "Error while trying to allocate space for new tuplelist" */

          error(A_SYSERR, "do_client", DO_CLIENT_011);
          goto proto_err;
        }

        if(process_tuples(tuple_list, &tuple_num, new_tlist, force_hosts, &max_date, hostdb, hostaux_db) == ERROR){

          /* "Error while trying to process incoming tuples from %s" */

          error(A_ERR, "do_client", DO_CLIENT_012, config_info[count].source_archie_hostname);
          goto proto_err;
        }

        /* Set up for recieving the information */

        if(get_sites(hostaux_db,config_info[count].source_archie_hostname, new_tlist, tuple_num, command_ifp, command_ofp, timeout, compress_trans) == ERROR){

          /* "Error while trying to obtain sites from remote server %s" */

          error(A_ERR, "do_client", DO_CLIENT_013, config_info[count].source_archie_hostname);
          goto proto_err;
        }

        /*
         * If there are more tuples available than you asked for 
         * modify the update time up to the latest date retrieved
         */

        if(orig_tuple_num >= attrib_ptr -> maxno)
        attrib_ptr -> update_time = max_date;
        else
        attrib_ptr -> update_time = curr_time;

        if(new_tlist)
        free(new_tlist);

      }
      else{

        if(get_headers(hostaux_db,tuple_list, tuple_num, &max_date, command_ifp, command_ofp) == ERROR){

          /* "Error while trying to retrieve headers from remote server %s" */

          error(A_ERR, "do_client", DO_CLIENT_014, config_info[count].source_archie_hostname);
        }

        /* Here we check first if the maxno is not 0 
           before we update Jul-11-95 */

        if((orig_tuple_num >= attrib_ptr -> maxno) &&
           (attrib_ptr -> maxno != 0) && max_date != 0)
        attrib_ptr -> update_time = max_date;
        else
        attrib_ptr -> update_time = curr_time;
		
      }


      if(tuple_list)
	    free(tuple_list);

      continue;

    proto_err:

      if(write_net_command(C_QUIT, (void *) NULL, command_ofp) == ERROR){

        /* "Error sending QUIT command to remote server %s" */

        error(A_ERR, "do_client", DO_CLIENT_015, config_info[count].source_archie_hostname);
      }

      if(fclose(command_ifp) == EOF){

        /* "Error closing connection with remote server %s" */

        error(A_ERR, "do_client", DO_CLIENT_017, config_info[count].source_archie_hostname);
      }

      if(fclose(command_ofp) == EOF){

        /* "Error closing connection with remote server %s" */

        error(A_ERR, "do_client", DO_CLIENT_017, config_info[count].source_archie_hostname);
      }

      continue;


    }

    if(write_net_command(C_QUIT, (void *) NULL, command_ofp) == ERROR){

      /* "Error sending QUIT command to remote server %s" */

      error(A_ERR, "do_client", DO_CLIENT_015, config_info[count].source_archie_hostname);
    }

    /* "Closing connection at %s" */
	       
    if(verbose)
    error(A_INFO,"do_client", DO_CLIENT_016, cvt_to_usertime(time((time_t *) NULL),0));

    fclose(command_ifp);
    fclose(command_ofp);

  }

#if 0
  if(fromhost[0])
  free_opts(fromhost_list);
#endif

  return(A_OK);
}


/*
 * get_tuple_list: retrieve a list of tuples from the remote site
 */


status_t get_tuple_list(command_ifp,tuple_list, tuple_num)
   FILE    *command_ifp;   /* file pointer of remote server connection */
   tuple_t *tuple_list;	   /* list of tuples to be returned */
   index_t  tuple_num;	   /* number of tuples to be retrieved */

{
  int i;
  char *nl_ptr;
  tuple_t input_buf;

  ptr_check(command_ifp, FILE, "send_tuples", ERROR);
  ptr_check(tuple_list, tuple_t, "send_tuples", ERROR);

  for(i = 0; i < tuple_num; i++){

    if(fgets(input_buf, MAX_TUPLE_SIZE, command_ifp) == (char *) NULL){

      /* "Error while reading tuplelist from remote site" */


      error(A_ERR, "get_tuple_list", GET_TUPLE_LIST_001);
      return(ERROR);
    }
	 
    if((nl_ptr = strchr(input_buf, CR)) != (char *) NULL)
    *nl_ptr = '\0';
    else{

      /* "Input received from remote site not terminated with CR" */

      error(A_ERR, "get_tuple_list", GET_TUPLE_LIST_002);
      return(ERROR);
    }
	 

    if(input_buf[0] == LF)
    nl_ptr = input_buf + 1;
    else
    nl_ptr = input_buf;

    strcpy(tuple_list[i], nl_ptr);

  }

  tuple_list[tuple_num][0] = '\0';

  return(A_OK);
}


/*
 * get_sites: get sites specified by <tuple_list>. Invoke the appropriate
 * network transfer program to do actual transfer
 */


status_t get_info(server_host, tuple, params, command_ifp, command_ofp, timeout, compress_trans, command, suff_num)
   hostname_t server_host;    /* remote server host */
   tuple_t tuple;	      /* tuples of information to be retrieved */
  net_scommand_t params[MAX_PARAMS];
   FILE    *command_ifp;      /* input command connection to remote server */
   FILE    *command_ofp;      /* output command connection to remote server */
   int	   timeout;
   int	   compress_trans;    /* compression mode */
  int     command;
  int     suff_num;
{

  int j;
  command_v_t *command_v;
  int result;
  int port;
  int data_fd;
  hostname_t host_select;
  pathname_t database_server;
  pathname_t database_name;
  pathname_t db_name;
  pathname_t port_num;
  int status;
  char **arglist;
  char *logname;
  int retry_seq = 0;
  char suffix[10],timeout_str[100];

 retry_site:

  if(write_net_command(command, params, command_ofp) == ERROR){

    /* "Error while trying to send C_SENDSITE command to remote server on %s" */

    error(A_ERR, "get_info", GET_SITES_002, server_host);
    return(ERROR);
  }

  if((command_v = read_net_command(command_ifp)) == (command_v_t *) NULL){

    /* "Error while trying to read command from connection" */

    error(A_ERR, "get_info", GET_SITES_003);
    return(ERROR);
  }

  if(command_v -> command != C_SITELIST){

    /* "Expected command C_SITELIST not sent" */

    error(A_ERR, "get_info", GET_SITES_004);
    return(ERROR);
  }

  port = atoi(command_v -> params[0]);

  if(cliconnect(server_host, port, &data_fd) != A_OK){

    /* "Can't connect to server host port %u" */

    error(A_ERR, "get_info", GET_SITES_005, port);
    return(ERROR);
  }

  if(data_fd < 0){

    /* "Unable to open remote connection to remote server" */

    error(A_ERR, "get_sites", GET_SITES_006);
    return(ERROR);
  }

  if(str_decompose(tuple,NET_DELIM_CHAR, host_select, db_name,port_num) == ERROR){

    /* "Unable to decompose given tuple %s" */

    error(A_ERR, "get_info", GET_SITES_007, tuple);
    return(ERROR);
  }

  /* This will force the last packet of the 3-way handshake to go through ..
     the ack of the connection is never resent .. and this may cause
     deadlock...
     */
  
  write(data_fd, "\n",1);
  
  sprintf(database_server,"%s/%s/%s%s",get_archie_home(), DEFAULT_BIN_DIR,DEFAULT_DB_SERVER_PREFIX,db_name);
  sprintf(database_name,"%s%s", db_name, DB_SUFFIX);
  
  if((arglist = (char **) malloc(MAX_NO_PARAMS * sizeof(char *))) == (char **) NULL){

    /* "Can't malloc space for argument list" */

    error(A_SYSERR, "get_info", GET_SITES_017); 
    return(ERROR);
  }

  /* set up argument list */

  j = 0;
  arglist[j++] = database_server;

  arglist[j++] = "-M";
  arglist[j++] = get_master_db_dir();

  arglist[j++] = "-I";

  logname = get_archie_logname();

  if(logname[0]){
    arglist[j++] = "-L";
    arglist[j++] = logname;
  }
  
  if ( command == C_SENDEXCERPT)
  arglist[j++] = "-e";

  if ( suff_num >= 0 ) {
    arglist[j++] = "-n";
    sprintf(suffix,"%d",suff_num);
    arglist[j++] = suffix;
  }
  
  if(verbose)
  arglist[j++] = "-v";   

  if ( timeout ) {
    sprintf(timeout_str,"%d",timeout);
    arglist[j++] = "-t";
    arglist[j++] = timeout_str;
  }

  
  arglist[j] = (char *) NULL;


  if((result = fork()) == 0){   /* Child process */

    if(dup2(data_fd, 0) == -1){

      /* "Can't dup2() remote server connection to local transmission" */

      error(A_SYSERR, "get_info", GET_SITES_008);
      exit(ERROR);
    }

    execvp(database_server, arglist); 

    /* "Can't execl() receive server %s" */

    error(A_SYSERR,"get_info", GET_SITES_009, database_server);
  }
   
  free(arglist);

  if(result == -1){

    /* "Can't vfork() database server %s" */

    error(A_SYSERR,"get_info", GET_SITES_010, database_server);
    return(ERROR);
  }

  if(wait(&status) == -1){

    /* "Error while in wait() for database server %s" */

    error(A_SYSERR, "get_info", GET_SITES_014, database_server);
  }


  if(WIFEXITED(status) && WEXITSTATUS(status)){

    /* "Database server %s exited abnormally with value %u" */

    error(A_ERR,"get_info", GET_SITES_012, database_server, WEXITSTATUS(status));
    if ( WEXITSTATUS(status) && retry_seq < MAX_RETRY_SEQ ) {
      retry_seq++;
      close(data_fd);
      goto retry_site;
    }
  }

  if(WIFSIGNALED(status)){

    /* "Database server %s terminated abnormally with signal %u" */

    error(A_ERR,"get_info", GET_SITES_013, database_server, WTERMSIG(status));
  }


  close(data_fd);

  return A_OK;
}


status_t get_sites( hostaux_db, server_host, tuple_list, tuple_num, command_ifp, command_ofp, timeout, compress_trans)
   file_info_t *hostaux_db;   /* auxiliary host database */
  hostname_t server_host;    /* remote server host */
   tuple_t *tuple_list;	      /* tuples of information to be retrieved */
   index_t tuple_num;	      /* number of tuples in list */
   FILE    *command_ifp;      /* input command connection to remote server */
   FILE    *command_ofp;      /* output command connection to remote server */
   int	   timeout;
   int	   compress_trans;    /* compression mode */
{

  int i;
  net_scommand_t params[MAX_PARAMS];
  int finished;
  int suff_num;
  
  pathname_t db_name;
  pathname_t port_num;
  hostname_t host_select;

  ptr_check(tuple_list, tuple_t, "get_sites", ERROR);
  ptr_check(command_ifp, FILE, "get_sites", ERROR);
  ptr_check(command_ofp, FILE, "get_sites", ERROR);
   
  if((server_host == (char *) NULL ) || (server_host[0] == '\0')){

    /* "NULL pointer or contents passed for server host" */

    error(A_ERR, "get_sites", GET_SITES_001);
    return(ERROR);
  }

  for(i=0; i < tuple_num; i++){
    error(A_INFO,"get_sites","%s, %s, %s", tuple_list[i], get_master_db_dir(), server_host);

    if(str_decompose(tuple_list[i],NET_DELIM_CHAR, host_select, db_name,port_num) == ERROR){

      /* "Unable to decompose given tuple %s" */

      error(A_ERR, "get_sites", GET_SITES_007, tuple_list[i]);
      return(ERROR);
    }

    if ( strcasecmp(db_name,WEBINDEX_DB_NAME) == 0 ) {
      /* strcpy(params[0], tuple_list[i]); */
      sprintf(params[0],"%s:%s",tuple_list[i],get_retrieve_time(hostaux_db,host_select,db_name,port_num));
    }
    else {
      strcpy(params[0],tuple_list[i]);
    }
      
    if(compress_trans == 1){

      /* actively compress */

      strcpy(params[1], "compress");
    }
    else if(compress_trans == -1){

      /* actively uncompress */

      strcpy(params[1], "uncompress");
    } else
    strcpy(params[1], "");



    srand(time((time_t *) NULL));
    rand(); rand();

    for(finished = 0; !finished;){
      pathname_t tmp,tmp1,tmp2,tmp3,tmp4;
      sprintf(tmp,"%s/%s/%s-%s_%d", get_master_db_dir(), DEFAULT_TMP_DIR,
              host_select, db_name, (suff_num = rand() % 100));
      
      sprintf(tmp1,"%s%s%s",tmp,SUFFIX_UPDATE,TMP_SUFFIX);
      sprintf(tmp2,"%s%s%s",tmp,SUFFIX_EXCERPT,TMP_SUFFIX);
      sprintf(tmp3,"%s%s",tmp,SUFFIX_UPDATE);
      sprintf(tmp4,"%s%s",tmp,SUFFIX_EXCERPT);


      if(access(tmp1, R_OK | F_OK) == -1 && access(tmp2, R_OK | F_OK) == -1 &&
         access(tmp3, R_OK | F_OK) == -1 && access(tmp4, R_OK | F_OK) == -1 )
      finished = 1;
    }
    
    if (get_info(server_host, tuple_list[i], params, command_ifp, command_ofp, timeout, compress_trans,C_SENDSITE,suff_num) == ERROR ) {

    }

    if ( strcasecmp(db_name, WEBINDEX_DB_NAME) == 0 ) {
      if (get_info(server_host, tuple_list[i], params, command_ifp, command_ofp, timeout, compress_trans,C_SENDEXCERPT,suff_num) == ERROR ) {

      }
    }
    
  }

  return(A_OK);
}

/*
 * get_headers: ask for and receive headers from remote server
 */


status_t get_headers(hostaux_db, tuple_list, tuple_num, max_date, command_ifp, command_ofp)
  file_info_t *hostaux_db;
   tuple_t *tuple_list;	   /* list of tuples to be processed */
   int     tuple_num;	   /* number of tuples in list */
   int	   *max_date;
   FILE *command_ifp;	   /* command connection file pointer */
   FILE *command_ofp;	   /* command connection file pointer */

{
  int i;
  header_t header_rec;
  command_v_t *command_v;
  hostname_t source_archie_host;
  hostname_t primary_hostname;
  hostname_t preferred_hostname;   
  char date_str[EXTERN_DATE_LEN + 1];
  char	ip_string[18];
  database_name_t db_name;
  pathname_t tmp_string;
  file_info_t *output_file = create_finfo();
  char port[10];
  int finished;
   
  ptr_check(tuple_list, tuple_t, "get_headers", ERROR);
  ptr_check(command_ifp, FILE, "get_headers", ERROR);
  ptr_check(command_ofp, FILE, "get_headers", ERROR);

  *max_date = 0;
   
  for(i=0; i < tuple_num; i++){

    error(A_INFO,"get_headers","%s, %s", tuple_list[i], get_master_db_dir());

    if(str_decompose(tuple_list[i],NET_DELIM_CHAR,source_archie_host,
                     date_str,primary_hostname, preferred_hostname,
                     ip_string, db_name,port) == ERROR){

      /* "Unable to decompose tuple %s" */

      error(A_ERR, "get_headers", GET_HEADERS_001, tuple_list[i]);
      return(ERROR);
    }

    if(*max_date < cvt_to_inttime(date_str, 0))
    *max_date = cvt_to_inttime(date_str, 0);

    if ( strcasecmp(db_name, WEBINDEX_DB_NAME) == 0 ) {
      sprintf(tmp_string,"%s:%s:%s:%s", primary_hostname, db_name,port,
              get_retrieve_time(hostaux_db,primary_hostname,db_name,port) ); 
    }
    else  {
      sprintf(tmp_string,"%s:%s", primary_hostname, db_name);
    }
    if(write_net_command(C_SENDHEADER, tmp_string, command_ofp) == ERROR){

      /* "Can't write C_SENDHEADER command to control connection" */

      error(A_ERR, "get_headers", GET_HEADERS_002);
      return(ERROR);
    }


    if((command_v = read_net_command(command_ifp)) == (command_v_t *) NULL){

      /* "Error while trying to read incoming command" */

      error(A_ERR, "get_headers", GET_HEADERS_003);
      return(ERROR);
    }

    if(command_v -> command != C_HEADER){

      /* "Got unexpected command. Expected C_HEADER" */

      error(A_ERR, "get_headers", GET_HEADERS_004);
      return(ERROR);
    }

    if(read_header(command_ifp, &header_rec, (u32 *) NULL,1,0) == ERROR){

      /* "Error while trying to read incoming header from control connection" */

      error(A_ERR, "get_headers", GET_HEADERS_005);
      return(ERROR);
    }


    /*
     * This has already been deleted. Don't worry about retrieving the
     * header
     */

    if(header_rec.current_status == DELETED){

      /* "Site/database already deleted %s. Ignoring" */

      error(A_INFO, "get_headers", GET_HEADERS_010, header_rec.primary_hostname);
      continue;
    }

    srand(time((time_t *) NULL));
    rand(); rand();

    for(finished = 0; !finished;) {

      sprintf(output_file -> filename, "%s/%s/%s-%s_%d%s%s",
              get_master_db_dir(), DEFAULT_TMP_DIR,
              header_rec.primary_hostname, header_rec.access_methods,
              rand()%100, SUFFIX_RETR,TMP_SUFFIX);
      
      if(access(output_file->filename, R_OK | F_OK) == -1 )
      finished = 1;
    }
      

    if(open_file(output_file, O_WRONLY) == ERROR){

      /* "Error while trying to open header file %s" */

      error(A_ERR, "get_headers", GET_HEADERS_006, output_file -> filename);
      return(ERROR);
    }

    if(!HDR_GET_SOURCE_ARCHIE_HOSTNAME(header_rec.header_flags)
       && get_archie_hostname(header_rec.source_archie_hostname, sizeof(header_rec.source_archie_hostname)) != (char *) NULL)
    HDR_SET_SOURCE_ARCHIE_HOSTNAME(header_rec.header_flags);


    if(write_header(output_file -> fp_or_dbm.fp, &header_rec, (u32 *) NULL, 0, 0) == ERROR){

      /* "Error while trying to write header into %s" */

      error(A_ERR, "get_headers", GET_HEADERS_007, output_file -> filename);
      return(ERROR);
    }

    if(close_file(output_file) == ERROR){

      /* "Error trying to close header file %s" */

      error(A_ERR, "get_headers", GET_HEADERS_008, output_file -> filename);
      return(ERROR);
    }


    for(finished = 0; !finished;) {

      sprintf(tmp_string, "%s/%s/%s-%s_%d%s",get_master_db_dir(), DEFAULT_TMP_DIR, header_rec.primary_hostname, header_rec.access_methods, rand()%100,SUFFIX_RETR);

      if(access(tmp_string, R_OK | F_OK) == -1 )
      finished = 1;
    }
      


    if(archie_rename(output_file -> filename, tmp_string) == -1){

      /* "Can't rename temporary header file %s to %s" */
         
      error(A_SYSERR,"get_headers", GET_HEADERS_009, output_file -> filename, tmp_string);
      return(ERROR);
    }

    error(A_INFO,"get_headers","Wrote header file %s", tmp_string);

  }
  return(A_OK);

}


/*
 * sig_handle: set the global variable when a signal is caught
 */


void sig_handle(sig)
   int sig;
{

   if(sig != SIGALRM){

      /* "program terminated with signal %d" */

      error(A_ERR,"sig_handle",SIG_HANDLE_001, sig);
      exit(ERROR);
   }
   else
      signal_set = 1;
   
}


char *get_retrieve_time( hostaux_db, host,dn, port )
  file_info_t *hostaux_db;
  hostname_t host;
  char *dn;
  char *port;
{

  hostdb_aux_t hostaux_rec;
  index_t index;
  pathname_t access;
  static char *beg =  "19700101000001";
  
  sprintf(access,"%s::",port);
  if ( get_hostaux_ent(host, dn, &index, NULL, 
                       access, &hostaux_rec, hostaux_db) == ERROR ) {
    return beg;
  }

  if (hostaux_rec.current_status != ACTIVE ) {
    return beg;
  }
  
  return cvt_from_inttime(hostaux_rec.retrieve_time);


}
