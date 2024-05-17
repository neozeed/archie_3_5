/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef AIX
#include <sys/select.h>
#include <sys/socket.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "protos.h"
#include "typedef.h"
#include "db_files.h"
#include "db_ops.h"
#include "host_db.h"
#include "domain.h"
#include "databases.h"
#include "tuple.h"
#include "listd.h"
#include "error.h"
#include "archie_strings.h"
#include "lang_exchange.h"
#include "archie_dbm.h"
#include "times.h"
#include "files.h"
#include "master.h"

extern status_t get_port();

/*
 * sort_retrieve: sort the t_hold_t type based on retrieve time
 */


int sort_retrieve(a, b)
   t_hold_t *a, *b;

{
   if(a -> retrieve_time < b -> retrieve_time)
      return(-1);
   else if(a -> retrieve_time > b -> retrieve_time)
      return(1);
   else
      return(0);
}


static status_t find_in_aceess( access, db_name )
  access_methods_t access;
  char *db_name;
{

  char **av;
  int i;

  av = (char**) str_sep(access,':');
  if ( av == NULL )
    return ERROR;
 
  for (i  = 0 ; av[i] != NULL && av[i][0] != '\0'; i++ ) {
    if ( strcasecmp(av[i],db_name) == 0 ) {
      free_opts(av);
      return A_OK;
    }
  }

  free_opts(av);
  return ERROR;
  
}

/*
 * compose_tuples: generate the tuples in hostdb fulfilling the criteria
 * list by the databases, relation, fromdate and domains variables. The
 * list of tuples generated is returned and tuple count contains the number
 * of tuples generated
 */

   

tuple_t **compose_tuples( databases, relation, fromdate, domains, hostbyaddr, hostdb, domaindb, hostaux_db, tuple_count)
   char *databases;	      /* colon separated list of databases in which to look */
   char *relation;	      /* the before or after given "fromdate" */
   date_time_t fromdate;      /* cutoff date */
   char *domains;	      /* colon separated list of domains */
   file_info_t *hostbyaddr;   /* host address cache */
   file_info_t *hostdb;	      /* primary host database */
   file_info_t *domaindb;     /* domain database */
   file_info_t *hostaux_db;   /* auxiliary host database */
   int *tuple_count;	      /* number of tuples returned */

