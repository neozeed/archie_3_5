#include <stdio.h>

#include "protos.h"
#include "defines.h"
#include "typedef.h"
#include "master.h"
#include "host_db.h"
#include "error.h"
#include "files.h"
#include "archie_dns.h"
#include "web.h"
#include "webindexdb_ops.h"
#include "extern_urls.h"
#include "lang_tools.h"


int verbose = 0;

static void strlower(str)
  char *str;
{
  while (*str != '\0' ){
    *str = tolower(*str);
    str++;
  }

}


static status_t process_externs PROTO((int,int, char *, file_info_t*, file_info_t *, file_info_t *, file_info_t * ));

int main(argc, argv)
  int argc;
  char **argv;
{

   extern char *optarg;

/*   extern int getopt PROTO((int, char **, char *)); */

   char **cmdline_ptr;
   int cmdline_args;

   pathname_t master_database_dir;
   pathname_t files_database_dir;
   file_info_t *hostdb = create_finfo();
   file_info_t *hostaux_db = create_finfo();
   file_info_t *hostbyaddr = create_finfo();
   file_info_t *domaindb = create_finfo(); 
   

   pathname_t logfile;
   pathname_t outname;
   hostname_t tmp_hostname;
   pathname_t domains;

   int interactive = 0;
   int logging = 0;
   int max_number = DEFAULT_MAX_NUMBER;

   pathname_t host_database_dir;

   int option;

   cmdline_ptr = argv + 1;
   cmdline_args = argc - 1;

   host_database_dir[0] = '\0';
   logfile[0] = outname[0] = master_database_dir[0] = '\0';
   files_database_dir[0] = '\0';
   tmp_hostname[0] = '\0';


   while((option = (int) getopt(argc, argv, "d:M:lvL:h:in:")) != EOF){

      switch(option){

	 /* Master database directory name */

	 case 'M':
	    strcpy(master_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;


	 /* log messages, given file */

	 case 'L':
       strcpy(logfile, optarg);
	    logging = 1;
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;


	 /* host db directory name. Ignored */

	 case 'h':
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

    case 'd':
      strcpy(domains,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;
      

	 case 'i':
      interactive = 1;
	    cmdline_ptr += 1;
	    cmdline_args -= 1;
	    break;      

    case 'n':
      max_number = atoi(optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;
      
	 /* log messages, default file */

	 case 'l':
	    logging = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

	 /* verbose mode */

	 case 'v':
	    verbose = 1;
	    cmdline_ptr ++;
	    cmdline_args --;
	    break;

	 default:

	    /* "Unknown option %c" */

	    error(A_ERR, argv[0], EXTERN_URLS_001, option);
	    exit(ERROR);
	    break;
      }
   }

   if(logging){
      if(logfile[0] == '\0'){
        if(open_alog((char *) NULL, A_INFO, tail(argv[0])) == ERROR){

          /*  "Can't open default log file" */

          error(A_ERR, argv[0], EXTERN_URLS_002);
          exit(ERROR);
        }
      }
      else{
        if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR){

          /* "Can't open log file %s" */

          error(A_ERR, argv[0], EXTERN_URLS_003, logfile);
          exit(ERROR);
        }
      }
    }
   
   if(set_master_db_dir(master_database_dir) == (char *) NULL){
     
     /* "Error while trying to set master database directory" */
     
     error(A_ERR,argv[0], EXTERN_URLS_006);
     exit(ERROR);
   }
   
  if( (char *)set_wfiles_db_dir(files_database_dir) == (char *) NULL){

    /* "Error while trying to set webindex database directory" */

    error(A_ERR, "extern_urls", "Error while trying to set webindex database directory" );
    exit(A_OK);
  }
   
   if(set_host_db_dir(host_database_dir) == (char *) NULL){
     
     /* "Error while trying to set host database directory" */
     
     error(A_ERR,"update_gopherindex", "Error ");
     exit(ERROR);
   }

   if(open_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db, O_RDWR) != A_OK){

     /* "Can't open host database directory" */

     error(A_ERR,"convert_hostdb", "Can't open host database directory" );
     exit(ERROR);
   }

   
   if ( process_externs( interactive,max_number,domains,hostbyaddr, hostdb, hostaux_db, domaindb ) == ERROR ) {
     fprintf(stderr,"Error processing external urls \n");
     close_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db);     
     return ERROR;
   }

   close_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db);
   return (A_OK);
}


status_t find_in_domain(server, domain_list, domain_count)
  char *server;
  domain_t domain_list[MAX_NO_DOMAINS];
  int domain_count;
{

  int i;

  char *domain;
  domain = strrchr(server,'.');
  if ( domain == NULL )
    domain = server;
  else
    domain++;
  
  for ( i = 0; i < domain_count ; i++ ) {
    if ( strcasecmp(domain, domain_list[i] ) == 0 )
      return A_OK;
  }
  
  return ERROR;
}

