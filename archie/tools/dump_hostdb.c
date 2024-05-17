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
 * dump_hostdb: Dump the host databases to an ASCII file
 *
   argv, argc are used.


   Parameters:	  
*/


#define	 DUMP_ALL    2
#define	 DUMP_SEMI   1
#define	 DUMP_PERM   0

static int verbose = 0;

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

  hostname_t *hostlist;
  int hostcount;

  hostdb_t hostdb_ent;
  hostdb_aux_t hostaux_ent;

  int dump_level = DUMP_ALL;

  char **hptr;

  char *tmp_str;

  int i;

  char **access_list;

  header_t out_header;

  file_info_t *hostbyaddr = create_finfo();
  file_info_t *hostdb = create_finfo();
  file_info_t *hostaux_db = create_finfo();
  file_info_t *domaindb = create_finfo();

  file_info_t *output_file = create_finfo();

  opterr = 0;

  domains[0] = host_database_dir[0] = master_database_dir[0] = '\0';

  /* ignore argv[0] */

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;

  while((option = (int) getopt(argc, argv, "h:M:D:o:e:v")) != EOF){

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

      /* output file */

    case 'o':
	    strcpy(output_file -> filename,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

    case 'v':
	    verbose = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

    default:
	    error(A_INFO,"dump_hostdb", "Usage: dump_hostdb [ -h <host dir> ]");
	    fprintf(stderr, "	    [ -M <master dir> ]\n");
	    fprintf(stderr, "	    [ -D <domains> ]\n");
	    fprintf(stderr, "	    [ -o <output file> ]\n");
	    fprintf(stderr, "	    [ -e <dump level> ]\n");
	    fprintf(stderr, "	    [ -v ]\n");
	    exit(A_OK);
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

    error(A_ERR,"dump_hostdb", "Error while trying to set host database directory" );
    exit(A_OK);
  }

  if(open_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db, O_RDONLY) != A_OK){

    /* "Error while trying to open host databases" */

    error(A_ERR,"dump_hostdb", "Error while trying to open host databases" );
    exit(A_OK);
  }


  if(output_file -> filename[0] == '\0')
  output_file -> fp_or_dbm.fp = stdout;
  else{

    if(open_file(output_file, O_WRONLY) == ERROR){

      /* "Can't open output file %s" */

      error(A_ERR, "dump_hostdb", "Can't open output file %s", output_file -> filename);
      exit(ERROR);
    }
  }
   

  if(domains[0] != '\0'){

    if(compile_domains(domains, domain_list, domaindb, &domain_count) == ERROR){

      /* "Can't compile given domain list %s" */

      error(A_ERR, "dump_hostdb", "Can't compile given domain list %s", domains);
      exit(ERROR);
    }
  }


  if(get_hostnames(hostdb, (hostname_t **) &hostlist, &hostcount, domain_list, domain_count) == ERROR){

    /* "Can't get list of hostnames from database" */

    error(A_ERR, "dump_hostdb", "Can't get list of hostnames from database" );
    exit(ERROR);
  }


  /* write header on dump file */

  fprintf(output_file -> fp_or_dbm.fp, "%d %s %d\n", hostcount, cvt_from_inttime(time((time_t *) NULL)), dump_level);

  for(i = hostcount - 1; i >= 0; i--){

    memset(&out_header, '\0', sizeof(header_t));

    if(get_dbm_entry(hostlist[i], strlen(hostlist[i])+1, &hostdb_ent, hostdb) == ERROR){

      /* "Can't find host %s in host database. Ignoring." */

      error(A_INTERR, "dump_hostdb", "Can't find host %s in host database. Ignoring." , hostlist[i]);
      continue;
    }

#if 0
    switch(dump_level){


    case DUMP_ALL:
    case DUMP_SEMI:

	    HDR_SET_PRIMARY_IPADDR(out_header.header_flags);
	    HDR_SET_OS_TYPE(out_header.header_flags);
	    HDR_SET_TIMEZONE(out_header.header_flags);
	    HDR_SET_ACCESS_METHODS(out_header.header_flags);
	    HDR_SET_PROSPERO_HOST(out_header.header_flags);

    case DUMP_PERM:

	    HDR_SET_PRIMARY_HOSTNAME(out_header.header_flags);

    }


    if(make_header_hostdb_entry(&out_header, &hostdb_ent, 0) == ERROR)
    exit(ERROR);

    if(verbose)
    error(A_INFO,"dump_hostdb", "Dumping %s", out_header.primary_hostname);

    if(write_header(output_file -> fp_or_dbm.fp, &out_header, (u32 *) NULL, 0, 0) == ERROR)
    exit(ERROR);
#endif 
    access_list = str_sep(hostdb_ent.access_methods, NET_DELIM_CHAR);

    for(hptr = access_list; *hptr != (char *) NULL; hptr++){
      int last,j;
      
      
      find_hostaux_last(hostdb_ent.primary_hostname,
                        *hptr, (index_t*)&last, hostaux_db);

      for ( j=0; j <= last; j++ ) {

        if ( get_hostaux_entry(hostdb_ent.primary_hostname,*hptr,
                               j, &hostaux_ent, hostaux_db) == ERROR ) {
          continue;
        }

        memset(&out_header, '\0', sizeof(header_t));        
        out_header.header_flags = 0;
        HDR_SET_PRIMARY_HOSTNAME(out_header.header_flags);
        switch(dump_level){

        case DUMP_ALL:

          HDR_SET_CURRENT_STATUS(out_header.header_flags);
          HDR_SET_GENERATED_BY(out_header.header_flags);	       
          HDR_SET_RETRIEVE_TIME(out_header.header_flags);
          HDR_SET_PARSE_TIME(out_header.header_flags);
          HDR_SET_UPDATE_TIME(out_header.header_flags);
          HDR_SET_NO_RECS(out_header.header_flags);
          HDR_SET_HCOMMENT(out_header.header_flags);
          HDR_SET_PRIMARY_IPADDR(out_header.header_flags);
          HDR_SET_OS_TYPE(out_header.header_flags);

          
        case DUMP_SEMI:
          HDR_SET_PREFERRED_HOSTNAME(out_header.header_flags);
          HDR_SET_SOURCE_ARCHIE_HOSTNAME(out_header.header_flags);
          HDR_SET_ACCESS_COMMAND(out_header.header_flags);
          HDR_SET_CURRENT_STATUS(out_header.header_flags);

        case DUMP_PERM:

          HDR_SET_ACCESS_METHODS(out_header.header_flags);

          break;
        }

        make_header_hostaux_entry(&out_header, &hostaux_ent, 0);
        out_header.update_time = hostaux_ent.update_time;
        strcpy(hostdb_ent.access_methods, *hptr);
		      
        if(make_header_hostdb_entry(&out_header, &hostdb_ent, 0) == ERROR)
        exit(ERROR);

        if(verbose)
        error(A_INFO, "dump_hostdb", "Dumping %s, %s", out_header.primary_hostname, out_header.access_methods);

        if(write_header(output_file -> fp_or_dbm.fp, &out_header, (u32 *) NULL, 0, 0) == ERROR)
        exit(ERROR);


      }
    }
	      
  }

  exit(A_OK);
  return(A_OK);
}
	       