{
   datum ip_search, ip_data;
   tuple_t **tuple_list;
   t_hold_t *th_hold;
   hostdb_t hostdb_entry;
   hostdb_aux_t hostaux_entry;
   hostbyaddr_t hbaddr;
   int tuple_max;
   database_name_t  database_list[MAX_NO_DATABASES];
   int count;
   domain_t domain_list[MAX_NO_DOMAINS];
   int domain_count;
   pathname_t tmp_string;
   int mycount;

   int all_databases = 0;
   int tuple_idx = 0;
   int database_count;

   ptr_check(databases, char, "compose_tuples", (tuple_t **) NULL);
   ptr_check(relation, char, "compose_tuples", (tuple_t **) NULL);
   ptr_check(domains, char, "compose_tuples", (tuple_t **) NULL);
   ptr_check(tuple_count, int, "compose_tuples", (tuple_t **) NULL);

   ptr_check(hostbyaddr, file_info_t, "compose_tuples", (tuple_t **) NULL);
   ptr_check(hostdb, file_info_t, "compose_tuples", (tuple_t **) NULL); 
   ptr_check(domaindb, file_info_t, "compose_tuples", (tuple_t **) NULL); 
   ptr_check(hostaux_db, file_info_t, "compose_tuples", (tuple_t **) NULL);

   tuple_idx = 0;

   tmp_string[0] = '\0';
   
   if(compile_domains(domains, domain_list, domaindb, &domain_count) == ERROR){

      /* "Can't compile domain list %s" */

      error(A_ERR,"compose_tuples", COMPOSE_TUPLES_001, domains );
      return((tuple_t **) NULL);
   }

   if(compile_database_list(databases, database_list, &database_count) == ERROR){

      /* "Can't compile database list %s" */

      error(A_ERR,"compose_tuples", COMPOSE_TUPLES_002, databases);
      return((tuple_t **) NULL);
   }

   if(strcmp(database_list[0],DATABASES_ALL) == 0)
      all_databases = 1;

   tuple_max = DEFAULT_TUPLE_LIST_SIZE;

   if((th_hold = (t_hold_t *) malloc(tuple_max * sizeof(t_hold_t))) == (t_hold_t *) NULL){

      /* "Can't malloc space for tuple hold list" */

      error(A_SYSERR,"compose_tuples", COMPOSE_TUPLES_003);
      return((tuple_t **) NULL);
   }

   ip_search = dbm_firstkey(hostbyaddr -> fp_or_dbm.dbm);

   if(ip_search.dptr == (char *) NULL){

      /* "Can't find first entry in hostbyaddr database" */

      error(A_INTERR,"compose_tuples", COMPOSE_TUPLES_004);
      return((tuple_t **) NULL);
   }

   do{

      ip_data = dbm_fetch(hostbyaddr -> fp_or_dbm.dbm, ip_search);

      if ( ip_data.dsize == 0 || ip_data.dptr == NULL ) {
        ip_search = dbm_nextkey(hostbyaddr -> fp_or_dbm.dbm);
      
        if(dbm_error(hostdb -> fp_or_dbm.dbm)){

          /* "Can't find next entry in hostbyaddr database" */

          error(A_ERR,"compose_tuples", COMPOSE_TUPLES_007);
          return((tuple_t **) NULL);
        }

        continue;
      }
      
      memcpy(&hbaddr, ip_data.dptr, sizeof(hostbyaddr_t));

      if(get_dbm_entry(hbaddr.primary_hostname,strlen(hbaddr.primary_hostname) + 1, &hostdb_entry, hostdb) == ERROR){

	 /* "Located %s in hostbyaddr database. Can't find in primary host database" */

	 error(A_ERR,"compose_tuples", COMPOSE_TUPLES_005, hbaddr.primary_hostname);
         ip_search = dbm_nextkey(hostbyaddr -> fp_or_dbm.dbm);

	 if(dbm_error(hostdb -> fp_or_dbm.dbm)){

	    /* "Can't find next entry in hostbyaddr database" */

	    error(A_INTERR,"compose_tuples", COMPOSE_TUPLES_007);
	    return((tuple_t **) NULL);
	 }

	 continue;
      }
      
      if(find_in_domains(hostdb_entry.primary_hostname, domain_list, domain_count) == 0){
         ip_search = dbm_nextkey(hostbyaddr -> fp_or_dbm.dbm);

	 if(dbm_error(hostdb -> fp_or_dbm.dbm)){

	    /* "Can't find next entry in hostbyaddr database" */

	    error(A_INTERR,"compose_tuples", COMPOSE_TUPLES_007);
	    return((tuple_t **) NULL);
	 }

	 continue;
      }

      if(all_databases)
         compile_database_list(hostdb_entry.access_methods, database_list, &database_count);

      for(count = 0; database_list[count][0] != '\0'; count++){
        index_t index;
        int i;

        if ( find_in_aceess( hostdb_entry.access_methods, database_list[count]) == ERROR )
        continue;
        
        find_hostaux_last(hostdb_entry.primary_hostname, database_list[count],
                          &index, hostaux_db);

        for ( i = 0; i <= (int)index; i++ ) {

          sprintf(tmp_string, "%s.%s.%d", hostdb_entry.primary_hostname, database_list[count],i);

          if(get_dbm_entry(tmp_string, strlen(tmp_string) + 1, &hostaux_entry, hostaux_db) == ERROR)
          continue;

          if((hostaux_entry.current_status == DISABLED)
             || (hostaux_entry.current_status == NOT_SUPPORTED))
          continue;

          /* construct the tuple */

          if(tuple_idx == tuple_max){
            t_hold_t *thold;

            /* Allocate more space */

            tuple_max += DEFAULT_TUPLE_LIST_SIZE;
	 
            if((thold = (t_hold_t *) realloc(th_hold, tuple_max * sizeof(t_hold_t))) == (t_hold_t *) NULL){

              /* "Can't realloc space for tuple hold list" */

              error(A_INTERR,"compose_tuples", COMPOSE_TUPLES_008);
              return((tuple_t **) NULL);
            }

            th_hold = thold;

          }

          strcpy(th_hold[tuple_idx].source_archie_hostname, hostaux_entry.source_archie_hostname);
          th_hold[tuple_idx].retrieve_time = hostaux_entry.retrieve_time;
          strcpy(th_hold[tuple_idx].primary_hostname, hostdb_entry.primary_hostname);
          strcpy(th_hold[tuple_idx].preferred_hostname, hostaux_entry.preferred_hostname);
          th_hold[tuple_idx].primary_ipaddr = hostdb_entry.primary_ipaddr;
          strcpy(th_hold[tuple_idx].database_name, database_list[count]);
          th_hold[tuple_idx].flags = hostaux_entry.flags;
          
          get_port(hostaux_entry.access_command, database_list[count],&th_hold[tuple_idx].port);

          tuple_idx++;
        }
      }
		
      ip_search = dbm_nextkey(hostbyaddr -> fp_or_dbm.dbm);
      
      if(dbm_error(hostdb -> fp_or_dbm.dbm)){

        /* "Can't find next entry in hostbyaddr database" */

        error(A_ERR,"compose_tuples", COMPOSE_TUPLES_007);
        return((tuple_t **) NULL);
      }

    } while(ip_search.dptr != (char *) NULL);


   qsort(th_hold, tuple_idx, sizeof(t_hold_t), sort_retrieve);

   if((tuple_list = (tuple_t **) malloc(tuple_max * sizeof(tuple_t *))) == (tuple_t **) NULL){

      /* "Can't malloc space for tuple list" */

      error(A_SYSERR,"compose_tuples", COMPOSE_TUPLES_009);
      return((tuple_t **) NULL);
   }

   for(count = 0, mycount = 0; count < tuple_idx; count++){
      
      if(((relation[0] == '>') && ((int) fromdate < (int) th_hold[count].retrieve_time))
	 || ((relation[0] == '<') && ((int) fromdate > (int) th_hold[count].retrieve_time))
	 || HADB_IS_FORCE_UPDATE(th_hold[count].flags)){

	 if((tuple_list[mycount] = (tuple_t *) malloc(MAX_TUPLE_SIZE)) == (tuple_t *) NULL){


	    error(A_SYSERR,"compose_tuples", COMPOSE_TUPLES_010);
	    return((tuple_t **) NULL);
	 }

   if ( strcasecmp(th_hold[count].database_name, WEBINDEX_DB_NAME) == 0 ) {
     sprintf((char *) tuple_list[mycount],
             "%s:%s:%s:%s:%s:%s:%d", th_hold[count].source_archie_hostname,
             cvt_from_inttime(th_hold[count].retrieve_time),
             th_hold[count].primary_hostname,
             th_hold[count].preferred_hostname,
             inet_ntoa(ipaddr_to_inet(th_hold[count].primary_ipaddr)),
             th_hold[count].database_name, th_hold[count].port);
   }
   else {
     sprintf((char *) tuple_list[mycount],
             "%s:%s:%s:%s:%s:%s", th_hold[count].source_archie_hostname,
             cvt_from_inttime(th_hold[count].retrieve_time),
             th_hold[count].primary_hostname,
             th_hold[count].preferred_hostname,
             inet_ntoa(ipaddr_to_inet(th_hold[count].primary_ipaddr)),
             th_hold[count].database_name);
   }
	 mycount++;

      }
   }

   *tuple_count = mycount;

   if(th_hold)
      free(th_hold);

   return(tuple_list);
}