status_t get_list( domain_list, domain_count, db, list)
  domain_t domain_list[MAX_NO_DOMAINS];
  int domain_count;
  file_info_t *db;
  char ***list;
{
  char **l1,**l2;
  int num = 0;
  int max = 0;
  datum key;
  pathname_t site,server;
  int port;
  
  
  for (key = dbm_firstkey(db->fp_or_dbm.dbm); key.dptr !=  NULL;
       key = dbm_nextkey(db->fp_or_dbm.dbm)) {


    strncpy(site, key.dptr, key.dsize);
    site[key.dsize] = '\0';
    sscanf(site, "%[^:]:%d", server, &port);

    if ( find_in_domain(server, domain_list, domain_count) == A_OK ) {
      if ( num >= max-1 ) {
        max += 10;
        l2 = (char**)malloc(sizeof(char*)*max);
        if ( l2 == NULL ) {
          return ERROR;
        }
        if ( l1 ) {
          memcpy(l2,l1,sizeof(char*)*num);
          free(l1);
        }
        l1 = l2;
      }

      l1[num] = (char*)malloc(sizeof(char)*(key.dsize+1));
      if ( l1[num] == NULL ) {
        return ERROR;
      }
      strncpy(l1[num] , key.dptr,key.dsize);
      l1[num][key.dsize] = '\0';
      num++;
    }
  }
  if ( l1 == NULL ) {
    return ERROR;
  }
  l1[num] = NULL;
  *list = l1;
  return A_OK;
}