/*
 * process_tuples: decide on the basis of the incoming tuples which sites
 * are appropriate
 */



status_t process_tuples(tuple_list, tuple_num, new_tlist, force_hosts, max_date, hostdb, hostaux_db)
   tuple_t *tuple_list;	      /* list of tuples to be processed */
   index_t *tuple_num;	      /* number of tuples */
   tuple_t *new_tlist;	      /* returned tuple list */
   char	   *force_hosts;      /* host that must be returned */
   int	   *max_date;	      /* maximum date of retrieved host */
   file_info_t *hostdb;	      /* primary host database */
   file_info_t *hostaux_db;   /* auxiliary host database */

{
   extern int verbose;

   int i;
   hostdb_t hostdb_ent;
   hostdb_aux_t hostaux_ent;
   hostname_t source_archie_host;
   hostname_t primary_hostname;
   hostname_t preferred_hostname;
   char	ip_string[18];
   char date_str[EXTERN_DATE_LEN + 1];
/*   pathname_t tmp_str;*/
   database_name_t db_name;
   int new_count;
   date_time_t this_date;
   char port[10];
   access_comm_t access_command;
   index_t index;

   ptr_check(tuple_list, tuple_t, "process_tuples", ERROR);
   ptr_check(tuple_num, index_t, "process_tuples", ERROR);
   ptr_check(new_tlist, tuple_t, "process_tuples", ERROR);
   ptr_check(hostdb, file_info_t, "process_tuples", ERROR);
   ptr_check(hostaux_db, file_info_t, "process_tuples", ERROR);
   

   for(i = 0, new_count = 0; i < *tuple_num; i++){
     port[0] = '\0';
     if(str_decompose(tuple_list[i],NET_DELIM_CHAR,source_archie_host,
                      date_str,primary_hostname, preferred_hostname,
                      ip_string, db_name,port) == ERROR &&
       str_decompose(tuple_list[i],NET_DELIM_CHAR,source_archie_host,
                      date_str,primary_hostname, preferred_hostname,
                      ip_string, db_name) == ERROR ) {

       /* "Can't decompose incoming tuple %s" */

       error(A_ERR, "process_tuples", PROCESS_TUPLES_001, tuple_list[i]);
       return(ERROR);
     }

     this_date = cvt_to_inttime(date_str,0);

     /* get all sites if force_hosts contains a host list */

     if(force_hosts[0] != '\0'){
       if ( strcmp(db_name,ANONFTP_DB_NAME) == 0 ) 
       sprintf(new_tlist[new_count++],"%s:%s", primary_hostname, db_name);
       else
       sprintf(new_tlist[new_count++],"%s:%s:%s", primary_hostname, db_name,port);
		       
       continue;
     }

     /* If the host entry does not exist then it is probably a new host.
        Add it to the new tuple list and continue */

     if(get_dbm_entry(make_lcase(primary_hostname), strlen(primary_hostname)+1,
                      &hostdb_ent, hostdb) == ERROR){

       if ( strcmp(db_name,ANONFTP_DB_NAME) == 0 ) 
       sprintf(new_tlist[new_count++],"%s:%s", primary_hostname, db_name);
       else
       sprintf(new_tlist[new_count++],"%s:%s:%s", primary_hostname, db_name,port);

       if( this_date > *max_date)
       *max_date = this_date;

       continue;
     }

     /* Construct the auxiliary hostdb name */
     
     sprintf(access_command,"%s::",port);
     if ( get_hostaux_ent(primary_hostname, db_name, &index,
                          preferred_hostname, access_command,
                          &hostaux_ent, hostaux_db) == ERROR){
#ifdef BLAH
     sprintf(tmp_str,"%s.%s",primary_hostname, db_name);

     /* As above: if we don't have it, then get it */

     if(get_dbm_entry(tmp_str, strlen(tmp_str) + 1, &hostaux_ent, hostaux_db) == ERROR){
#endif
       if ( strcmp(db_name,ANONFTP_DB_NAME) == 0 ) 
       sprintf(new_tlist[new_count++],"%s:%s", primary_hostname, db_name);
       else
       sprintf(new_tlist[new_count++],"%s:%s:%s", primary_hostname, db_name,port);

       if(this_date > *max_date)
       *max_date = this_date;

       continue;
     }

     /* Otherwise this is a host we already have */

     /* Make sure the retrieve time is greater than what we
        already have AND it hasn't been disabled, or marked for
        deletion */

     if((hostaux_ent.retrieve_time < this_date)
        && (hostaux_ent.current_status != DISABLED)
        && (hostaux_ent.current_status != DEL_BY_ARCHIE)
        && (hostaux_ent.current_status != DEL_BY_ADMIN)){
       if ( strcmp(db_name,ANONFTP_DB_NAME) == 0 ) 
       sprintf(new_tlist[new_count++],"%s:%s", primary_hostname, db_name);
       else
       sprintf(new_tlist[new_count++],"%s:%s:%s", primary_hostname, db_name,port);

     }

     if( this_date > *max_date)
     *max_date = this_date;

   }

   if(verbose){

      /* "%d sites match given criteria" */

      error(A_INFO, "process_tuples", PROCESS_TUPLES_002, new_count);
   }

   new_tlist[new_count][0] = '\0';
   *tuple_num = new_count;
   return(A_OK);
}

/*
 * send_header: given the tuple, obtain the information for that host from
 * the primary and auxiliary host databases and construct the associated
 * header
 */


status_t send_header(tuple, header_rec, hostdb, hostaux_db)
    void	*tuple;		 /* list of tuples */
    header_t    *header_rec;	 /* returned header record */
    file_info_t *hostdb;	 /* primary host database */
    file_info_t *hostaux_db;	 /* auxiliary host database */
{
   hostname_t primary_hostname;
   hostdb_t hostdb_entry;
   hostdb_aux_t hostaux_entry;
   pathname_t database;
   pathname_t tmp_string;
   char port[10],date_str[50];
   index_t index;
   int ret;
   
   ptr_check(tuple, void, "send_header", ERROR);
   ptr_check(header_rec, header_t, "send_header", ERROR);
   ptr_check(hostdb, file_info_t, "send_header", ERROR);   ptr_check(hostaux_db, file_info_t, "send_header", ERROR);   

   /* Return is composed of primary_hostname:database pair */

   port[0] = '\0';
   date_str[0] = '\0';
   ret = sscanf(tuple,"%[^:]:%[^:]:%[^:]:%s", primary_hostname, database,port,date_str);

   if(get_dbm_entry(primary_hostname, strlen(primary_hostname) + 1,
		    &hostdb_entry, hostdb) == ERROR){

      /* "Error trying to read primary database entry for tuple %s" */

      error(A_ERR, "send_header", SEND_HEADER_001, tuple);
      return(ERROR);
   }

/*   strcat(port,"::"); */

   sprintf(tmp_string,"%s.%s", primary_hostname, database);

   error(A_INFO,"send_header","Sending %s %s", tmp_string, port);

   if ( get_hostaux_ent(primary_hostname, database, &index,
                        NULL,port, &hostaux_entry, hostaux_db) == ERROR ) {
                        
/*
   if(get_dbm_entry(tmp_string, strlen(tmp_string) + 1,
		    &hostaux_entry, hostaux_db) == ERROR){
*/
      /* "Error trying to obtain auxiliary host database entry for %s" */

      error(A_ERR, "send_header", SEND_HEADER_002, tuple);
      return(ERROR);
   }


   header_rec -> header_flags = 0;
   HDR_HOSTDB_ALSO(header_rec -> header_flags);
   HDR_HOSTAUX_ALSO(header_rec -> header_flags);

   HDR_UNSET_PARSE_TIME(header_rec -> header_flags);
#if 0
   HDR_UNSET_RETRIEVE_TIME(header_rec -> header_flags);
#endif

   make_header_hostdb_entry(header_rec,&hostdb_entry,0);
   make_header_hostaux_entry(header_rec,&hostaux_entry,0);   

   header_rec -> format = FRAW;
   HDR_SET_FORMAT(header_rec -> header_flags);

   header_rec -> generated_by = SERVER;
   HDR_SET_GENERATED_BY(header_rec -> header_flags);

   header_rec -> update_status = SUCCEED;
   HDR_SET_UPDATE_STATUS(header_rec -> header_flags);

   /* set header to only that database name that is being requested */

   strcpy(header_rec -> access_methods, database);

   if(!HDR_GET_SOURCE_ARCHIE_HOSTNAME(header_rec -> header_flags)
      && get_archie_hostname(header_rec -> source_archie_hostname, sizeof(header_rec -> source_archie_hostname)) != (char *) NULL)
      HDR_SET_SOURCE_ARCHIE_HOSTNAME(header_rec -> header_flags);

   return(A_OK);
}

void get_date(tuple,date_str)
  char *tuple;
  char *date_str;
{

  char t1[100],t2[100],t3[100];
  sscanf(tuple,"%[^:]:%[^:]:%[^:]:%s", t1,t2,t3,date_str);

}



/*
 * send_site: with the given tuple, spawn appropriate process for
 * transferring information about that site/database
 */

static status_t sendinfo(tuple, hostdb, hostaux_db,  timeout, compress_trans, excerpt)
   void *tuple;		   /* tuple for site */
   file_info_t *hostdb;	   /* primary hostdatabase */
   file_info_t *hostaux_db;/* auxiliary host database */
   int timeout;
   int compress_trans;
  int excerpt;
{
   extern int verbose;

   char *logname;
   int i;
   int socket_fd;
   int new_socket_fd;
   int port;
   int addrlen;
   struct sockaddr_in cliaddr;
   pathname_t database_server;
   pathname_t database_name;
   hostname_t primary_hostname;
   header_t header_rec;
   int status;
   int result;
   fd_set readmask;
   struct timeval timeval_struct;
   int finished;
   char **arglist;
   char port_num[20],date_string[20];
   char *t;
   
   ptr_check(tuple, void, "sendinfo", ERROR);
   ptr_check(hostdb, file_info_t, "sendinfo", ERROR);
   ptr_check(hostaux_db, file_info_t, "sendinfo", ERROR);

   port = 0;

   /* get a port number for the server */
   
   if(get_new_port(&port, &socket_fd) == ERROR){

      /* "Can't get data channel port number" */

      error(A_INTERR,"sendinfo", SENDSITE_001);

      return(ERROR);
   }
   
   /* Send port number to client */

   if(write_net_command(C_SITELIST, (void *) &port, stdout) == ERROR){

      /* "Can't issue SITELIST command on connection" */

      error(A_INTERR,"sendinfo", SENDSITE_002);
      return(ERROR);
   }

   memset((char *)&cliaddr, 0, sizeof( struct sockaddr_in )) ;
   addrlen = sizeof(struct sockaddr_in);

   /* Accept client connection. If no connection in timeout <blah> */

   FD_ZERO(&readmask);

   FD_SET(socket_fd, &readmask);

   timeval_struct.tv_sec = (long) timeout;
   timeval_struct.tv_usec = 0L;

   if((finished = select(FD_SETSIZE, &readmask, (fd_set *) NULL, (fd_set *) NULL, &timeval_struct)) < 0){

      /* "Error returned from select()" */

      error(A_SYSERR, "sendinfo", SENDSITE_016);
      return(ERROR);
   }
   else if(finished == 0){

      /* "Timeout of %d seconds waiting for remote client to connect" */

      error(A_ERR, "sendinfo", SENDSITE_015, timeout);
      return(ERROR);
   }

   if((new_socket_fd = accept( socket_fd, (struct sockaddr_in *)&cliaddr, &addrlen)) == -1){

      /* "Can't accept() connection for data channel" */

      error(A_SYSERR, "sendinfo", SENDSITE_003);
      return(ERROR);
   }


   if(close(socket_fd) == -1){

      /* "Can't shutdown accepting socket" */

      error(A_SYSERR, "sendinfo", SENDSITE_014);
   }

   /* this will determine what database is being accessed */

   if(send_header(tuple, &header_rec, hostdb, hostaux_db) == ERROR){

      /* "Error while trying to send header to remote client" */

      error(A_ERR, "sendinfo", SENDSITE_004);
      return(ERROR);
   }

   /* Name of server program */

   sprintf(database_server,"%s/%s/%s%s",get_archie_home(), DEFAULT_BIN_DIR,DEFAULT_DB_SERVER_PREFIX,  header_rec.access_methods);

   /* Name of database directory */


   get_date(tuple,date_string);
      

   sprintf(database_name,"%s%s", header_rec.access_methods, DB_SUFFIX);
   sprintf(port_num, "%s",header_rec.access_command);
   if ( (t = strchr(port_num,':' )) != NULL ) {
     *t = '\0';
   }
   else {
     strcpy(port_num,"80");
   }
  
           
   /* Name of host */

   strcpy(primary_hostname, header_rec.primary_hostname);

   
   error(A_INFO,"sendinfo","%s, %s, %s", primary_hostname, tail(database_name), tail(get_master_db_dir()));

   if((arglist = (char **) malloc(MAX_NO_PARAMS * sizeof(char *))) == (char **) NULL){

      /* "Can't malloc space for argument list" */

      error(A_SYSERR, "sendinfo", SENDSITE_017); 
      return(ERROR);
   }

   /* set up argument list */

   i = 0;
   arglist[i++] = database_server;

   arglist[i++] = "-M";
   arglist[i++] = get_master_db_dir();

   arglist[i++] = "-O";
   arglist[i++] = primary_hostname;

   arglist[i++] = "-p";
   arglist[i++] = port_num;
   
   logname = get_archie_logname();

   if(logname[0]){
      arglist[i++] = "-L";
      arglist[i++] = logname;
   }

   if ( strcasecmp(header_rec.access_methods,WEBINDEX_DB_NAME) == 0 ) {
     arglist[i++] = "-d";
     arglist[i++] = date_string;
   }
   
   if ( excerpt )
   arglist[i++] = "-e";
   if(compress_trans)
      arglist[i++] = "-c";

   if(verbose)
      arglist[i++] = "-v";   

   arglist[i] = (char *) NULL;

   if ( verbose ) {
     int yy;
     char tt[300];
     tt[0] = '\0';
     for ( yy = 0; yy < i; yy++ ) {
       strcat(tt,arglist[yy]);
       strcat(tt," ");
     }
     error(A_INFO, "arserver", tt);
   }
   
   if((result = fork()) == 0){   /* Child process */

      if(dup2(new_socket_fd, 1) == -1){

	 /* "Can't dup2() remote connection to stdin of exchange process" */

         error(A_SYSERR, "sendinfo", SENDSITE_006);
	 return(ERROR);
      }

      execvp(database_server,  arglist);

      /* "Can't execl() send server %s" */

      error(A_SYSERR,"sendinfo", SENDSITE_007, database_server);
   }

   free(arglist);

   if(result == -1){

      /* "Can't fork() database server %s" */

      error(A_SYSERR,"sendinfo",SENDSITE_012, database_server);
      return(ERROR);
   }

      
   if(wait(&status) == -1){

      /* "Error in wait() for exchange process %s" */

      error(A_SYSERR, "sendinfo", SENDSITE_008, database_server);
   }

   if(WIFEXITED(status) && WEXITSTATUS(status)){

      /* "exchange program %s exited abnormally with value %u" */

      error(A_ERR,"sendinfo", SENDSITE_009, database_server, WEXITSTATUS(status));
   }

   if(WIFSIGNALED(status)){

      /* "Database server terminated abnormally with signal %u" */

      error(A_ERR,"sendinfo", SENDSITE_010,WTERMSIG(status));
   }
      

   if(close(new_socket_fd) == -1){

      /* "Can't close socket" */

      error(A_SYSERR, "sendinfo", SENDSITE_011);
   }

   return(A_OK);
}



status_t sendsite(tuple, hostdb, hostaux_db, timeout, compress_trans)
   void *tuple;		   /* tuple for site */
   file_info_t *hostdb;	   /* primary hostdatabase */
   file_info_t *hostaux_db;/* auxiliary host database */
   int timeout;
   int compress_trans;
{
  return sendinfo(tuple,hostdb,hostaux_db, timeout, compress_trans,0);
}



status_t sendexcerpt(tuple, hostdb, hostaux_db, timeout, compress_trans)
   void *tuple;		   /* tuple for site */
   file_info_t *hostdb;	   /* primary hostdatabase */
   file_info_t *hostaux_db;/* auxiliary host database */
   int timeout;
   int compress_trans;
{
  return sendinfo(tuple,hostdb,hostaux_db, timeout, compress_trans,1);
}