status_t process_externs(interactive, max_number, domains, hostbyaddr, hostdb, hostaux_db, domaindb)
  int interactive;
  int max_number;
  char *domains;
  file_info_t *domaindb;
  file_info_t *hostbyaddr;
  file_info_t *hostdb;
  file_info_t *hostaux_db;
{

  int i,j,k;
  int domain_count;
  domain_t domain_list[MAX_NO_DOMAINS];
  hostdb_t hostdb_entry;
  hostdb_aux_t hostaux_entry;
  hostbyaddr_t hostaddr_entry;
  int replace_aux = 0;
  int replace_db = 0;
  AR_DNS *dns;
  pathname_t tmp_str;
  index_t index;
  file_info_t *extern_db = create_finfo();
  char **externs_list;
  host_status_t host_status;
  hostname_t tmp_hostname;
  int replace_addr;
  
  sprintf(extern_db->filename,"%s/%s", get_wfiles_db_dir(), DEFAULT_EXTERN_URLS_DB);
  
  extern_db->fp_or_dbm.dbm = dbm_open(extern_db->filename, O_RDWR, DEFAULT_FILE_PERMS);
  if (extern_db->fp_or_dbm.dbm == (DBM*)NULL ) {
    error(A_ERR, "process_externs","Cannot open database of external urls: %s\n",extern_db->filename);
    dbm_close(extern_db->fp_or_dbm.dbm);
    return ERROR;
  }


  if(compile_domains(domains, domain_list, domaindb, &domain_count) == ERROR){
    dbm_close(extern_db->fp_or_dbm.dbm);
    return ERROR;
  }

  
  i = 0;
  if ( get_list(domains, domain_count, extern_db, &externs_list) == ERROR ) {
    dbm_close(extern_db->fp_or_dbm.dbm);
    return ERROR;
  }

  for ( k = 0; externs_list[k] != NULL ; k++ ) {
    pathname_t server;
    int port,port_no;
    
    replace_aux = 0;
    
    sscanf(externs_list[k], "%[^:]:%d", server, &port);
    strlower(server);
    
    if ( verbose ) 
    error(A_INFO,"extern_urls", "Found site %s port %d",server,port);

    if ( interactive ) {
      char answer[80];
      fprintf(stdout, "Do you want to add site %s with port number %d to the database (y/n) ? \n",server,port);
      fgets(answer,79,stdin);
      if ( answer[0] != 'y' && answer[0] != 'Y' )
      continue;
    }

      
    memset(&hostdb_entry, 0, sizeof(hostdb_t));
    memset(&hostaux_entry, 0, sizeof(hostdb_aux_t));

    if ( (dns = ar_open_dns_name(server,DNS_LOCAL_FIRST,hostdb)) == (AR_DNS*)NULL) {
      /* "Can't locate host '%s' in local database or DNS" */

      error(A_ERR,"extern_urls", "Can't locate host '%s' in local database or DNS", server);

    }
    else {

      strcpy(hostdb_entry.primary_hostname, dns->h_name);
      strlower(hostdb_entry.primary_hostname);
      
      if ( strcasecmp(dns->h_name, server) ) { /* Preferred name */
        strcpy(hostaux_entry.preferred_hostname, server);
        strlower(hostaux_entry.preferred_hostname);
      }

      if(get_dbm_entry(dns->h_name, strlen(dns->h_name) +1, &hostdb_entry, hostdb) == ERROR){

        /* site not found */

        strcpy(hostdb_entry.primary_hostname, dns->h_name);
        strlower(hostdb_entry.primary_hostname);
          
        if(( host_status = check_new_hentry(hostbyaddr, hostdb, &hostdb_entry, hostaux_db, &hostaux_entry, /*flags, control_flags*/ 0,0)) != HOST_OK){

          switch (host_status) {
          case HOST_STORED_OTHERWISE:
            
            error(A_ERR,"process_externs", "Host %s already stored, under hostname %s, skipping.",server,hostdb_entry.primary_hostname);
            continue;
            break;
            
          case HOST_PADDR_EXISTS:
            break;
            
          default:
            error(A_ERR,"process_externs", "Error while verifying new host entry, skipping");
            continue;
            break;
          }
          
          
        }
        if ( host_status == HOST_PADDR_EXISTS )
        replace_addr = 1;
        else
        replace_addr = 0;
        
        hostdb_entry.primary_ipaddr = *get_dns_addr(dns);
        strcpy(hostdb_entry.access_methods,WEBINDEX_DB_NAME);
/*        index = -1; */
        hostaddr_entry.primary_ipaddr = hostdb_entry.primary_ipaddr;
        strcpy(hostaddr_entry.primary_hostname,hostdb_entry.primary_hostname);
          
        if(put_dbm_entry(&(hostdb_entry.primary_ipaddr), sizeof(ip_addr_t),
                         &hostaddr_entry, sizeof(hostbyaddr_t),
                         hostbyaddr, replace_addr)){

          /* "Can't modify host address cache" */

          error(A_ERR, "process_externs", "Can't modify host address cache");
            
        }
          
      }
      else {                    /* site in database */


        replace_db = 1;
        find_hostaux_last(hostdb_entry.primary_hostname, WEBINDEX_DB_NAME,
                          &index, hostaux_db);

        if ( index < 0 ) {      /* No hostaux entry */

          if ( hostdb_entry.access_methods[0] == '\0' ) {
            strcpy(hostdb_entry.access_methods,WEBINDEX_DB_NAME);
          }
          else {
            pathname_t tmp;
            char **aux_list;
            int unfound = 0;
            
            aux_list = str_sep(hostdb_entry.access_methods,':');
            while((*aux_list != (char *) NULL) && (unfound = strcasecmp(*aux_list, WEBINDEX_DB_NAME)))
            aux_list++;

            if(!unfound){
            
              sprintf(tmp,"%s:%s",hostdb_entry.access_methods,WEBINDEX_DB_NAME);
              strcpy(hostdb_entry.access_methods,tmp);
            }
          }
            
        }
        else {                  /* Hostaux entry */
          for ( j = 0; j <= (int)index; j++ ) {
              
            sprintf(tmp_str,"%s.%s.%d",hostdb_entry.primary_hostname,
                    WEBINDEX_DB_NAME, j);
            if(get_dbm_entry(tmp_str, strlen(tmp_str) +1, &hostaux_entry,
                             hostaux_db) == ERROR){
              continue;
                
            }
            port_no = 0;
            (void)get_port(hostaux_entry.access_command, WEBINDEX_DB_NAME,
                           &port_no);

            if ( port == port_no ) { /* Hostaux  entry already there */
              replace_aux = 1;
              break;
            }
          }
        }
      }
      
      if ( replace_aux == 1 ) {
        error(A_INFO,"extern_urls","Site is already present, skipping");
        if ( delete_dbm_entry(externs_list[k], strlen(externs_list[k]),extern_db) == ERROR ) {
          fprintf(stderr,"Error deleting external url from databse\n");
        }
        continue;
      }
      
      if ( (int)index< 0 || j == (int)index+1 ) { /* New hostaux */
        if ( strcasecmp(dns->h_name, server) ) { /* Preferred name */
          strcpy(hostaux_entry.preferred_hostname, server);
        }
        sprintf(hostaux_entry.access_command,"%d::",port);
        hostaux_entry.current_status = ACTIVE;            
      }
      index = index+1;
      strcpy(hostaux_entry.source_archie_hostname, get_archie_hostname(tmp_hostname,sizeof (tmp_hostname)));

      sprintf(tmp_str, "%s.%s.%d", hostdb_entry.primary_hostname,
              WEBINDEX_DB_NAME, (int)index);

      if ( strcasecmp(dns->h_name, server) ) { /* Preferred name */
        strcpy(hostaux_entry.preferred_hostname, server);
        strlower(hostaux_entry.preferred_hostname);
      }

      if ( put_dbm_entry(tmp_str, strlen(tmp_str)+1, &hostaux_entry,
                         sizeof(hostdb_aux_t), hostaux_db,
                         replace_aux) == ERROR ) {
          
        error(A_ERR, "process_externs","Error adding hostaux entry");
      }

      if ( put_dbm_entry(hostdb_entry.primary_hostname,
                         strlen(hostdb_entry.primary_hostname)+1,
                         &hostdb_entry, sizeof(hostdb_t), hostdb,
                         replace_db) == ERROR ) {
          
        error(A_ERR,"process_externs", "Error adding hostdb entry");
      }

      if ( delete_dbm_entry(externs_list[k], strlen(externs_list[k]),extern_db) == ERROR ) {
        error(A_ERR,"process_externs", "Error deleting external url from databse");
      }
        
      i++;
        
    }
      
    if ( max_number > 0  && i >= max_number )
      break;    
  }

  dbm_close(extern_db->fp_or_dbm.dbm);
  return A_OK;

}



